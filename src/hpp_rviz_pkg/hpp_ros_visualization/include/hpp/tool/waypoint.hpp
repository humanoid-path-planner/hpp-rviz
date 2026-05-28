#pragma once


#include <rviz_common/tool.hpp>
#include <visualization_msgs/msg/interactive_marker.hpp>
#include <interactive_markers/menu_handler.hpp>
#include <interactive_markers/interactive_marker_server.hpp>
#include <rviz_common/ros_integration/ros_node_abstraction_iface.hpp>
#include <rviz_common/display_context.hpp>
#include <rviz_common/interaction/selection_manager.hpp>
#include <rclcpp/rclcpp.hpp>
#include <OgreVector3.h>
#include <rviz_common/interaction/view_picker_iface.hpp>
#include <geometry_msgs/msg/point_stamped.hpp>


namespace hpp {
namespace tool {

class Waypoint: public rviz_common::Tool {
    Q_OBJECT
    public:
        Waypoint();
        ~Waypoint();

        void onInitialize() override;
        void activate() override;
        void deactivate() override;
        int processMouseEvent(rviz_common::ViewportMouseEvent& event) override;
    private:
        rviz_common::ros_integration::RosNodeAbstractionIface::WeakPtr node_ptr_;
        std::shared_ptr<interactive_markers::InteractiveMarkerServer> server_;
        interactive_markers::MenuHandler menu_handler_;
        interactive_markers::MenuHandler::EntryHandle edit_menu_handle_;
        interactive_markers::MenuHandler::EntryHandle delete_menu_handle_;
        rclcpp::Subscription<geometry_msgs::msg::PoseStamped>::SharedPtr waypoint_sub_;
        int waypoint_count_ = 0;

        void createInteractiveWaypoint(const geometry_msgs::msg::PoseStamped& pos);
        void processFeedback(const visualization_msgs::msg::InteractiveMarkerFeedback::ConstSharedPtr& feedback);
        void onWaypointReceived(const geometry_msgs::msg::PoseStamped::SharedPtr msg);

};

}
}