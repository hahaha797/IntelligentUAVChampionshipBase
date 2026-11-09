# UAV：RMUA2026 / ROS 2 Humble

本目录包含两个并排工作的仓库，当前均在 `ysy` 分支：

| 仓库 | 来源分支 | 用途与说明 |
| --- | --- | --- |
| [IntelligentUAVChampionshipBase](../../IntelligentUAVChampionshipBase/README.md) | `RMUA2026` | ROS 2 消息、定位、控制及开发环境 |
| [IntelligentUAVChampionshipSimulator](../../IntelligentUAVChampionshipSimulator/README.md) | `RMUA2026-01` | 官方仿真引擎容器及 ROS 2 适配层 |

宿主机使用 Ubuntu 22.04、ROS 2 Humble、Python 3.10。开发代码已迁移到 ROS 2；官方 Unreal 引擎仍是 ROS 1 二进制，在独立 Noetic 容器内运行，通过 WebSocket 与宿主机 ROS 2 适配层通信。

## 本机日常使用

以下路径对应当前机器。迁移到其他位置时，请修改 `UAV_ROOT`；两个仓库应保持并排。

```bash
export UAV_ROOT=/home/pk620/桌面/UAV
export ROS_DOMAIN_ID=20
source "$UAV_ROOT/IntelligentUAVChampionshipBase/basic_dev/env_ros2.bash"
cd "$UAV_ROOT/IntelligentUAVChampionshipSimulator"
./run_simulator.sh 123
```

该命令同时启动图形仿真和 ROS 2 适配层。窗口停在停机坪、显示 `Pending` 是等待状态；启动仿真不会自动启动控制器。当前已有仿真运行时，直接使用现有窗口，不要重复启动同名容器。

另开终端观察数据：

```bash
export UAV_ROOT=/home/pk620/桌面/UAV
export ROS_DOMAIN_ID=20
source "$UAV_ROOT/IntelligentUAVChampionshipBase/basic_dev/env_ros2.bash"
ros2 node list
ros2 topic echo /airsim_node/drone_1/imu/imu --once --qos-reliability best_effort
ros2 topic info /airsim_node/drone_1/front_left/Scene --verbose
```

所有终端需使用相同的 `ROS_DOMAIN_ID`；本次配置使用 `20`。激活脚本沿用终端已有设置；新终端如未配置，请先执行 `export ROS_DOMAIN_ID=20`，再运行上述命令。

在启动终端按 `Ctrl-C` 会停止容器和适配层。没有窗口需求时，将启动命令换为 `./run_simulator_offscreen.sh 123`。结束项目终端后，重新打开普通终端即可恢复日常 Python 环境。

## 安装、开发与排障

- [Base 环境配置与维护](../../IntelligentUAVChampionshipBase/docs/ENVIRONMENT.md)：首次安装顺序、venv、构建缓存、版本记录及环境诊断。
- [Base 开发说明](../../IntelligentUAVChampionshipBase/README.md)：功能包、运行命令、接口变更和集成测试。
- [Simulator 使用与排障](../../IntelligentUAVChampionshipSimulator/docs/OPERATIONS.md)：启动、停止、参数、日志、下载与图形问题。
- [Simulator 架构与验证范围](../../IntelligentUAVChampionshipSimulator/README.md)：适配边界、消息配置及真实引擎验证结果。

## 当前验证记录

2026-11-09 已完成七个 ROS 2 包构建、三项集成测试、Docker GPU 检查和官方 `12.0.0.5` 引擎启动。种子 `123` 的 1280×720 图形窗口正常显示，ROS 2 实际接收到 960×720 前视图像、IMU 和初始位姿。完整竞速闭环、长时间性能及全部服务实际调用仍需后续验证。

本次启动日志位于 `/tmp/uav-simulator-runtime.log`，构建记录位于 `/tmp/uav-simulator-build.log`；这些是本次会话的临时文件，新启动不会自动沿用该日志文件。系统安装日志位于 `/var/log/uav-environment-install.log`。
