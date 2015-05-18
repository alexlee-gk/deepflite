include_directories(
  ${Boost_INCLUDE_DIRS}
  ${Eigen_INCLUDE_DIRS}
  ${OpenCV_INCLUDE_DIRS}
  ${AprilTags_INCLUDE_DIRS}
  ${telemetry_INCLUDE_DIRS}
  ${mavlink_INCLUDE_DIRS}
)

add_executable(show show.cpp)
target_link_libraries(show ${Boost_LIBRARIES} ${OpenCV_LIBS} apriltags telemetry mavlink)
