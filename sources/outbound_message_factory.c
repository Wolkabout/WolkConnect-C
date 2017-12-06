/*
 * Copyright 2017 WolkAbout Technology s.r.o.
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

#include "outbound_message_factory.h"
#include "outbound_message.h"
#include "parser.h"
#include "reading.h"

#include <stdbool.h>
#include <stddef.h>
#include <string.h>

#define READINGS_PATH_JSON "readings/"
#define READINGS_PATH_WOLKSENSE "sensors/"

#define ACTUATORS_STATUS_PATH "actuators/status/"

#define EVENTS_PATH "events/"

size_t outbound_message_make_from_readings(parser_t* parser, char* device_serial, reading_t* first_reading, size_t num_readings,
                                         outbound_message_t* outbound_message)
{
    char topic[TOPIC_SIZE];
    char payload[PAYLOAD_SIZE];
    size_t num_serialized;

    memset(topic, '\0', sizeof(topic));
    memset(payload, '\0', sizeof(payload));

    const parser_type_t parser_type = parser_get_type(parser);
    if (parser_type == PARSER_TYPE_JSON)
    {
        strcpy(topic, READINGS_PATH_JSON);
        strcat(topic, device_serial);
        strcat(topic, "/");
        strcat(topic, manifest_item_get_reference(reading_get_manifest_item(first_reading)));
    }
    else if (parser_type == PARSER_TYPE_MQTT)
    {
        strcpy(topic, READINGS_PATH_WOLKSENSE);
        strcat(topic, device_serial);
    }

    num_serialized = parser->serialize_readings(first_reading, num_readings, payload, PAYLOAD_SIZE);
    outbound_message_init(outbound_message, topic, payload);
    return num_serialized;
}

size_t outbound_message_make_from_actuator_status(parser_t* parser, char* device_serial, reading_t* first_reading, size_t num_readings,
                                                 outbound_message_t* outbound_message)
{
    char topic[TOPIC_SIZE];
    char payload[PAYLOAD_SIZE];
    size_t num_serialized = 0;

    memset(topic, '\0', sizeof(topic));
    memset(payload, '\0', sizeof(payload));

    const parser_type_t parser_type = parser_get_type(parser);
    if (parser_type == PARSER_TYPE_JSON)
    {
        strcpy(topic, ACTUATORS_STATUS_PATH);
        strcat(topic, device_serial);
        strcat(topic, "/");
        strcat(topic, manifest_item_get_reference(reading_get_manifest_item(first_reading)));
    }
    else if (parser_type == PARSER_TYPE_MQTT)
    {
        strcpy(topic, READINGS_PATH_WOLKSENSE);
        strcat(topic, device_serial);
    }

    num_serialized = parser->serialize_readings(first_reading, num_readings, payload, PAYLOAD_SIZE);
    outbound_message_init(outbound_message, topic, payload);
    return num_serialized;
}

size_t outbound_message_make_from_alarm(parser_t* parser, char* device_serial, reading_t* first_reading, size_t num_readings, outbound_message_t* outbound_message)
{
    char topic[TOPIC_SIZE];
    char payload[PAYLOAD_SIZE];
    size_t num_serialized = 0;

    const parser_type_t parser_type = parser_get_type(parser);
    if (parser_type != PARSER_TYPE_JSON)
    {
        /* Wolksense protocol does not support alarms */
        return num_serialized;
    }

    memset(topic, '\0', sizeof(topic));
    memset(payload, '\0', sizeof(payload));

    strcpy(topic, EVENTS_PATH);
    strcat(topic, device_serial);
    strcat(topic, "/");
    strcat(topic, manifest_item_get_reference(reading_get_manifest_item(first_reading)));

    num_serialized = parser->serialize_readings(first_reading, num_readings, payload, PAYLOAD_SIZE);
    outbound_message_init(outbound_message, topic, payload);
    return num_serialized;
}
