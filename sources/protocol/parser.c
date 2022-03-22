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

#include "protocol/parser.h"
#include "model/attribute.h"
#include "model/file_management/file_management_parameter.h"
#include "model/file_management/file_management_status.h"
#include "protocol/json_parser.h"
#include "utility/wolk_utils.h"

#include <stdbool.h>
#include <stddef.h>
#include <string.h>


void parser_init(parser_t* parser)
{
    /* Sanity check */
    WOLK_ASSERT(parser);

    parser->is_initialized = true;

    parser->serialize_feeds = json_serialize_feeds;

    parser->serialize_file_management_status = json_serialize_file_management_status;
    parser->deserialize_file_management_parameter = json_deserialize_file_management_parameter;
    parser->deserialize_file_delete = json_deserialize_file_delete;
    parser->serialize_file_management_packet_request = json_serialize_file_management_packet_request;
    parser->serialize_file_management_url_download_status = json_serialize_file_management_url_download_status;
    parser->serialize_file_management_file_list = json_serialize_file_management_file_list_update;

    parser->deserialize_firmware_update_parameter = json_deserialize_firmware_update_parameter;
    parser->serialize_firmware_update_status = json_serialize_firmware_update_status;

    parser->deserialize_time = json_deserialize_time;
    parser->deserialize_details_synchronization = json_deserialize_details_synchronization;
    parser->deserialize_readings_value_message = json_deserialize_feeds_value_message;
    parser->deserialize_parameter_message = json_deserialize_parameter_message;

    parser->create_topic = json_create_topic;

    parser->serialize_feed_registration = json_serialize_feed_registration;
    parser->serialize_feed_removal = json_serialize_feed_removal;
    parser->serialize_pull_feed_values = json_serialize_pull_feed_values;
    parser->serialize_pull_parameters = json_serialize_pull_parameters;
    parser->serialize_sync_parameters = json_serialize_sync_parameters;
    parser->serialize_sync_time = json_serialize_sync_time;
    parser->serialize_sync_details_synchronization = json_serialize_sync_details_synchronization;
    parser->serialize_attribute = json_serialize_attribute;
    parser->serialize_parameter = json_serialize_parameter;

    strncpy(parser->FILE_MANAGEMENT_UPLOAD_INITIATE_TOPIC, JSON_FILE_MANAGEMENT_UPLOAD_INITIATE_TOPIC,
            TOPIC_MESSAGE_TYPE_SIZE);
    strncpy(parser->FILE_MANAGEMENT_BINARY_REQUEST_TOPIC, JSON_FILE_MANAGEMENT_FILE_BINARY_REQUEST_TOPIC,
            TOPIC_MESSAGE_TYPE_SIZE);
    strncpy(parser->FILE_MANAGEMENT_BINARY_RESPONSE_TOPIC, JSON_FILE_MANAGEMENT_FILE_BINARY_RESPONSE_TOPIC,
            TOPIC_MESSAGE_TYPE_SIZE);
    strncpy(parser->FILE_MANAGEMENT_UPLOAD_ABORT_TOPIC, JSON_FILE_MANAGEMENT_FILE_UPLOAD_ABORT_TOPIC,
            TOPIC_MESSAGE_TYPE_SIZE);
    strncpy(parser->FILE_MANAGEMENT_URL_DOWNLOAD_INITIATE_TOPIC, JSON_FILE_MANAGEMENT_URL_DOWNLOAD_INITIATE_TOPIC,
            TOPIC_MESSAGE_TYPE_SIZE);
    strncpy(parser->FILE_MANAGEMENT_URL_DOWNLOAD_ABORT_TOPIC, JSON_FILE_MANAGEMENT_URL_DOWNLOAD_ABORT_TOPIC,
            TOPIC_MESSAGE_TYPE_SIZE);
    strncpy(parser->FILE_MANAGEMENT_FILE_LIST_TOPIC, JSON_FILE_MANAGEMENT_FILE_LIST_TOPIC, TOPIC_MESSAGE_TYPE_SIZE);
    strncpy(parser->FILE_MANAGEMENT_FILE_DELETE_TOPIC, JSON_FILE_MANAGEMENT_FILE_DELETE_TOPIC, TOPIC_MESSAGE_TYPE_SIZE);
    strncpy(parser->FILE_MANAGEMENT_FILE_PURGE_TOPIC, JSON_FILE_MANAGEMENT_FILE_PURGE_TOPIC, TOPIC_MESSAGE_TYPE_SIZE);
    strncpy(parser->FIRMWARE_UPDATE_INSTALL_TOPIC, JSON_FIRMWARE_UPDATE_INSTALL_TOPIC, TOPIC_MESSAGE_TYPE_SIZE);
    strncpy(parser->FIRMWARE_UPDATE_ABORT_TOPIC, JSON_FIRMWARE_UPDATE_ABORT_TOPIC, TOPIC_MESSAGE_TYPE_SIZE);
    strncpy(parser->P2D_TOPIC, JSON_P2D_TOPIC, TOPIC_MESSAGE_TYPE_SIZE);
    strncpy(parser->D2P_TOPIC, JSON_D2P_TOPIC, TOPIC_MESSAGE_TYPE_SIZE);
    strncpy(parser->FEED_REGISTRATION_TOPIC, JSON_FEED_REGISTRATION_TOPIC, TOPIC_MESSAGE_TYPE_SIZE);
    strncpy(parser->FEED_REMOVAL_TOPIC, JSON_FEED_REMOVAL_TOPIC, TOPIC_MESSAGE_TYPE_SIZE);
    strncpy(parser->FEED_VALUES_MESSAGE_TOPIC, JSON_FEED_VALUES_MESSAGE_TOPIC, TOPIC_MESSAGE_TYPE_SIZE);
    strncpy(parser->PULL_FEED_VALUES_TOPIC, JSON_PULL_FEEDS_TOPIC, TOPIC_MESSAGE_TYPE_SIZE);
    strncpy(parser->ATTRIBUTE_REGISTRATION_TOPIC, JSON_ATTRIBUTE_REGISTRATION_TOPIC, TOPIC_MESSAGE_TYPE_SIZE);
    strncpy(parser->PARAMETERS_TOPIC, JSON_PARAMETERS_TOPIC, TOPIC_MESSAGE_TYPE_SIZE);
    strncpy(parser->PULL_PARAMETERS_TOPIC, JSON_PULL_PARAMETERS_TOPIC, TOPIC_MESSAGE_TYPE_SIZE);
    strncpy(parser->SYNC_PARAMETERS_TOPIC, JSON_SYNC_PARAMETERS_TOPIC, TOPIC_MESSAGE_TYPE_SIZE);
    strncpy(parser->SYNC_TIME_TOPIC, JSON_SYNC_TIME_TOPIC, TOPIC_MESSAGE_TYPE_SIZE);
    strncpy(parser->ERROR_TOPIC, JSON_ERROR_TOPIC, TOPIC_MESSAGE_TYPE_SIZE);
    strncpy(parser->DETAILS_SYNCHRONIZATION_TOPIC, JSON_DETAILS_SYNCHRONIZATION_TOPIC, TOPIC_MESSAGE_TYPE_SIZE);
}

size_t parser_serialize_feeds(parser_t* parser, feed_t* readings, data_type_t type, size_t num_readings,
                              size_t reading_element_size, char* buffer, size_t buffer_size)
{
    /* Sanity check */
    WOLK_ASSERT(parser);
    WOLK_ASSERT(num_readings > 0);
    WOLK_ASSERT(buffer_size >= PAYLOAD_SIZE);

    return parser->serialize_feeds(readings, type, num_readings, reading_element_size, buffer, buffer_size);
}

bool parser_serialize_file_management_status(parser_t* parser, const char* device_key,
                                             file_management_packet_request_t* file_management_packet_request,
                                             file_management_status_t* status, outbound_message_t* outbound_message)
{
    /* Sanity check */
    WOLK_ASSERT(parser);
    WOLK_ASSERT(device_key);
    WOLK_ASSERT(status);
    WOLK_ASSERT(outbound_message);

    return parser->serialize_file_management_status(device_key, file_management_packet_request, status,
                                                    outbound_message);
}

bool parser_deserialize_file_management_parameter(parser_t* parser, char* buffer, size_t buffer_size,
                                                  file_management_parameter_t* parameter)
{
    /* Sanity check */
    WOLK_ASSERT(parser);
    WOLK_ASSERT(buffer);
    WOLK_ASSERT(parameter);

    return parser->deserialize_file_management_parameter(buffer, buffer_size, parameter);
}

size_t parser_deserialize_file_delete(parser_t* parser, char* buffer, size_t buffer_size, file_list_t* file_list)
{
    /* Sanity check */
    WOLK_ASSERT(parser);
    WOLK_ASSERT(buffer);
    WOLK_ASSERT(parameter);

    return parser->deserialize_file_delete(buffer, buffer_size, file_list);
}

bool parser_serialize_file_management_packet_request(parser_t* parser, const char* device_key,
                                                     file_management_packet_request_t* file_management_packet_request,
                                                     outbound_message_t* outbound_message)
{
    /* Sanity check */
    WOLK_ASSERT(parser);
    WOLK_ASSERT(device_key);
    WOLK_ASSERT(file_management_packet_request);
    WOLK_ASSERT(outbound_message);

    return parser->serialize_file_management_packet_request(device_key, file_management_packet_request,
                                                            outbound_message);
}

bool parser_serialize_file_management_url_download(parser_t* parser, const char* device_key,
                                                   file_management_parameter_t* file_management_parameter,
                                                   file_management_status_t* status,
                                                   outbound_message_t* outbound_message)
{
    /* Sanity check */
    WOLK_ASSERT(parser);
    WOLK_ASSERT(device_key);
    WOLK_ASSERT(file_management_parameter);
    WOLK_ASSERT(status);
    WOLK_ASSERT(outbound_message);

    return parser->serialize_file_management_url_download_status(device_key, file_management_parameter, status,
                                                                 outbound_message);
}

bool parser_serialize_file_management_file_list(parser_t* parser, const char* device_key, file_list_t* file_list,
                                                size_t file_list_items, outbound_message_t* outbound_message)
{
    /* Sanity Check */
    WOLK_ASSERT(parser);
    WOLK_ASSERT(device_key);
    WOLK_ASSERT(file_list);
    WOLK_ASSERT(file_list_items);
    WOLK_ASSERT(outbound_message);

    return parser->serialize_file_management_file_list(device_key, file_list, file_list_items, outbound_message);
}

bool parse_deserialize_firmware_update_parameter(parser_t* parser, char* device_key, char* buffer, size_t buffer_size,
                                                 firmware_update_t* firmware_update_parameter)
{
    WOLK_ASSERT(parser);
    WOLK_ASSERT(buffer);
    WOLK_ASSERT(buffer_size);
    WOLK_ASSERT(firmware_update_parameter);

    return parser->deserialize_firmware_update_parameter(device_key, buffer, buffer_size, firmware_update_parameter);
}

bool parse_serialize_firmware_update_status(parser_t* parser, const char* device_key,
                                            firmware_update_t* firmware_update, outbound_message_t* outbound_message)
{
    /* Sanity Check */
    WOLK_ASSERT(parser);
    WOLK_ASSERT(device_key);
    WOLK_ASSERT(firmware_update);
    WOLK_ASSERT(outbound_message);

    return parser->serialize_firmware_update_status(device_key, firmware_update, outbound_message);
}

bool parser_is_initialized(parser_t* parser)
{
    /* Sanity check */
    WOLK_ASSERT(parser);

    return parser->is_initialized;
}

bool parser_deserialize_time(parser_t* parser, char* buffer, size_t buffer_size, utc_command_t* utc_command)
{
    return parser->deserialize_time(buffer, buffer_size, utc_command);
}

bool parser_deserialize_details_synchronization(parser_t* parser, char* buffer, size_t buffer_size,
                                                feed_registration_t* feeds, size_t* number_of_feeds,
                                                attribute_t* attributes, size_t* number_of_attributes)
{
    return parser->deserialize_details_synchronization(buffer, buffer_size, feeds, number_of_feeds, attributes,
                                                       number_of_attributes);
}

bool parser_create_topic(parser_t* parser, char* direction, char* device_key, char* message_type, char* topic)
{
    return parser->create_topic(direction, device_key, message_type, topic);
}
size_t parser_deserialize_feeds_message(parser_t* parser, char* buffer, size_t buffer_size, feed_t* readings_received)
{
    return parser->deserialize_readings_value_message(buffer, buffer_size, readings_received);
}
size_t parser_deserialize_parameter_message(parser_t* parser, char* buffer, size_t buffer_size,
                                            parameter_t* parameter_message)
{
    return parser->deserialize_parameter_message(buffer, buffer_size, parameter_message);
}
bool parser_serialize_feed_registration(parser_t* parser, const char* device_key, feed_registration_t* feed,
                                        size_t number_of_feeds, outbound_message_t* outbound_message)
{
    return parser->serialize_feed_registration(device_key, feed, number_of_feeds, outbound_message);
}
bool parser_serialize_feed_removal(parser_t* parser, const char* device_key, feed_registration_t* feed,
                                   size_t number_of_feeds, outbound_message_t* outbound_message)
{
    return parser->serialize_feed_removal(device_key, feed, number_of_feeds, outbound_message);
}
bool parser_serialize_pull_feed_values(parser_t* parser, const char* device_key, outbound_message_t* outbound_message)
{
    return parser->serialize_pull_feed_values(device_key, outbound_message);
}
bool parser_serialize_pull_parameters(parser_t* parser, const char* device_key, outbound_message_t* outbound_message)
{
    return parser->serialize_pull_parameters(device_key, outbound_message);
}
bool parser_serialize_sync_parameters(parser_t* parser, const char* device_key, parameter_t* parameters,
                                      size_t number_of_parameters, outbound_message_t* outbound_message)
{
    return parser->serialize_sync_parameters(device_key, parameters, number_of_parameters, outbound_message);
}
bool parser_serialize_sync_time(parser_t* parser, const char* device_key, outbound_message_t* outbound_message)
{
    return parser->serialize_sync_time(device_key, outbound_message);
}
bool parser_serialize_details_synchronization(parser_t* parser, const char* device_key,
                                              outbound_message_t* outbound_message)
{
    return parser->serialize_sync_details_synchronization(device_key, outbound_message);
}
bool parser_serialize_attribute(parser_t* parser, const char* device_key, attribute_t* attributes,
                                size_t number_of_attributes, outbound_message_t* outbound_message)
{
    return parser->serialize_attribute(device_key, attributes, number_of_attributes, outbound_message);
}
bool parser_serialize_parameter(parser_t* parser, const char* device_key, parameter_t* parameter,
                                size_t number_of_parameters, outbound_message_t* outbound_message)
{
    return parser->serialize_parameter(device_key, parameter, number_of_parameters, outbound_message);
}
