#!/usr/bin/env bash
set -e
exec docker run -it --net host --name basic_dev --rm \
    -e ROS_DOMAIN_ID="${ROS_DOMAIN_ID:-0}" -e ROS_LOCALHOST_ONLY="${ROS_LOCALHOST_ONLY:-0}" \
    basic_dev:humble "$@"
