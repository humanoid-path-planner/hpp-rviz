#!/usr/bin/env python

# Copyright (c) 2014, 2020 CNRS Author: Florent Lamiraux, Guilhem Saurel
#
# This file is part of hpp-gepetto-viewer. hpp-gepetto-viewer is free software:
# you can redistribute it and/or modify it under the terms of the GNU Lesser
# General Public License as published by the Free Software Foundation, either
# version 3 of the License, or (at your option) any later version.
#
# hpp-gepetto-viewer is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
# FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Lesser Public License
# for more details.  You should have received a copy of the GNU Lesser General
# Public License along with hpp-gepetto-viewer.  If not, see
# <http://www.gnu.org/licenses/>.

# dependency: python-rospkg

cmake_minimum_required(VERSION 3.22)
project(hpp_rviz VERSION 8.0.0 LANGUAGES C CXX)

set(PROJECT_USE_CMAKE_EXPORT TRUE)

find_package(ament_cmake REQUIRED)

find_package(rosidl_default_generators REQUIRED)
find_package(std_msgs REQUIRED)

set(HPP_RVIZ_MSG_TARGET ${PROJECT_NAME})
rosidl_generate_interfaces(${HPP_RVIZ_MSG_TARGET}
    src/msg/PathInfo.msg
    src/msg/PinocchioJoint.msg
    src/msg/HppVectorConfiguration.msg
    src/msg/HppLandMark.msg
    DEPENDENCIES std_msgs
)
ament_export_dependencies(rosidl_default_runtime)

rosidl_get_typesupport_target(cpp_typesupport_target
    ${HPP_RVIZ_MSG_TARGET} rosidl_typesupport_cpp
)

find_package(ament_cmake_ros REQUIRED)
find_package(pluginlib REQUIRED)
find_package(rviz_common REQUIRED)
find_package(rviz_default_plugins REQUIRED)
find_package(rviz_rendering REQUIRED)
find_package(sensor_msgs REQUIRED)
find_package(visualization_msgs REQUIRED)

set(HPP_RVIZ_PLUGIN_TARGET "hpp_ros_visualization")

set(MOC_FILE_TMP
    src/include/hpp/display/trajectory.hpp
    src/include/hpp/panel/trajectory_slider.hpp
    src/include/hpp/doubleSlider.hpp
    src/include/hpp/tool/landMark.hpp
    src/include/hpp/tool/interactiveLandMark.hpp
    src/include/hpp/display/landMark.hpp
    src/include/hpp/display/landMarkProperty.hpp
)

if(QT_VERSION_MAJOR EQUAL 6)
    qt6_wrap_cpp(MOC_FILES ${MOC_FILE_TMP})
else()
    qt5_wrap_cpp(MOC_FILES ${MOC_FILE_TMP})
endif()

add_library(
    ${HPP_RVIZ_PLUGIN_TARGET}
    src/src/display/trajectory.cpp
    src/src/panel/trajectory_slider.cpp
    src/src/panel/ui.cpp
    src/src/panel/communication.cpp
    src/src/tool/landMark.cpp
    src/src/display/landMark.cpp
    ${MOC_FILES}
)

target_link_libraries(
    ${HPP_RVIZ_PLUGIN_TARGET}
    PRIVATE
        pluginlib::pluginlib
        rviz_common::rviz_common
        rviz_default_plugins::rviz_default_plugins
        rviz_rendering::rviz_rendering
        ${sensor_msgs_TARGETS}
        ${std_msgs_TARGETS}
        ${visualization_msgs_TARGETS}
        ${cpp_typesupport_target}
)

install(
    TARGETS ${HPP_RVIZ_PLUGIN_TARGET}
    EXPORT export_${HPP_RVIZ_PLUGIN_TARGET}
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib
    RUNTIME DESTINATION bin
)

install(DIRECTORY src/include/ DESTINATION include)
install(FILES rviz_common_plugins.xml DESTINATION share/${PROJECT_NAME})
install(
    FILES src/icons/Trajectory.png src/icons/LandMark.png
    DESTINATION share/${PROJECT_NAME}/icons/classes
)
ament_export_include_directories(include)
ament_export_targets(export_${HPP_RVIZ_PLUGIN_TARGET})
pluginlib_export_plugin_description_file(rviz_common rviz_common_plugins.xml)

ament_package()
