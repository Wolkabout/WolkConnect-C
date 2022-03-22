/*
 * Copyright 2022 WolkAbout Technology s.r.o.
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

#include "model/attribute.h"
#include "model/feed.h"
#include "model/file_management/file_management.h"
#include "model/file_management/file_management_packet_request.h"
#include "model/file_management/file_management_parameter.h"
#include "model/file_management/file_management_status.h"
#include "model/firmware_update.h"
#include "model/outbound_message.h"
#include "model/parameter.h"
#include "model/utc_command.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif


const char JSON_P2D_TOPIC[TOPIC_DIRECTION_SIZE];
const char JSON_D2P_TOPIC[TOPIC_DIRECTION_SIZE];
const char JSON_FEED_REGISTRATION_TOPIC[TOPIC_MESSAGE_TYPE_SIZE];
const char JSON_FEED_REMOVAL_TOPIC[TOPIC_MESSAGE_TYPE_SIZE];
const char JSON_FEED_VALUES_MESSAGE_TOPIC[TOPIC_MESSAGE_TYPE_SIZE];
const char JSON_PULL_FEEDS_TOPIC[TOPIC_MESSAGE_TYPE_SIZE];
const char JSON_ATTRIBUTE_REGISTRATION_TOPIC[TOPIC_MESSAGE_TYPE_SIZE];
const char JSON_PARAMETERS_TOPIC[TOPIC_MESSAGE_TYPE_SIZE];
const char JSON_PULL_PARAMETERS_TOPIC[TOPIC_MESSAGE_TYPE_SIZE];
const char JSON_SYNC_PARAMETERS_TOPIC[TOPIC_MESSAGE_TYPE_SIZE];
const char JSON_SYNC_TIME_TOPIC[TOPIC_MESSAGE_TYPE_SIZE];
const char JSON_ERROR_TOPIC[TOPIC_MESSAGE_TYPE_SIZE];
const char JSON_DETAILS_SYNCHRONIZATION_TOPIC[TOPIC_MESSAGE_TYPE_SIZE];

const char JSON_FILE_MANAGEMENT_UPLOAD_INITIATE_TOPIC[TOPIC_MESSAGE_TYPE_SIZE];
const char JSON_FILE_MANAGEMENT_FILE_UPLOAD_STATUS_TOPIC[TOPIC_MESSAGE_TYPE_SIZE];
const char JSON_FILE_MANAGEMENT_FILE_UPLOAD_ABORT_TOPIC[TOPIC_MESSAGE_TYPE_SIZE];
const char JSON_FILE_MANAGEMENT_FILE_BINARY_REQUEST_TOPIC[TOPIC_MESSAGE_TYPE_SIZE];
const char JSON_FILE_MANAGEMENT_FILE_BINARY_RESPONSE_TOPIC[TOPIC_MESSAGE_TYPE_SIZE];
const char JSON_FILE_MANAGEMENT_URL_DOWNLOAD_INITIATE_TOPIC[TOPIC_MESSAGE_TYPE_SIZE];
const char JSON_FILE_MANAGEMENT_URL_DOWNLOAD_ABORT_TOPIC[TOPIC_MESSAGE_TYPE_SIZE];
const char JSON_FILE_MANAGEMENT_URL_DOWNLOAD_STATUS_TOPIC[TOPIC_MESSAGE_TYPE_SIZE];
const char JSON_FILE_MANAGEMENT_FILE_LIST_TOPIC[TOPIC_MESSAGE_TYPE_SIZE];
const char JSON_FILE_MANAGEMENT_FILE_DELETE_TOPIC[TOPIC_MESSAGE_TYPE_SIZE];
const char JSON_FILE_MANAGEMENT_FILE_PURGE_TOPIC[TOPIC_MESSAGE_TYPE_SIZE];

const char JSON_FIRMWARE_UPDATE_INSTALL_TOPIC[TOPIC_MESSAGE_TYPE_SIZE];
const char JSON_FIRMWARE_UPDATE_STATUS_TOPIC[TOPIC_MESSAGE_TYPE_SIZE];
const char JSON_FIRMWARE_UPDATE_ABORT_TOPIC[TOPIC_MESSAGE_TYPE_SIZE];

size_t json_serialize_feeds(feed_t* feeds, data_type_t type, size_t number_of_feeds, size_t feed_element_size,
                            char* buffer, size_t buffer_size);

size_t json_deserialize_feeds_value_message(char* buffer, size_t buffer_size, feed_t* feeds_received);

bool json_create_topic(char direction[TOPIC_DIRECTION_SIZE], const char device_key[DEVICE_KEY_SIZE],
                       char message_type[TOPIC_MESSAGE_TYPE_SIZE], char topic[TOPIC_SIZE]);

bool json_serialize_feed_registration(const char* device_key, feed_registration_t* feed, size_t number_of_feeds,
                                      outbound_message_t* outbound_message);
bool json_serialize_feed_removal(const char* device_key, feed_registration_t* feed, size_t number_of_feeds,
                                 outbound_message_t* outbound_message);
bool json_serialize_pull_feed_values(const char* device_key, outbound_message_t* outbound_message);
bool json_serialize_parameter(const char* device_key, parameter_t* parameter, size_t number_of_parameters,
                              outbound_message_t* outbound_message);
bool json_serialize_pull_parameters(const char* device_key, outbound_message_t* outbound_message);
bool json_serialize_sync_parameters(const char* device_key, parameter_t* parameters, size_t number_of_parameters,
                                    outbound_message_t* outbound_message);
size_t json_deserialize_parameter_message(char* buffer, size_t buffer_size, parameter_t* parameter_message);

bool json_serialize_attribute(const char* device_key, attribute_t* attributes, size_t number_of_attributes,
                              outbound_message_t* outbound_message);
bool json_serialize_sync_time(const char* device_key, outbound_message_t* outbound_message);
bool json_serialize_sync_details_synchronization(const char* device_key, outbound_message_t* outbound_message);
bool json_deserialize_time(char* buffer, size_t buffer_size, utc_command_t* utc_command);
bool json_deserialize_details_synchronization(char* buffer, size_t buffer_size, feed_registration_t* feeds,
                                              size_t* number_of_feeds, attribute_t* attributes,
                                              size_t* number_of_attributes);

bool json_serialize_file_management_status(const char* device_key,
                                           file_management_packet_request_t* file_management_packet_request,
                                           file_management_status_t* status, outbound_message_t* outbound_message);

bool json_deserialize_file_management_parameter(char* buffer, size_t buffer_size,
                                                file_management_parameter_t* parameter);

size_t json_deserialize_file_delete(char* buffer, size_t buffer_size, file_list_t* file_list);

bool json_serialize_file_management_packet_request(const char* device_key,
                                                   file_management_packet_request_t* file_management_packet_request,
                                                   outbound_message_t* outbound_message);

bool json_serialize_file_management_url_download_status(const char* device_key,
                                                        file_management_parameter_t* file_management_parameter,
                                                        file_management_status_t* status,
                                                        outbound_message_t* outbound_message);
bool json_serialize_file_management_file_list_update(const char* device_key, file_list_t* file_list,
                                                     size_t file_list_items, outbound_message_t* outbound_message);

bool json_deserialize_firmware_update_parameter(char* device_key, char* buffer, size_t buffer_size,
                                                firmware_update_t* parameter);
bool json_serialize_firmware_update_status(const char* device_key, firmware_update_t* firmware_update,
                                           outbound_message_t* outbound_message);

#ifdef __cplusplus
}
#endif

#endif
