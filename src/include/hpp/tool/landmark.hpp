#pragma once

#include <OgreVector3.h>

#include <geometry_msgs/msg/point_stamped.hpp>
#include <hpp_rviz/msg/landmark.hpp>
#include <interactive_markers/interactive_marker_server.hpp>
#include <interactive_markers/menu_handler.hpp>
#include <rclcpp/rclcpp.hpp>
#include <rviz_common/display_context.hpp>
#include <rviz_common/interaction/selection_manager.hpp>
#include <rviz_common/interaction/view_picker_iface.hpp>
#include <rviz_common/ros_integration/ros_node_abstraction_iface.hpp>
#include <rviz_common/tool.hpp>
#include <visualization_msgs/msg/interactive_marker.hpp>

#include "hpp_rviz/msg/landmark.hpp"
#include "interactiveLandmark.hpp"

namespace hpp {
namespace tool {

class Landmark : public rviz_common::Tool {
  Q_OBJECT
 public:
  Landmark();
  ~Landmark();

  void onInitialize() override;
  void activate() override;
  void deactivate() override;
  int processMouseEvent(rviz_common::ViewportMouseEvent& event) override;

 private:
  std::map<std::string, std::unique_ptr<InteractiveLandmark>>
      interactive_Landmarks_;
  rviz_common::ros_integration::RosNodeAbstractionIface::WeakPtr node_ptr_;
  std::shared_ptr<interactive_markers::InteractiveMarkerServer> server_;
  interactive_markers::MenuHandler menu_handler_;
  interactive_markers::MenuHandler::EntryHandle edit_menu_handle_;
  interactive_markers::MenuHandler::EntryHandle delete_menu_handle_;
  rclcpp::Subscription<hpp_rviz::msg::Landmark>::SharedPtr
      Landmark_sub_;
  rclcpp::Subscription<hpp_rviz::msg::Landmark>::SharedPtr
      Landmark_visibility_sub_;
  int Landmark_count_ = 0;

  void onLandmarkVisibilityReceived(
      const hpp_rviz::msg::Landmark::SharedPtr msg);
  void createInteractiveLandmark(Ogre::Vector3 translation, Ogre::Quaternion orientation, std::string name);
  void processFeedback(
      const visualization_msgs::msg::InteractiveMarkerFeedback::ConstSharedPtr&
          feedback);
  void onLandmarkReceived(const hpp_rviz::msg::Landmark msg);
};

}  // namespace tool
}  // namespace hpp
