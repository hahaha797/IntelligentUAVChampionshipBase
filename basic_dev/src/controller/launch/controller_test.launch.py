from launch import LaunchDescription
from launch_ros.actions import Node

def generate_launch_description():
    return LaunchDescription([
        Node(package="imu_gps_odometry", executable="imu_gps_odometry", name="imu_gps_odometry", output="screen"),
        Node(package="controller_test", executable="controller_test", name="controller_test", output="screen"),
    ])
