#include <rclcpp/rclcpp.hpp>

#include "nav_msgs/msg/odometry.hpp"
#include "sensor_msgs/msg/imu.hpp"
#include <geometry_msgs/msg/pose_stamped.hpp>
#include <time.h>
#include <stdlib.h>
#include <random>
#include <mutex>
#include "eskf.hpp"

std::random_device rd{};
std::mt19937 gen{rd()};
std::normal_distribution<double> gauss_dist{0.0, 1.0};
ErrorStateKalmanFilter* g_eskf_ptr;
int odo_cnt = 0;
rclcpp::Publisher<nav_msgs::msg::Odometry>::SharedPtr g_eskf_odom_puber;
void odom_local_ned_cb(const geometry_msgs::msg::PoseStamped::ConstSharedPtr& msg);
void init_pose_ned_cb(const geometry_msgs::msg::PoseStamped::ConstSharedPtr& msg);
void imu_cb(const sensor_msgs::msg::Imu::ConstSharedPtr& msg);
