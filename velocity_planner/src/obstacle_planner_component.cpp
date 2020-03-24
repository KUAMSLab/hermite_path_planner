#include <velocity_planner/obstacle_planner_component.h>
#include <hermite_path_planner/hermite_path_generator.h>
#include <color_names/color_names.h>

namespace velocity_planner
{
    ObstaclePlannerComponent::ObstaclePlannerComponent(const rclcpp::NodeOptions & options)
    : Node("obstacle_planner", options),
        buffer_(get_clock()),
        listener_(buffer_),
        viz_(get_name())
    {
        marker_pub_ = this->create_publisher<visualization_msgs::msg::MarkerArray>
            ("~/marker", 1);
        obstacle_marker_pub_ = this->create_publisher<visualization_msgs::msg::MarkerArray>
            ("~/obstacle/marker", 1);
        hermite_path_pub_ = this->create_publisher<hermite_path_msgs::msg::HermitePathStamped>
            ("~/hermite_path", 1);
        update_pub_ = this->create_publisher<hermite_path_msgs::msg::HermitePathStamped>
            ("/planner_concatenator/update", 1);
        std::string hermite_path_topic;
        declare_parameter("hermite_path_topic","/hermite_path_planner/hermite_path");
        get_parameter("hermite_path_topic",hermite_path_topic);
        hermite_path_sub_ = this->create_subscription<hermite_path_msgs::msg::HermitePathStamped>
            (hermite_path_topic, 1, std::bind(&ObstaclePlannerComponent::hermitePathCallback, this, std::placeholders::_1));
        std::string obstacle_scan_topic;
        declare_parameter("obstacle_scan_topic","/obstacle_scan");
        get_parameter("obstacle_scan_topic",obstacle_scan_topic);
        declare_parameter("robot_width",1.5);
        get_parameter("robot_width",robot_width_);
        declare_parameter("stop_margin",1.5);
        get_parameter("stop_margin",stop_margin_);
        scan_sub_ = this->create_subscription<sensor_msgs::msg::LaserScan>
            (obstacle_scan_topic, 1, std::bind(&ObstaclePlannerComponent::scanCallback, this, std::placeholders::_1));
        std::string current_pose_topic;
        declare_parameter("current_pose_topic","/current_pose");
        get_parameter("current_pose_topic",current_pose_topic);
        current_pose_sub_ = this->create_subscription<geometry_msgs::msg::PoseStamped>
            (current_pose_topic, 1, std::bind(&ObstaclePlannerComponent::currentPoseCallback, this, std::placeholders::_1));
    }

    void ObstaclePlannerComponent::currentPoseCallback(const geometry_msgs::msg::PoseStamped::SharedPtr data)
    {
        current_pose_ = *data;
    }

    void ObstaclePlannerComponent::hermitePathCallback(const hermite_path_msgs::msg::HermitePathStamped::SharedPtr data)
    {
        path_ = *data;
        path_ = addObstacleConstraints();
        if(path_)
        {
            marker_pub_->publish(viz_.generateDeleteMarker());
            marker_pub_->publish(viz_.generateMarker(path_.get(),color_names::makeColorMsg("lime",1.0)));
            hermite_path_pub_->publish(path_.get());
        }
        else
        {
            marker_pub_->publish(viz_.generateDeleteMarker());
        }
    }

    geometry_msgs::msg::PoseStamped ObstaclePlannerComponent::TransformToMapFrame(geometry_msgs::msg::PoseStamped pose)
    {
        if(pose.header.frame_id == "map")
        {
            return pose;
        }
        tf2::TimePoint time_point = tf2::TimePoint(
            std::chrono::seconds(pose.header.stamp.sec) +
            std::chrono::nanoseconds(pose.header.stamp.nanosec));
        geometry_msgs::msg::TransformStamped transform_stamped = 
            buffer_.lookupTransform("map", pose.header.frame_id,
                time_point, tf2::durationFromSec(1.0));
        tf2::doTransform(pose, pose, transform_stamped);
        return pose;
    }

    geometry_msgs::msg::PointStamped ObstaclePlannerComponent::TransformToMapFrame(geometry_msgs::msg::PointStamped point)
    {
        if(point.header.frame_id == "map")
        {
            return point;
        }
        tf2::TimePoint time_point = tf2::TimePoint(
            std::chrono::seconds(point.header.stamp.sec) +
            std::chrono::nanoseconds(point.header.stamp.nanosec));
        geometry_msgs::msg::TransformStamped transform_stamped = 
            buffer_.lookupTransform("map", point.header.frame_id,
                time_point, tf2::durationFromSec(1.0));
        tf2::doTransform(point, point, transform_stamped);
        return point;
    }

    boost::optional<hermite_path_msgs::msg::HermitePathStamped> ObstaclePlannerComponent::addObstacleConstraints()
    {
        if(!path_ || !current_pose_ || !scan_)
        {
            return boost::none;
        }
        hermite_path_planner::HermitePathGenerator generator(0.0);
        geometry_msgs::msg::PoseStamped pose_transformed;
        tf2::TimePoint time_point = tf2::TimePoint(
            std::chrono::seconds(current_pose_->header.stamp.sec) +
            std::chrono::nanoseconds(current_pose_->header.stamp.nanosec));
        geometry_msgs::msg::TransformStamped transform_stamped = 
            buffer_.lookupTransform(path_.get().header.frame_id, current_pose_->header.frame_id,
                time_point, tf2::durationFromSec(1.0));
        tf2::doTransform(current_pose_.get(), pose_transformed, transform_stamped);
        auto current_t = generator.getLongitudinalDistanceInFrenetCoordinate(path_->path,pose_transformed.pose.position);
        if(!current_t)
        {
            return boost::none;
        }
        std::set<double> t_values;
        for(int i=0; i<(int)scan_->ranges.size(); i++)
        {
            if(scan_->range_max >= scan_->ranges[i]  && scan_->ranges[i] >= scan_->range_min)
            {
                double theta = scan_->angle_min + scan_->angle_increment * (double)i;
                geometry_msgs::msg::PointStamped p;
                p.point.x = scan_->ranges[i] * std::cos(theta);
                p.point.y = scan_->ranges[i] * std::sin(theta);
                p.point.z = 0.0;
                p.header = scan_->header;
                p = TransformToMapFrame(p);
                auto t_value = generator.getLongitudinalDistanceInFrenetCoordinate(path_->path,p.point);
                if(t_value)
                {
                    geometry_msgs::msg::Point nearest_point = generator.getPointOnHermitePath(path_->path,t_value.get());
                    double lat_dist = std::sqrt(std::pow(nearest_point.x-p.point.x,2)+std::pow(nearest_point.y-p.point.y,2));
                    if(std::fabs(lat_dist) < std::fabs(robot_width_) && t_value.get() > current_t.get())
                    {
                        t_values.insert(t_value.get());
                    }
                }
            }
        }
        if(t_values.size() != 0)
        {
            double t = *t_values.begin();
            double length = generator.getLength(path_.get().path,200);
            double target_t = t - (stop_margin_/length);
            hermite_path_msgs::msg::HermitePathStamped path = path_.get();
            path.reference_velocity.clear();
            hermite_path_msgs::msg::ReferenceVelocity v;
            v.linear_velocity = 0.0;
            v.t = target_t;
            path.reference_velocity.push_back(v);
            obstacle_marker_pub_->publish(viz_.generateDeleteMarker());
            obstacle_marker_pub_->publish(viz_.generateObstacleMarker(t,path_.get(),color_names::makeColorMsg("red",0.8)));
            return path;
        }
        else
        {
            obstacle_marker_pub_->publish(viz_.generateDeleteMarker());
            return path_.get();
        }
        return boost::none;
    }

    void ObstaclePlannerComponent::scanCallback(const sensor_msgs::msg::LaserScan::SharedPtr data)
    {
        scan_ = *data;
        path_ = addObstacleConstraints();
        if(path_ && path_.get().reference_velocity.size() != 0)
        {
            update_pub_->publish(path_.get());
        }
    }
}