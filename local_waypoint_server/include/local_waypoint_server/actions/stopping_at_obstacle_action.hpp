// Copyright (c) 2020 OUXT Polaris
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#ifndef LOCAL_WAYPOINT_SERVER__ACTIONS__STOPPING_AT_OBSTACLE_ACTION_HPP_
#define LOCAL_WAYPOINT_SERVER__ACTIONS__STOPPING_AT_OBSTACLE_ACTION_HPP_

#include <behaviortree_cpp_v3/action_node.h>
#include <behaviortree_cpp_v3/bt_factory.h>
#include <string>
#include <memory>

namespace local_waypoint_server
{
class StoppingAtObstacleAction : public BT::SyncActionNode
{
public:
  explicit StoppingAtObstacleAction(const std::string & name, const BT::NodeConfiguration & config);
  BT::NodeStatus tick() override;
  static BT::PortsList providedPorts();
};
}  // local_waypoint_server

#endif  // LOCAL_WAYPOINT_SERVER__ACTIONS__STOPPING_AT_OBSTACLE_ACTION_HPP_
