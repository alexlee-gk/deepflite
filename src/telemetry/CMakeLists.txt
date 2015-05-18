include_directories(
  ${Boost_INCLUDE_DIRS}
  ${Mavlink_INCLUDE_DIRS}
  ${common_INCLUDE_DIRS}
)

add_library(telemetry MavlinkInterface.cpp MavlinkInterface.h)
target_link_libraries(telemetry ${Boost_LIBRARIES} common)
