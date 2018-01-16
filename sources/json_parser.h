/*
 * Copyright 2017-2018 WolkAbout Technology s.r.o.
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

#include "configuration_item.h"
#include "configuration_item_command.h"
#include "firmware_update_command.h"
#include "firmware_update_packet_request.h"
#include "firmware_update_status.h"
#include "outbound_message.h"
#include "reading.h"

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

size_t json_serialize_configuration_items(configuration_item_t* first_config_item, size_t num_config_items,
                                          char* buffer, size_t buffer_size);

size_t json_deserialize_configuration_items(char* buffer, size_t buffer_size,
                                            configuration_item_command_t* commands_buffer, size_t commands_buffer_size);

bool json_serialize_firmware_update_status(const char* device_key, firmware_update_status_t* status,
                                           outbound_message_t* outbound_message);

bool json_deserialize_firmware_update_command(char* buffer, size_t buffer_size, firmware_update_command_t* command);

bool json_serialize_firmware_update_packet_request(const char* device_key,
                                                   firmware_update_packet_request_t* firmware_update_packet_request,
                                                   outbound_message_t* outbound_message);

bool json_serialize_firmware_update_version(const char* device_key, const char* version,
                                            outbound_message_t* outbound_message);

#ifdef __cplusplus
}
#endif

#endif
