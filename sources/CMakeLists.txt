include_directories (${CMAKE_SOURCE_DIR}/dependencies/MQTTPacket/src)

file(GLOB SOURCES "*.c")
add_library(conn_lib ${SOURCES})


target_link_libraries(conn_lib paho-embed-mqtt3c )

