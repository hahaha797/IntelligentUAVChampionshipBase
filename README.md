# RMUA2026 — ROS 2 Humble 开发环境（ysy）

目标环境：Ubuntu 22.04、ROS 2 Humble、系统 Python 3.10。两个仓库并排位于 UAV 目录。
原版说明保留在 [README_ROS1.md](README_ROS1.md)，其中 Noetic/catkin 命令仅适用于历史版本。

两仓库统一使用入口见 [UAV 工作区说明](docs/WORKSPACE.md)。

详细步骤见 [环境配置与维护](docs/ENVIRONMENT.md)，仿真运行见 [Simulator 操作说明](../IntelligentUAVChampionshipSimulator/docs/OPERATIONS.md)。

## 构建和运行

```bash
export UAV_ROOT=/home/pk620/桌面/UAV  # 其他机器请修改此路径
cd "$UAV_ROOT/IntelligentUAVChampionshipBase/basic_dev"
./scripts/setup_venv.sh
./build_ros2.sh
source env_ros2.bash
ros2 run basic_dev basic_dev
```

只需创建或更新 Python 环境时（首次安装系统依赖的顺序见环境配置文档）：

```bash
cd "$UAV_ROOT/IntelligentUAVChampionshipBase/basic_dev"
./scripts/setup_venv.sh
source env_ros2.bash
python -c "import rclpy, cv2, cv_bridge; print('环境正常')"
```

`.venv` 位于 UAV 根目录，使用系统 Python 3.10 和 `--system-site-packages` 访问 ROS 2 系统库。
`env_ros2.bash` 自动激活它并关闭用户 site-packages，避免 Conda 及 `~/.local` 中 NumPy 2.x
污染 Humble/OpenCV。NumPy 固定为 1.24.4，依赖声明在 `requirements-humble.txt`，
本次实际安装版本记录在 `requirements-humble.lock.txt`。重新安装相同版本可执行
`python -m pip install -r requirements-humble.lock.txt`。

`build_ros2.sh` 优先使用项目 `.venv`，未创建时使用系统 Python。由于 Humble 的
rosidl/CMake 不能正确读取含中文路径的生成接口列表，build/install/log 存放在
`$HOME/.cache/rmua2026-ros2`，源码仍在当前仓库。可用 `UAV_ROS2_CACHE` 指定其他**英文路径**，
构建和运行时需保持一致。`env_ros2.bash` 保留已有 `ROS_DOMAIN_ID`（本机为 20），各进程应使用相同值。

Docker、GPU runtime 和系统开发依赖可通过同目录下的安装脚本配置：

```bash
cd "$UAV_ROOT/IntelligentUAVChampionshipBase/basic_dev"
sudo bash scripts/install_system_deps.sh "$USER"
```

脚本使用 Docker 和 NVIDIA 官方软件源，安装 Engine、Compose、Buildx、NVIDIA Container Toolkit、
venv/colcon/rosdep/vcstool 和项目系统依赖，启用 Docker 服务、配置 GPU runtime，并运行容器检查。
日志：`/var/log/uav-environment-install.log`。首次加入 docker 组后需注销再登录，或在新终端执行
`newgrp docker`。脚本不升级 NVIDIA 驱动，也不删除已有的容器引擎。管理员认证必须在本机完成。

安装后检查：`./scripts/check_environment.sh`。
如果仅缺少开发依赖，也可单独安装：

```bash
sudo apt install build-essential cmake git python3-colcon-common-extensions \
  ros-humble-cv-bridge ros-humble-message-filters ros-humble-pcl-conversions \
  libopencv-dev libeigen3-dev libpcl-dev python3-websocket python3-yaml
```

仓库内包含上游 `geographic_msgs` 源码及来源说明；g2o 首次构建自动下载固定提交的源码并校验
SHA256。视觉里程计保留 g2o 优化流程，改用 Eigen 求解器和 OpenCV CPU StereoSGBM，
不依赖自编译 CUDA OpenCV、写死的 `/usr/local` 路径或 SuiteSparse。
若相邻 Simulator 仓库存在，构建脚本同时构建 `rmua_simulator_bridge`。

## 功能包

| 包 | 用途 | 命令 |
| --- | --- | --- |
| airsim_ros | ROS 2 消息和服务 | `ros2 interface list` |
| geographic_msgs | ROS 2 地理消息依赖 | 参见包内 `UPSTREAM.md` |
| basic_dev | 订阅图像、IMU、GPS、点云和真值的基础示例 | `ros2 run basic_dev basic_dev` |
| imu_gps_odometry | IMU/GPS ESKF，输出 `/eskf_odom` | `ros2 run imu_gps_odometry imu_gps_odometry` |
| controller_test | 原 PWM 控制器示例 | `ros2 launch controller_test controller_test.launch.py` |
| odometry | 双目/IMU视觉里程计实验示例 | `ros2 launch odometry odometry.launch.py` |
| rmua_simulator_bridge | 官方 ROS 1 二进制与 ROS 2 通信适配 | 见相邻 Simulator 仓库 |

控制器 launch 同时启动 ESKF，接收初始位姿后，根据原示例逻辑在收到 1000 次里程计回调后开始
输出 PWM，目标仍是初始机体系下 `(1, 1, 0.5)`。这不是完整的竞速导航程序。
视觉里程计使用上游的相机标定参数，CPU 替换后的实时性能和飞行精度仍需真实场景验证。
RViz2 配置位于 `basic_dev/src/odometry/launch/config.rviz`，可用 `rviz2 -d <配置路径>` 打开。
消息坐标保持模拟器的 NED 约定；不会隐式改成 ENU。里程计 frame 为 `world_ned`、`drone_1_ned`。

## 2026 速度和 PWM 接口

以模拟器仓库提供的 VelCmd 为准，已修正 Base 原版仅有 `Twist` 的过期定义。

```bash
ros2 topic pub --once /airsim_node/drone_1/vel_body_cmd airsim_ros/msg/VelCmd \
  '{vx: 0.0, vy: 0.0, vz: 0.0, yaw_rate: 0.0, va: 4, stop: 1}'
ros2 service call /airsim_node/drone_1/takeoff airsim_ros/srv/Takeoff \
  '{wait_on_last_task: true}'
```

`va` 是加速度（官方上限 8 m/s²），`stop=1` 表示急停。`yaw_rate` 延续官方角速度单位。
ROS 2 字段使用 snake_case：`yaw_rate`、`rotor_pwm0..3`、`wait_on_last_task`；
适配层映射到二进制原有的 `yawRate`、`rotorPWM0..3`、`waitOnLastTask`。
Base 节点的 IMU/GPS/点云订阅使用 SensorDataQoS；图像订阅使用 Reliable（深度 2），避免当前环境中大图像分片丢失。
适配层遥测发布端统一使用 Reliable（深度 1），可与上述两类订阅者匹配。
初始位姿和终点采用 transient_local，支持后启动的节点。

## 容器

```bash
cd "$UAV_ROOT/IntelligentUAVChampionshipBase/basic_dev"
export ROS_DOMAIN_ID=20  # 与适配层及其他节点保持一致
docker build -t basic_dev:humble .
./run_basic_dev.sh
```

开发容器基于 Humble/Jammy，默认运行基础示例；可向 `run_basic_dev.sh` 传入其他 ROS 2 命令。
模拟器二进制保留在独立 Noetic 容器，宿主机无需安装 ROS 1。

## 验证

```bash
cd "$UAV_ROOT/IntelligentUAVChampionshipBase"
source basic_dev/env_ros2.bash
# 集成测试额外依赖，生产运行不需要 websockets 服务端库
python -m pip install 'websockets>=12,<14'
python basic_dev/tests/test_ros2_runtime.py
```

集成测试使用本地模拟 rosbridge 端点，验证时间戳/二进制图像转换、初始位姿保留、速度/PWM、
起飞服务和断线重连；同时运行 ESKF、控制器及视觉/基础节点，检查输出、合成双目图像处理和启动退出。
它不替代官方模拟器、GPU 容器和真实赛道上的闭环测试。
