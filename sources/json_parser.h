/*
 * Copyright 2018 WolkAbout Technology s.r.o.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef JSON_PARSER_H
#define JSON_PARSER_H

#include "actuator_command.h"
#include "configuration_command.h"
#include "configuration_item.h"
#include "file_management_packet_request.h"
#include "file_management_parameter.h"
#include "file_management_status.h"
#include "outbound_message.h"
#include "reading.h"
#include "utc_command.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

size_t json_serialize_readings(reading_t* first_reading, size_t num_readings, char* buffer, size_t buffer_size);

size_t json_deserialize_actuator_commands(char* topic, size_t topic_size, char* buffer, size_t buffer_size,
                                          actuator_command_t* commands_buffer, size_t commands_buffer_size);

bool json_serialize_readings_topic(reading_t* first_Reading, size_t num_readings, const char* device_key, char* buffer,
                                   size_t buffer_size);

size_t json_serialize_configuration(const char* device_key, char (*reference)[CONFIGURATION_REFERENCE_SIZE],
                                    char (*value)[CONFIGURATION_VALUE_SIZE], size_t num_configuration_items,
                                    outbound_message_t* outbound_message);

size_t json_deserialize_configuration_command(char* buffer, size_t buffer_size,
                                              configuration_command_t* commands_buffer, size_t commands_buffer_size);

bool json_serialize_file_management_status(const char* device_key,
                                           file_management_packet_request_t* file_management_packet_request,
                                           file_management_status_t* status, outbound_message_t* outbound_message);

bool json_deserialize_file_management_parameter(char* buffer, size_t buffer_size,
                                                file_management_parameter_t* parameter);

bool json_serialize_file_management_packet_request(const char* device_key,
                                                   file_management_packet_request_t* file_management_packet_request,
                                                   outbound_message_t* outbound_message);

bool json_serialize_file_management_url_download_status(const char* device_key,
                                                        file_management_parameter_t* file_management_parameter,
                                                        file_management_status_t* status,
                                                        outbound_message_t* outbound_message);
bool json_serialize_file_management_file_list_update(const char* device_key, char* file_list, size_t file_list_items,
                                                     outbound_message_t* outbound_message);

bool json_serialize_ping_keep_alive_message(const char* device_key, outbound_message_t* outbound_message);
bool json_deserialize_pong_keep_alive_message(char* buffer, size_t buffer_size, utc_command_t* utc_command);

#ifdef __cplusplus
}
#endif

#endif
