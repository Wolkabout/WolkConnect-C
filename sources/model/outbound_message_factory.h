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

#include "model/attribute.h"
#include "model/feed.h"
#include "model/file_management/file_management_status.h"
#include "model/outbound_message.h"
#include "model/reading.h"
#include "protocol/parser.h"
#include "size_definitions.h"

#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

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

bool outbound_message_make_from_file_management_file_list(parser_t* parser, const char* device_key, char* file_list,
                                                          size_t file_list_items, outbound_message_t* outbound_message);

bool outbound_message_make_from_firmware_update_status(parser_t* parser, const char* device_key,
                                                       firmware_update_t* firmware_update,
                                                       outbound_message_t* outbound_message);

bool outbound_message_make_from_firmware_update_version(parser_t* parser, const char* device_key,
                                                        char* firmware_update_version,
                                                        outbound_message_t* outbound_message);

bool outbound_message_feed_registration(parser_t* parser, const char* device_key, feed_t* feed, size_t number_of_feeds,
                                        outbound_message_t* outbound_message);

bool outbound_message_feed_removal(parser_t* parser, const char* device_key, feed_t* feed, size_t number_of_feeds,
                                   outbound_message_t* outbound_message);

size_t outbound_message_make_from_readings(parser_t* parser, const char* device_key, reading_t* readings,
                                           data_type_t type, size_t readings_number, size_t reading_element_size,
                                           outbound_message_t* outbound_message);

bool outbound_message_pull_feed_values(parser_t* parser, const char* device_key, outbound_message_t* outbound_message);

bool outbound_message_attribute_registration(parser_t* parser, const char* device_key, attribute_t* attribute,
                                             outbound_message_t* outbound_message);

bool outbound_message_update_parameters(parser_t* parser, const char* device_key, parameter_t* parameter,
                                        size_t number_of_parameters, outbound_message_t* outbound_message);

bool outbound_message_pull_parameters(parser_t* parser, const char* device_key, outbound_message_t* outbound_message);

bool outbound_message_synchronize_parameters(parser_t* parser, const char* device_key, parameter_t* parameters,
                                             size_t number_of_parameters, outbound_message_t* outbound_message);

bool outbound_message_synchronize_time(parser_t* parser, const char* device_key, outbound_message_t* outbound_message);

#ifdef __cplusplus
}
#endif

#endif
