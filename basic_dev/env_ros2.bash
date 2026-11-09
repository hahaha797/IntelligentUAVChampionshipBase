# Source this file from Bash to use the Humble workspace (also in Conda shells).
source /opt/ros/humble/setup.bash
export PATH="/usr/bin:$PATH"
uav_ros2_cache="${UAV_ROS2_CACHE:-$HOME/.cache/rmua2026-ros2}"
if [[ ! -f "$uav_ros2_cache/install/setup.bash" ]]; then
    echo "Build the workspace first: ./build_ros2.sh" >&2
    return 1
fi
source "$uav_ros2_cache/install/setup.bash"
unset uav_ros2_cache
# Activate the project venv after sourcing ROS, so Conda cannot shadow it.
uav_venv="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)/.venv"
if [[ -f "$uav_venv/bin/activate" ]]; then
    export PYTHONNOUSERSITE=1
    source "$uav_venv/bin/activate"
fi
unset uav_venv
