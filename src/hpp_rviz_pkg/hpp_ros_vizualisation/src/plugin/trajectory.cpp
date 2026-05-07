
#include <hpp/plugin/trajectory.hpp>

namespace hpp {

void TrajectoryDisplay::onInitialize() {
  rviz_default_plugins::displays::PathDisplay::onInitialize();

  auto* window_manager = context_->getWindowManager();
  if (window_manager) {
    hpp::TrajectorySlider* hpp_panel = new hpp::TrajectorySlider();
    hpp_panel->initialize(context_);
    window_manager->addPane("HPP Trajectory Control", hpp_panel);
  }

  setTopic("/hpp_path", "");
}

}  // namespace hpp

#include <pluginlib/class_list_macros.hpp>
PLUGINLIB_EXPORT_CLASS(hpp::TrajectoryDisplay, rviz_common::Display)
