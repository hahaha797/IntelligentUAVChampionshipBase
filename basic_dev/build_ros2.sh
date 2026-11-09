#!/usr/bin/env bash
set -e
workspace="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
source /opt/ros/humble/setup.bash
export PATH="/usr/bin:$PATH"
venv_python="$workspace/../../.venv/bin/python"
if [[ -x "$venv_python" ]]; then
    python="$venv_python"
    export PYTHONNOUSERSITE=1
else
    python=/usr/bin/python3
fi
# Humble rosidl reads generated IDL lists with CMake file(STRINGS), which
# breaks non-ASCII paths. Put generated files in a real ASCII cache directory.
cache="${UAV_ROS2_CACHE:-$HOME/.cache/rmua2026-ros2}"
mkdir -p "$cache"
if [[ -f "$cache/workspace.path" && "$(cat "$cache/workspace.path")" != "$workspace" ]]; then
    echo "Cache belongs to another checkout; set UAV_ROS2_CACHE to a different ASCII path." >&2
    exit 1
fi
printf '%s\n' "$workspace" > "$cache/workspace.path"
sources=("$workspace/src")
bridge="$workspace/../../IntelligentUAVChampionshipSimulator/ros2/src"
if [[ -d "$bridge" ]]; then sources+=("$bridge"); fi
export CMAKE_BUILD_PARALLEL_LEVEL="${CMAKE_BUILD_PARALLEL_LEVEL:-6}"
exec "$python" -m colcon --log-base "$cache/log" build \
  --base-paths "${sources[@]}" --build-base "$cache/build" \
  --install-base "$cache/install" --symlink-install --parallel-workers 3 \
  --cmake-args -DCMAKE_BUILD_TYPE=Release -DPython3_EXECUTABLE="$python" \
  -DBUILD_TESTING=OFF "$@"
