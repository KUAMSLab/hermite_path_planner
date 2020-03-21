#include <velocity_planner/stop_planner_component.h>
#include <color_names/color_names.h>

namespace velocity_planner
{
    StopPlannerComponent::StopPlannerComponent(const rclcpp::NodeOptions & options)
    : Node("stop_planner", options), generator_(0.0), viz_(get_name())
    {
        marker_pub_ = this->create_publisher<visualization_msgs::msg::MarkerArray>
            ("~/marker", 1);
        hermite_path_pub_ = this->create_publisher<hermite_path_msgs::msg::HermitePathStamped>
            ("~/hermite_path", 1);
        std::string hermite_path_topic;
        declare_parameter("hermite_path_topic","/hermite_path_planner/hermite_path");
        get_parameter("hermite_path_topic",hermite_path_topic);
        declare_parameter("max_deceleration",0.1);
        get_parameter("max_deceleration",max_deceleration_);
        declare_parameter("max_linear_velocity",0.5);
        get_parameter("max_linear_velocity",max_linear_velocity_);
        declare_parameter("section_length",0.5);
        get_parameter("section_length",section_length_);
        hermite_path_sub_ = this->create_subscription<hermite_path_msgs::msg::HermitePathStamped>
            (hermite_path_topic, 1, std::bind(&StopPlannerComponent::hermitePathCallback, this, std::placeholders::_1));
    }

    void StopPlannerComponent::hermitePathCallback(const hermite_path_msgs::msg::HermitePathStamped::SharedPtr data)
    {
        path_ = *data;
        double length = generator_.getLength(path_.get().path,200);
        int i = 0;
        while(true)
        {
            double t = (length - ((double)i*section_length_))/length;
            hermite_path_msgs::msg::ReferenceVelocity ref;
            ref.t = t;
            ref.linear_velocity = std::sqrt(2*max_deceleration_*(1-t));
            if(ref.linear_velocity > max_linear_velocity_)
            {
                break;
            }
            if(ref.t <= 0.0)
            {
                break;
            }
            ref.from_node = get_name();
            path_.get().reference_velocity.push_back(ref);
            i++;
        }
        marker_pub_->publish(viz_.generateDeleteMarker());
        marker_pub_->publish(viz_.generateMarker(path_.get(),color_names::makeColorMsg("magenta",1.0)));
        hermite_path_pub_->publish(path_.get());
    }
}