#include "controllerTest.hpp"

int main(int argc, char** argv)
{
    rclcpp::init(argc, argv);
    auto n = std::make_shared<rclcpp::Node>("controller_test");
    //无人机信息通过如下命令订阅，当收到消息时自动回调对应的函数
    g_takeoff_client = n->create_client<airsim_ros::srv::Takeoff>("/airsim_node/drone_1/takeoff");
    g_pwm_publisher = n->create_publisher<airsim_ros::msg::RotorPWM>("/airsim_node/drone_1/rotor_pwm_cmd", 10);
    auto odom_suber = n->create_subscription<nav_msgs::msg::Odometry>("/eskf_odom", rclcpp::SensorDataQoS(), odom_cb);
    // auto gt_suber = n->create_subscription<geometry_msgs::msg::PoseStamped>("/airsim_node/drone_1/debug/pose_gt", rclcpp::SensorDataQoS(), gt_cb);
    auto init_pose_suber = n->create_subscription<geometry_msgs::msg::PoseStamped>("/airsim_node/initial_pose", rclcpp::QoS(1).transient_local(), init_pose_cb);
    auto end_pose_suber = n->create_subscription<geometry_msgs::msg::PoseStamped>("/airsim_node/end_goal", rclcpp::QoS(1).transient_local(), end_position_cb);
    auto tf_cmd = std::make_shared<airsim_ros::srv::Takeoff::Request>();
    tf_cmd->wait_on_last_task = true;
    // g_takeoff_client.call(tf_cmd);
    rclcpp::spin(n);
    g_pwm_publisher.reset();
    g_takeoff_client.reset();
    rclcpp::shutdown();
    return 0;
}

void init_pose_cb(const geometry_msgs::msg::PoseStamped::ConstSharedPtr& msg)
{
    Eigen::Quaternion Q(msg->pose.orientation.w, msg->pose.orientation.x, msg->pose.orientation.y, msg->pose.orientation.z);
    Eigen::Matrix3d rotationM = Q.normalized().toRotationMatrix();
    // std::cout<<"initial pose: \n"<<rotationM<<std::endl;
    Eigen::Vector3d pos(msg->pose.position.x, msg->pose.position.y, msg->pose.position.z);
    // std::cout<<"initial pos: "<<pos.transpose()<<std::endl;
    Tw0 = Eigen::Matrix4d::Identity();
    Tw0.block(0, 0, 3, 3)=rotationM;
    Tw0.block(0, 3, 3 ,1) = pos;
    Twb_last = Tw0;
    // std::cout<<"Tw0:\n"<<Tw0<<std::endl;
    get_init_pose = true;
}

void end_position_cb(const geometry_msgs::msg::PoseStamped::ConstSharedPtr& msg)
{
    Pwend = Eigen::Vector3d(msg->pose.position.x, msg->pose.position.y, msg->pose.position.z);
    // std::cout<<"end pos: "<<Pwend.transpose()<<std::endl;
    if(get_init_pose && !get_end_goal)
    {
        for(auto ps: globalPaths)
        {
            if((ps[0]-Tw0.block(0, 3, 3, 1)).norm() < 10)
            {
                for(int i = 0; i < ps.size(); i++)
                {
                    globalPath.emplace_back(ps[i]);
                }
                break;
            }
        }
        for(auto ps: globalPaths)
        {
            if((ps[0]-Pwend).norm() < 10)
            {
                for(int i = 0; i < ps.size(); i++)
                {
                    globalPath.emplace_back(ps[ps.size()-1-i]);
                }
                break;
            }
        }
        get_end_goal = true;
    }
}

void odom_cb(const nav_msgs::msg::Odometry::ConstSharedPtr& msg)
{
    cb_cnt ++;
    if(cb_cnt / 100 < 10)return;
    if(! get_init_pose)return;
    Eigen::Quaternion Q(msg->pose.pose.orientation.w, msg->pose.pose.orientation.x, msg->pose.pose.orientation.y, msg->pose.pose.orientation.z);
    Eigen::Matrix4d Twb = Eigen::Matrix4d::Identity();
    Twb.block(0, 0, 3 ,3) = Q.normalized().toRotationMatrix();
    Twb(0, 3) = msg->pose.pose.position.x;
    Twb(1, 3) = msg->pose.pose.position.y;
    Twb(2, 3) = msg->pose.pose.position.z;
    // std::cout<<"Twb rt:\n"<<Twb<<std::endl;
    // std::cout<<phi<<" "<<theta<<" "<<psi<<std::endl;
    Eigen::VectorXf X_des, X_real;
    X_des.resize(12);
    X_real.resize(12);
    // Twb_last = Twb;
    // std::cout<<"Tw0:\n"<<Tw0<<std::endl;
    Eigen::Matrix4d TWfluWned;
    TWfluWned << 1, 0, 0, 0, 
                0, -1, 0, 0,
                0, 0, -1, 0, 
                0, 0, 0, 1;
    Eigen::Matrix4d TWflu0 = TWfluWned * Tw0 * TWfluWned.inverse();
    Eigen::Matrix4d TWflub = TWfluWned * Twb * TWfluWned.inverse();
    Eigen::Matrix4d T0flub = TWflu0.inverse() * TWflub;
    Eigen::Vector3d VWned(msg->twist.twist.linear.x, msg->twist.twist.linear.y, msg->twist.twist.linear.z);
    Eigen::Vector3d VBned = Twb.block(0, 0, 3 ,3).inverse() * VWned;
    Eigen::Vector3d VBflu = TWfluWned.block<3, 3>(0, 0) * VBned;
    Eigen::Vector3d Wned(msg->twist.twist.angular.x, msg->twist.twist.angular.y, msg->twist.twist.angular.z);
    Eigen::Vector3d Wflu = TWfluWned.block<3, 3>(0, 0) * Wned;
    const float phi = std::asin(T0flub(2, 1));
    const float theta = std::atan2(-T0flub(2, 0)/std::cos(phi), T0flub(2, 2)/std::cos(phi));
    const float psi = std::atan2(-T0flub(0, 1)/std::cos(phi), T0flub(1, 1)/std::cos(phi));
    X_real<<T0flub(0, 3), T0flub(1, 3), T0flub(2, 3), 
        VBflu.x(), VBflu.y(), VBflu.z(), 
        phi, theta, psi, Wflu.x(), Wflu.y(), Wflu.z();
    X_des << 1.0, 1.0, 0.5, 0, 0, 0, 0, 0, 0, 0, 0, 0;
    Eigen::Vector4f output = g_PDcontroller.execute( X_des, X_real);
    airsim_ros::msg::RotorPWM pwm_cmd;
    pwm_cmd.rotor_pwm0 = output[0];
    pwm_cmd.rotor_pwm1 = output[1];
    pwm_cmd.rotor_pwm2 = output[2];
    pwm_cmd.rotor_pwm3 = output[3];
    // // std::cout<<pwm_cmd.rotor_pwm0<<" "<<pwm_cmd.rotor_pwm1<<" "<<pwm_cmd.rotor_pwm2<<" "<<pwm_cmd.rotor_pwm3<<" "<<std::endl;
    if(X_real[0]<3.0)
    {
        g_pwm_publisher->publish(pwm_cmd);
    }else
    {
        if(trigger_port > 12)
            trigger_port = 0;
    }

}
