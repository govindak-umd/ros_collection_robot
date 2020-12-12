#include "../include/line.h"
#include "../include/polygon.h"
#include "../include/navigator.h"
#include "../include/order_manager.h"
#include "../include/node.h"
#include "../include/decoder.h"

Navigator::Navigator() {
    odom_sub_ = nh_.subscribe("/odom", 10,
                                &Navigator::odomCallback, this);
    vel_pub_ = nh_.advertise<geometry_msgs::Twist>("/cmd_vel", 1, true);
    lidar_sub_ = nh_.subscribe("/scan",10,
                               &Navigator::lidarCallback, this);
    determined_pose = false;
    std::string fname = ros::package::getPath("ros_collection_robot") + "/config/waypoints.yaml";

//    followWayPoints(fname);
}

void Navigator::lidarCallback(const sensor_msgs::LaserScan::ConstPtr& msg){
    geometry_msgs::Point point;
    for (int i = 0; i < msg->ranges.size(); ++i) {
        if (msg->ranges[i] < msg->range_min || msg->ranges[i] > msg->range_max)
            continue;

        double theta = (i*3.14)/180;
        double phi = theta+robot_rotation_;
        if (phi > 2*3.14)
            phi -= 2*3.14;

        point.x = msg->ranges[i]*cos(phi)+robot_location_.x-0.024;
        point.y = msg->ranges[i]*sin(phi)+robot_location_.y;

        if (!planner.map_.insideObstacle(point)){
            ROS_INFO_STREAM("Cube Detected");
        }
    }




}

void Navigator::odomCallback(const nav_msgs::Odometry::ConstPtr &msg){
    geometry_msgs::Pose robot_pose = msg->pose.pose;

    robot_location_ = robot_pose.position;
    geometry_msgs::Quaternion quat = robot_pose.orientation;

    tf::Quaternion q(quat.x, quat.y, quat.z, quat.w);
    tf::Matrix3x3 m(q);
    double roll, pitch, yaw;
    m.getRPY(roll, pitch, yaw);

    if (yaw < 0)
        yaw += 2*3.14;

    robot_rotation_ = yaw;
    determined_pose = true;
}

void Navigator::facePoint(geometry_msgs::Point goal_pt) {
    // determine heading necessary to face goal
    while (!determined_pose){
        ros::spinOnce();
    }

    double pi = 3.14159;
    double goal_heading = atan2(goal_pt.y - robot_location_.y,
                                goal_pt.x - robot_location_.x);

    if (goal_heading < 0)
        goal_heading += 2 * pi;

    // determine cw and ccw angles necessary to reach heading
    double diff = goal_heading - robot_rotation_;

    int direction = 1; //-1 for cw 1 for ccw
    if (diff < 0)
        direction = -1;

    if (abs(diff) > pi)
        direction *= -1;

    // rotate until heading is correct
    geometry_msgs::Twist vel;
    vel.angular.z = 0.25*direction;

    ros::Rate r(30);
    while(ros::ok()){
        ros::spinOnce();
        diff = goal_heading - robot_rotation_;
        if (abs(diff)<0.05) {
            break;
        }

        vel_pub_.publish(vel);
        r.sleep();
    }

    stop();
}

void Navigator::driveToPoint(geometry_msgs::Point goal) {
    double pi = 3.14159;
    double heading = atan2(goal.y - robot_location_.y,
                           goal.x - robot_location_.x);
    if (heading < 0)
        heading += 2 * pi;

    double diff = heading - robot_rotation_;

    if (abs(diff) > pi)
        diff = 2*pi - abs(diff);

    if (abs(diff) > 0.5){
        stop();
        ros::Duration(0.5).sleep();
        facePoint(goal);
    }

    geometry_msgs::Twist vel;
    vel.linear.x = 0.2;
    vel_pub_.publish(vel);

    ros::Rate r(30);
    double kp = 0.2;
    double max_speed = 0.3;

    while(ros::ok()){
        ros::spinOnce();
        double distance = sqrt(pow(goal.x-robot_location_.x,2)+
                    pow(goal.y-robot_location_.y,2));

        double heading = atan2(goal.y - robot_location_.y,
                                              goal.x - robot_location_.x);
        if (heading < 0)
            heading += 2 * pi;

        double diff = heading - robot_rotation_;

        int direction = 1; //-1 for cw 1 for ccw
        if (diff < 0)
            direction = -1;

        if (abs(diff) > pi){
            direction *= -1;
            diff = 2*pi - abs(diff);
        }

        double speed = abs(diff)*kp;
        if (speed > max_speed)
            speed = max_speed;

        vel.angular.z = direction*speed;

        vel_pub_.publish(vel);

        if (distance<0.1) {
            break;
        }
        r.sleep();
    }
}

void Navigator::stop(){
    geometry_msgs::Twist vel;
    vel.linear.x = 0;
    vel.angular.z = 0;
    vel_pub_.publish(vel);
}

void Navigator::followWayPoints(std::string fname) {
    YAML::Node data = YAML::LoadFile(fname);
    YAML::Node waypoint_data = data["waypoints"];
    geometry_msgs::Point point;
    for (int i = 0; i < waypoint_data.size(); ++i) {
        point.x = waypoint_data[i][0].as<double>();
        point.y = waypoint_data[i][1].as<double>();

        waypoints.push_back(point);
    }

    for (geometry_msgs::Point pt:waypoints) {
            ROS_INFO_STREAM("Driving to (" << pt.x << "," << pt.y << ")");
            driveToPoint(pt);
    }

    stop();
}

void Navigator::goToCollectionObject(){

}

void Navigator::driveToDropOff(){

}

void Navigator::returnToEulerPath(){

}

ros::NodeHandle Navigator::getNodeHandle(){
    return nh_;
}

int main(int argc, char **argv){
    ros::init(argc, argv, "navigator");
    Navigator nav;
    ros::NodeHandle nh = nav.getNodeHandle();

    ros::spin();

//    for(auto pt:nav.waypoints){
//        ROS_INFO_STREAM(pt);
//    }


//    OrderManager order_manager;
//    order_manager.generateOrder();
//    order_manager.spawnCubes();

//    Decoder decoder(nh);

//    ros::Rate r(1);
//    while(ros::ok()){
////        auto marker_ids = decoder.detectTags();
////        for (auto id:marker_ids){
////            ROS_INFO_STREAM("ID: " << id);
////        }
//
//        ros::spinOnce();
//        r.sleep();
//    }

//    ros::Rate r(1);
//
//    while(ros::ok()){
//
//        ros::spin();
//    }

//    PathPlanner planner;
//
//    geometry_msgs::Point start;
//    start.x = 2;
//    start.y = 2;
//
//    geometry_msgs::Point end;
//    end.x = 10;
//    end.y = 2;
//
//    if (planner.map_.insideObstacle(end)){
//        ROS_WARN_STREAM("Invalid goal position");
//    } else {
//        std::vector <geometry_msgs::Point> path;
//        path = planner.AStar(start, end);
//
//        ROS_INFO_STREAM("Length of path: " << path.size());
//
//        for (geometry_msgs::Point pt:path) {
//            ROS_INFO_STREAM("Driving to (" << pt.x << "," << pt.y << ")");
//            nav.driveToPoint(pt);
//        }
//        nav.stop();
//    }
}