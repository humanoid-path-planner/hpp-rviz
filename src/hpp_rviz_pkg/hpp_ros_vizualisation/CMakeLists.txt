cmake_minimum_required(VERSION 3.5)
project(hpp_ros_vizualisation)

if(NOT CMAKE_C_STANDARD)
    set(CMAKE_C_STANDARD 99)
endif()
if(NOT CMAKE_CXX_STANDARD)
    set(CMAKE_CXX_STANDARD 17)
endif()
if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

find_package(Qt6 QUIET COMPONENTS Widgets)
find_package(Qt5 QUIET COMPONENTS Widgets)
find_package(ament_cmake REQUIRED)
find_package(ament_cmake_ros REQUIRED)
find_package(pluginlib REQUIRED)
find_package(rviz_common REQUIRED)
find_package(rviz_default_plugins REQUIRED)
find_package(rviz_rendering REQUIRED)
find_package(sensor_msgs REQUIRED)
find_package(std_msgs REQUIRED)
find_package(hpp_msgs REQUIRED)

get_target_property(
    _rviz_common_link
    rviz_common::rviz_common
    INTERFACE_LINK_LIBRARIES
)
if("${_rviz_common_link}" MATCHES "Qt6::")
    set(QT_VERSION_MAJOR 6)
elseif("${_rviz_common_link}" MATCHES "Qt5::")
    set(QT_VERSION_MAJOR 5)
elseif(Qt6_FOUND)
    set(QT_VERSION_MAJOR 6)
elseif(Qt5_FOUND)
    set(QT_VERSION_MAJOR 5)
else()
    message(FATAL_ERROR "Neither Qt6 nor Qt5 was found")
endif()

set(CMAKE_AUTOMOC ON)
if(QT_VERSION_MAJOR EQUAL 6)
    qt6_wrap_cpp(MOC_FILES
      include/hpp/plugin/trajectory.hpp
      include/hpp/panel/trajectory_slider.hpp
      include/hpp/doubleSlider.hpp
    )
else()
    qt5_wrap_cpp(MOC_FILES
      include/hpp/plugin/trajectory.hpp
      include/hpp/panel/trajectory_slider.hpp
      include/hpp/doubleSlider.hpp
    )
endif()

add_library(
    trajectory
    src/plugin/trajectory.cpp
    src/panel/trajectory_slider.cpp
    src/panel/ui.cpp
    src/panel/communication.cpp
    ${MOC_FILES}
)

target_include_directories(
    trajectory
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

ament_target_dependencies(trajectory
  pluginlib
  rviz_common
  rviz_default_plugins
  rviz_rendering
  sensor_msgs
  std_msgs
  hpp_msgs
)

target_link_libraries(
    trajectory
    pluginlib::pluginlib
    rviz_common::rviz_common
    rviz_rendering::rviz_rendering
)

install(
    TARGETS trajectory
    EXPORT export_hpp_trajectory_plugin
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib
    RUNTIME DESTINATION bin
)
install(DIRECTORY include/ DESTINATION include)
install(FILES rviz_common_plugins.xml DESTINATION share/${PROJECT_NAME})
install(
    FILES icons/Trajectory.png
    DESTINATION share/${PROJECT_NAME}/icons/classes
)

ament_export_include_directories(include)
ament_export_targets(export_hpp_trajectory_plugin)
pluginlib_export_plugin_description_file(rviz_common rviz_common_plugins.xml)

if(BUILD_TESTING)
    find_package(ament_lint_auto REQUIRED)
    ament_lint_auto_find_test_dependencies()
endif()

ament_package()
