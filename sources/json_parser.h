#ifndef JSON_SERIALIZER_H
#define JSON_SERIALIZER_H

#include "command.h"
#include "reading.h"

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

size_t json_serialize_readings(reading_t* first_reading, size_t num_readings, char* buffer, size_t buffer_size);

size_t json_deserialize_commands(char* buffer, size_t buffer_size, command_t* commands_buffer, size_t commands_buffer_size);

#ifdef __cplusplus
}
#endif

#endif
