#pragma once
#include <hpp_rviz/msg/hpp_land_mark.hpp>
#include <rclcpp/rclcpp.hpp>
#include <rviz_common/display.hpp>
#include <rviz_common/display_context.hpp>
#include <rviz_common/properties/int_property.hpp>
#include <rviz_common/properties/string_property.hpp>
#include <rviz_common/ros_integration/ros_node_abstraction_iface.hpp>
#include <rviz_default_plugins/displays/interactive_markers/interactive_marker_display.hpp>
#include <string>
#include <visualization_msgs/msg/interactive_marker_init.hpp>
#include <visualization_msgs/msg/interactive_marker_update.hpp>

#include "landMarkProperty.hpp"

namespace hpp {

namespace displays {
class LandMarkDisplay
    : public rviz_default_plugins::displays::InteractiveMarkerDisplay {
  Q_OBJECT
 public:
  LandMarkDisplay() = default;
  ~LandMarkDisplay() = default;

  void onInitialize() override;

 private:
  void onLandMarkEnabledChanged(const std::string& name, bool enabled);
  void onUpdateMessage(
      const visualization_msgs::msg::InteractiveMarkerUpdate::SharedPtr msg);

  std::map<std::string, std::unique_ptr<LandMarkProperty>> LandMark_properties_;

  rviz_common::properties::Property* group_property_{nullptr};

  rviz_common::ros_integration::RosNodeAbstractionIface::WeakPtr node_ptr_;

  rclcpp::Subscription<
      visualization_msgs::msg::InteractiveMarkerUpdate>::SharedPtr update_sub_;
  rclcpp::Publisher<hpp_rviz::msg::HppLandMark>::SharedPtr
      LandMark_visiblilty_pub_;
};

}  // namespace displays
}  // namespace hpp
