# 环境配置与维护

适用于 Ubuntu 22.04、ROS 2 Humble、系统 Python 3.10。本文所有命令均在宿主机 Bash 执行。
两个仓库应并排放在同一 `UAV` 目录；下文使用当前机器路径，其他机器请修改第一行。

## 已配置机器的使用方式

```bash
export UAV_ROOT=/home/pk620/桌面/UAV
export ROS_DOMAIN_ID=20
source "$UAV_ROOT/IntelligentUAVChampionshipBase/basic_dev/env_ros2.bash"
python -c "import sys, numpy, cv2, rclpy, cv_bridge; print(sys.executable, numpy.__version__, cv2.__version__)"
"$UAV_ROOT/IntelligentUAVChampionshipBase/basic_dev/scripts/check_environment.sh"
```

项目根目录存在 `activate_uav.bash` 时，也可 source 该文件。新克隆只有两个仓库时，直接 source 上述 `env_ros2.bash` 即可。
该脚本加载 Humble、工作区以及 `.venv`，保留终端原有的 `ROS_DOMAIN_ID`；它本身不会固定设置为 20。

## 新机器首次配置顺序

前提：已安装 Ubuntu 22.04、ROS 2 Humble 和可用的 NVIDIA 驱动。系统依赖脚本不负责安装 Humble 本体或替换显卡驱动。

```bash
export UAV_ROOT=/home/pk620/桌面/UAV
cd "$UAV_ROOT/IntelligentUAVChampionshipBase/basic_dev"
# 安装 Docker、NVIDIA 容器运行时及开发依赖，需要管理员认证
sudo bash scripts/install_system_deps.sh "$USER"
```

首次加入 docker 组后，注销并重新登录，或执行 `newgrp docker`，然后继续：

```bash
export UAV_ROOT=/home/pk620/桌面/UAV
cd "$UAV_ROOT/IntelligentUAVChampionshipBase/basic_dev"
./scripts/setup_venv.sh
./build_ros2.sh
export ROS_DOMAIN_ID=20
source env_ros2.bash
./scripts/check_environment.sh
python -m pip check
cd "$UAV_ROOT/IntelligentUAVChampionshipSimulator"
./build_simulator.sh
./run_simulator.sh 123
```

首次构建需要网络：ROS 2 视觉包下载固定版本 g2o，仿真构建下载官方引擎和容器依赖。当前已经配置完成的机器无需重复系统安装或镜像构建。

## Python 虚拟环境

`.venv` 位于两仓库共同的父目录。创建脚本使用 `/usr/bin/python3` 和 `--system-site-packages`，以访问 apt 安装的 `rclpy`、`cv_bridge` 等二进制库。
激活项目时设置 `PYTHONNOUSERSITE=1`，避免 `~/.local` 下 NumPy 2.x 混入环境。不要用 Conda 的其他 Python 版本重建此 venv。

| 文件或脚本 | 用途 |
| --- | --- |
| `basic_dev/requirements-humble.txt` | 依赖范围；NumPy 固定为 1.24.4 |
| `basic_dev/requirements-humble.lock.txt` | 本次实际安装版本快照 |
| `basic_dev/scripts/setup_venv.sh` | 创建 venv 并安装项目依赖 |
| `basic_dev/env_ros2.bash` | 激活已构建的工作区和 venv |
| `basic_dev/scripts/check_environment.sh` | 检查 Python、ROS 接口、Docker 工具及 GPU |

需要按快照重新安装时，先激活环境，再运行：

```bash
python -m pip install -r "$UAV_ROOT/IntelligentUAVChampionshipBase/basic_dev/requirements-humble.lock.txt"
python -m pip check
```

ROS 系统库通过 apt 管理；pip 快照不能代替系统依赖安装。

## 构建与缓存

```bash
cd "$UAV_ROOT/IntelligentUAVChampionshipBase/basic_dev"
./build_ros2.sh
source env_ros2.bash
ros2 pkg prefix rmua_simulator_bridge
```

构建脚本搜索 Base 的 `src` 和相邻 Simulator 的 `ros2/src`。两者都存在时共构建七个包：
`airsim_ros`、`geographic_msgs`、`basic_dev`、`imu_gps_odometry`、`controller_test`、`odometry`、`rmua_simulator_bridge`。

构建产物默认放在 `$HOME/.cache/rmua2026-ros2/{build,install,log}`，源码仍保留在 UAV 内。
这是为避开 Humble rosidl/CMake 读取含中文路径的接口生成列表时出现的问题。需要独立缓存时：

```bash
export UAV_ROS2_CACHE="$HOME/.cache/rmua2026-ros2-ysy"
cd "$UAV_ROOT/IntelligentUAVChampionshipBase/basic_dev"
./build_ros2.sh
source env_ros2.bash
```

`UAV_ROS2_CACHE` 应为英文路径，构建和运行的每个终端都需保持一致。
缓存中的 `workspace.path` 防止不同源码目录混用产物；看到 `Cache belongs to another checkout` 时，为当前 checkout 指定新缓存。
默认 CMake 并发为 6、colcon 包并发为 3；内存不足时可先用 `CMAKE_BUILD_PARALLEL_LEVEL=2 ./build_ros2.sh` 降低单包编译并发。

## 本次安装版本记录

以下是 2026-11-09 的本机记录，用于排障对照，不是要求所有机器固定安装这些系统版本。

| 组件 | 本次验证值 |
| --- | --- |
| Python / NumPy / OpenCV | 3.10.12 / 1.24.4 / 4.5.4 |
| ROS | ROS 2 Humble |
| Docker Engine / Compose / Buildx | 29.9.3 / 5.7.0 / 0.38.1 |
| NVIDIA Container Toolkit | 1.20.1 |
| GPU / 驱动 | RTX 4090 / 580.178.04 |
| 官方仿真引擎 | 12.0.0.5，UE 4.27.2 |

## 常见环境问题

| 现象 | 检查与处理 |
| --- | --- |
| `Build the workspace first` | 先运行 `build_ros2.sh`，并确认构建和激活使用同一个 `UAV_ROS2_CACHE`。 |
| 导入 cv_bridge 时出现 NumPy ABI 错误 | 重新 source `env_ros2.bash`；确认 Python 指向 UAV/.venv、NumPy 为 1.24.4、`PYTHONNOUSERSITE=1`。 |
| `permission denied` 访问 Docker socket | 确认 `id -nG` 含 docker；新加入组后重新登录或 `newgrp docker`。当前旧终端也可用 `sg docker -c 'docker ps'` 检查。 |
| `ros2 topic list` 看不到其他终端节点 | 确认各进程的 `ROS_DOMAIN_ID` 一致；变量变更后重启相关节点，必要时执行 `ros2 daemon stop`。 |
| GPU 容器不可用 | 检查宿主机 `nvidia-smi`，再检查 `docker info` 中 NVIDIA runtime；查看系统安装日志。 |

更多启动、日志和图形问题见相邻仓库的 [仿真操作说明](../../IntelligentUAVChampionshipSimulator/docs/OPERATIONS.md)。
