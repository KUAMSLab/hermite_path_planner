#ifndef VELOCITY_PLANNER_VELOCITY_VISUALIZER_H_INCLUDED
#define VELOCITY_PLANNER_VELOCITY_VISUALIZER_H_INCLUDED

#include <hermite_path_planner/hermite_path_generator.h>
#include <quaternion_operation/quaternion_operation.h>
#include <hermite_path_msgs/msg/hermite_path_stamped.hpp>
#include <rclcpp/rclcpp.hpp>
#include <visualization_msgs/msg/marker_array.hpp>

namespace velocity_planner
{
class VelocityVisualizer
{
public:
  VelocityVisualizer(std::string node_name);
  visualization_msgs::msg::MarkerArray generateMarker(
    hermite_path_msgs::msg::HermitePathStamped path);
  visualization_msgs::msg::MarkerArray generateMarker(
    hermite_path_msgs::msg::HermitePathStamped path, std_msgs::msg::ColorRGBA color_ref_velocity);
  visualization_msgs::msg::MarkerArray generateDeleteMarker();
  visualization_msgs::msg::MarkerArray generatePolygonMarker(
    hermite_path_msgs::msg::HermitePathStamped path, double ratio = 0.0);
  visualization_msgs::msg::MarkerArray generateObstacleMarker(
    double t, hermite_path_msgs::msg::HermitePathStamped path, std_msgs::msg::ColorRGBA color,
    double width = 5.0);

private:
  std::string node_name_;
  hermite_path_planner::HermitePathGenerator generator_;
  double getVelocity(
    std::vector<hermite_path_msgs::msg::ReferenceVelocity> vel, double t, double l);
};
}  // namespace velocity_planner

#endif  //VELOCITY_PLANNER_VELOCITY_VISUALIZER_H_INCLUDED