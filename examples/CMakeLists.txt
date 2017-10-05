include_directories (${CMAKE_SOURCE_DIR}/dependencies/MQTTPacket/src)
include_directories (${CMAKE_SOURCE_DIR}/sources)

set(CMAKE_C_FLAGS "std=c89")

add_executable(MQTTexample main.c)

target_link_libraries(MQTTexample paho-embed-mqtt3c conn_lib pthread)

