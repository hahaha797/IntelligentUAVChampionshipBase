#!/usr/bin/env bash
set -euo pipefail
basic_dev="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
uav_root="$(cd "$basic_dev/../.." && pwd)"
venv="$uav_root/.venv"
if [[ ! -f "$venv/pyvenv.cfg" ]]; then
    # System pip can bootstrap pip inside the venv even before python3-venv is installed.
    /usr/bin/python3 -m venv --without-pip --system-site-packages "$venv"
fi
"$venv/bin/python" -c 'import sys; assert sys.version_info[:2] == (3, 10), "ROS Humble requires this project venv to use Python 3.10"'
export PYTHONNOUSERSITE=1
"$venv/bin/python" -m pip install --upgrade 'pip<26'
"$venv/bin/python" -m pip install -r "$basic_dev/requirements-humble.txt"
printf 'Activate with: source %q/env_ros2.bash\n' "$basic_dev"
