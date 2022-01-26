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

#ifndef PARSER_H
#define PARSER_H

#include "model/attribute.h"
#include "model/feed.h"
#include "model/feed_value_message.h"
#include "model/file_management/file_management_packet_request.h"
#include "model/file_management/file_management_parameter.h"
#include "model/file_management/file_management_status.h"
#include "model/firmware_update.h"
#include "model/outbound_message.h"
#include "model/parameter.h"
#include "model/reading.h"
#include "model/utc_command.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    bool is_initialized;

    char FILE_MANAGEMENT_UPLOAD_INITIATE_TOPIC[TOPIC_SIZE];
    char FILE_MANAGEMENT_CHUNK_UPLOAD_TOPIC[TOPIC_SIZE];
    char FILE_MANAGEMENT_UPLOAD_ABORT_TOPIC[TOPIC_SIZE];

    char FILE_MANAGEMENT_URL_DOWNLOAD_INITIATE_TOPIC[TOPIC_SIZE];
    char FILE_MANAGEMENT_URL_DOWNLOAD_ABORT_TOPIC[TOPIC_SIZE];

    char FILE_MANAGEMENT_FILE_DELETE_TOPIC[TOPIC_SIZE];
    char FILE_MANAGEMENT_FILE_PURGE_TOPIC[TOPIC_SIZE];

    char FIRMWARE_UPDATE_INSTALL_TOPIC[TOPIC_SIZE];
    char FIRMWARE_UPDATE_ABORT_TOPIC[TOPIC_SIZE];

    char P2D_TOPIC[TOPIC_SIZE];
    char D2P_TOPIC[TOPIC_SIZE];
    char FEED_REGISTRATION_TOPIC[TOPIC_SIZE];
    char FEED_REMOVAL_TOPIC[TOPIC_SIZE];
    char FEED_VALUES_MESSAGE_TOPIC[TOPIC_SIZE];
    char PULL_FEED_VALUES_TOPIC[TOPIC_SIZE];
    char ATTRIBUTE_REGISTRATION_TOPIC[TOPIC_SIZE];
    char PARAMETERS_TOPIC[TOPIC_SIZE];
    char PULL_PARAMETERS_TOPIC[TOPIC_SIZE];
    char SYNC_PARAMETERS_TOPIC[TOPIC_SIZE];
    char SYNC_TIME_TOPIC[TOPIC_SIZE];
    char ERROR_TOPIC[TOPIC_SIZE];

    size_t (*serialize_readings)(reading_t* readings, size_t num_readings, char* buffer, size_t buffer_size);

    bool (*serialize_file_management_status)(const char* device_key,
                                             file_management_packet_request_t* file_management_packet_request,
                                             file_management_status_t* status, outbound_message_t* outbound_message);
    bool (*deserialize_file_management_parameter)(char* buffer, size_t buffer_size,
                                                  file_management_parameter_t* parameter);
    bool (*serialize_file_management_packet_request)(const char* device_key,
                                                     file_management_packet_request_t* file_management_packet_request,
                                                     outbound_message_t* outbound_message);
    bool (*serialize_file_management_url_download_status)(const char* device_key,
                                                          file_management_parameter_t* file_management_parameter,
                                                          file_management_status_t* status,
                                                          outbound_message_t* outbound_message);
    bool (*serialize_file_management_file_list)(const char* device_key, char* file_list, size_t file_list_items,
                                                outbound_message_t* outbound_message);

    bool (*deserialize_firmware_update_parameter)(char* device_key, char* buffer, size_t buffer_size,
                                                  firmware_update_t* parameter);
    bool (*serialize_firmware_update_status)(const char* device_key, firmware_update_t* firmware_update,
                                             outbound_message_t* outbound_message);
    bool (*serialize_firmware_update_version)(const char* device_key, char* firmware_update_version,
                                              outbound_message_t* outbound_message);

    bool (*deserialize_time)(char* buffer, size_t buffer_size, utc_command_t* utc_command);

    bool (*create_topic)(char direction[DIRECTION_SIZE], const char device_key[DEVICE_KEY_SIZE],
                         char message_type[MESSAGE_TYPE_SIZE], char topic[TOPIC_SIZE]);
    size_t (*deserialize_feed_value_message)(char* buffer, size_t buffer_size, feed_value_message_t* feed_value_message,
                                             size_t msg_buffer_size);
    size_t (*deserialize_parameter_message)(char* buffer, size_t buffer_size, parameter_t* parameter_message,
                                            size_t msg_buffer_size);
    bool (*serialize_feed_registration)(const char* device_key, feed_t* feed, outbound_message_t* outbound_message);
    bool (*serialize_feed_removal)(const char* device_key, feed_t* feed, outbound_message_t* outbound_message);
    bool (*serialize_pull_feed_values)(const char* device_key, outbound_message_t* outbound_message);
    bool (*serialize_pull_parameters)(const char* device_key, outbound_message_t* outbound_message);
    bool (*serialize_sync_parameters)(const char* device_key, outbound_message_t* outbound_message);
    bool (*serialize_sync_time)(const char* device_key, outbound_message_t* outbound_message);
    bool (*serialize_attribute)(const char* device_key, attribute_t* attribute, outbound_message_t* outbound_message);
    bool (*serialize_parameter)(const char* device_key, parameter_t* parameter, outbound_message_t* outbound_message);

} parser_t;

void parser_init(parser_t* parser);

/**** Reading ****/
/* Note: Actuator status is considered reading, hence it's serialization is tied to function in this section */
size_t parser_serialize_readings(parser_t* parser, reading_t* readings, size_t num_readings, char* buffer,
                                 size_t buffer_size);
/**** Reading ****/

/**** File Management ****/
bool parser_serialize_file_management_status(parser_t* parser, const char* device_key,
                                             file_management_packet_request_t* file_management_packet_request,
                                             file_management_status_t* status, outbound_message_t* outbound_message);

bool parser_deserialize_file_management_parameter(parser_t* parser, char* buffer, size_t buffer_size,
                                                  file_management_parameter_t* parameter);

bool parser_serialize_file_management_packet_request(parser_t* parser, const char* device_key,
                                                     file_management_packet_request_t* file_management_packet_request,
                                                     outbound_message_t* outbound_message);

bool parser_serialize_file_management_url_download(parser_t* parser, const char* device_key,
                                                   file_management_parameter_t* parameter,
                                                   file_management_status_t* status,
                                                   outbound_message_t* outbound_message);

bool parser_serialize_file_management_file_list(parser_t* parser, const char* device_key, char* file_list,
                                                size_t file_list_items, outbound_message_t* outbound_message);
/**** File Management ****/

/**** Firmware Update ****/
bool parse_deserialize_firmware_update_parameter(parser_t* parser, char* device_key, char* buffer, size_t buffer_size,
                                                 firmware_update_t* firmware_update_parameter);

bool parse_serialize_firmware_update_status(parser_t* parser, const char* device_key,
                                            firmware_update_t* firmware_update, outbound_message_t* outbound_message);

bool parse_serialize_firmware_update_version(parser_t* parser, const char* device_key, char* firmware_update_version,
                                             outbound_message_t* outbound_message);
/**** Firmware Update ****/

bool parser_is_initialized(parser_t* parser);

/**** Utility ****/
bool parser_deserialize_time(parser_t* parser, char* buffer, size_t buffer_size, utc_command_t* utc_command);

bool parser_create_topic(parser_t* parser, char direction[DIRECTION_SIZE], char device_key[DEVICE_KEY_SIZE],
                         char message_type[MESSAGE_TYPE_SIZE], char topic[TOPIC_SIZE]);

size_t parser_deserialize_feed_value_message(parser_t* parser, char* buffer, size_t buffer_size,
                                             feed_value_message_t* feed_value_message, size_t msg_buffer_size);

size_t parser_deserialize_parameter_message(parser_t* parser, char* buffer, size_t buffer_size,
                                            parameter_t* parameter_message, size_t msg_buffer_size);

bool parser_serialize_feed_registration(parser_t* parser, const char* device_key, feed_t* feed,
                                        outbound_message_t* outbound_message);

bool parser_serialize_feed_removal(parser_t* parser, const char* device_key, feed_t* feed,
                                   outbound_message_t* outbound_message);
bool parser_serialize_pull_feed_values(parser_t* parser, const char* device_key, outbound_message_t* outbound_message);

bool parser_serialize_pull_parameters(parser_t* parser, const char* device_key, outbound_message_t* outbound_message);

bool parser_serialize_sync_parameters(parser_t* parser, const char* device_key, outbound_message_t* outbound_message);

bool parser_serialize_sync_time(parser_t* parser, const char* device_key, outbound_message_t* outbound_message);

bool parser_serialize_attribute(parser_t* parser, const char* device_key, attribute_t* attribute,
                                outbound_message_t* outbound_message);

bool parser_serialize_parameter(parser_t* parser, const char* device_key, parameter_t* parameter,
                                outbound_message_t* outbound_message);

#ifdef __cplusplus
}
#endif

#endif
