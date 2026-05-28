#pragma once
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QSlider>
#include <QTimer>
#include <QTreeWidget>
#include <QWidget>
#include <hpp/doubleSlider.hpp>
#include <hpp_msgs/msg/path_info.hpp>
#include <hpp_msgs/msg/pinocchio_joint.hpp>
#include <hpp_msgs/msg/hpp_vector_configuration.hpp>
#include <rclcpp/rclcpp.hpp>
#include <rviz_common/display_context.hpp>
#include <rviz_common/panel.hpp>
#include <rviz_common/ros_integration/ros_node_abstraction_iface.hpp>
#include <sensor_msgs/msg/joint_state.hpp>
#include <QPlainTextEdit>
#include <QApplication>
#include <QClipboard>

namespace hpp {

enum ObjType { FreeFlyer, Others };

const double trajectoryFps = 100.0;

class TrajectorySlider : public rviz_common::Panel {
  Q_OBJECT

 public:
  explicit TrajectorySlider(QWidget* parent = nullptr);
  ~TrajectorySlider() override;

  void onInitialize() override;

 private Q_SLOTS:
  void onSlide(int value);
  void onTogglePlay();
  void onTimer();
  void onTValueChanged(double value);
  void onComboChanged(const QString& text);

 private:
  void onPlay();
  void onPause();
  void onPathInfoReceive(const hpp_msgs::msg::PathInfo::SharedPtr msg);
  void publishTime();
  void updateTimeDisplay();
  void initializeUi();
  void onJointValueChanged(std::string name, double value);
  void onFreeFlyerValueChanged(std::string name, double value, int index);
  void onSceneObjReceive(
      const hpp_msgs::msg::HppVectorConfiguration::SharedPtr msg);

  std::map<std::string, QTreeWidgetItem*> objPosInSceneTree_;
  std::map<std::string, std::array<double, 7>> freeflyerValues_;

  // Widgets
  QLabel* info_label_;
  QSlider* slider_;
  QLabel* time_label_;
  QPushButton* toogle_play_button_;

  QLabel* t_label_;
  QDoubleSpinBox* t_spinbox_;
  QHBoxLayout* t_layout_;

  QLabel* speed_label_;
  QDoubleSpinBox* speed_spinbox_;
  QHBoxLayout* speed_layout_;

  QLabel* target_frame_label_;
  QComboBox* target_frame_selector_;
  QHBoxLayout* target_frame_layout_;

  QTimer* play_timer_;

  QTreeWidget* tree_;
  QTreeWidgetItem* grpJoints_;

  QPlainTextEdit* hpp_vector_configuration_edit_;
  QPushButton* copy_button_;

  QTreeWidgetItem* addJointSliderItem(QTreeWidgetItem* parent,
                                      const std::string& name,
                                      const QString& label, double min,
                                      double max, double defaultVal);
  void addFreeFlyerSliderItem(QTreeWidgetItem* parent, const std::string& name,
                              double min, double max,
                              const std::vector<double>& defaultVal);
  QTreeWidgetItem* createSliderTreeItem(QTreeWidgetItem* parent,
                                        const QString& label, double min,
                                        double max, double defaultVal);
  double computeStep(double min, double max);

  QTreeWidgetItem* getOrCreateNamespaceItem(QTreeWidgetItem* parent,
                                            const QString& ns);
  // State
  double path_length_{0.0};
  double current_time_{0.0};
  double speed_{1.0};
  bool is_playing_{false};
  // ROS
  rviz_common::ros_integration::RosNodeAbstractionIface::WeakPtr node_ptr_;
  rclcpp::Publisher<hpp_msgs::msg::PathInfo>::SharedPtr time_pub_;
  rclcpp::Subscription<hpp_msgs::msg::PathInfo>::SharedPtr length_sub_;
  rclcpp::Subscription<hpp_msgs::msg::HppVectorConfiguration>::SharedPtr
      scene_obj_sub_;
  rclcpp::Publisher<hpp_msgs::msg::PathInfo>::SharedPtr target_frame_pub_;
  rclcpp::Publisher<hpp_msgs::msg::PinocchioJoint>::SharedPtr joint_value_pub_;
};

}  // namespace hpp
