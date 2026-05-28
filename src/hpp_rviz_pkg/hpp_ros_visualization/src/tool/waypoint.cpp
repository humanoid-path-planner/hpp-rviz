#include <hpp/tool/waypoint.hpp>
#include <QDialog>
#include <QDialogButtonBox>
#include <QDoubleSpinBox>
#include <QFormLayout>
#include <QVBoxLayout>
#include <pluginlib/class_list_macros.hpp>
#include <rcutils/logging_macros.h>
#include <rviz_common/viewport_mouse_event.hpp>
#include "hpp/interactiveWaypoint.hpp"
#include "hpp/Position3d.hpp"
namespace hpp {
namespace tool {


Waypoint::Waypoint() = default;

Waypoint::~Waypoint() = default;

void Waypoint::onInitialize() {
    rviz_common::Tool::onInitialize();
    node_ptr_ = context_->getRosNodeAbstraction().lock();
    rclcpp::Node::SharedPtr node = node_ptr_.lock()->get_raw_node();

    server_ = std::make_shared<interactive_markers::InteractiveMarkerServer>(
        "hpp_waypoint_server", node);

    // Setup menu handler
   
    edit_menu_handle_ = menu_handler_.insert("Edit Position",
        std::bind(&Waypoint::processFeedback, this, std::placeholders::_1));
    delete_menu_handle_ = menu_handler_.insert("Delete",
        std::bind(&Waypoint::processFeedback, this, std::placeholders::_1));

    // Subscribe to waypoint topic for precise placement
    waypoint_sub_ = node->create_subscription<geometry_msgs::msg::PoseStamped>(
        "/hpp/waypoint", 10,
        std::bind(&Waypoint::onWaypointReceived, this, std::placeholders::_1));
}

void Waypoint::activate() {}

void Waypoint::deactivate() {}

int Waypoint::processMouseEvent(rviz_common::ViewportMouseEvent& event) {
   if (event.leftDown()) 
    {
        Ogre::Vector3 position_3d;
        if (context_->getViewPicker()->get3DPoint(
                event.panel,
                event.x,
                event.y,
                position_3d))
        {
            geometry_msgs::msg::PoseStamped pos_msg;
            pos_msg.header.frame_id = "world";
            pos_msg.pose.position.x = position_3d.x;
            pos_msg.pose.position.y = position_3d.y;
            pos_msg.pose.position.z = position_3d.z;
            pos_msg.pose.orientation.w = 1.0;
            createInteractiveWaypoint(pos_msg);
        
        }
    }

    return 0;
}

void Waypoint::createInteractiveWaypoint(const geometry_msgs::msg::PoseStamped& pos)
{
    struct Position3d position;
    position.x = pos.pose.position.x;
    position.y = pos.pose.position.y;
    position.z = pos.pose.position.z;
    position.qx = pos.pose.orientation.x;
    position.qy = pos.pose.orientation.y;
    position.qz = pos.pose.orientation.z;
    position.qw = pos.pose.orientation.w;

    const int waypoint_id = waypoint_count_++;

    InteractiveWaypoint int_marker(
        position,
        "waypoint_" + std::to_string(waypoint_id),
        "Waypoint_" + std::to_string(waypoint_id));

    server_->insert(
        int_marker,
        std::bind(
            &Waypoint::processFeedback,
            this,
            std::placeholders::_1));

    menu_handler_.apply(*server_, int_marker.name);
    server_->applyChanges();
}


void Waypoint::processFeedback(
    const visualization_msgs::msg::InteractiveMarkerFeedback::ConstSharedPtr& feedback)
{
    if (feedback->event_type ==
        visualization_msgs::msg::InteractiveMarkerFeedback::POSE_UPDATE)
    {
        RCUTILS_LOG_INFO(
            "Waypoint %s moved to [%.2f, %.2f, %.2f]",
            feedback->marker_name.c_str(),
            feedback->pose.position.x,
            feedback->pose.position.y,
            feedback->pose.position.z);
    }
    else if (feedback->event_type ==
        visualization_msgs::msg::InteractiveMarkerFeedback::MENU_SELECT)
    {
        
        if (feedback->menu_entry_id == edit_menu_handle_)
        {
            QDialog dialog;
            dialog.setWindowTitle("Edit waypoint position");

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

            auto* buttons = new QDialogButtonBox(
                QDialogButtonBox::Ok | QDialogButtonBox::Cancel,
                Qt::Horizontal,
                &dialog);
            layout->addWidget(buttons);

            QObject::connect(buttons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
            QObject::connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);

            if (dialog.exec() == QDialog::Accepted)
            {
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
        }
        else if (feedback->menu_entry_id == delete_menu_handle_)
        {
            server_->erase(feedback->marker_name);
            server_->applyChanges();
        }
    }
}

void Waypoint::onWaypointReceived(const geometry_msgs::msg::PoseStamped::SharedPtr msg)
{
    
    createInteractiveWaypoint(*msg);
}
} // namespace tool
} // namespace hpp

PLUGINLIB_EXPORT_CLASS(hpp::tool::Waypoint, rviz_common::Tool)
