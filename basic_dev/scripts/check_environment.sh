#!/usr/bin/env bash
set -o pipefail
basic_dev="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
source "$basic_dev/env_ros2.bash" || exit 1
failed=0
python - <<'PY' || failed=1
import sys
import numpy, cv2, cv_bridge, rclpy, yaml, websocket, websockets, colcon_core
from airsim_ros.msg import VelCmd
print('Python:', sys.executable, sys.version.split()[0])
print('venv:', sys.prefix)
print('NumPy:', numpy.__version__, 'OpenCV:', cv2.__version__)
print('ROS 2 and custom interfaces: OK')
PY
if command -v docker >/dev/null; then
    docker version || failed=1
    docker compose version || failed=1
    docker buildx version || failed=1
    docker info --format 'Runtimes: {{range $name, $runtime := .Runtimes}}{{$name}} {{end}}' || failed=1
else
    echo 'Docker is not installed.' >&2
    failed=1
fi
nvidia-smi --query-gpu=name,driver_version --format=csv,noheader || failed=1
exit "$failed"
