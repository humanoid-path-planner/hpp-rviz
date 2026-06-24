#pragma once

#include <OgreVector3.h>

#include <geometry_msgs/msg/point_stamped.hpp>
#include <hpp_rviz/msg/hpp_land_mark.hpp>
#include <interactive_markers/interactive_marker_server.hpp>
#include <interactive_markers/menu_handler.hpp>
#include <rclcpp/rclcpp.hpp>
#include <rviz_common/display_context.hpp>
#include <rviz_common/interaction/selection_manager.hpp>
#include <rviz_common/interaction/view_picker_iface.hpp>
#include <rviz_common/ros_integration/ros_node_abstraction_iface.hpp>
#include <rviz_common/tool.hpp>
#include <visualization_msgs/msg/interactive_marker.hpp>

#include "interactiveLandMark.hpp"

namespace hpp {
namespace tool {

class LandMark : public rviz_common::Tool {
  Q_OBJECT
 public:
  LandMark();
  ~LandMark();

  void onInitialize() override;
  void activate() override;
  void deactivate() override;
  int processMouseEvent(rviz_common::ViewportMouseEvent& event) override;

 private:
  std::map<std::string, std::unique_ptr<InteractiveLandMark>>
      interactive_LandMarks_;
  rviz_common::ros_integration::RosNodeAbstractionIface::WeakPtr node_ptr_;
  std::shared_ptr<interactive_markers::InteractiveMarkerServer> server_;
  interactive_markers::MenuHandler menu_handler_;
  interactive_markers::MenuHandler::EntryHandle edit_menu_handle_;
  interactive_markers::MenuHandler::EntryHandle delete_menu_handle_;
  rclcpp::Subscription<geometry_msgs::msg::PoseStamped>::SharedPtr
      LandMark_sub_;
  rclcpp::Subscription<hpp_rviz::msg::HppLandMark>::SharedPtr
      LandMark_visibility_sub_;
  int LandMark_count_ = 0;

  void onLandMarkVisibilityReceived(
      const hpp_rviz::msg::HppLandMark::SharedPtr msg);
  void createInteractiveLandMark(const geometry_msgs::msg::PoseStamped& pos);
  void processFeedback(
      const visualization_msgs::msg::InteractiveMarkerFeedback::ConstSharedPtr&
          feedback);
  void onLandMarkReceived(const geometry_msgs::msg::PoseStamped::SharedPtr msg);
};

}  // namespace tool
}  // namespace hpp
