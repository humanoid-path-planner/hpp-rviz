#include "../../include/hpp/tool/landmark.hpp"

#include <QDialog>
#include <QDialogButtonBox>
#include <QDoubleSpinBox>
#include <QFormLayout>
#include <QVBoxLayout>
#include <pluginlib/class_list_macros.hpp>
#include <rviz_common/viewport_mouse_event.hpp>
#include <string>

#include "hpp_rviz/msg/landmark.hpp"

namespace hpp {
namespace tool {

Landmark::Landmark() = default;

Landmark::~Landmark() = default;

void Landmark::onInitialize() {
  rviz_common::Tool::onInitialize();
  node_ptr_ = context_->getRosNodeAbstraction().lock();
  rclcpp::Node::SharedPtr node = node_ptr_.lock()->get_raw_node();

  server_ = std::make_shared<interactive_markers::InteractiveMarkerServer>(
      "hpp_landmark_server", node);

  // Setup menu handler

  Landmark_sub_ = node->create_subscription<hpp_rviz::msg::Landmark>(
      "/hpp_landmark_server/landmark", 10,
      std::bind(&Landmark::onLandmarkReceived, this, std::placeholders::_1));

  Landmark_visibility_sub_ = node->create_subscription<hpp_rviz::msg::Landmark>(
      "/hpp_landmark_server/landmark_visibility", 10,
      std::bind(&Landmark::onLandmarkVisibilityReceived, this,
                std::placeholders::_1));
}

void Landmark::activate() {}
void Landmark::deactivate() {}

void Landmark::onLandmarkVisibilityReceived(
    const hpp_rviz::msg::Landmark::SharedPtr msg) {
  std::string name = msg->name;
  bool enable = msg->enable;
  QMetaObject::invokeMethod(
      this,
      [this, name, enable]() {
        auto it = interactive_Landmarks_.find(name);
        if (it == interactive_Landmarks_.end()) {
          RCUTILS_LOG_WARN("Landmark '%s' not found", name.c_str());
          return;
        }

        visualization_msgs::msg::InteractiveMarker marker_to_insert =
            *(it->second);
        if (!enable) {
          marker_to_insert.scale = 0.001f;
          marker_to_insert.controls[0].markers[0].scale.x = 0.001f;
          marker_to_insert.controls[0].markers[0].scale.y = 0.001f;
          marker_to_insert.controls[0].markers[0].scale.z = 0.001f;
        }

        server_->insert(
            marker_to_insert,
            std::bind(&Landmark::processFeedback, this, std::placeholders::_1));

        if (enable) {
          menu_handler_.apply(*server_, marker_to_insert.name);
        }
        server_->applyChanges();
      },
      Qt::QueuedConnection);
}

int Landmark::processMouseEvent(rviz_common::ViewportMouseEvent& event) {
  if (event.leftDown()) {
    Ogre::Vector3 position_3d;
    if (context_->getViewPicker()->get3DPoint(event.panel, event.x, event.y,
                                              position_3d)) {
      Ogre::Quaternion quat(1, 0, 0, 0);
      createInteractiveLandmark(position_3d, quat, "");
    }
  }

  return 0;
}

void Landmark::createInteractiveLandmark(Ogre::Vector3 translation,
                                         Ogre::Quaternion orientation,
                                         std::string name) {
  const int Landmark_id = Landmark_count_++;
  if (name.empty()) name = "Landmark_" + std::to_string(Landmark_id);

  InteractiveLandmark int_marker(translation, orientation, name,
                                 "Landmark_" + std::to_string(Landmark_id));

  server_->insert(int_marker, std::bind(&Landmark::processFeedback, this,
                                        std::placeholders::_1));

  menu_handler_.apply(*server_, int_marker.name);
  interactive_Landmarks_[int_marker.name] =
      std::make_unique<InteractiveLandmark>(int_marker);
  server_->applyChanges();
}

void Landmark::processFeedback(
    const visualization_msgs::msg::InteractiveMarkerFeedback::ConstSharedPtr&
        feedback) {
  if (feedback->event_type ==
      visualization_msgs::msg::InteractiveMarkerFeedback::POSE_UPDATE) {
    server_->setPose(feedback->marker_name, feedback->pose);
    server_->applyChanges();
    auto it = interactive_Landmarks_.find(feedback->marker_name);
    if (it != interactive_Landmarks_.end()) {
      it->second->pose = feedback->pose;
    }
  } else if (feedback->event_type ==
             visualization_msgs::msg::InteractiveMarkerFeedback::MENU_SELECT) {
    if (feedback->menu_entry_id == edit_menu_handle_) {
      QDialog dialog;
      dialog.setWindowTitle("Edit Landmark position");

      auto* layout = new QVBoxLayout(&dialog);
      auto* form = new QFormLayout();

      auto makeSpin = [](double value, double min, double max, double step) {
        auto* spin = new QDoubleSpinBox();
        spin->setRange(min, max);
        spin->setDecimals(6);
        spin->setSingleStep(step);
        spin->setValue(value);
        return spin;
      };

      auto* x_spin = makeSpin(feedback->pose.position.x, -1e6, 1e6, 0.01);
      auto* y_spin = makeSpin(feedback->pose.position.y, -1e6, 1e6, 0.01);
      auto* z_spin = makeSpin(feedback->pose.position.z, -1e6, 1e6, 0.01);

      auto* qx_spin = makeSpin(feedback->pose.orientation.x, -1.0, 1.0, 0.01);
      auto* qy_spin = makeSpin(feedback->pose.orientation.y, -1.0, 1.0, 0.01);
      auto* qz_spin = makeSpin(feedback->pose.orientation.z, -1.0, 1.0, 0.01);
      auto* qw_spin = makeSpin(feedback->pose.orientation.w, -1.0, 1.0, 0.01);
      form->addRow("X:", x_spin);
      form->addRow("Y:", y_spin);
      form->addRow("Z:", z_spin);
      form->addRow("Qx:", qx_spin);
      form->addRow("Qy:", qy_spin);
      form->addRow("Qz:", qz_spin);
      form->addRow("Qw:", qw_spin);

      layout->addLayout(form);

      auto* buttons =
          new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel,
                               Qt::Horizontal, &dialog);
      layout->addWidget(buttons);

      QObject::connect(buttons, &QDialogButtonBox::accepted, &dialog,
                       &QDialog::accept);
      QObject::connect(buttons, &QDialogButtonBox::rejected, &dialog,
                       &QDialog::reject);

      if (dialog.exec() == QDialog::Accepted) {
        geometry_msgs::msg::Pose pose = feedback->pose;
        pose.position.x = x_spin->value();
        pose.position.y = y_spin->value();
        pose.position.z = z_spin->value();
        pose.orientation.x = qx_spin->value();
        pose.orientation.y = qy_spin->value();
        pose.orientation.z = qz_spin->value();
        pose.orientation.w = qw_spin->value();

        server_->setPose(feedback->marker_name, pose);
        server_->applyChanges();
      }
    } else if (feedback->menu_entry_id == delete_menu_handle_) {
      server_->erase(feedback->marker_name);
      server_->applyChanges();
    }
  }
}

void Landmark::onLandmarkReceived(const hpp_rviz::msg::Landmark landmark) {
  Ogre::Vector3 translation(landmark.tx, landmark.ty, landmark.tz);
  Ogre::Quaternion orientation(landmark.ow, landmark.ox, landmark.oy,
                               landmark.oz);
  createInteractiveLandmark(translation, orientation, landmark.name);
}
}  // namespace tool
}  // namespace hpp

PLUGINLIB_EXPORT_CLASS(hpp::tool::Landmark, rviz_common::Tool)
