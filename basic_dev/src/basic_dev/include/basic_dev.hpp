#pragma once
#include <rclcpp/rclcpp.hpp>
#include <airsim_ros/msg/vel_cmd.hpp>
#include <airsim_ros/srv/takeoff.hpp>
#include <airsim_ros/srv/land.hpp>
#include <airsim_ros/srv/reset.hpp>
#include <geometry_msgs/msg/pose_stamped.hpp>
#include <sensor_msgs/msg/imu.hpp>
#include <sensor_msgs/msg/image.hpp>
#include <sensor_msgs/msg/point_cloud2.hpp>
#include <cv_bridge/cv_bridge.h>
#include <Eigen/Dense>

class BasicDev : public rclcpp::Node
{
public:
    BasicDev();
private:
    void pose_cb(const geometry_msgs::msg::PoseStamped::ConstSharedPtr& msg);
    void gps_cb(const geometry_msgs::msg::PoseStamped::ConstSharedPtr& msg);
    void imu_cb(const sensor_msgs::msg::Imu::ConstSharedPtr& msg);
    void lidar_cb(const sensor_msgs::msg::PointCloud2::ConstSharedPtr& msg);
    void image_cb(const sensor_msgs::msg::Image::ConstSharedPtr& msg);
    rclcpp::Subscription<geometry_msgs::msg::PoseStamped>::SharedPtr pose_sub_, gps_sub_;
    rclcpp::Subscription<sensor_msgs::msg::Imu>::SharedPtr imu_sub_;
    rclcpp::Subscription<sensor_msgs::msg::PointCloud2>::SharedPtr lidar_sub_;
    rclcpp::Subscription<sensor_msgs::msg::Image>::SharedPtr left_sub_, right_sub_;
    rclcpp::Publisher<airsim_ros::msg::VelCmd>::SharedPtr vel_publisher_;
    rclcpp::Client<airsim_ros::srv::Takeoff>::SharedPtr takeoff_client_;
    rclcpp::Client<airsim_ros::srv::Land>::SharedPtr land_client_;
    rclcpp::Client<airsim_ros::srv::Reset>::SharedPtr reset_client_;
};
