#include "parser.h"
#include "mqtt_parser.h"
#include "json_parser.h"
#include "utils.h"

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

void initialize_parser(parser_t* parser, parser_type_t parser_type)
{
    parser->type = parser_type;

    switch(parser->type)
    {
    case PARSER_TYPE_MQTT:
        parser->serialize_readings = mqtt_serialize_readings;
        parser->deserialize_commands = mqtt_deserialize_commands;
        break;

    case PARSER_TYPE_JSON:
        /* Fallthrough */
    default:
        /* Sanity check */
        ASSERT(false);
    }
}

size_t parser_serialize_readings(parser_t* parser, reading_t* first_reading, size_t num_readings, char* buffer, size_t buffer_size)
{
    return parser->serialize_readings(first_reading, num_readings, buffer, buffer_size);
}

size_t parser_deserialize_commands(parser_t* parser, char* buffer, size_t buffer_size, command_t* commands_buffer, size_t commands_buffer_size)
{
    UNUSED(buffer_size);
    return parser->deserialize_commands(buffer, buffer_size, commands_buffer, commands_buffer_size);
}
