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
include(ExternalProject)

set(PAHO_MQTT_EMBEDDED_C_VERSION "1.1.0")

if(NOT DEFINED CMAKE_PREFIX_PATH)
    set(CMAKE_PREFIX_PATH "${CMAKE_CURRENT_BINARY_DIR}/build")
endif(NOT DEFINED CMAKE_PREFIX_PATH)
include_directories(SYSTEM "${CMAKE_PREFIX_PATH}/include")

ExternalProject_Add(pahomqttembeddedc
        SOURCE_DIR "${CMAKE_CURRENT_LIST_DIR}/pahomqttembeddedc"
        GIT_REPOSITORY "https://github.com/eclipse/paho.mqtt.embedded-c"
        GIT_TAG "v${PAHO_MQTT_EMBEDDED_C_VERSION}"
        UPDATE_COMMAND ""
        CMAKE_ARGS -DCMAKE_LIBRARY_OUTPUT_DIRECTORY=${CMAKE_LIBRARY_OUTPUT_DIRECTORY} -DCMAKE_INSTALL_PREFIX=${CMAKE_PREFIX_PATH} -DCMAKE_PREFIX_PATH=${CMAKE_PREFIX_PATH} -DCMAKE_INSTALL_RPATH='.'
        )
