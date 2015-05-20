include_directories(
  ${Boost_INCLUDE_DIRS}
  ${Eigen_INCLUDE_DIRS}
  ${OpenCV_INCLUDE_DIRS}
  ${AprilTags_INCLUDE_DIRS}
  ${telemetry_INCLUDE_DIRS}
  ${mavlink_INCLUDE_DIRS}
)

add_library(camera utils.cpp utils.h)

add_executable(show show.cpp)
target_link_libraries(show camera ${Boost_LIBRARIES} ${OpenCV_LIBS} apriltags telemetry mavlink)

add_executable(intrinsic_calib intrinsic_calib.cpp)
target_link_libraries(intrinsic_calib camera ${Boost_LIBRARIES} ${OpenCV_LIBS} apriltags)
