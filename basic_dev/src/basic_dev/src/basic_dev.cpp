#include "basic_dev.hpp"

BasicDev::BasicDev() : Node("basic_dev")
{
    using std::placeholders::_1;
    const auto qos = rclcpp::SensorDataQoS();
    pose_sub_ = create_subscription<geometry_msgs::msg::PoseStamped>(
        "/airsim_node/drone_1/debug/pose_gt", qos, std::bind(&BasicDev::pose_cb, this, _1));
    gps_sub_ = create_subscription<geometry_msgs::msg::PoseStamped>(
        "/airsim_node/drone_1/gps", qos, std::bind(&BasicDev::gps_cb, this, _1));
    imu_sub_ = create_subscription<sensor_msgs::msg::Imu>(
        "/airsim_node/drone_1/imu/imu", qos, std::bind(&BasicDev::imu_cb, this, _1));
    lidar_sub_ = create_subscription<sensor_msgs::msg::PointCloud2>(
        "/airsim_node/drone_1/lidar", qos, std::bind(&BasicDev::lidar_cb, this, _1));
    left_sub_ = create_subscription<sensor_msgs::msg::Image>(
        "/airsim_node/drone_1/front_left/Scene", rclcpp::QoS(2), std::bind(&BasicDev::image_cb, this, _1));
    right_sub_ = create_subscription<sensor_msgs::msg::Image>(
        "/airsim_node/drone_1/front_right/Scene", rclcpp::QoS(2), std::bind(&BasicDev::image_cb, this, _1));
    vel_publisher_ = create_publisher<airsim_ros::msg::VelCmd>("/airsim_node/drone_1/vel_body_cmd", 10);
    takeoff_client_ = create_client<airsim_ros::srv::Takeoff>("/airsim_node/drone_1/takeoff");
    land_client_ = create_client<airsim_ros::srv::Land>("/airsim_node/drone_1/land");
    reset_client_ = create_client<airsim_ros::srv::Reset>("/airsim_node/reset");
    // This example observes sensors. Use async_send_request() to call flight services.
}

void BasicDev::pose_cb(const geometry_msgs::msg::PoseStamped::ConstSharedPtr& msg)
{
    RCLCPP_INFO_THROTTLE(get_logger(), *get_clock(), 1000, "Pose: %.3f %.3f %.3f",
        msg->pose.position.x, msg->pose.position.y, msg->pose.position.z);
}
void BasicDev::gps_cb(const geometry_msgs::msg::PoseStamped::ConstSharedPtr& msg)
{
    RCLCPP_INFO_THROTTLE(get_logger(), *get_clock(), 1000, "GPS: %.3f %.3f %.3f",
        msg->pose.position.x, msg->pose.position.y, msg->pose.position.z);
}
void BasicDev::imu_cb(const sensor_msgs::msg::Imu::ConstSharedPtr& msg)
{
    RCLCPP_DEBUG(get_logger(), "IMU stamp: %.9f", rclcpp::Time(msg->header.stamp).seconds());
}
void BasicDev::lidar_cb(const sensor_msgs::msg::PointCloud2::ConstSharedPtr& msg)
{
    RCLCPP_DEBUG(get_logger(), "Lidar: %u x %u", msg->width, msg->height);
}
void BasicDev::image_cb(const sensor_msgs::msg::Image::ConstSharedPtr& msg)
{
    try {
        auto image = cv_bridge::toCvShare(msg, "bgr8");
        RCLCPP_DEBUG(get_logger(), "Image: %d x %d", image->image.cols, image->image.rows);
    } catch (const cv_bridge::Exception& e) {
        RCLCPP_WARN(get_logger(), "Image conversion failed: %s", e.what());
    }
}
int main(int argc, char** argv)
{
    rclcpp::init(argc, argv);
    rclcpp::spin(std::make_shared<BasicDev>());
    rclcpp::shutdown();
    return 0;
}
