#!/usr/bin/env bash
set -e
cd "$(dirname "${BASH_SOURCE[0]}")"
source ./env_ros2.bash
if [[ $# -eq 0 ]]; then set -- ros2 run basic_dev basic_dev; fi
exec "$@"
