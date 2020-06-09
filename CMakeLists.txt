#
# Copyright 2018 WolkAbout Technology s.r.o.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#
cmake_minimum_required(VERSION 3.10)

project(WolkConnector-C C)

set(CMAKE_C_STANDARD 99)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -pedantic                         \
                                    -Wall                             \
                                    -Wextra                           \
                                    -Wstrict-prototypes               \
                                    -Wpointer-arith                   \
                                    -Wmissing-declarations            \
                                    -Wno-switch-enum                  \
                                    -Wno-bad-function-cast            \
                                    -Wnested-externs                  \
                                    -Wunreachable-code                \
                                    -Wfloat-equal                     \
                                    -Winline                          \
                                    -Wundef                           \
                                    -Wno-redundant-decls              \
                                    -Wold-style-definition            \
                                    -Wno-conversion                   \
                                    -Wshadow                          \
                                    -Wparentheses                     \
                                    -Wsequence-point                  \
                                    -Wno-declaration-after-statement  \
                                    -Wuninitialized                   \
                                    -Wcast-align                      \
                                    -Wshadow                          \
                                    -Wcast-qual                       \
                                    -Wstrict-prototypes               \
                                    -Wmissing-prototypes              \
                                    -Wconversion")

set(LIBRARY_NAME "WolkConnector-C")

include_directories("sources")
file(GLOB_RECURSE HEADER_FILES "sources/*.h")
file(GLOB_RECURSE SOURCE_FILES "sources/*.c")

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/out/bin")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/out/lib")

link_directories(${CMAKE_LIBRARY_OUTPUT_DIRECTORY})

#Dependencies
add_subdirectory(dependencies)
include_directories(dependencies/pahomqttembeddedc/MQTTPacket/src)

add_library(${LIBRARY_NAME} SHARED ${HEADER_FILES} ${SOURCE_FILES})
target_link_libraries(${LIBRARY_NAME} paho-embed-mqtt3c ssl crypto)
set_target_properties(${LIBRARY_NAME} PROPERTIES LINK_FLAGS "-Wl,-rpath,./")

add_dependencies(${LIBRARY_NAME} pahomqttembeddedc)

# Format code
add_custom_target(format
                  COMMAND "clang-format" -i -sort-includes -style=file ${HEADER_FILES} ${SOURCE_FILES}
                  WORKING_DIRECTORY "${CMAKE_BINARY_DIR}"
                  COMMENT "[Formatting source code]"
                  VERBATIM)

# Run Unit tests
set(TEST_FILES "${CMAKE_SOURCE_DIR}/tests")

add_custom_target(tests
        WORKING_DIRECTORY "${TEST_FILES}"
        COMMAND ceedling test:all
        COMMENT "[Run Unit tests]"
        VERBATIM)

# Examples
add_subdirectory(examples/simple)
add_subdirectory(examples/full_feature_set)
