#include "imu_gps_odometry.hpp"

int main(int argc, char** argv)
{
    //(重力， P_位置不确定度_std, P_速度不确定度_std, P_角度不确定度_std, P_角速度bias不确定度_std, P_加速度bias不确定度_std,
    //gps位置测量噪声_std gpsz姿态测量噪声_std, imu角速度测量噪声_std, imu加速度测量噪声_std)
    g_eskf_ptr = new ErrorStateKalmanFilter(-9.81083, 0.1, 0.1, 0.1, 0.0003158085227, 0.001117221, 0.5*10, 1.0, 0.00143, 0.0386);
    rclcpp::init(argc, argv);
    auto n = std::make_shared<rclcpp::Node>("imu_gps_odometry");
    g_eskf_odom_puber = n->create_publisher<nav_msgs::msg::Odometry>("/eskf_odom", 10);
    auto odom_suber = n->create_subscription<geometry_msgs::msg::PoseStamped>("/airsim_node/drone_1/gps", rclcpp::SensorDataQoS(), odom_local_ned_cb);//状态真值，用于赛道一
    auto imu_suber = n->create_subscription<sensor_msgs::msg::Imu>("airsim_node/drone_1/imu/imu", rclcpp::SensorDataQoS(), imu_cb);//imu数据
    auto init_pose_suber = n->create_subscription<geometry_msgs::msg::PoseStamped>("/airsim_node/initial_pose", rclcpp::QoS(1).transient_local(), init_pose_ned_cb);
    rclcpp::spin(n);
    g_eskf_odom_puber.reset();
    rclcpp::shutdown();
    delete g_eskf_ptr;
    return 0;
}

void init_pose_ned_cb(const geometry_msgs::msg::PoseStamped::ConstSharedPtr& msg)
{
    if(!g_eskf_ptr->m_isInitailed)
    {
        Eigen::Quaternion tmpq(msg->pose.orientation.w, msg->pose.orientation.x, msg->pose.orientation.y, msg->pose.orientation.z);
        Eigen::Matrix4d r0 = Eigen::Matrix4d::Identity();
        r0.block<3,3>(0, 0) = tmpq.toRotationMatrix();
        r0.block<3,1>(0, 3) << msg->pose.position.x, msg->pose.position.y,msg->pose.position.z;
        g_eskf_ptr->Init(r0, Eigen::Vector3d::Zero(),rclcpp::Time(msg->header.stamp).nanoseconds());
        g_eskf_ptr->m_isInitailed = true;
    }
}

void odom_local_ned_cb(const geometry_msgs::msg::PoseStamped::ConstSharedPtr& msg)
{
    // RCLCPP_INFO(rclcpp::get_logger("imu_gps_odometry"), "Get odom_local_ned_cd\n  orientation: %f-%f-%f-%f\n  position: %f-%f-%f\n",
    // msg->pose.orientation.w, msg->pose.orientation.x, msg->pose.orientation.y, msg->pose.orientation.z, //姿态四元数
    // msg->pose.position.x, msg->pose.position.y,msg->pose.position.z);
    odo_cnt ++;
    if (!g_eskf_ptr->m_isInitailed) return;
    g_eskf_ptr->correct(Eigen::Vector3d(msg->pose.position.x, msg->pose.position.y,msg->pose.position.z), 
        Eigen::Quaterniond(msg->pose.orientation.w,msg->pose.orientation.x, msg->pose.orientation.y,msg->pose.orientation.z));
}

void imu_cb(const sensor_msgs::msg::Imu::ConstSharedPtr& msg)
{
    // RCLCPP_INFO(rclcpp::get_logger("imu_gps_odometry"), "Get imu data.\n %f %f %f \n %f %f %f", msg->angular_velocity.x, msg->angular_velocity.y,
    // msg->angular_velocity.z, msg->linear_acceleration.x, msg->linear_acceleration.y, msg->linear_acceleration.z);
    if(g_eskf_ptr->m_isInitailed)
    {
        Eigen::Vector3d pos, vel, angle_vel;
        Eigen::Quaterniond q;
        if (!g_eskf_ptr->Predict(Eigen::Vector3d(msg->linear_acceleration.x, msg->linear_acceleration.y, msg->linear_acceleration.z),
            Eigen::Vector3d(msg->angular_velocity.x, msg->angular_velocity.y, msg->angular_velocity.z), 
            pos, vel, angle_vel, q, rclcpp::Time(msg->header.stamp).nanoseconds())) return;
        nav_msgs::msg::Odometry msg2;
        msg2.header.stamp = msg->header.stamp;
        msg2.header.frame_id = "world_ned";
        msg2.child_frame_id = "drone_1_ned";
        msg2.pose.pose.position.x = pos.x();
        msg2.pose.pose.position.y = pos.y();
        msg2.pose.pose.position.z = pos.z();
        msg2.pose.pose.orientation.w = q.w();
        msg2.pose.pose.orientation.x = q.x();
        msg2.pose.pose.orientation.y = q.y();
        msg2.pose.pose.orientation.z = q.z();
        msg2.twist.twist.linear.x = vel.x();
        msg2.twist.twist.linear.y = vel.y();
        msg2.twist.twist.linear.z = vel.z();
        msg2.twist.twist.angular.x = angle_vel.x();
        msg2.twist.twist.angular.y = angle_vel.y();
        msg2.twist.twist.angular.z = angle_vel.z();
        g_eskf_odom_puber->publish(msg2);
    }
}
