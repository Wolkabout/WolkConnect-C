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

size_t outbound_message_make_from_readings(parser_t* parser, const char* device_serial, reading_t* first_reading, size_t num_readings,
                                           outbound_message_t* outbound_message)
{
    char topic[TOPIC_SIZE];
    char payload[PAYLOAD_SIZE];
    size_t num_serialized = 0;

    memset(topic, '\0', sizeof(topic));
    memset(payload, '\0', sizeof(payload));

    if (!parser_serialize_topic(parser, device_serial, first_reading, num_readings, topic, sizeof(topic)))
    {
        return num_serialized;
    }

    num_serialized = parser_serialize_readings(parser, first_reading, num_readings, payload, sizeof(payload));
    outbound_message_init(outbound_message, topic, payload);
    return num_serialized;
}

size_t outbound_message_make_from_actuator_status(parser_t* parser, const char* device_serial, actuator_status_t* actuator_status, const char* reference,
                                                  outbound_message_t* outbound_message)
{
    manifest_item_t manifest_item;
    reading_t reading;
    char topic[TOPIC_SIZE];
    char payload[PAYLOAD_SIZE];
    size_t num_serialized = 0;

    memset(topic, '\0', sizeof(topic));
    memset(payload, '\0', sizeof(payload));

    manifest_item_init(&manifest_item, reference, READING_TYPE_ACTUATOR, DATA_TYPE_STRING);
    reading_init(&reading, &manifest_item);
    reading_set_data(&reading, actuator_status_get_value(actuator_status));
    reading_set_actuator_state(&reading, actuator_status_get_state(actuator_status));

    if (!parser_serialize_topic(parser, device_serial, &reading, 1, topic, sizeof(topic)))
    {
        return num_serialized;
    }

    num_serialized = parser_serialize_readings(parser, &reading, 1, payload, sizeof(payload));
    outbound_message_init(outbound_message, topic, payload);
    return num_serialized;
}
