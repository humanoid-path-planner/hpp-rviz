#include "../../include/hpp/display/landmark.hpp"

#include <QMetaObject>
#include <QStringList>
#include <rviz_common/properties/bool_property.hpp>
#include <rviz_common/properties/color_property.hpp>
#include <rviz_common/properties/float_property.hpp>
#include <rviz_common/properties/property.hpp>

namespace hpp {
namespace displays {

void LandmarkDisplay::onInitialize() {
  rviz_default_plugins::displays::InteractiveMarkerDisplay::onInitialize();

  group_property_ = new rviz_common::properties::Property(
      "Landmarks", QVariant(), "Landmarks description", this);

  const std::string server_topic = "/hpp_landmark_server";
  setTopic(QString::fromStdString(server_topic), "");

  auto* show_axes = findProperty("Show Axes");
  if (show_axes) {
    show_axes->setValue(true);
    show_axes->hide();
  }

  // Cacher d'autres options du parent
  auto* show_visual_aids = findProperty("Show Visual Aids");
  if (show_visual_aids) show_visual_aids->hide();

  auto* enable_transparency = findProperty("Enable Transparency");
  if (enable_transparency) enable_transparency->hide();

  node_ptr_ = context_->getRosNodeAbstraction();
  auto node_lock = node_ptr_.lock();
  if (!node_lock) {
    setStatus(rviz_common::properties::StatusProperty::Error, "Landmark",
              "Unable to access ROS node.");
    return;
  }
  auto node = node_lock->get_raw_node();

  auto update_qos = rclcpp::QoS(rclcpp::KeepLast(100)).reliable();
  update_sub_ = node->create_subscription<
      visualization_msgs::msg::InteractiveMarkerUpdate>(
      server_topic + "/update", update_qos,
      std::bind(&LandmarkDisplay::onUpdateMessage, this,
                std::placeholders::_1));

  Landmark_visiblilty_pub_ = node->create_publisher<hpp_rviz::msg::Landmark>(
      server_topic + "/landmark_visibility", 10);

  Landmark_visiblilty_pub_ = node->create_publisher<hpp_rviz::msg::Landmark>(
      server_topic + "/landmark_visibility", 10);

  setStatus(rviz_common::properties::StatusProperty::Ok, "Landmark",
            "Listening for interactive marker updates.");
}

void LandmarkDisplay::onLandmarkEnabledChanged(const std::string& name,
                                               bool enabled) {
  RCUTILS_LOG_INFO("Landmark '%s' enabled changed to %s", name.c_str(),
                   enabled ? "true" : "false");
  auto it = Landmark_properties_.find(name);
  if (it == Landmark_properties_.end()) return;
  hpp_rviz::msg::Landmark msg;
  msg.name = name;
  msg.enable = enabled;
  Landmark_visiblilty_pub_->publish(msg);
}

void LandmarkDisplay::onUpdateMessage(
    const visualization_msgs::msg::InteractiveMarkerUpdate::SharedPtr msg) {
  QMetaObject::invokeMethod(
      this,
      [this, msg]() {
        for (const auto& marker : msg->markers) {
          if (Landmark_properties_.count(marker.name)) {
            continue;
          }
          auto wp = std::make_unique<LandmarkProperty>(
              QString::fromStdString(marker.name),
              QString::fromStdString("Landmark " + marker.name),
              marker.header.frame_id,
              Ogre::Vector3((float)marker.pose.position.x,
                            (float)marker.pose.position.y,
                            (float)marker.pose.position.z),
              Ogre::Quaternion((float)marker.pose.orientation.w,
                               (float)marker.pose.orientation.x,
                               (float)marker.pose.orientation.y,
                               (float)marker.pose.orientation.z),
              group_property_);

          connect(wp.get(), &LandmarkProperty::enabledChanged, this,
                  &LandmarkDisplay::onLandmarkEnabledChanged);

          Landmark_properties_.emplace(marker.name, std::move(wp));
        }

        for (const auto& erased_name : msg->erases) {
          auto it = Landmark_properties_.find(erased_name);
          if (it != Landmark_properties_.end()) {
            it->second->removeFromParent();
            Landmark_properties_.erase(it);
          }
        }

        for (const auto& marker : msg->poses) {
          auto it = Landmark_properties_.find(marker.name);
          if (it != Landmark_properties_.end()) {
            it->second->setPosition(Ogre::Vector3(
                (float)marker.pose.position.x, (float)marker.pose.position.y,
                (float)marker.pose.position.z));
            it->second->setOrientation(
                Ogre::Quaternion((float)marker.pose.orientation.w,
                                 (float)marker.pose.orientation.x,
                                 (float)marker.pose.orientation.y,
                                 (float)marker.pose.orientation.z));
          }
        }
      },
      Qt::QueuedConnection);
}

}  // namespace displays
}  // namespace hpp

#include <pluginlib/class_list_macros.hpp>
PLUGINLIB_EXPORT_CLASS(hpp::displays::LandmarkDisplay, rviz_common::Display)
