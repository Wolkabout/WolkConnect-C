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
#include "actuator_command.h"
#include "base64.h"
#include "file_management_packet_request.h"
#include "file_management_parameter.h"
#include "file_management_status.h"
#include "firmware_update.h"
#include "jsmn.h"
#include "reading.h"
#include "size_definitions.h"
#include "wolk_utils.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static const char* PING_TOPIC = "ping/";
static const char* READINGS_TOPIC = "d2p/sensor_reading/d/";

static const char* ACTUATORS_STATUS_TOPIC = "d2p/actuator_status/d/";

static const char* EVENTS_TOPIC = "d2p/events/d/";

static const char* CONFIGURATION_GET_TOPIC = "d2p/configuration_get/d/";

static const char* FILE_MANAGEMENT_UPLOAD_STATUS_TOPIC = "d2p/file_upload_status/d/";
static const char* FILE_MANAGEMENT_PACKET_REQUEST_TOPIC = "d2p/file_binary_request/d/";
static const char* FILE_MANAGEMENT_URL_DOWNLOAD_STATUS = "d2p/file_url_download_status/d/";
static const char* FILE_MANAGEMENT_FILE_LIST_UPDATE = "d2p/file_list_update/d/";

static const char* FIRMWARE_UPDATE_STATUS_TOPIC = "d2p/firmware_update_status/d/";
static const char* FIRMWARE_UPDATE_VERSION_TOPIC = "d2p/firmware_version_update/d/";

static bool all_readings_have_equal_rtc(reading_t* first_reading, size_t num_readings)
{
    reading_t* current_reading = first_reading;
    uint64_t rtc = reading_get_rtc(current_reading);

    for (size_t i = 0; i < num_readings; ++i, ++current_reading) {
        if (rtc != reading_get_rtc(current_reading)) {
            return false;
        }
    }

    return true;
}

static bool serialize_sensor(reading_t* reading, char* buffer, size_t buffer_size)
{
    char data_buffer[PARSER_INTERNAL_BUFFER_SIZE];
    if (!reading_get_delimited_data(reading, data_buffer, PARSER_INTERNAL_BUFFER_SIZE)) {
        return false;
    }

    if (reading_get_rtc(reading) > 0
        && snprintf(buffer, buffer_size, "{\"utc\":%ld,\"data\":\"%s\"}", reading_get_rtc(reading), data_buffer)
               >= (int)buffer_size) {
        return false;
    } else if (reading_get_rtc(reading) == 0
               && snprintf(buffer, buffer_size, "{\"data\":\"%s\"}", data_buffer) >= (int)buffer_size) {
        return false;
    }

    return true;
}

static bool serialize_actuator(reading_t* reading, char* buffer, size_t buffer_size)
{
    char data_buffer[PARSER_INTERNAL_BUFFER_SIZE];
    if (!reading_get_delimited_data(reading, data_buffer, PARSER_INTERNAL_BUFFER_SIZE)) {
        return false;
    }

    switch (reading_get_actuator_state(reading)) {
    case ACTUATOR_STATE_READY:
        if (snprintf(buffer, buffer_size, "{\"status\":%s,\"value\":\"%s\"}", "\"READY\"", data_buffer)
            >= (int)buffer_size) {
            return false;
        }
        break;

    case ACTUATOR_STATE_BUSY:
        if (snprintf(buffer, buffer_size, "{\"status\":%s,\"value\":\"%s\"}", "\"BUSY\"", data_buffer)
            >= (int)buffer_size) {
            return false;
        }
        break;

    case ACTUATOR_STATE_ERROR:
        if (snprintf(buffer, buffer_size, "{\"status\":%s,\"value\":\"%s\"}", "\"ERROR\"", data_buffer)
            >= (int)buffer_size) {
            return false;
        }
        break;

    default:
        /* Sanity check */
        WOLK_ASSERT(false);
        return false;
    }

    return true;
}

static bool serialize_alarm(reading_t* reading, char* buffer, size_t buffer_size)
{
    char data_buffer[PARSER_INTERNAL_BUFFER_SIZE];
    if (!reading_get_delimited_data(reading, data_buffer, PARSER_INTERNAL_BUFFER_SIZE)) {
        return false;
    }

    if (reading_get_rtc(reading) > 0
        && snprintf(buffer, buffer_size, "{\"utc\":%ld,\"data\":\"%s\"}", reading_get_rtc(reading), data_buffer)
               >= (int)buffer_size) {
        return false;
    } else if (reading_get_rtc(reading) == 0
               && snprintf(buffer, buffer_size, "{\"data\":\"%s\"}", data_buffer) >= (int)buffer_size) {
        return false;
    }

    return true;
}

static bool serialize_reading(reading_t* reading, char* buffer, size_t buffer_size)
{
    switch (manifest_item_get_reading_type(reading_get_manifest_item(reading))) {
    case READING_TYPE_SENSOR:
        return serialize_sensor(reading, buffer, buffer_size);

    case READING_TYPE_ACTUATOR:
        return serialize_actuator(reading, buffer, buffer_size);

    case READING_TYPE_ALARM:
        return serialize_alarm(reading, buffer, buffer_size);

    default:
        /* Sanity check*/
        WOLK_ASSERT(false);
        return false;
    }
}

static size_t serialize_readings(reading_t* first_reading, size_t num_readings, char* buffer, size_t buffer_size)
{
    WOLK_UNUSED(num_readings);

    return serialize_reading(first_reading, buffer, buffer_size) ? 1 : 0;
}

size_t json_serialize_readings(reading_t* first_reading, size_t num_readings, char* buffer, size_t buffer_size)
{
    /* Sanity check */
    WOLK_ASSERT(num_readings > 0);

    if (num_readings > 1 && all_readings_have_equal_rtc(first_reading, num_readings)) {
        return serialize_readings(first_reading, num_readings, buffer, buffer_size);
    } else {
        return serialize_reading(first_reading, buffer, buffer_size) ? 1 : 0;
    }
}

static bool json_token_str_equal(const char* json, jsmntok_t* tok, const char* s)
{
    if (tok->type == JSMN_STRING && (int)strlen(s) == tok->end - tok->start
        && strncmp(json + tok->start, s, (size_t)(tok->end - tok->start)) == 0) {
        return true;
    }

    return false;
}

static bool deserialize_actuator_command(char* topic, size_t topic_size, char* buffer, size_t buffer_size,
                                         actuator_command_t* command)
{
    WOLK_UNUSED(topic_size);

    jsmn_parser parser;
    jsmntok_t tokens[10]; /* No more than 10 JSON token(s) are expected, check
                             jsmn documentation for token definition */
    jsmn_init(&parser);
    int parser_result = jsmn_parse(&parser, buffer, buffer_size, tokens, WOLK_ARRAY_LENGTH(tokens));

    /* Received JSON must be valid, and top level element must be object */
    if (parser_result < 1 || tokens[0].type != JSMN_OBJECT || parser_result >= (int)WOLK_ARRAY_LENGTH(tokens)) {
        return false;
    }

    char command_buffer[COMMAND_MAX_SIZE];
    char value_buffer[READING_SIZE];

    /* Obtain reference */
    char* reference_start = strrchr(topic, '/');
    if (reference_start == NULL) {
        return false;
    }

    /*Obtain command type*/
    char* command_start = strchr(topic, '/');
    if (command_start == NULL) {
        return false;
    }
    strncpy(command_buffer, strtok(command_start, "/"), COMMAND_MAX_SIZE);
    if (strlen(command_buffer) == NULL) {
        return false;
    }

    /*Obtain values*/
    for (int i = 1; i < parser_result; i++) {
        if (json_token_str_equal(buffer, &tokens[i], "value")) {
            if (snprintf(value_buffer, WOLK_ARRAY_LENGTH(value_buffer), "%.*s", tokens[i + 1].end - tokens[i + 1].start,
                         buffer + tokens[i + 1].start)
                >= (int)WOLK_ARRAY_LENGTH(value_buffer)) {
                return false;
            }

            i++;

        } else {
            return false;
        }
    }

    /*Init actuation*/
    if (strcmp(command_buffer, "actuator_set") == 0) {
        actuator_command_init(command, ACTUATOR_COMMAND_TYPE_SET, "", value_buffer);
    } else {
        actuator_command_init(command, ACTUATOR_COMMAND_TYPE_UNKNOWN, "", value_buffer);
    }

    strncpy(&command->reference[0], reference_start + 1, MANIFEST_ITEM_REFERENCE_SIZE);
    return true;
}

size_t json_deserialize_actuator_commands(char* topic, size_t topic_size, char* buffer, size_t buffer_size,
                                          actuator_command_t* commands_buffer, size_t commands_buffer_size)
{
    WOLK_UNUSED(topic_size);
    WOLK_UNUSED(buffer_size);
    WOLK_UNUSED(commands_buffer_size);

    return deserialize_actuator_command(topic, topic_size, buffer, buffer_size, commands_buffer) ? 1 : 0;
}

bool json_serialize_readings_topic(reading_t* first_Reading, size_t num_readings, const char* device_key, char* buffer,
                                   size_t buffer_size)
{
    WOLK_UNUSED(num_readings);

    manifest_item_t* manifest_item = reading_get_manifest_item(first_Reading);
    reading_type_t reading_type = manifest_item_get_reading_type(manifest_item);

    memset(buffer, '\0', buffer_size);

    switch (reading_type) {
    case READING_TYPE_SENSOR:
        strcpy(buffer, READINGS_TOPIC);
        strcat(buffer, device_key);
        strcat(buffer, "/r/");
        strcat(buffer, manifest_item_get_reference(manifest_item));
        break;

    case READING_TYPE_ACTUATOR:
        strcpy(buffer, ACTUATORS_STATUS_TOPIC);
        strcat(buffer, device_key);
        strcat(buffer, "/r/");
        strcat(buffer, manifest_item_get_reference(manifest_item));
        break;

    case READING_TYPE_ALARM:
        strcpy(buffer, EVENTS_TOPIC);
        strcat(buffer, device_key);
        strcat(buffer, "/r/");
        strcat(buffer, manifest_item_get_reference(manifest_item));
        break;

    default:
        /* Sanity check */
        WOLK_ASSERT(false);
        return false;
    }

    return true;
}

static char* replace_str(char* str, char* orig, char* rep, int start)
{
    static char temp[PARSER_INTERNAL_BUFFER_SIZE];
    static char buffer[PARSER_INTERNAL_BUFFER_SIZE];

    WOLK_ASSERT(sizeof(temp) > strlen(str) - start);
    strcpy(temp, str + start);

    char* p;
    if (!(p = strstr(temp, orig))) {
        return temp;
    }

    WOLK_ASSERT(sizeof(buffer) > p - temp);
    strncpy(buffer, temp, p - temp);
    buffer[p - temp] = '\0';

    sprintf(buffer + (p - temp), "%s%s", rep, p + strlen(orig));
    sprintf(str + start, "%s", buffer);

    return replace_str(str, orig, rep, start + p - temp + 2);
}

size_t json_serialize_configuration(const char* device_key, char (*reference)[CONFIGURATION_REFERENCE_SIZE],
                                    char (*value)[CONFIGURATION_VALUE_SIZE], size_t num_configuration_items,
                                    outbound_message_t* outbound_message)
{
    outbound_message_init(outbound_message, "", "");

    strncpy(outbound_message->topic, CONFIGURATION_GET_TOPIC, strlen(CONFIGURATION_GET_TOPIC));
    /* Serialize topic */
    if (snprintf(outbound_message->topic + strlen(CONFIGURATION_GET_TOPIC), WOLK_ARRAY_LENGTH(outbound_message->topic),
                 "%s", device_key)
        >= (int)WOLK_ARRAY_LENGTH(outbound_message->topic)) {
        return 0;
    }

    /* Serialize payload */
    char* payload = &outbound_message->payload[0];
    const size_t payload_size = sizeof(outbound_message->payload);
    memset(payload, '\0', payload_size);

    if (snprintf(payload, payload_size, "{\"values\":{") >= (int)payload_size) {
        return 0;
    }

    for (size_t i = 0; i < num_configuration_items; ++i) {
        char* configuration_item_reference = reference[i];
        char* configuration_item_value = value[i];

        /* Escape value according to JSON specification */
        replace_str(configuration_item_value, "\\", "\\\\", 0);
        replace_str(configuration_item_value, "\"", "\\\"", 0);

        /* -1 so we can have enough space left to append closing '}' */
        size_t num_bytes_to_write = payload_size - strlen(payload);
        if (snprintf(payload + strlen(payload), payload_size - strlen(payload) - 1, "\"%s\":\"%s\"",
                     configuration_item_reference, configuration_item_value)
            >= (int)num_bytes_to_write - 1) {
            break;
        }

        if (i >= num_configuration_items - 1) {
            break;
        }

        /* +4 for '"', +1 for ':', +1 for ',' delimiter between configuration
         * items, +1 for closing '}' => +7 */
        if (strlen(payload) + strlen(configuration_item_reference) + strlen(configuration_item_value) + 7
            > payload_size) {
            break;
        }

        num_bytes_to_write = payload_size - strlen(payload);
        if (snprintf(payload + strlen(payload), payload_size - strlen(payload), ",") >= (int)num_bytes_to_write) {
            break;
        }
    }

    const size_t num_bytes_to_write = payload_size - strlen(payload);
    if (snprintf(payload + strlen(payload), payload_size - strlen(payload), "}}") >= (int)num_bytes_to_write) {
        return 0;
    }
    return 1;
}

size_t json_deserialize_configuration_command(char* buffer, size_t buffer_size,
                                              configuration_command_t* commands_buffer, size_t commands_buffer_size)
{
    WOLK_UNUSED(commands_buffer_size);

    jsmn_parser parser;
    jsmntok_t tokens[50];

    configuration_command_t* current_config_command = commands_buffer;
    current_config_command->num_configuration_items = 0;

    size_t num_deserialized_config_items = 0;

    jsmn_init(&parser);
    int num_json_tokens = jsmn_parse(&parser, buffer, buffer_size, &tokens[0], WOLK_ARRAY_LENGTH(tokens));

    /* Received JSON must be valid, and top level element must be object*/
    if (num_json_tokens < 1 || tokens[0].type != JSMN_OBJECT) {
        return num_deserialized_config_items;
    }

    configuration_command_init(current_config_command, CONFIGURATION_COMMAND_TYPE_SET);

    for (int i = 0; i < num_json_tokens; i += 2) {
        if (i + 1 >= num_json_tokens || tokens[i + 1].type != JSMN_STRING) {
            continue;
        }

        num_deserialized_config_items++;

        char configuration_item_reference[CONFIGURATION_REFERENCE_SIZE];
        char configuration_item_value[CONFIGURATION_VALUE_SIZE];

        if (snprintf(configuration_item_reference, WOLK_ARRAY_LENGTH(configuration_item_reference), "%.*s",
                     tokens[i + 1].end - tokens[i + 1].start, buffer + tokens[i + 1].start)
            >= (int)WOLK_ARRAY_LENGTH(configuration_item_reference)) {
            continue;
        }

        if (snprintf(configuration_item_value, WOLK_ARRAY_LENGTH(configuration_item_value), "%.*s",
                     tokens[i + 2].end - tokens[i + 2].start, buffer + tokens[i + 2].start)
            >= (int)WOLK_ARRAY_LENGTH(configuration_item_value)) {
            continue;
        }

        configuration_command_add(current_config_command, configuration_item_reference, configuration_item_value);
    }

    return num_deserialized_config_items;
}

static const char* file_management_status_as_str(file_management_status_t* status)
{
    /* Sanity check */
    WOLK_ASSERT(status);

    switch (status->state) {
    case FILE_MANAGEMENT_STATE_FILE_TRANSFER:
        return "FILE_TRANSFER";

    case FILE_MANAGEMENT_STATE_FILE_READY:
        return "FILE_READY";

    case FILE_MANAGEMENT_STATE_ERROR:
        return "ERROR";

    case FILE_MANAGEMENT_STATE_ABORTED:
        return "ABORTED";

    default:
        WOLK_ASSERT(false);
        return "";
    }
}

bool json_serialize_file_management_status(const char* device_key,
                                           file_management_packet_request_t* file_management_packet_request,
                                           file_management_status_t* status, outbound_message_t* outbound_message)
{
    outbound_message_init(outbound_message, "", "");

    /* Serialize topic */
    strncpy(outbound_message->topic, FILE_MANAGEMENT_UPLOAD_STATUS_TOPIC, strlen(FILE_MANAGEMENT_UPLOAD_STATUS_TOPIC));
    if (snprintf(outbound_message->topic + strlen(FILE_MANAGEMENT_UPLOAD_STATUS_TOPIC),
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
    char value_buffer[COMMAND_ARGUMENT_SIZE];

    for (int i = 1; i < parser_result; i++) {
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
    strncpy(outbound_message->topic, FILE_MANAGEMENT_PACKET_REQUEST_TOPIC,
            strlen(FILE_MANAGEMENT_PACKET_REQUEST_TOPIC));
    if (snprintf(outbound_message->topic + strlen(FILE_MANAGEMENT_PACKET_REQUEST_TOPIC),
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
    strncpy(outbound_message->topic, FILE_MANAGEMENT_URL_DOWNLOAD_STATUS, strlen(FILE_MANAGEMENT_URL_DOWNLOAD_STATUS));
    if (snprintf(outbound_message->topic + strlen(FILE_MANAGEMENT_URL_DOWNLOAD_STATUS),
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
    strncpy(outbound_message->topic, FILE_MANAGEMENT_FILE_LIST_UPDATE, strlen(FILE_MANAGEMENT_FILE_LIST_UPDATE));
    if (snprintf(outbound_message->topic + strlen(FILE_MANAGEMENT_FILE_LIST_UPDATE),
                 WOLK_ARRAY_LENGTH(outbound_message->topic), "%s", device_key)
        >= (int)WOLK_ARRAY_LENGTH(outbound_message->topic)) {
        return false;
    }

    /* Serialize payload */
    strncpy(outbound_message->payload, "[", strlen("["));
    for (int i = 0; i < file_list_items; i++) {
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

bool json_serialize_ping_keep_alive_message(const char* device_key, outbound_message_t* outbound_message)
{
    outbound_message_init(outbound_message, "", "");

    /* Serialize topic */
    strncpy(outbound_message->topic, PING_TOPIC, strlen(PING_TOPIC));

    if (snprintf(outbound_message->topic + strlen(PING_TOPIC), WOLK_ARRAY_LENGTH(outbound_message->topic), "%s",
                 device_key)
        >= (int)WOLK_ARRAY_LENGTH(outbound_message->topic)) {
        return false;
    }

    return true;
}

bool json_deserialize_pong_keep_alive_message(char* buffer, size_t buffer_size, utc_command_t* utc_command)
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

    char value_buffer[READING_SIZE];
    /*Obtain values*/
    for (int i = 1; i < parser_result; i++) {
        if (i >= parser_result || tokens[i].type != JSMN_STRING) {
            continue;
        }

        if (json_token_str_equal(buffer, &tokens[i], "value")) {
            if (tokens[i + 1].type == JSMN_PRIMITIVE) {
                if (snprintf(value_buffer, WOLK_ARRAY_LENGTH(value_buffer), "%.*s",
                             tokens[i + 1].end - tokens[i + 1].start, buffer + tokens[i + 1].start)
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
    }

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
    char value_buffer[COMMAND_ARGUMENT_SIZE];

    for (int i = 1; i < parser_result; i++) {
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
    strncpy(outbound_message->topic, FIRMWARE_UPDATE_STATUS_TOPIC, strlen(FIRMWARE_UPDATE_STATUS_TOPIC));
    if (snprintf(outbound_message->topic + strlen(FIRMWARE_UPDATE_STATUS_TOPIC),
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
    strncpy(outbound_message->topic, FIRMWARE_UPDATE_VERSION_TOPIC, strlen(FIRMWARE_UPDATE_VERSION_TOPIC));
    if (snprintf(outbound_message->topic + strlen(FIRMWARE_UPDATE_VERSION_TOPIC),
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
