include_directories (${CMAKE_SOURCE_DIR}/dependencies/MQTTPacket/src)

#link_directories(${CMAKE_SOURCE_DIR}/out/lib)

add_executable(MQTTexample main.c)

target_link_libraries(MQTTexample paho-embed-mqtt3c)

