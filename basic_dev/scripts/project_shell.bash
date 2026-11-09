# Dedicated interactive project shell; does not modify the user's global shell config.
if [[ -f "$HOME/.bashrc" ]]; then source "$HOME/.bashrc"; fi
uav_project_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/../../.." && pwd)"
cd "$uav_project_root"
source "$uav_project_root/activate_uav.bash"
printf '\nUAV environment ready\nPython: %s\nROS: %s, domain: %s\n' "$(command -v python)" "$ROS_DISTRO" "$ROS_DOMAIN_ID"
docker version --format 'Docker client: {{.Client.Version}} / server: {{.Server.Version}}'
printf 'Working directory: %s\n\n' "$PWD"
unset uav_project_root
