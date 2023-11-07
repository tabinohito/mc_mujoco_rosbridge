cmake_minimum_required(VERSION 3.0.2)
project(mc_mujoco_rosbridge)

set(PROJECT_NAME mc_mujoco_rosbridge)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(PROJECT_GENERATED_HEADERS_SKIP_DEPRECATED ON)
set(PROJECT_GENERATED_HEADERS_SKIP_CONFIG ON)
set(PROJECT_GENERATED_HEADERS_SKIP_WARNING ON)
set(PROJECT_DESCRIPTION "")
set(PROJECT_USE_CMAKE_EXPORT TRUE)
set(CXX_DISABLE_WERROR ON)
set(CMAKE_COLOR_DIAGNOSTICS ON)

# MuJoCo
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}")
find_package(mujoco REQUIRED)
find_library(GLFW libglfw.so.3)

if(NOT DEFINED MUJOCO_ROOT_DIR)
  message(FATAL_ERROR "Set \"MUJOCO_ROOT_DIR\"")
elseif(NOT EXISTS ${MUJOCO_ROOT_DIR})
  message(FATAL_ERROR "\"MUJOCO_ROOT_DIR\" is invalid path: ${MUJOCO_ROOT_DIR}")
endif()
set(MUJOCO_INCLUDE_DIR "${MUJOCO_ROOT_DIR}/include")
set(MUJOCO_LIB_DIR "${MUJOCO_ROOT_DIR}/lib")
set(MUJOCO_BIN_DIR "${MUJOCO_ROOT_DIR}/bin")
file(GLOB LIB_MUJOCO ${MUJOCO_BIN_DIR}/libmujoco[0-9][0-9][0-9].so
     ${MUJOCO_LIB_DIR}/libmujoco.so.*)
if(EXISTS ${LIB_MUJOCO})
  message(STATUS "MuJoCo lib found: " ${LIB_MUJOCO})
else()
  message(FATAL_ERROR "MuJoCo lib not found.")
endif()

if(DEFINED CATKIN_DEVEL_PREFIX)
  set(DOXYGEN_HTML_OUTPUT html)

  find_package(catkin REQUIRED COMPONENTS 
    message_generation 
    roscpp 
    geometry_msgs
    visualization_msgs 
    eigen_conversions
    pluginlib
    rospy
    urdf
    tf2
    tf2_ros
    std_msgs
    backward_ros REQUIRED
    message_generation #この行を追加)
  )

  add_message_files(
    FILES
    ScalarStamped.msg
  )

  generate_messages(
    DEPENDENCIES
      std_msgs
      geometry_msgs
  )

  catkin_package(
    CATKIN_DEPENDS
    message_runtime
    roscpp
    pluginlib
    urdf
    tf2
    tf2_ros
    geometry_msgs
    visualization_msgs
    eigen_conversions
    INCLUDE_DIRS
    include
    LIBRARIES
    MujocoSensors)
else()
  set(DOXYGEN_HTML_OUTPUT doxygen-html)
endif()

add_subdirectory(src)
# add_subdirectory(xml)

# if(DEFINED CATKIN_DEVEL_PREFIX)
#   add_subdirectory(node)
# endif()

# if(INSTALL_DOCUMENTATION)
#   add_subdirectory(doc)
# endif()
