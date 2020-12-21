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

#ifndef OUTBOUND_MESSAGE_FACTORY_H
#define OUTBOUND_MESSAGE_FACTORY_H

#include "actuator_status.h"
#include "file_management_status.h"
#include "outbound_message.h"
#include "parser.h"
#include "reading.h"
#include "size_definitions.h"

#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

size_t outbound_message_make_from_readings(parser_t* parser, const char* device_key, reading_t* first_reading,
                                           size_t num_readings, outbound_message_t* outbound_message);

bool outbound_message_make_from_actuator_status(parser_t* parser, const char* device_key,
                                                actuator_status_t* actuator_status, const char* reference,
                                                outbound_message_t* outbound_message);

bool outbound_message_make_from_configuration(parser_t* parser, const char* device_key,
                                              char (*reference)[CONFIGURATION_REFERENCE_SIZE],
                                              char (*value)[CONFIGURATION_VALUE_SIZE], size_t num_configuration_items,
                                              outbound_message_t* outbound_message);

bool outbound_message_make_from_file_management_status(parser_t* parser, const char* device_key,
                                                       file_management_packet_request_t* file_management_packet_request,
                                                       file_management_status_t* file_management_status,
                                                       outbound_message_t* outbound_message);

bool outbound_message_make_from_file_management_url_download_status(
    parser_t* parser, const char* device_key, file_management_parameter_t* file_management_parameter,
    file_management_status_t* status, outbound_message_t* outbound_message);

bool outbound_message_make_from_file_management_packet_request(
    parser_t* parser, const char* device_key, file_management_packet_request_t* file_management_packet_request,
    outbound_message_t* outbound_message);

bool outbound_message_make_from_keep_alive_message(parser_t* parser, const char* device_key,
                                                   outbound_message_t* outbound_message);

#ifdef __cplusplus
}
#endif

#endif
