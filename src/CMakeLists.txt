include_directories(
  ${CMAKE_SOURCE_DIR}/ext
  ${CMAKE_SOURCE_DIR}/src
)

add_subdirectory(common)
add_subdirectory(camera)
add_subdirectory(telemetry)

