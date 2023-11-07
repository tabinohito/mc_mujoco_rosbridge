set(INCLUDE_INSTALL_DIR "include")
add_library(Sensors SHARED MujocoSensors.cpp)
target_include_directories(
  Sensors PUBLIC $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
                       $<INSTALL_INTERFACE:include>)
target_include_directories(Sensors PRIVATE ${MUJOCO_INCLUDE_DIR})
target_link_libraries(Sensors PUBLIC ${LIB_MUJOCO})
if(DEFINED CATKIN_DEVEL_PREFIX)
  target_include_directories(Sensors PUBLIC ${catkin_INCLUDE_DIRS})
  target_link_libraries(Sensors PUBLIC ${catkin_LIBRARIES})
  add_dependencies(Sensors ${${PROJECT_NAME}_EXPORTED_TARGETS})
  add_compile_definitions(ENABLE_ROS)
endif()

add_library(SensorsPlugin SHARED plugin.cpp)
target_include_directories(SensorsPlugin PRIVATE ${MUJOCO_INCLUDE_DIR})
target_link_libraries(SensorsPlugin PUBLIC Sensors)

install(
  TARGETS Sensors SensorsPlugin
  EXPORT "${TARGETS_EXPORT_NAME}"
  LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}"
  ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}"
  RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}")
install(DIRECTORY ${PROJECT_SOURCE_DIR}/include/mc_mujoco_rosbridge
        DESTINATION "${INCLUDE_INSTALL_DIR}")
