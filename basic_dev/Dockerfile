FROM ros:humble-ros-base-jammy
SHELL ["/bin/bash", "-c"]
RUN apt-get update && apt-get install -y --no-install-recommends \
    build-essential cmake git python3-colcon-common-extensions \
    ros-humble-cv-bridge ros-humble-message-filters ros-humble-pcl-conversions \
    libopencv-dev libeigen3-dev libpcl-dev && rm -rf /var/lib/apt/lists/*
COPY src /basic_dev/src
COPY build_ros2.sh env_ros2.bash setup.bash /basic_dev/
WORKDIR /basic_dev
ENV UAV_ROS2_CACHE=/opt/rmua2026-ros2
RUN bash build_ros2.sh
ENTRYPOINT ["/bin/bash", "/basic_dev/setup.bash"]
