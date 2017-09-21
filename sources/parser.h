#ifndef PARSER_H
#define PARSER_H

#include "reading.h"
#include "command.h"

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    PARSER_TYPE_MQTT = 0,
    PARSER_TYPE_JSON
} parser_type_t;

typedef struct {
    parser_type_t type;

    size_t (*serialize_readings)(reading_t* first_reading, size_t num_readings, char* buffer, size_t buffer_size);
    size_t (*deserialize_commands)(char* buffer, size_t buffer_size, command_t* commands_buffer, size_t commands_buffer_size);
} parser_t;

void initialize_parser(parser_t* parser, parser_type_t parser_type);

size_t parser_serialize_readings(parser_t* parser, reading_t* first_reading, size_t num_readings, char* buffer, size_t buffer_size);
size_t parser_deserialize_commands(parser_t* parser, char* buffer, size_t buffer_size, command_t* commands_buffer, size_t commands_buffer_size);

#ifdef __cplusplus
}
#endif

#endif
