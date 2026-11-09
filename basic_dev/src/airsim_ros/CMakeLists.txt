cmake_minimum_required(VERSION 3.16)
project(airsim_ros)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
find_package(ament_cmake REQUIRED)
find_package(std_msgs REQUIRED)
find_package(geometry_msgs REQUIRED)
find_package(geographic_msgs REQUIRED)
find_package(rosidl_default_generators REQUIRED)
rosidl_generate_interfaces(${PROJECT_NAME}
  "msg/Altimeter.msg"
  "msg/AngleRateThrottle.msg"
  "msg/CarControls.msg"
  "msg/CarState.msg"
  "msg/Environment.msg"
  "msg/GPSYaw.msg"
  "msg/GimbalAngleEulerCmd.msg"
  "msg/GimbalAngleQuatCmd.msg"
  "msg/PoseCmd.msg"
  "msg/RotorPWM.msg"
  "msg/VelCmd.msg"
  "msg/VelCmdGroup.msg"
  "srv/DebugSphere.srv"
  "srv/Land.srv"
  "srv/LandGroup.srv"
  "srv/MeterReport.srv"
  "srv/Reset.srv"
  "srv/SetGPSPosition.srv"
  "srv/SetLocalPosition.srv"
  "srv/Takeoff.srv"
  "srv/TakeoffGroup.srv"
  "srv/TriggerPort.srv"
  DEPENDENCIES std_msgs geometry_msgs geographic_msgs
)
ament_export_dependencies(rosidl_default_runtime)
ament_package()
