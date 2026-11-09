#!/usr/bin/env bash
# Official installation procedures:
# https://docs.docker.com/engine/install/ubuntu/
# https://docs.nvidia.com/datacenter/cloud-native/container-toolkit/latest/install-guide.html
set -Eeuo pipefail
export PATH=/usr/sbin:/usr/bin:/sbin:/bin
if [[ $EUID -ne 0 ]]; then
    echo "Run with sudo: sudo bash $0 ${USER}" >&2
    exit 1
fi
target_user="${1:-${SUDO_USER:-}}"
if [[ -z "$target_user" ]] || ! id "$target_user" >/dev/null 2>&1 || [[ "$(id -u "$target_user")" == 0 ]]; then
    echo 'Pass the ordinary login user as the first argument.' >&2; exit 1
fi
source /etc/os-release
[[ "$ID" == ubuntu && "$VERSION_ID" == 22.04 ]] || { echo 'This installer targets Ubuntu 22.04.' >&2; exit 1; }
exec > >(tee -a /var/log/uav-environment-install.log) 2>&1
trap 'echo "Installation failed at line $LINENO; see /var/log/uav-environment-install.log"' ERR
export DEBIAN_FRONTEND=noninteractive
# Do not remove pre-existing container engines automatically.
for pkg in docker.io docker-compose docker-compose-v2 docker-doc docker-buildx podman-docker containerd runc; do
    if dpkg-query -W -f='${Status}' "$pkg" 2>/dev/null | /usr/bin/grep -qx 'install ok installed'; then
        echo "Conflicting existing package: $pkg; resolve it before rerunning." >&2; exit 1
    fi
done
apt-get update -o Acquire::Retries=2
apt-get install -y --no-install-recommends ca-certificates curl gnupg \
    python3-venv python3-pip python3-colcon-common-extensions python3-rosdep python3-vcstool \
    build-essential cmake git pkg-config unzip \
    ros-humble-cv-bridge ros-humble-message-filters ros-humble-pcl-conversions \
    libopencv-dev libeigen3-dev libpcl-dev python3-websocket python3-yaml
install -d -m 0755 /etc/apt/keyrings
curl -fL --retry 2 --connect-timeout 20 https://download.docker.com/linux/ubuntu/gpg -o /etc/apt/keyrings/docker.asc
chmod a+r /etc/apt/keyrings/docker.asc
cat > /etc/apt/sources.list.d/docker.sources <<DOCKER
Types: deb
URIs: https://download.docker.com/linux/ubuntu
Suites: jammy
Components: stable
Architectures: $(dpkg --print-architecture)
Signed-By: /etc/apt/keyrings/docker.asc
DOCKER
tmp_keys="$(mktemp -d)"
trap 'rm -rf "$tmp_keys"' EXIT
curl -fL --retry 2 --connect-timeout 20 https://nvidia.github.io/libnvidia-container/gpgkey -o "$tmp_keys/nvidia.asc"
gpg --batch --yes --dearmor -o /usr/share/keyrings/nvidia-container-toolkit-keyring.gpg "$tmp_keys/nvidia.asc"
curl -fL --retry 2 --connect-timeout 20 https://nvidia.github.io/libnvidia-container/stable/deb/nvidia-container-toolkit.list \
    -o "$tmp_keys/nvidia.list"
sed 's#deb https://#deb [signed-by=/usr/share/keyrings/nvidia-container-toolkit-keyring.gpg] https://#g' \
    "$tmp_keys/nvidia.list" > /etc/apt/sources.list.d/nvidia-container-toolkit.list
apt-get update -o Acquire::Retries=2
apt-get install -y --no-install-recommends \
    docker-ce docker-ce-cli containerd.io docker-buildx-plugin docker-compose-plugin \
    nvidia-container-toolkit
if [[ -f /etc/docker/daemon.json ]]; then
    cp -a /etc/docker/daemon.json "/etc/docker/daemon.json.uav-backup-$(date +%Y%m%d%H%M%S)"
fi
nvidia-ctk runtime configure --runtime=docker
systemctl enable --now docker
systemctl restart docker
usermod -aG docker "$target_user"
docker version
docker compose version
docker buildx version
docker run --rm hello-world
docker run --rm --gpus all ubuntu:22.04 nvidia-smi
printf '\nDocker, NVIDIA runtime and Humble system dependencies installed.\n'
printf 'Log out and back in for docker group membership, or use newgrp docker.\n'
