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

#include "json_parser.h"
#include "model/feed_value_message.h"
#include "model/file_management/file_management_packet_request.h"
#include "model/file_management/file_management_parameter.h"
#include "model/file_management/file_management_status.h"
#include "model/firmware_update.h"
#include "model/reading.h"
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

const char* JSON_READINGS_TOPIC = "d2p/sensor_reading/d/";

const char* JSON_FILE_MANAGEMENT_UPLOAD_STATUS_TOPIC = "d2p/file_upload_status/d/";
const char* JSON_FILE_MANAGEMENT_PACKET_REQUEST_TOPIC = "d2p/file_binary_request/d/";
const char* JSON_FILE_MANAGEMENT_URL_DOWNLOAD_STATUS = "d2p/file_url_download_status/d/";
const char* JSON_FILE_MANAGEMENT_FILE_LIST_UPDATE = "d2p/file_list_update/d/";

const char* JSON_FIRMWARE_UPDATE_STATUS_TOPIC = "d2p/firmware_update_status/d/";
const char* JSON_FIRMWARE_UPDATE_VERSION_TOPIC = "d2p/firmware_version_update/d/";

const char JSON_P2D_TOPIC[TOPIC_DIRECTION_SIZE] = "p2d";
const char JSON_D2P_TOPIC[TOPIC_DIRECTION_SIZE] = "d2p";
const char JSON_FEED_REGISTRATION_TOPIC[MESSAGE_TYPE_SIZE] = "feed_registration";
const char JSON_FEED_REMOVAL_TOPIC[MESSAGE_TYPE_SIZE] = "feed_removal";
const char JSON_FEED_VALUES_MESSAGE_TOPIC[MESSAGE_TYPE_SIZE] = "feed_values";
const char JSON_PULL_FEEDS_TOPIC[MESSAGE_TYPE_SIZE] = "pull_feed_values";
const char JSON_ATTRIBUTE_REGISTRATION_TOPIC[MESSAGE_TYPE_SIZE] = "attribute_registration";
const char JSON_PARAMETERS_TOPIC[MESSAGE_TYPE_SIZE] = "parameters";
const char JSON_PULL_PARAMETERS_TOPIC[MESSAGE_TYPE_SIZE] = "pull_parameters";
const char JSON_SYNC_PARAMETERS_TOPIC[MESSAGE_TYPE_SIZE] = "synchronize_parameters";
const char JSON_SYNC_TIME_TOPIC[MESSAGE_TYPE_SIZE] = "time";
const char JSON_ERROR_TOPIC[MESSAGE_TYPE_SIZE] = "error";


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
    outbound_message_init(outbound_message, "", "");

    /* Serialize topic */
    strncpy(outbound_message->topic, JSON_FILE_MANAGEMENT_UPLOAD_STATUS_TOPIC,
            strlen(JSON_FILE_MANAGEMENT_UPLOAD_STATUS_TOPIC));
    if (snprintf(outbound_message->topic + strlen(JSON_FILE_MANAGEMENT_UPLOAD_STATUS_TOPIC),
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
    jsmn_parser parser;
    jsmntok_t tokens[12]; /* No more than 12 JSON token(s) are expected, check
                             jsmn documentation for token definition */
    jsmn_init(&parser);
    const int parser_result = jsmn_parse(&parser, buffer, buffer_size, tokens, WOLK_ARRAY_LENGTH(tokens));

    /* Received JSON must be valid, and top level element must be object */
    if (parser_result < 1 || tokens[0].type != JSMN_OBJECT || parser_result >= (int)WOLK_ARRAY_LENGTH(tokens)) {
        return false;
    }

    file_management_parameter_init(parameter);

    /* Obtain command type and value */
    char value_buffer[READING_ELEMENT_SIZE];

    for (size_t i = 1; i < parser_result; i++) {
        if (json_token_str_equal(buffer, &tokens[i], "fileName")) {
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
        } else if (json_token_str_equal(buffer, &tokens[i], "fileSize")) {
            if (snprintf(value_buffer, WOLK_ARRAY_LENGTH(value_buffer), "%.*s", tokens[i + 1].end - tokens[i + 1].start,
                         buffer + tokens[i + 1].start)
                >= (int)WOLK_ARRAY_LENGTH(value_buffer)) {
                return false;
            }

            file_management_parameter_set_file_size(parameter, (size_t)atoi(value_buffer));
            i++;
        } else if (json_token_str_equal(buffer, &tokens[i], "fileHash")) {
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
        } else if (json_token_str_equal(buffer, &tokens[i], "fileUrl")) {
            if (snprintf(value_buffer, WOLK_ARRAY_LENGTH(value_buffer), "%.*s", tokens[i + 1].end - tokens[i + 1].start,
                         buffer + tokens[i + 1].start)
                >= (int)WOLK_ARRAY_LENGTH(value_buffer)) {
                return false;
            }

            file_management_parameter_set_file_url(parameter, value_buffer);
            i++;
        } else if (json_token_str_equal(buffer, &tokens[i], "result")) {
            if (snprintf(value_buffer, WOLK_ARRAY_LENGTH(value_buffer), "%.*s", tokens[i + 1].end - tokens[i + 1].start,
                         buffer + tokens[i + 1].start)
                >= (int)WOLK_ARRAY_LENGTH(value_buffer)) {
                return false;
            }
            file_management_parameter_set_result(parameter, value_buffer);
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
    outbound_message_init(outbound_message, "", "");

    /* Serialize topic */
    strncpy(outbound_message->topic, JSON_FILE_MANAGEMENT_PACKET_REQUEST_TOPIC,
            strlen(JSON_FILE_MANAGEMENT_PACKET_REQUEST_TOPIC));
    if (snprintf(outbound_message->topic + strlen(JSON_FILE_MANAGEMENT_PACKET_REQUEST_TOPIC),
                 WOLK_ARRAY_LENGTH(outbound_message->topic), "%s", device_key)
        >= (int)WOLK_ARRAY_LENGTH(outbound_message->topic)) {
        return false;
    }

    /* Serialize payload */
    if (snprintf(outbound_message->payload, WOLK_ARRAY_LENGTH(outbound_message->payload),
                 "{\"fileName\": \"%s\", \"chunkIndex\":%llu, \"chunkSize\":%llu}",
                 file_management_packet_request_get_file_name(file_management_packet_request),
                 (unsigned long long int)file_management_packet_request_get_chunk_index(file_management_packet_request),
                 (unsigned long long int)file_management_packet_request_get_chunk_size(file_management_packet_request))
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
    outbound_message_init(outbound_message, "", "");

    /* Serialize topic */
    strncpy(outbound_message->topic, JSON_FILE_MANAGEMENT_URL_DOWNLOAD_STATUS,
            strlen(JSON_FILE_MANAGEMENT_URL_DOWNLOAD_STATUS));
    if (snprintf(outbound_message->topic + strlen(JSON_FILE_MANAGEMENT_URL_DOWNLOAD_STATUS),
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
    outbound_message_init(outbound_message, "", "");
    memset(outbound_message->topic, '\0', TOPIC_SIZE);
    memset(outbound_message->payload, '\0', PAYLOAD_SIZE);

    /* Serialize topic */
    strncpy(outbound_message->topic, JSON_FILE_MANAGEMENT_FILE_LIST_UPDATE,
            strlen(JSON_FILE_MANAGEMENT_FILE_LIST_UPDATE));
    if (snprintf(outbound_message->topic + strlen(JSON_FILE_MANAGEMENT_FILE_LIST_UPDATE),
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
    jsmntok_t tokens[12]; /* No more than 12 JSON token(s) are expected, check
                             jsmn documentation for token definition */
    jsmn_init(&parser);
    const int parser_result = jsmn_parse(&parser, buffer, buffer_size, tokens, WOLK_ARRAY_LENGTH(tokens));

    /* Received JSON must be valid, and top level element must be object */
    if (parser_result < 1 || tokens[0].type != JSMN_OBJECT || parser_result >= (int)WOLK_ARRAY_LENGTH(tokens)) {
        return false;
    }

    firmware_update_parameter_init(parameter);

    /* Obtain command type and value */
    char value_buffer[READING_ELEMENT_SIZE];

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
    outbound_message_init(outbound_message, "", "");

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

bool json_serialize_firmware_update_version(const char* device_key, char* firmware_update_version,
                                            outbound_message_t* outbound_message)
{
    outbound_message_init(outbound_message, "", "");

    memset(outbound_message->topic, '\0', sizeof(outbound_message->topic));
    memset(outbound_message->payload, '\0', sizeof(outbound_message->payload));

    /* Serialize topic */
    strncpy(outbound_message->topic, JSON_FIRMWARE_UPDATE_VERSION_TOPIC, strlen(JSON_FIRMWARE_UPDATE_VERSION_TOPIC));
    if (snprintf(outbound_message->topic + strlen(JSON_FIRMWARE_UPDATE_VERSION_TOPIC),
                 WOLK_ARRAY_LENGTH(outbound_message->topic), "%s", device_key)
        >= (int)WOLK_ARRAY_LENGTH(outbound_message->topic)) {
        return false;
    }

    /* Serialize payload */
    if (snprintf(outbound_message->payload, WOLK_ARRAY_LENGTH(outbound_message->payload), "%s", firmware_update_version)
        >= (int)WOLK_ARRAY_LENGTH(outbound_message->payload)) {
        return false;
    }

    return true;
}

bool json_deserialize_time(char* buffer, size_t buffer_size, utc_command_t* utc_command)
{
    jsmn_parser parser;
    jsmntok_t tokens[10]; /* No more than 10 JSON token(s) are expected, check
                             jsmn documentation for token definition */
    jsmn_init(&parser);
    int parser_result = jsmn_parse(&parser, buffer, buffer_size, tokens, WOLK_ARRAY_LENGTH(tokens));

    /* Received JSON must be valid, and top level element must be object */
    if (parser_result < 1 || tokens[0].type != JSMN_OBJECT || parser_result >= (int)WOLK_ARRAY_LENGTH(tokens)) {
        return false;
    }

    char value_buffer[READING_ELEMENT_SIZE];
    /*Obtain values*/
    for (size_t i = 1; i < parser_result; i++) {

        if (tokens[i].type == JSMN_PRIMITIVE) {
            if (snprintf(value_buffer, WOLK_ARRAY_LENGTH(value_buffer), "%.*s", tokens[i].end - tokens[i].start,
                         buffer + tokens[i].start)
                >= (int)WOLK_ARRAY_LENGTH(value_buffer)) {
                return false;
            }
            uint64_t conversion_result = strtod(value_buffer, NULL);
            if (conversion_result == NULL) {
                return false;
            }
            utc_command->utc = conversion_result;
        }
    }

    return true;
}

bool json_create_topic(char direction[TOPIC_DIRECTION_SIZE], const char device_key[DEVICE_KEY_SIZE],
                       char message_type[MESSAGE_TYPE_SIZE], char topic[TOPIC_SIZE])
{
    topic[0] = '\0';
    strcat(topic, direction);
    strcat(topic, "/");
    strcat(topic, device_key);
    strcat(topic, "/");
    strcat(topic, message_type);

    return true;
}
size_t json_deserialize_feed_value_message(char* buffer, size_t buffer_size, feed_value_message_t* feed_value_message,
                                           size_t msg_buffer_size)
{
    jsmn_parser parser;
    jsmntok_t tokens[10]; /* No more than 10 JSON token(s) are expected, check
                             jsmn documentation for token definition */
    jsmn_init(&parser);
    int parser_result = jsmn_parse(&parser, buffer, buffer_size, tokens, WOLK_ARRAY_LENGTH(tokens));

    /* Received JSON must be valid, and top level element must be object */
    if (parser_result < 1 || tokens[0].type != JSMN_OBJECT || parser_result >= (int)WOLK_ARRAY_LENGTH(tokens)) {
        return false;
    }
    // TODO Figure out the JSMN parser
}
size_t json_deserialize_parameter_message(char* buffer, size_t buffer_size, parameter_t* parameter_message,
                                          size_t msg_buffer_size)
{
    // TODO
    return 0;
}

static bool serialize_reading(reading_t* reading, data_type_t type, size_t reading_element_size, char* buffer,
                              size_t buffer_size)
{
    char data_buffer[PAYLOAD_SIZE] = "";

    for (size_t i = 0; i < reading_element_size; ++i) {
        strcat(data_buffer, reading->reading_data[i]);

        if (i < reading_element_size - 1)
            strcat(data_buffer, ",");
    }

    if (reading_get_utc(reading) > 0
        && snprintf(buffer, buffer_size, "[{\"%s\":%s%s%s,\"timestamp\":%ld}]", reading->reference,
                    type == NUMERIC ? "" : "\"", data_buffer, type == NUMERIC ? "" : "\"", reading_get_utc(reading))
               >= (int)buffer_size) {
        return false;
    } else if (reading_get_utc(reading) == 0
               && snprintf(buffer, buffer_size, "[{\"%s\":%s%s%s}]", reading->reference, type == NUMERIC ? "" : "\"",
                           data_buffer, type == NUMERIC ? "" : "\"")
                      >= (int)buffer_size) {
        return false;
    }

    return true;
}

static size_t serialize_readings(reading_t* readings, data_type_t type, size_t num_readings, char* buffer,
                                 size_t buffer_size)
{
    WOLK_UNUSED(num_readings);

    char data_buffer[PAYLOAD_SIZE] = "";
    strcat(buffer, "[");

    for (size_t i = 0; i < num_readings; ++i) {
        // when it consists of more readings with the same reference it has to have utc
        if (reading_get_utc(readings) == 0)
            return false;

        if (snprintf(data_buffer, buffer_size, "{\"%s\":%s%s%s,\"timestamp\":%ld}%s", readings->reference,
                     type == NUMERIC ? "" : "\"", readings->reading_data[i], type == NUMERIC ? "" : "\"",
                     reading_get_utc(readings), (num_readings > 1 && i < (num_readings - 1)) ? "," : "")
            >= (int)buffer_size)
            return false;

        strcat(buffer, data_buffer);
    }

    strcat(buffer, "]");
    return true;
}

size_t json_serialize_readings(reading_t* readings, data_type_t type, size_t number_of_readings,
                               size_t reading_element_size, char* buffer, size_t buffer_size)
{
    /* Sanity check */
    WOLK_ASSERT(num_readings > 0);

    if (number_of_readings > 1) {
        return serialize_readings(readings, type, number_of_readings, buffer, buffer_size);
    } else {
        return serialize_reading(readings, type, reading_element_size, buffer, buffer_size) ? 1 : 0;
    }
}
bool json_serialize_attribute(const char* device_key, attribute_t* attribute, outbound_message_t* outbound_message)
{
    char payload[PAYLOAD_SIZE];
    char topic[TOPIC_SIZE] = "";
    json_create_topic(JSON_D2P_TOPIC, device_key, JSON_ATTRIBUTE_REGISTRATION_TOPIC, topic);

    if (sprintf(payload, "[{\"name\": \"%s\", \"dataType\": \"%s\", \"value\": \"%s\"}]", attribute->name,
                attribute->data_type, attribute->value)
        >= (int)WOLK_ARRAY_LENGTH(outbound_message->payload))
        return false;

    strncpy(outbound_message->topic, topic, TOPIC_SIZE);
    strncpy(outbound_message->payload, payload, PAYLOAD_SIZE);

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

        strcat(outbound_message->payload, payload);
        parameter++;
    }

    strncpy(outbound_message->topic, topic, TOPIC_SIZE);
    strcat(outbound_message->payload, "}");

    return true;
}

bool json_serialize_feed_registration(const char* device_key, feed_t* feed, size_t number_of_feeds,
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

        if (strlen(payload)
            > (PAYLOAD_SIZE - strlen(outbound_message->payload))) // TODO: apply the same check everywhere
            return false;

        strcat(outbound_message->payload, payload);
        feed++;
    }

    strcat(outbound_message->payload, "]");
    strncpy(outbound_message->topic, topic, TOPIC_SIZE);

    return true;
}
bool json_serialize_feed_removal(const char* device_key, feed_t* feed, size_t number_of_feeds,
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

    strncpy(outbound_message->topic, topic, TOPIC_SIZE);
    strcat(outbound_message->payload, "[");

    for (int i = 0; i < number_of_parameters; ++i) {
        strcat(outbound_message->payload, "\"");
        strcat(outbound_message->payload, parameters->name);
        strcat(outbound_message->payload, "\"");

        if (number_of_parameters > 1 && i < (number_of_parameters - 1))
            strcat(outbound_message->payload, ",");
        parameters++;
    }

    strcat(outbound_message->payload, "]");

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
