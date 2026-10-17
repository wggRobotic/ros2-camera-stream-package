cmake_minimum_required(VERSION 3.8)
project(camera_stream_package)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

find_package(ament_cmake REQUIRED)
find_package(rclcpp REQUIRED)
find_package(OpenCV REQUIRED)
find_package(PkgConfig REQUIRED)
find_package(Threads REQUIRED)

pkg_check_modules(GSTREAMER REQUIRED IMPORTED_TARGET
  gstreamer-1.0
  gstreamer-app-1.0
  gstreamer-rtsp-server-1.0
)

add_executable(camera_stream_node
  src/camera_stream_node.cpp
)

ament_target_dependencies(camera_stream_node
  rclcpp
)

target_include_directories(camera_stream_node PRIVATE
  ${OpenCV_INCLUDE_DIRS}
)

target_link_libraries(camera_stream_node
  PkgConfig::GSTREAMER
  Threads::Threads
  ${OpenCV_LIBS}
)

install(TARGETS camera_stream_node
  DESTINATION lib/${PROJECT_NAME}
)

if(BUILD_TESTING)
  find_package(ament_lint_auto REQUIRED)
  set(ament_cmake_copyright_FOUND TRUE)
  set(ament_cmake_cpplint_FOUND TRUE)
  ament_lint_auto_find_test_dependencies()
endif()

ament_package()
