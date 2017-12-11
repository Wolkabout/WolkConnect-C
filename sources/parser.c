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

#include "parser.h"
#include "configuration_item.h"
#include "json_parser.h"
#include "wolk_utils.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

void parser_init(parser_t* parser, parser_type_t parser_type)
{
    parser->type = parser_type;
    parser->is_initialized = true;

    switch(parser->type)
    {
    case PARSER_TYPE_JSON:
        parser->serialize_readings = json_serialize_readings;
        parser->deserialize_commands = json_deserialize_commands;
        parser->serialize_topic = json_serialize_topic;
        parser->serialize_configuration_items = json_serialize_configuration_items;
        parser->deserialize_configuration_items = json_deserialize_configuration_items;
        break;

    default:
        /* Sanity check */
        WOLK_ASSERT(false);
    }
}

size_t parser_serialize_readings(parser_t* parser, reading_t* first_reading, size_t num_readings, char* buffer, size_t buffer_size)
{
    /* Sanity check */
    WOLK_ASSERT(num_readings > 0);
    WOLK_ASSERT(buffer_size >= PAYLOAD_SIZE);

    return parser->serialize_readings(first_reading, num_readings, buffer, buffer_size);
}

size_t parser_deserialize_commands(parser_t* parser, char* buffer, size_t buffer_size, actuator_command_t* commands_buffer, size_t commands_buffer_size)
{
    /* Sanity check */
    WOLK_ASSERT(buffer_size < PAYLOAD_SIZE);
    WOLK_ASSERT(commands_buffer_size > 0);

    return parser->deserialize_commands(buffer, buffer_size, commands_buffer, commands_buffer_size);
}

bool parser_serialize_topic(parser_t* parser, char* device_serial, reading_t* first_reading, size_t num_readings, char* buffer, size_t buffer_size)
{
    /* Sanity check */
    WOLK_ASSERT(num_readings > 0);
    WOLK_ASSERT(buffer_size >= TOPIC_SIZE);

    return parser->serialize_topic(first_reading, num_readings, device_serial, buffer, buffer_size);
}

size_t parser_serialize_configuration_items(parser_t* parser, configuration_item_t* first_config_item, size_t num_config_items, char* buffer, size_t buffer_size)
{
    /* Sanity check */
    WOLK_ASSERT(num_config_items > 0);
    WOLK_ASSERT(buffer_size >= PAYLOAD_SIZE);

    return parser->serialize_configuration_items(first_config_item, num_config_items, buffer, buffer_size);
}

size_t parser_deserialize_configuration_items(parser_t* parser, char* buffer, size_t buffer_size, configuration_item_command_t *first_config_item, size_t num_config_items)
{
    /* Sanity check */
    WOLK_ASSERT(buffer_size < PAYLOAD_SIZE);
    WOLK_ASSERT(num_config_items > 0);

    return parser->deserialize_configuration_items(buffer, buffer_size, first_config_item, num_config_items);
}

parser_type_t parser_get_type(parser_t* parser)
{
    return parser->type;
}

bool parser_is_initialized(parser_t* parser)
{
    return parser->is_initialized;
}
