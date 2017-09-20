include_directories (${CMAKE_SOURCE_DIR}/dependencies/MQTTPacket/src)

#link_directories(${CMAKE_SOURCE_DIR}/out/lib)
file(GLOB SOURCES "*.c")
add_library(conn_lib ${SOURCES})

#add_executable(MQTTexample main.c)

target_link_libraries(conn_lib paho-embed-mqtt3c )

