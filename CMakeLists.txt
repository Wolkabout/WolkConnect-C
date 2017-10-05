cmake_minimum_required(VERSION 2.8)

set(PROJECT_NAME WolkConnector)
project(${PROJECT_NAME})

set (CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/out/bin/")

add_subdirectory(dependencies/MQTTPacket/src)
add_subdirectory(examples)
add_subdirectory(sources)
