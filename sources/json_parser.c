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
#include "firmware_update_command.h"
#include "firmware_update_packet_request.h"
#include "firmware_update_status.h"
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

static const char* READINGS_TOPIC = "readings/";
static const char* ACTUATORS_STATUS_TOPIC = "actuators/status/";
static const char* EVENTS_TOPIC = "events/";

static bool all_readings_have_equal_rtc(reading_t* first_reading, size_t num_readings)
{
    reading_t* current_reading = first_reading;
    uint32_t rtc = reading_get_rtc(current_reading);

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
        && snprintf(buffer, buffer_size, "{\"utc\":%u,\"data\":\"%s\"}", reading_get_rtc(reading), data_buffer)
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
        && snprintf(buffer, buffer_size, "{\"utc\":%u,\"data\":\"%s\"}", reading_get_rtc(reading), data_buffer)
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
    jsmn_parser parser;
    jsmntok_t tokens[10]; /* No more than 10 JSON token(s) are expected, check
                             jsmn documentation for token definition */
    jsmn_init(&parser);
    int parser_result = jsmn_parse(&parser, buffer, buffer_size, tokens, WOLK_ARRAY_LENGTH(tokens));

    /* Received JSON must be valid, and top level element must be object */
    if (parser_result < 1 || tokens[0].type != JSMN_OBJECT || parser_result >= (int)WOLK_ARRAY_LENGTH(tokens)) {
        return false;
    }

    /* Obtain command type and value */
    char command_buffer[COMMAND_MAX_SIZE];
    char value_buffer[READING_SIZE];
    for (int i = 1; i < parser_result; i++) {
        if (json_token_str_equal(buffer, &tokens[i], "command")) {
            if (snprintf(command_buffer, WOLK_ARRAY_LENGTH(command_buffer), "%.*s",
                         tokens[i + 1].end - tokens[i + 1].start, buffer + tokens[i + 1].start)
                >= (int)WOLK_ARRAY_LENGTH(command_buffer)) {
                return false;
            }

            i++;
        } else if (json_token_str_equal(buffer, &tokens[i], "value")) {
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

    /* Obtain actuator reference */
    char* reference_start = strrchr(topic, '/');
    if (reference_start == NULL) {
        return false;
    }

    if (strcmp(command_buffer, "STATUS") == 0) {
        actuator_command_init(command, ACTUATOR_COMMAND_TYPE_STATUS, "", "");
    } else if (strcmp(command_buffer, "SET") == 0) {
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
        strcat(buffer, "/");
        strcat(buffer, manifest_item_get_reference(manifest_item));
        break;

    case READING_TYPE_ACTUATOR:
        strcpy(buffer, ACTUATORS_STATUS_TOPIC);
        strcat(buffer, device_key);
        strcat(buffer, "/");
        strcat(buffer, manifest_item_get_reference(manifest_item));
        break;

    case READING_TYPE_ALARM:
        strcpy(buffer, EVENTS_TOPIC);
        strcat(buffer, device_key);
        strcat(buffer, "/");
        strcat(buffer, manifest_item_get_reference(manifest_item));
        break;

    default:
        /* Sanity check */
        WOLK_ASSERT(false);
        return false;
    }

    return true;
}

bool json_serialize_configuration(const char* device_key, char (*reference)[CONFIGURATION_REFERENCE_SIZE],
                                  char (*value)[CONFIGURATION_VALUE_SIZE], size_t num_configuration_items,
                                  outbound_message_t* outbound_message)
{
    outbound_message_init(outbound_message, "", "");

    /* Serialize topic */
    if (snprintf(outbound_message->topic, WOLK_ARRAY_LENGTH(outbound_message->topic), "configurations/current/%s",
                 device_key)
        >= (int)WOLK_ARRAY_LENGTH(outbound_message->topic)) {
        return false;
    }

    /* Serialize payload */
    char* payload = &outbound_message->payload[0];
    const size_t payload_size = sizeof(outbound_message->payload);
    memset(payload, '\0', payload_size);

    if (snprintf(payload, sizeof(payload), "{") >= (int)payload_size) {
        return false;
    }

    for (size_t i = 0; i < num_configuration_items; ++i) {
        char* configuration_item_reference = reference[i];
        char* configuration_item_value = value[i];

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
    if (snprintf(payload + strlen(payload), payload_size - strlen(payload), "}") >= (int)num_bytes_to_write) {
        return false;
    }
    return true;
}

size_t json_deserialize_configuration_command(char* buffer, size_t buffer_size,
                                              configuration_command_t* commands_buffer, size_t commands_buffer_size)
{
    jsmn_parser parser;
    jsmntok_t tokens[50];

    configuration_command_t* current_config_command = commands_buffer;
    size_t num_deserialized_config_items = 0;

    char command_buffer[COMMAND_MAX_SIZE];
    memset(commands_buffer, '\0', WOLK_ARRAY_LENGTH(command_buffer));

    jsmn_init(&parser);
    int num_json_tokens = jsmn_parse(&parser, buffer, buffer_size, tokens, WOLK_ARRAY_LENGTH(tokens));

    /* Received JSON must be valid, and top level element must be object*/
    if (num_json_tokens < 1 || tokens[0].type != JSMN_OBJECT) {
        return num_deserialized_config_items;
    }

    for (int i = 1; i < num_json_tokens && num_deserialized_config_items < commands_buffer_size; i++) {
        if (json_token_str_equal(buffer, &tokens[i], "command")) {
            if (snprintf(command_buffer, WOLK_ARRAY_LENGTH(command_buffer), "%.*s",
                         tokens[i + 1].end - tokens[i + 1].start, buffer + tokens[i + 1].start)
                >= (int)WOLK_ARRAY_LENGTH(command_buffer)) {
                continue;
            }

            if (strcmp(command_buffer, "CURRENT") == 0) {
                configuration_command_init(current_config_command, CONFIGURATION_COMMAND_TYPE_CURRENT);
                ++num_deserialized_config_items;
                break;
            } else if (strcmp(command_buffer, "SET") == 0) {
                configuration_command_init(current_config_command, CONFIGURATION_COMMAND_TYPE_SET);
            }

            i += 1;
        } else if (json_token_str_equal(buffer, &tokens[i], "values")) {
            if (strlen(command_buffer) == 0) {
                continue;
            }

            for (int j = i + 2; j < num_json_tokens; j += 2) {
                char configuration_item_reference[CONFIGURATION_REFERENCE_SIZE];
                char configuration_item_value[CONFIGURATION_VALUE_SIZE];

                if (snprintf(configuration_item_reference, WOLK_ARRAY_LENGTH(configuration_item_reference), "%.*s",
                             tokens[j].end - tokens[j].start, buffer + tokens[j].start)
                    >= (int)WOLK_ARRAY_LENGTH(configuration_item_reference)) {
                    continue;
                }

                if (snprintf(configuration_item_value, WOLK_ARRAY_LENGTH(configuration_item_value), "%.*s",
                             tokens[j + 1].end - tokens[j + 1].start, buffer + tokens[j + 1].start)
                    >= (int)WOLK_ARRAY_LENGTH(configuration_item_value)) {
                    continue;
                }

                configuration_command_add(current_config_command, configuration_item_reference,
                                          configuration_item_value);
            }

            ++num_deserialized_config_items;
            break;
        }
    }

    return num_deserialized_config_items;
}

static const char* firmware_update_status_as_str(firmware_update_status_t* status)
{
    /* Sanity check */
    WOLK_ASSERT(status);

    switch (status->status) {
    case FIRMWARE_UPDATE_STATE_FILE_TRANSFER:
        return "FILE_TRANSFER";

    case FIRMWARE_UPDATE_STATE_FILE_READY:
        return "FILE_READY";

    case FIRMWARE_UPDATE_STATE_INSTALLATION:
        return "INSTALLATION";

    case FIRMWARE_UPDATE_STATE_COMPLETED:
        return "COMPLETED";

    case FIRMWARE_UPDATE_STATE_ERROR:
        return "ERROR";

    case FIRMWARE_UPDATE_STATE_ABORTED:
        return "ABORTED";

    default:
        WOLK_ASSERT(false);
        return "";
    }
}

bool json_serialize_firmware_update_status(const char* device_key, firmware_update_status_t* status,
                                           outbound_message_t* outbound_message)
{
    outbound_message_init(outbound_message, "", "");

    /* Serialize topic */
    if (snprintf(outbound_message->topic, WOLK_ARRAY_LENGTH(outbound_message->topic), "service/status/firmware/%s",
                 device_key)
        >= (int)WOLK_ARRAY_LENGTH(outbound_message->topic)) {
        return false;
    }

    /* Serialize payload */
    const firmware_update_error_t error = firmware_update_status_get_error(status);
    if (error == FIRMWARE_UPDATE_ERROR_NONE) {
        if (snprintf(outbound_message->payload, WOLK_ARRAY_LENGTH(outbound_message->payload), "{\"status\":\"%s\"}",
                     firmware_update_status_as_str(status))
            >= (int)WOLK_ARRAY_LENGTH(outbound_message->payload)) {
            return false;
        }
    } else {
        if (snprintf(outbound_message->payload, WOLK_ARRAY_LENGTH(outbound_message->payload),
                     "{\"status\":%s,\"error\":%d}", firmware_update_status_as_str(status),
                     firmware_update_status_get_error(status))
            >= (int)WOLK_ARRAY_LENGTH(outbound_message->payload)) {
            return false;
        }
    }

    return true;
}

bool json_deserialize_firmware_update_command(char* buffer, size_t buffer_size, firmware_update_command_t* command)
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

    firmware_update_command_init(command);

    /* Obtain command type and value */
    char value_buffer[COMMAND_ARGUMENT_SIZE];

    for (int i = 1; i < parser_result; i++) {
        if (json_token_str_equal(buffer, &tokens[i], "command")) {
            if (snprintf(value_buffer, WOLK_ARRAY_LENGTH(value_buffer), "%.*s", tokens[i + 1].end - tokens[i + 1].start,
                         buffer + tokens[i + 1].start)
                >= (int)WOLK_ARRAY_LENGTH(value_buffer)) {
                return false;
            }

            if (strcmp(value_buffer, "FILE_UPLOAD") == 0) {
                firmware_update_command_set_type(command, FIRMWARE_UPDATE_COMMAND_TYPE_FILE_UPLOAD);
            } else if (strcmp(value_buffer, "URL_DOWNLOAD") == 0) {
                firmware_update_command_set_type(command, FIRMWARE_UPDATE_COMMAND_TYPE_URL_DOWNLOAD);
            } else if (strcmp(value_buffer, "INSTALL") == 0) {
                firmware_update_command_set_type(command, FIRMWARE_UPDATE_COMMAND_TYPE_INSTALL);
            } else if (strcmp(value_buffer, "ABORT") == 0) {
                firmware_update_command_set_type(command, FIRMWARE_UPDATE_COMMAND_TYPE_ABORT);
            } else {
                firmware_update_command_set_type(command, FIRMWARE_UPDATE_COMMAND_TYPE_UNKNOWN);
            }
            i++;
        } else if (json_token_str_equal(buffer, &tokens[i], "fileName")) {
            if (snprintf(value_buffer, WOLK_ARRAY_LENGTH(value_buffer), "%.*s", tokens[i + 1].end - tokens[i + 1].start,
                         buffer + tokens[i + 1].start)
                >= (int)WOLK_ARRAY_LENGTH(value_buffer)) {
                return false;
            }

            firmware_update_command_set_filename(command, value_buffer);
            i++;
        } else if (json_token_str_equal(buffer, &tokens[i], "fileSize")) {
            if (snprintf(value_buffer, WOLK_ARRAY_LENGTH(value_buffer), "%.*s", tokens[i + 1].end - tokens[i + 1].start,
                         buffer + tokens[i + 1].start)
                >= (int)WOLK_ARRAY_LENGTH(value_buffer)) {
                return false;
            }

            firmware_update_command_set_file_size(command, (size_t)atoi(value_buffer));
            i++;
        } else if (json_token_str_equal(buffer, &tokens[i], "fileHash")) {
            if (snprintf(value_buffer, WOLK_ARRAY_LENGTH(value_buffer), "%.*s", tokens[i + 1].end - tokens[i + 1].start,
                         buffer + tokens[i + 1].start)
                >= (int)WOLK_ARRAY_LENGTH(value_buffer)) {
                return false;
            }

            const size_t output_size = base64_decode(value_buffer, NULL, strlen(value_buffer));
            if (output_size > FIRMWARE_UPDATE_HASH_SIZE) {
                return false;
            }
            base64_decode(value_buffer, (BYTE*)command->file_hash, strlen(value_buffer));
            i++;
        } else if (json_token_str_equal(buffer, &tokens[i], "autoInstall")) {
            if (snprintf(value_buffer, WOLK_ARRAY_LENGTH(value_buffer), "%.*s", tokens[i + 1].end - tokens[i + 1].start,
                         buffer + tokens[i + 1].start)
                >= (int)WOLK_ARRAY_LENGTH(value_buffer)) {
                return false;
            }


            if (value_buffer[0] == 't') {
                firmware_update_command_set_auto_install(command, true);
            } else {
                firmware_update_command_set_auto_install(command, false);
            }
            i++;
        } else if (json_token_str_equal(buffer, &tokens[i], "fileUrl")) {
            if (snprintf(value_buffer, WOLK_ARRAY_LENGTH(value_buffer), "%.*s", tokens[i + 1].end - tokens[i + 1].start,
                         buffer + tokens[i + 1].start)
                >= (int)WOLK_ARRAY_LENGTH(value_buffer)) {
                return false;
            }

            firmware_update_command_set_file_url(command, value_buffer);
            i++;
        } else {
            return false;
        }
    }
    return true;
}

bool json_serialize_firmware_update_packet_request(const char* device_key,
                                                   firmware_update_packet_request_t* firmware_update_packet_request,
                                                   outbound_message_t* outbound_message)
{
    outbound_message_init(outbound_message, "", "");

    /* Serialize topic */
    if (snprintf(outbound_message->topic, WOLK_ARRAY_LENGTH(outbound_message->topic), "service/status/file/%s",
                 device_key)
        >= (int)WOLK_ARRAY_LENGTH(outbound_message->topic)) {
        return false;
    }

    /* Serialize payload */
    if (snprintf(outbound_message->payload, WOLK_ARRAY_LENGTH(outbound_message->payload),
                 "{\"fileName\":\"%s\",\"chunkIndex\":%llu,\"chunkSize\":%llu}",
                 firmware_update_packet_request_get_file_name(firmware_update_packet_request),
                 (unsigned long long int)firmware_update_packet_request_get_chunk_index(firmware_update_packet_request),
                 (unsigned long long int)firmware_update_packet_request_get_chunk_size(firmware_update_packet_request))
        >= (int)WOLK_ARRAY_LENGTH(outbound_message->payload)) {
        return false;
    }

    return true;
}

bool json_serialize_firmware_update_version(const char* device_key, const char* version,
                                            outbound_message_t* outbound_message)
{
    outbound_message_init(outbound_message, "", "");

    /* Serialize topic */
    if (snprintf(outbound_message->topic, WOLK_ARRAY_LENGTH(outbound_message->topic), "firmware/version/%s", device_key)
        >= (int)WOLK_ARRAY_LENGTH(outbound_message->topic)) {
        return false;
    }

    /* Serialize payload */
    if (snprintf(outbound_message->payload, WOLK_ARRAY_LENGTH(outbound_message->payload), "%s", version)
        >= (int)WOLK_ARRAY_LENGTH(outbound_message->payload)) {
        return false;
    }

    return true;
}
