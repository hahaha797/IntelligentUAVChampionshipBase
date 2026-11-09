#pragma once
#include <rclcpp/rclcpp.hpp>

#include "airsim_ros/msg/vel_cmd.hpp"
#include "airsim_ros/msg/pose_cmd.hpp"
#include "airsim_ros/srv/takeoff.hpp"
#include "airsim_ros/srv/takeoff.hpp"
#include "airsim_ros/msg/rotor_pwm.hpp"
#include "nav_msgs/msg/odometry.hpp"

#include <geometry_msgs/msg/pose_stamped.hpp>
#include <time.h>
#include <stdlib.h>
#include <Eigen/Dense>
#include <random>
#include "PDcontroller.hpp"
#include <fstream>


rclcpp::Client<airsim_ros::srv::Takeoff>::SharedPtr g_takeoff_client;
std::vector<std::vector<Eigen::Vector3d>> globalPaths;
std::vector<Eigen::Vector3d> globalPath;
UAVLinearController g_PDcontroller;
std::random_device rd{};
std::mt19937 gen{rd()};
std::normal_distribution<double> gauss_dist{0.0, 1.0};
rclcpp::Publisher<airsim_ros::msg::RotorPWM>::SharedPtr g_pwm_publisher;
Eigen::Matrix4d Tw0, Twb_last;
Eigen::Vector3d Pwend;
int next_goal_index;
bool get_init_pose, get_end_goal;
int trigger_port = 1;
int cb_cnt = 0;
bool istakeoff = false;
void init_pose_cb(const geometry_msgs::msg::PoseStamped::ConstSharedPtr& msg);
void end_position_cb(const geometry_msgs::msg::PoseStamped::ConstSharedPtr& msg);
void gt_cb(const geometry_msgs::msg::PoseStamped::ConstSharedPtr& msg);
void odom_cb(const nav_msgs::msg::Odometry::ConstSharedPtr& msg);
