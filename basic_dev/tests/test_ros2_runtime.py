#!/usr/bin/python3
"""Runtime integration checks. Requires a built workspace and websockets>=12."""
import base64
import importlib.util
import json
import math
from pathlib import Path
import queue
import signal
import subprocess
import threading
import time
import unittest

import rclpy
import numpy as np
from rclpy.qos import QoSProfile, DurabilityPolicy
from rosidl_runtime_py.convert import message_to_ordereddict
from ament_index_python.packages import get_package_prefix
from geometry_msgs.msg import PoseStamped
from sensor_msgs.msg import Imu, Image, PointCloud2
from nav_msgs.msg import Odometry
from airsim_ros.msg import VelCmd, RotorPWM
from airsim_ros.srv import Takeoff
from websockets.sync.server import serve

script = Path(__file__).resolve().parents[3] / 'IntelligentUAVChampionshipSimulator/ros2/src/rmua_simulator_bridge/scripts/rmua_simulator_bridge.py'
spec = importlib.util.spec_from_file_location('bridge', script)
bridge = importlib.util.module_from_spec(spec)
spec.loader.exec_module(bridge)


class RuntimeTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        rclpy.init()

    @classmethod
    def tearDownClass(cls):
        rclpy.shutdown()

    def setUp(self):
        self.node = rclpy.create_node('uav_runtime_test_' + str(time.monotonic_ns()))
        self.processes = []

    def tearDown(self):
        for proc in self.processes:
            proc.send_signal(signal.SIGINT)
        for proc in self.processes:
            try:
                proc.wait(timeout=6)
            except subprocess.TimeoutExpired:
                proc.kill()
                proc.wait()
            self.assertIn(proc.returncode, (0, -signal.SIGINT), f'node crashed: {proc.args}')
        self.node.destroy_node()

    def start(self, package, executable, *args):
        path = str(Path(get_package_prefix(package)) / 'lib' / package / executable)
        proc = subprocess.Popen([path, *args],
                                stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
        self.processes.append(proc)
        return proc

    def until(self, predicate, timeout=8):
        deadline = time.monotonic() + timeout
        while time.monotonic() < deadline:
            if predicate():
                return
            rclpy.spin_once(self.node, timeout_sec=0.02)
        self.assertTrue(predicate(), 'condition timed out')

    def test_bridge_transport(self):
        commands = queue.Queue()
        subscriptions = queue.Queue()
        calls = queue.Queue()
        peers = []
        pose = PoseStamped()
        pose.header.stamp.sec = 12
        pose.header.stamp.nanosec = 345
        pose.pose.orientation.w = 1.0
        pose.pose.position.x = 2.5
        image = Image(height=1, width=1, encoding='bgr8', step=3, data=[1, 2, 3])
        image_wire = bridge.to_ros1(message_to_ordereddict(image))
        image_wire['data'] = base64.b64encode(bytes(image.data)).decode()
        cloud = PointCloud2(height=1, width=1, point_step=4, row_step=4, data=[0, 1, 2, 3])
        cloud_wire = bridge.to_ros1(message_to_ordereddict(cloud))
        cloud_wire['data'] = base64.b64encode(bytes(cloud.data)).decode()
        def handler(ws):
            peers.append(ws)
            try:
                for raw in ws:
                    packet = json.loads(raw)
                    if packet['op'] == 'subscribe':
                        topic = packet['topic']
                        subscriptions.put(topic)
                        if topic.endswith('initial_pose') or topic.endswith('/gps'):
                            msg = bridge.to_ros1(message_to_ordereddict(pose))
                        elif topic.endswith('front_left/Scene'):
                            msg = image_wire
                        else:
                            continue
                        ws.send(json.dumps({'op': 'publish', 'topic': topic, 'msg': msg}))
                    elif packet['op'] == 'publish':
                        commands.put(packet)
                    elif packet['op'] == 'call_service':
                        calls.put(packet)
                        if not packet['args']['waitOnLastTask']:
                            continue  # Simulate an unresponsive engine service.
                        ws.send(json.dumps({'op': 'service_response', 'id': packet['id'],
                            'service': packet['service'], 'result': True, 'values': {'success': True}}))
            except Exception:
                pass
        server = serve(handler, '127.0.0.1', 0)
        port = server.socket.getsockname()[1]
        worker = threading.Thread(target=server.serve_forever, daemon=True)
        worker.start()
        try:
            self.start('rmua_simulator_bridge', 'rmua_simulator_bridge', '--ros-args',
                       '-p', f'url:=ws://127.0.0.1:{port}', '-p', 'service_timeout:=0.5')
            self.until(lambda: subscriptions.qsize() >= 11)
            # A late subscriber must receive initial pose through transient-local QoS.
            retained, images, gps, clouds = [], [], [], []
            self.node.create_subscription(PoseStamped, '/airsim_node/initial_pose', retained.append,
                QoSProfile(depth=1, durability=DurabilityPolicy.TRANSIENT_LOCAL))
            self.node.create_subscription(Image, '/airsim_node/drone_1/front_left/Scene', images.append, 10)
            self.node.create_subscription(PoseStamped, '/airsim_node/drone_1/gps', gps.append, 10)
            self.node.create_subscription(PointCloud2, '/airsim_node/drone_1/lidar', clouds.append, 10)
            self.until(lambda: bool(retained))
            self.assertEqual(retained[0].header.stamp.nanosec, 345)
            self.assertEqual(retained[0].pose.position.x, 2.5)
            # Send fresh telemetry after DDS discovery.
            peers[-1].send(json.dumps({'op': 'publish', 'topic': '/airsim_node/drone_1/front_left/Scene', 'msg': image_wire}))
            self.until(lambda: bool(images))
            self.assertEqual(list(images[-1].data), [1, 2, 3])
            peers[-1].send(json.dumps({'op': 'publish', 'topic': '/airsim_node/drone_1/lidar', 'msg': cloud_wire}))
            peers[-1].send(json.dumps({'op': 'publish', 'topic': '/airsim_node/drone_1/gps', 'msg': bridge.to_ros1(message_to_ordereddict(pose))}))
            self.until(lambda: bool(clouds) and bool(gps))
            self.assertEqual(list(clouds[-1].data), [0, 1, 2, 3])
            self.assertEqual(gps[-1].header.stamp.sec, 12)
            pub = self.node.create_publisher(VelCmd, '/airsim_node/drone_1/vel_body_cmd', 10)
            self.until(lambda: pub.get_subscription_count() > 0)
            pub.publish(VelCmd(vx=1.5, yaw_rate=12.0, va=4, stop=1))
            self.until(lambda: not commands.empty())
            cmd = commands.get()['msg']
            self.assertEqual((cmd['vx'], cmd['yawRate'], cmd['va'], cmd['stop']), (1.5, 12.0, 4, 1))
            pwm_pub = self.node.create_publisher(RotorPWM, '/airsim_node/drone_1/rotor_pwm_cmd', 10)
            self.until(lambda: pwm_pub.get_subscription_count() > 0)
            pwm_pub.publish(RotorPWM(rotor_pwm0=0.25))
            self.until(lambda: not commands.empty())
            self.assertEqual(commands.get()['msg']['rotorPWM0'], 0.25)
            client = self.node.create_client(Takeoff, '/airsim_node/drone_1/takeoff')
            self.until(client.service_is_ready)
            future = client.call_async(Takeoff.Request(wait_on_last_task=True))
            self.until(future.done)
            self.assertTrue(future.result().success)
            self.assertTrue(calls.get()['args']['waitOnLastTask'])
            future = client.call_async(Takeoff.Request(wait_on_last_task=False))
            self.until(future.done)
            self.assertFalse(future.result().success)
            # Reconnect must restore telemetry subscriptions; flight commands aren't replayed.
            peers[-1].close()
            self.until(lambda: len(peers) >= 2 and subscriptions.qsize() >= 22)
            self.assertTrue(commands.empty())
        finally:
            server.shutdown()
            for peer in peers:
                peer.close()
            worker.join(timeout=3)

    def test_estimator_and_controller(self):
        estimator = self.start('imu_gps_odometry', 'imu_gps_odometry')
        controller = self.start('controller_test', 'controller_test')
        initial_pub = self.node.create_publisher(PoseStamped, '/airsim_node/initial_pose',
            QoSProfile(depth=1, durability=DurabilityPolicy.TRANSIENT_LOCAL))
        imu_pub = self.node.create_publisher(Imu, '/airsim_node/drone_1/imu/imu', 10)
        odometry, pwm = [], []
        self.node.create_subscription(Odometry, '/eskf_odom', odometry.append, 10)
        self.node.create_subscription(RotorPWM, '/airsim_node/drone_1/rotor_pwm_cmd', pwm.append, 10)
        self.until(lambda: initial_pub.get_subscription_count() >= 2 and imu_pub.get_subscription_count() > 0)
        pose = PoseStamped()
        pose.header.stamp.sec = 100
        pose.pose.orientation.w = 1.0
        initial_pub.publish(pose)
        # Wait for initial callback before driving a 100 Hz synthetic stationary IMU.
        for _ in range(10):
            rclpy.spin_once(self.node, timeout_sec=0.02)
        for i in range(1, 1051):
            imu = Imu()
            imu.header.stamp.sec = 100 + i // 100
            imu.header.stamp.nanosec = (i % 100) * 10000000
            imu.linear_acceleration.z = -9.81083
            imu_pub.publish(imu)
            rclpy.spin_once(self.node, timeout_sec=0.003)
        self.until(lambda: len(odometry) > 10)
        self.until(lambda: len(pwm) > 0)
        for msg in odometry:
            self.assertTrue(all(math.isfinite(v) for v in [msg.pose.pose.position.z,
                msg.pose.pose.orientation.w, msg.twist.twist.linear.z]))
            self.assertEqual(msg.header.frame_id, 'world_ned')
        for msg in pwm:
            self.assertTrue(all(math.isfinite(v) and 0 <= v <= 1 for v in
                [msg.rotor_pwm0, msg.rotor_pwm1, msg.rotor_pwm2, msg.rotor_pwm3]))
        self.assertIsNone(estimator.poll())
        self.assertIsNone(controller.poll())

    def test_visual_and_basic_startup(self):
        visual = self.start('odometry', 'odometry')
        basic = self.start('basic_dev', 'basic_dev')
        self.until(lambda: '/odometry' in ['/' + name for name, ns in self.node.get_node_names_and_namespaces()])
        display = []
        self.node.create_subscription(Image, '/display_image', display.append, 10)
        left = self.node.create_publisher(Image, '/airsim_node/drone_1/front_left/Scene', 10)
        right = self.node.create_publisher(Image, '/airsim_node/drone_1/front_right/Scene', 10)
        self.until(lambda: left.get_subscription_count() >= 2 and right.get_subscription_count() >= 2)
        self.until(lambda: self.node.count_publishers("/display_image") > 0)
        for _ in range(10):
            rclpy.spin_once(self.node, timeout_sec=0.05)
        rng = np.random.default_rng(42)
        texture = rng.integers(0, 256, (720, 960, 3), dtype=np.uint8)
        for pub, data in [(left, texture), (right, np.roll(texture, -16, axis=1))]:
            msg = Image(height=720, width=960, encoding='bgr8', step=2880, data=data.tobytes())
            msg.header.stamp.sec = 101
            pub.publish(msg)
        self.until(lambda: bool(display))
        self.assertGreater(len(display[0].data), 0)
        for _ in range(20):
            rclpy.spin_once(self.node, timeout_sec=0.02)
        self.assertIsNone(visual.poll())
        self.assertIsNone(basic.poll())


if __name__ == '__main__':
    unittest.main(verbosity=2)
