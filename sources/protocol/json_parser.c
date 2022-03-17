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

#include "json_parser.h"
#include "model/file_management/file_management_packet_request.h"
#include "model/file_management/file_management_parameter.h"
#include "model/file_management/file_management_status.h"
#include "model/firmware_update.h"
#include "size_definitions.h"
#include "utility/base64.h"
#include "utility/jsmn.h"
#include "utility/wolk_utils.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define JSON_TOKEN_SIZE FEEDS_MAX_NUMBER
#define UTC_MILLISECONDS_LENGTH 14


const char JSON_P2D_TOPIC[TOPIC_DIRECTION_SIZE] = "p2d";
const char JSON_D2P_TOPIC[TOPIC_DIRECTION_SIZE] = "d2p";
const char JSON_FEED_REGISTRATION_TOPIC[TOPIC_MESSAGE_TYPE_SIZE] = "feed_registration";
const char JSON_FEED_REMOVAL_TOPIC[TOPIC_MESSAGE_TYPE_SIZE] = "feed_removal";
const char JSON_FEED_VALUES_MESSAGE_TOPIC[TOPIC_MESSAGE_TYPE_SIZE] = "feed_values";
const char JSON_PULL_FEEDS_TOPIC[TOPIC_MESSAGE_TYPE_SIZE] = "pull_feed_values";
const char JSON_ATTRIBUTE_REGISTRATION_TOPIC[TOPIC_MESSAGE_TYPE_SIZE] = "attribute_registration";
const char JSON_PARAMETERS_TOPIC[TOPIC_MESSAGE_TYPE_SIZE] = "parameters";
const char JSON_PULL_PARAMETERS_TOPIC[TOPIC_MESSAGE_TYPE_SIZE] = "pull_parameters";
const char JSON_SYNC_PARAMETERS_TOPIC[TOPIC_MESSAGE_TYPE_SIZE] = "synchronize_parameters";
const char JSON_SYNC_TIME_TOPIC[TOPIC_MESSAGE_TYPE_SIZE] = "time";
const char JSON_SYNC_DETAILS_SYNCHRONIZATION_TOPIC[TOPIC_MESSAGE_TYPE_SIZE] = "details_synchronization";
const char JSON_ERROR_TOPIC[TOPIC_MESSAGE_TYPE_SIZE] = "error";
const char JSON_DETAILS_SYNCHRONIZATION_TOPIC[TOPIC_MESSAGE_TYPE_SIZE] = "details_synchronization";

const char JSON_FILE_MANAGEMENT_UPLOAD_INITIATE_TOPIC[TOPIC_MESSAGE_TYPE_SIZE] = "file_upload_initiate";
const char JSON_FILE_MANAGEMENT_FILE_UPLOAD_STATUS_TOPIC[TOPIC_MESSAGE_TYPE_SIZE] = "file_upload_status";
const char JSON_FILE_MANAGEMENT_FILE_UPLOAD_ABORT_TOPIC[TOPIC_MESSAGE_TYPE_SIZE] = "file_upload_abort";
const char JSON_FILE_MANAGEMENT_FILE_BINARY_REQUEST_TOPIC[TOPIC_MESSAGE_TYPE_SIZE] = "file_binary_request";
const char JSON_FILE_MANAGEMENT_FILE_BINARY_RESPONSE_TOPIC[TOPIC_MESSAGE_TYPE_SIZE] = "file_binary_response";
const char JSON_FILE_MANAGEMENT_URL_DOWNLOAD_INITIATE_TOPIC[TOPIC_MESSAGE_TYPE_SIZE] = "file_url_download_initiate";
const char JSON_FILE_MANAGEMENT_URL_DOWNLOAD_ABORT_TOPIC[TOPIC_MESSAGE_TYPE_SIZE] = "file_url_download_abort";
const char JSON_FILE_MANAGEMENT_URL_DOWNLOAD_STATUS_TOPIC[TOPIC_MESSAGE_TYPE_SIZE] = "file_url_download_status";
const char JSON_FILE_MANAGEMENT_FILE_LIST_TOPIC[TOPIC_MESSAGE_TYPE_SIZE] = "file_list";
const char JSON_FILE_MANAGEMENT_FILE_DELETE_TOPIC[TOPIC_MESSAGE_TYPE_SIZE] = "file_delete";
const char JSON_FILE_MANAGEMENT_FILE_PURGE_TOPIC[TOPIC_MESSAGE_TYPE_SIZE] = "file_purge";

const char JSON_FIRMWARE_UPDATE_INSTALL_TOPIC[TOPIC_MESSAGE_TYPE_SIZE] = "firmware_update_install";
const char JSON_FIRMWARE_UPDATE_STATUS_TOPIC[TOPIC_MESSAGE_TYPE_SIZE] = "firmware_update_status";
const char JSON_FIRMWARE_UPDATE_ABORT_TOPIC[TOPIC_MESSAGE_TYPE_SIZE] = "firmware_update_abort";


static bool json_token_str_equal(const char* json, jsmntok_t* tok, const char* s)
{
    if (tok->type == JSMN_STRING && (int)strlen(s) == tok->end - tok->start
        && strncmp(json + tok->start, s, (size_t)(tok->end - tok->start)) == 0) {
        return true;
    }

    return false;
}

bool json_serialize_file_management_status(const char* device_key,
                                           file_management_packet_request_t* file_management_packet_request,
                                           file_management_status_t* status, outbound_message_t* outbound_message)
{
    /* Serialize topic */
    strncpy(outbound_message->topic, JSON_FILE_MANAGEMENT_FILE_UPLOAD_STATUS_TOPIC,
            strlen(JSON_FILE_MANAGEMENT_FILE_UPLOAD_STATUS_TOPIC));
    if (snprintf(outbound_message->topic + strlen(JSON_FILE_MANAGEMENT_FILE_UPLOAD_STATUS_TOPIC),
                 WOLK_ARRAY_LENGTH(outbound_message->topic), "%s", device_key)
        >= (int)WOLK_ARRAY_LENGTH(outbound_message->topic)) {
        return false;
    }

    /* Serialize payload */
    if (snprintf(outbound_message->payload, WOLK_ARRAY_LENGTH(outbound_message->payload),
                 "{\"fileName\": \"%s\", \"status\": \"%s\"}",
                 file_management_packet_request_get_file_name(file_management_packet_request),
                 file_management_status_as_str(status))
        >= (int)WOLK_ARRAY_LENGTH(outbound_message->payload)) {
        return false;
    }

    file_management_error_t error = file_management_status_get_error(status);
    if (error >= 0) {
        if (snprintf(outbound_message->payload + strlen(outbound_message->payload) - 1,
                     WOLK_ARRAY_LENGTH(outbound_message->payload), ",\"error\":%d}", error)
            >= (int)WOLK_ARRAY_LENGTH(outbound_message->payload)) {
            return false;
        }
    }

    return true;
}

bool json_deserialize_file_management_parameter(char* buffer, size_t buffer_size,
                                                file_management_parameter_t* parameter)
{
    jsmn_parser parser = {0};
    jsmntok_t tokens[JSON_TOKEN_SIZE] = {0};
    jsmn_init(&parser);
    const int parser_result = jsmn_parse(&parser, buffer, buffer_size, tokens, WOLK_ARRAY_LENGTH(tokens));

    /* Received JSON must be valid, and top level element must be object */
    if (parser_result < 1 || tokens[0].type != JSMN_OBJECT || parser_result >= (int)WOLK_ARRAY_LENGTH(tokens)) {
        return false;
    }

    file_management_parameter_init(parameter);

    /* Obtain command type and value */
    char value_buffer[FEED_ELEMENT_SIZE] = "";

    for (size_t i = 1; i < parser_result; i++) {
        if (json_token_str_equal(buffer, &tokens[i], "name")) {
            if (snprintf(value_buffer, WOLK_ARRAY_LENGTH(value_buffer), "%.*s", tokens[i + 1].end - tokens[i + 1].start,
                         buffer + tokens[i + 1].start)
                >= (int)WOLK_ARRAY_LENGTH(value_buffer)) {
                return false;
            }

            if (strlen(value_buffer) > FILE_MANAGEMENT_FILE_NAME_SIZE) {
                /* Leave file name array empty */
                return true;
            } else {
                file_management_parameter_set_filename(parameter, value_buffer);
                i++;
            }
        } else if (json_token_str_equal(buffer, &tokens[i], "size")) {
            if (snprintf(value_buffer, WOLK_ARRAY_LENGTH(value_buffer), "%.*s", tokens[i + 1].end - tokens[i + 1].start,
                         buffer + tokens[i + 1].start)
                >= (int)WOLK_ARRAY_LENGTH(value_buffer)) {
                return false;
            }

            file_management_parameter_set_file_size(parameter, (size_t)atoi(value_buffer));
            i++;
        } else if (json_token_str_equal(buffer, &tokens[i], "hash")) {
            if (snprintf(value_buffer, WOLK_ARRAY_LENGTH(value_buffer), "%.*s", tokens[i + 1].end - tokens[i + 1].start,
                         buffer + tokens[i + 1].start)
                >= (int)WOLK_ARRAY_LENGTH(value_buffer)) {
                return false;
            }

            const size_t output_size = base64_decode(value_buffer, NULL, strlen(value_buffer));
            if (output_size > FILE_MANAGEMENT_HASH_SIZE) {
                return false;
            }
            base64_decode(value_buffer, (BYTE*)parameter->file_hash, strlen(value_buffer));
            i++;
        }
        //TODO: will be used for serialize file_url_download_abort msg
        else if (json_token_str_equal(buffer, &tokens[i], "fileUrl")) {
            if (snprintf(value_buffer, WOLK_ARRAY_LENGTH(value_buffer), "%.*s", tokens[i + 1].end - tokens[i + 1].start,
                         buffer + tokens[i + 1].start)
                >= (int)WOLK_ARRAY_LENGTH(value_buffer)) {
                return false;
            }

            file_management_parameter_set_file_url(parameter, value_buffer);
            i++;
        } else {
            return false;
        }
    }
    return true;
}

bool json_serialize_file_management_packet_request(const char* device_key,
                                                   file_management_packet_request_t* file_management_packet_request,
                                                   outbound_message_t* outbound_message)
{
    char topic[TOPIC_SIZE] = "";

    /* Serialize topic */
    json_create_topic(JSON_D2P_TOPIC, device_key, JSON_FILE_MANAGEMENT_FILE_BINARY_REQUEST_TOPIC, topic);
    strncpy(outbound_message->topic, topic, TOPIC_SIZE);

    /* Serialize payload */
    if (snprintf(outbound_message->payload, WOLK_ARRAY_LENGTH(outbound_message->payload),
                 "{\"name\": \"%s\", \"chunkIndex\":%llu}",
                 file_management_packet_request_get_file_name(file_management_packet_request),
                 (unsigned long long int)file_management_packet_request_get_chunk_index(file_management_packet_request))
        >= (int)WOLK_ARRAY_LENGTH(outbound_message->payload)) {
        return false;
    }

    return true;
}

bool json_serialize_file_management_url_download_status(const char* device_key,
                                                        file_management_parameter_t* file_management_parameter,
                                                        file_management_status_t* status,
                                                        outbound_message_t* outbound_message)
{
    /* Serialize topic */
    strncpy(outbound_message->topic, JSON_FILE_MANAGEMENT_URL_DOWNLOAD_STATUS_TOPIC,
            strlen(JSON_FILE_MANAGEMENT_URL_DOWNLOAD_STATUS_TOPIC));
    if (snprintf(outbound_message->topic + strlen(JSON_FILE_MANAGEMENT_URL_DOWNLOAD_STATUS_TOPIC),
                 WOLK_ARRAY_LENGTH(outbound_message->topic), "%s", device_key)
        >= (int)WOLK_ARRAY_LENGTH(outbound_message->topic)) {
        return false;
    }

    /* Serialize payload */
    if (snprintf(outbound_message->payload, WOLK_ARRAY_LENGTH(outbound_message->payload),
                 "{\"fileUrl\": \"%s\", \"status\": \"%s\"}",
                 file_management_parameter_get_file_url(file_management_parameter),
                 file_management_status_as_str(status))
        >= (int)WOLK_ARRAY_LENGTH(outbound_message->payload)) {
        return false;
    }

    file_management_error_t error = file_management_status_get_error(status);
    if (error >= 0) {
        if (snprintf(outbound_message->payload + strlen(outbound_message->payload) - 1,
                     WOLK_ARRAY_LENGTH(outbound_message->payload), ",\"error\":%d}", error)
            >= (int)WOLK_ARRAY_LENGTH(outbound_message->payload)) {
            return false;
        }
    }

    file_management_state_t state = file_management_status_get_state(status);
    if (state == FILE_MANAGEMENT_STATE_FILE_READY) {
        if (snprintf(outbound_message->payload + strlen(outbound_message->payload) - 1,
                     WOLK_ARRAY_LENGTH(outbound_message->payload), ",\"fileName\":\"%s\"}",
                     file_management_packet_request_get_file_name(file_management_parameter))
            >= (int)WOLK_ARRAY_LENGTH(outbound_message->payload)) {
            return false;
        }
    }

    return true;
}

bool json_serialize_file_management_file_list_update(const char* device_key, char* file_list, size_t file_list_items,
                                                     outbound_message_t* outbound_message)
{
    /* Serialize topic */
    strncpy(outbound_message->topic, JSON_FILE_MANAGEMENT_FILE_LIST_TOPIC,
            strlen(JSON_FILE_MANAGEMENT_FILE_LIST_TOPIC));
    if (snprintf(outbound_message->topic + strlen(JSON_FILE_MANAGEMENT_FILE_LIST_TOPIC),
                 WOLK_ARRAY_LENGTH(outbound_message->topic), "%s", device_key)
        >= (int)WOLK_ARRAY_LENGTH(outbound_message->topic)) {
        return false;
    }

    /* Serialize payload */
    strncpy(outbound_message->payload, "[", strlen("["));
    for (size_t i = 0; i < file_list_items; i++) {
        if (snprintf(outbound_message->payload + strlen(outbound_message->payload),
                     WOLK_ARRAY_LENGTH(outbound_message->payload), "{\"fileName\":\"%s\"},",
                     (const char*)file_list + (i * FILE_MANAGEMENT_FILE_NAME_SIZE))
            >= (int)WOLK_ARRAY_LENGTH(outbound_message->payload)) {
            return false;
        }
    }

    file_list_items == 0 ? strncpy(outbound_message->payload + strlen(outbound_message->payload), "]", strlen("]"))
                         : strncpy(outbound_message->payload + strlen(outbound_message->payload) - 1, "]", strlen("]"));

    return true;
}

static const char* firmware_update_status_as_str(firmware_update_t* firmware_update)
{
    /* Sanity check */
    WOLK_ASSERT(firmware_update);

    switch (firmware_update->status) {
    case FIRMWARE_UPDATE_STATUS_INSTALLATION:
        return "INSTALLATION";

    case FIRMWARE_UPDATE_STATUS_COMPLETED:
        return "COMPLETED";

    case FIRMWARE_UPDATE_STATUS_ERROR:
        return "ERROR";

    case FIRMWARE_UPDATE_STATUS_ABORTED:
        return "ABORTED";

    default:
        WOLK_ASSERT(false);
        return "";
    }
}

bool json_deserialize_firmware_update_parameter(char* device_key, char* buffer, size_t buffer_size,
                                                firmware_update_t* parameter)
{
    jsmn_parser parser;
    jsmntok_t tokens[JSON_TOKEN_SIZE];
    jsmn_init(&parser);
    const int parser_result = jsmn_parse(&parser, buffer, buffer_size, tokens, WOLK_ARRAY_LENGTH(tokens));

    /* Received JSON must be valid, and top level element must be object */
    if (parser_result < 1 || tokens[0].type != JSMN_OBJECT || parser_result >= (int)WOLK_ARRAY_LENGTH(tokens)) {
        return false;
    }

    firmware_update_parameter_init(parameter);

    /* Obtain command type and value */
    char value_buffer[FEED_ELEMENT_SIZE];

    for (size_t i = 1; i < parser_result; i++) {
        if (json_token_str_equal(buffer, &tokens[i], "devices")) {
            if (!json_token_str_equal(buffer, &tokens[i + 2], device_key)) {
                return false;
            }
            i += 2; // jump over JSON array
        } else if (json_token_str_equal(buffer, &tokens[i], "fileName")) {
            if (snprintf(value_buffer, WOLK_ARRAY_LENGTH(value_buffer), "%.*s", tokens[i + 1].end - tokens[i + 1].start,
                         buffer + tokens[i + 1].start)
                >= (int)WOLK_ARRAY_LENGTH(value_buffer)) {
                return false;
            }

            firmware_update_parameter_set_filename(parameter, value_buffer);
            i++;
        } else {
            return false;
        }
    }
    return true;
}

bool json_serialize_firmware_update_status(const char* device_key, firmware_update_t* firmware_update,
                                           outbound_message_t* outbound_message)
{
    /* Serialize topic */
    strncpy(outbound_message->topic, JSON_FIRMWARE_UPDATE_STATUS_TOPIC, strlen(JSON_FIRMWARE_UPDATE_STATUS_TOPIC));
    if (snprintf(outbound_message->topic + strlen(JSON_FIRMWARE_UPDATE_STATUS_TOPIC),
                 WOLK_ARRAY_LENGTH(outbound_message->topic), "%s", device_key)
        >= (int)WOLK_ARRAY_LENGTH(outbound_message->topic)) {
        return false;
    }

    /* Serialize payload */
    if (snprintf(outbound_message->payload, WOLK_ARRAY_LENGTH(outbound_message->payload), "{\"status\": \"%s\"}",
                 firmware_update_status_as_str(firmware_update))
        >= (int)WOLK_ARRAY_LENGTH(outbound_message->payload)) {
        return false;
    }

    size_t error = firmware_update->error;
    if (error >= 0) {
        if (snprintf(outbound_message->payload + strlen(outbound_message->payload) - 1,
                     WOLK_ARRAY_LENGTH(outbound_message->payload), ",\"error\":%d}", error)
            >= (int)WOLK_ARRAY_LENGTH(outbound_message->payload)) {
            return false;
        }
    }

    return true;
}

bool json_deserialize_time(char* buffer, size_t buffer_size, utc_command_t* utc_command)
{
    if (strlen(buffer) == 0)
        return false;

    char* string_part;
    uint64_t conversion_utc = strtoul(buffer, &string_part, 10);
    if (conversion_utc == NULL) {
        return false;
    }
    utc_command->utc = conversion_utc;

    return true;
}

bool json_deserialize_details_synchronization(char* buffer, size_t buffer_size, feed_registration_t* feeds,
                                              size_t* number_of_feeds, attribute_t* attributes,
                                              size_t* number_of_attributes)
{
    jsmn_parser parser;
    jsmntok_t tokens[JSON_TOKEN_SIZE];

    jsmn_init(&parser);
    int parser_result = jsmn_parse(&parser, buffer, buffer_size, tokens, WOLK_ARRAY_LENGTH(tokens));

    /* Received JSON must be valid, and top level element must be array */
    if (parser_result < 1 || tokens[0].type != JSMN_OBJECT || parser_result >= (int)WOLK_ARRAY_LENGTH(tokens)) {
        return false;
    }
    // OBJ*STR-ARRAY-STR-..-STR-STR-ARRAY-STR-..-STR
    for (int i = 1; i < parser_result; ++i) { // at 1st position expects json object; 0 position is json array
        if (tokens[i].type == JSMN_ARRAY) {
            if (json_token_str_equal(buffer, &tokens[i - 1], "feeds")) {
                for (int j = i; tokens[j + 1].type != JSMN_ARRAY; ++j) { // 1st str is value, 2nd is element name
                    if (tokens[j].type == JSMN_STRING) {
                        if (snprintf(feeds->name, WOLK_ARRAY_LENGTH(feeds->name), "%.*s",
                                     tokens[j].end - tokens[j].start, buffer + tokens[j].start)
                            >= (int)WOLK_ARRAY_LENGTH(feeds->name)) {
                            return false;
                        }
                        *number_of_feeds += 1;
                        feeds++;
                    }
                }
            } else if (json_token_str_equal(buffer, &tokens[i - 1], "attributes")) {
                for (int j = i; tokens[j + 1].type != JSMN_ARRAY; ++j) { // 1st str is value, 2nd is element name
                    if (tokens[j].type == JSMN_STRING) {
                        if (snprintf(attributes->name, WOLK_ARRAY_LENGTH(attributes->name), "%.*s",
                                     tokens[j].end - tokens[j].start, buffer + tokens[j].start)
                            >= (int)WOLK_ARRAY_LENGTH(attributes->name)) {
                            return false;
                        }
                        *number_of_attributes += 1;
                        attributes++;
                    }
                }
            } else
                return false;
        }
    }
}

bool json_create_topic(char direction[TOPIC_DIRECTION_SIZE], const char device_key[DEVICE_KEY_SIZE],
                       char message_type[TOPIC_MESSAGE_TYPE_SIZE], char topic[TOPIC_SIZE])
{
    topic[0] = '\0';
    strcat(topic, direction);
    strcat(topic, "/");
    strcat(topic, device_key);
    strcat(topic, "/");
    strcat(topic, message_type);

    return true;
}

size_t json_deserialize_feeds_value_message(char* buffer, size_t buffer_size, feed_t* feeds_received)
{
    uint8_t number_of_deserialized_feeds = 0;
    jsmn_parser parser;
    jsmntok_t tokens[JSON_TOKEN_SIZE];

    jsmn_init(&parser);
    int parser_result = jsmn_parse(&parser, buffer, buffer_size, tokens, WOLK_ARRAY_LENGTH(tokens));

    /* Received JSON must be valid, and top level element must be array */
    if (parser_result < 1 || tokens[0].type != JSMN_ARRAY || parser_result >= (int)WOLK_ARRAY_LENGTH(tokens)) {
        return false;
    }

    for (int i = 1; i < parser_result; ++i) { // at 1st position expects json object; 0 position is json array
        if (tokens[i].type == JSMN_OBJECT) {
            // object equals feed
            for (int j = i + 1; tokens[j].type != JSMN_OBJECT; ++j) { // at 2nd position expects string
                if (tokens[j].type == JSMN_STRING) {
                    // get timestamp
                    if (json_token_str_equal(buffer, &tokens[j], "timestamp")) {
                        char timestamp[UTC_MILLISECONDS_LENGTH] = "";

                        if (snprintf(timestamp, WOLK_ARRAY_LENGTH(timestamp), "%.*s",
                                     tokens[j + 1].end - tokens[j + 1].start, buffer + tokens[j + 1].start)
                            >= (int)WOLK_ARRAY_LENGTH(timestamp)) {
                            return false;
                        }
                        char* string_part;
                        feed_set_utc(feeds_received, strtoul(timestamp, &string_part, 10));
                    } else {
                        // get reference
                        if (snprintf(feeds_received->reference, WOLK_ARRAY_LENGTH(feeds_received->reference), "%.*s",
                                     tokens[j].end - tokens[j].start, buffer + tokens[j].start)
                            >= (int)WOLK_ARRAY_LENGTH(feeds_received->reference)) {
                            return false;
                        }
                        // get value
                        if (snprintf(feeds_received->data, WOLK_ARRAY_LENGTH(feeds_received->data), "%.*s",
                                     tokens[j + 1].end - tokens[j + 1].start, buffer + tokens[j + 1].start)
                            >= (int)WOLK_ARRAY_LENGTH(feeds_received->data)) {
                            return false;
                        }
                    }
                }
            }
            // move to next feed
            feeds_received++;
            number_of_deserialized_feeds++;
        }
    }
    return number_of_deserialized_feeds;
}

size_t json_deserialize_parameter_message(char* buffer, size_t buffer_size, parameter_t* parameter_message)
{
    uint8_t number_of_deserialized_parameters = 0;
    jsmn_parser parser;
    jsmntok_t tokens[JSON_TOKEN_SIZE];

    jsmn_init(&parser);
    int parser_result = jsmn_parse(&parser, buffer, buffer_size, tokens, WOLK_ARRAY_LENGTH(tokens));

    /* Received JSON must be valid, and top level element must be array */
    if (parser_result < 1 || tokens[0].type != JSMN_OBJECT || parser_result >= (int)WOLK_ARRAY_LENGTH(tokens)) {
        return false;
    }
    // at 2nd position expects first json string; 1st position is json object
    for (int i = 1; i < parser_result; i += 2) {
        if (tokens[i].type == JSMN_STRING) {
            // get name
            if (snprintf(parameter_message->name, WOLK_ARRAY_LENGTH(parameter_message->name), "%.*s",
                         tokens[i].end - tokens[i].start, buffer + tokens[i].start)
                >= (int)WOLK_ARRAY_LENGTH(parameter_message->name)) {
                return false;
            }
            // get value
            if (snprintf(parameter_message->value, WOLK_ARRAY_LENGTH(parameter_message->value), "%.*s",
                         tokens[i + 1].end - tokens[i + 1].start, buffer + tokens[i + 1].start)
                >= (int)WOLK_ARRAY_LENGTH(parameter_message->value)) {
                return false;
            }
            // move to next parameter
            parameter_message++;
            number_of_deserialized_parameters++;
        }
    }

    return number_of_deserialized_parameters;
}

static bool serialize_feed(feed_t* feed, data_type_t type, size_t feed_element_size, char* buffer, size_t buffer_size)
{
    char data_buffer[PAYLOAD_SIZE] = "";

    for (size_t i = 0; i < feed_element_size; ++i) {
        strcat(data_buffer, feed->data[i]);

        if (i < feed_element_size - 1)
            strcat(data_buffer, ",");
    }

    if (feed_get_utc(feed) > 0
        && snprintf(buffer, buffer_size, "[{\"%s\":%s%s%s,\"timestamp\":%ld}]", feed->reference,
                    type == NUMERIC ? "" : "\"", data_buffer, type == NUMERIC ? "" : "\"", feed_get_utc(feed))
               >= (int)buffer_size) {
        return false;
    } else if (feed_get_utc(feed) == 0
               && snprintf(buffer, buffer_size, "[{\"%s\":%s%s%s}]", feed->reference, type == NUMERIC ? "" : "\"",
                           data_buffer, type == NUMERIC ? "" : "\"")
                      >= (int)buffer_size) {
        return false;
    }

    return true;
}

static size_t serialize_feeds(feed_t* feeds, data_type_t type, size_t number_of_feeds, char* buffer, size_t buffer_size)
{
    WOLK_UNUSED(number_of_feeds);

    char data_buffer[PAYLOAD_SIZE] = "";
    strcat(buffer, "[");

    for (size_t i = 0; i < number_of_feeds; ++i) {
        // when it consists of more feeds with the same reference it has to have utc
        if (feed_get_utc(feeds) == 0)
            return false;

        if (snprintf(data_buffer, buffer_size, "{\"%s\":%s%s%s,\"timestamp\":%ld}%s", feeds->reference,
                     type == NUMERIC ? "" : "\"", feeds->data[0], type == NUMERIC ? "" : "\"", feed_get_utc(feeds),
                     (number_of_feeds > 1 && i < (number_of_feeds - 1)) ? "," : "")
            >= (int)buffer_size)
            return false;

        strcat(buffer, data_buffer);
        feeds++;
    }

    strcat(buffer, "]");
    return true;
}

size_t json_serialize_feeds(feed_t* feeds, data_type_t type, size_t number_of_feeds, size_t feed_element_size,
                            char* buffer, size_t buffer_size)
{
    /* Sanity check */
    WOLK_ASSERT(number_of_feeds > 0);

    if (number_of_feeds > 1) {
        WOLK_UNUSED(feed_element_size);
        return serialize_feeds(feeds, type, number_of_feeds, buffer, buffer_size);
    } else {
        return serialize_feed(feeds, type, feed_element_size, buffer, buffer_size) ? 1 : 0;
    }
}
bool json_serialize_attribute(const char* device_key, attribute_t* attributes, size_t number_of_attributes,
                              outbound_message_t* outbound_message)
{
    char payload[PAYLOAD_SIZE] = "";
    char topic[TOPIC_SIZE] = "";
    json_create_topic(JSON_D2P_TOPIC, device_key, JSON_ATTRIBUTE_REGISTRATION_TOPIC, topic);

    strncpy(outbound_message->payload, "[", 1);
    for (int i = 0; i < number_of_attributes; ++i) {
        if (sprintf(payload, "{\"name\": \"%s\", \"dataType\": \"%s\", \"value\": \"%s\"}%s", attributes->name,
                    attributes->data_type, attributes->value,
                    number_of_attributes > 1 && i < (number_of_attributes - 1) ? "," : "")
            >= (int)WOLK_ARRAY_LENGTH(outbound_message->payload))
            return false;

        if (strlen(payload) > (PAYLOAD_SIZE - strlen(outbound_message->payload)))
            return false;

        strcat(outbound_message->payload, payload);
        attributes++;
    }
    strcat(outbound_message->payload, "]");

    strncpy(outbound_message->topic, topic, TOPIC_SIZE);

    return true;
}
bool json_serialize_parameter(const char* device_key, parameter_t* parameter, size_t number_of_parameters,
                              outbound_message_t* outbound_message)
{
    char topic[TOPIC_SIZE] = "";
    char payload[PAYLOAD_SIZE] = "";

    json_create_topic(JSON_D2P_TOPIC, device_key, JSON_PARAMETERS_TOPIC, topic);

    strcat(outbound_message->payload, "{");
    for (int i = 0; i < number_of_parameters; ++i) {
        if (sprintf(payload, "\"%s\": \"%s\"%s", parameter->name, parameter->value,
                    (number_of_parameters > 1 && i < (number_of_parameters - 1)) ? "," : "")
            >= (int)WOLK_ARRAY_LENGTH(outbound_message->payload))
            return false;

        if (strlen(payload) > (PAYLOAD_SIZE - strlen(outbound_message->payload)))
            return false;

        strcat(outbound_message->payload, payload);
        parameter++;
    }
    strcat(outbound_message->payload, "}");

    strncpy(outbound_message->topic, topic, TOPIC_SIZE);

    return true;
}

bool json_serialize_feed_registration(const char* device_key, feed_registration_t* feed, size_t number_of_feeds,
                                      outbound_message_t* outbound_message)
{
    char payload[PAYLOAD_SIZE] = "";
    char topic[TOPIC_SIZE] = "";
    json_create_topic(JSON_D2P_TOPIC, device_key, JSON_FEED_REGISTRATION_TOPIC, topic);

    strcat(outbound_message->payload, "[");
    for (int i = 0; i < number_of_feeds; ++i) {
        if (sprintf(payload, "{\"name\": \"%s\", \"type\": \"%s\", \"unitGuid\": \"%s\", \"reference\": \"%s\"}%s",
                    feed->name, feed_type_to_string(feed->feedType), feed->unit, feed->reference,
                    (number_of_feeds > 1 && i < (number_of_feeds - 1) ? "," : ""))
            >= (int)WOLK_ARRAY_LENGTH(outbound_message->payload))
            return false;

        if (strlen(payload) > (PAYLOAD_SIZE - strlen(outbound_message->payload)))
            return false;

        strcat(outbound_message->payload, payload);
        feed++;
    }
    strcat(outbound_message->payload, "]");

    strncpy(outbound_message->topic, topic, TOPIC_SIZE);

    return true;
}
bool json_serialize_feed_removal(const char* device_key, feed_registration_t* feed, size_t number_of_feeds,
                                 outbound_message_t* outbound_message)
{
    char payload[PAYLOAD_SIZE] = "";
    char topic[TOPIC_SIZE] = "";
    json_create_topic(JSON_D2P_TOPIC, device_key, JSON_FEED_REMOVAL_TOPIC, topic);

    strcat(outbound_message->payload, "[");
    for (int i = 0; i < number_of_feeds; ++i) {
        if (sprintf(payload, "\"%s\"%s", feed->name, (number_of_feeds > 1 && i < (number_of_feeds - 1) ? "," : ""))
            >= (int)WOLK_ARRAY_LENGTH(outbound_message->payload))
            return false;

        if (strlen(payload) > (PAYLOAD_SIZE - strlen(outbound_message->payload)))
            return false;

        strcat(outbound_message->payload, payload);
        feed++;
    }
    strcat(outbound_message->payload, "]");

    strncpy(outbound_message->topic, topic, TOPIC_SIZE);

    return true;
}
bool json_serialize_pull_feed_values(const char* device_key, outbound_message_t* outbound_message)
{
    char topic[TOPIC_SIZE] = "";
    json_create_topic(JSON_D2P_TOPIC, device_key, JSON_PULL_FEEDS_TOPIC, topic);

    strncpy(outbound_message->topic, topic, TOPIC_SIZE);
    strncpy(outbound_message->payload, "", PAYLOAD_SIZE);

    return true;
}
bool json_serialize_pull_parameters(const char* device_key, outbound_message_t* outbound_message)
{
    char topic[TOPIC_SIZE] = "";
    json_create_topic(JSON_D2P_TOPIC, device_key, JSON_PULL_PARAMETERS_TOPIC, topic);

    strncpy(outbound_message->topic, topic, TOPIC_SIZE);
    strncpy(outbound_message->payload, "", PAYLOAD_SIZE);

    return true;
}
bool json_serialize_sync_parameters(const char* device_key, parameter_t* parameters, size_t number_of_parameters,
                                    outbound_message_t* outbound_message)
{
    char topic[TOPIC_SIZE] = "";
    json_create_topic(JSON_D2P_TOPIC, device_key, JSON_SYNC_PARAMETERS_TOPIC, topic);

    strcat(outbound_message->payload, "[");
    for (int i = 0; i < number_of_parameters; ++i) {
        strcat(outbound_message->payload, "\"");
        if (strlen(parameters->name) > (PAYLOAD_SIZE - strlen(outbound_message->payload)))
            return false;
        strcat(outbound_message->payload, parameters->name);
        strcat(outbound_message->payload, "\"");

        if (number_of_parameters > 1 && i < (number_of_parameters - 1))
            strcat(outbound_message->payload, ",");

        parameters++;
    }
    strcat(outbound_message->payload, "]");

    strncpy(outbound_message->topic, topic, TOPIC_SIZE);

    return true;
}
bool json_serialize_sync_time(const char* device_key, outbound_message_t* outbound_message)
{
    char topic[TOPIC_SIZE] = "";
    json_create_topic(JSON_D2P_TOPIC, device_key, JSON_SYNC_TIME_TOPIC, topic);

    strncpy(outbound_message->topic, topic, TOPIC_SIZE);
    strncpy(outbound_message->payload, "", PAYLOAD_SIZE);

    return true;
}

bool json_serialize_sync_details_synchronization(const char* device_key, outbound_message_t* outbound_message)
{
    char topic[TOPIC_SIZE] = "";
    json_create_topic(JSON_D2P_TOPIC, device_key, JSON_SYNC_DETAILS_SYNCHRONIZATION_TOPIC, topic);

    strncpy(outbound_message->topic, topic, TOPIC_SIZE);
    strncpy(outbound_message->payload, "", PAYLOAD_SIZE);

    return true;
}
