include_directories (${CMAKE_SOURCE_DIR}/dependencies/MQTTPacket/src)
include_directories (${CMAKE_SOURCE_DIR}/sources)

#link_directories(${CMAKE_SOURCE_DIR}/out/lib)

add_executable(MQTTexample main.c)

target_link_libraries(MQTTexample paho-embed-mqtt3c conn_lib)

