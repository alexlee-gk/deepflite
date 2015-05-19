
file(GLOB Mavlink_HEADERS "*.h")
set(Mavlink_HEADERS  ${Mavlink_HEADERS} ${CMAKE_SOURCE_DIR}/ext/mavlink/common)


add_library(mavlink serial_port.cpp ${Mavlink_HEADERS})

