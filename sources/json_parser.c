#include "json_parser.h"
#include "reading.h"
#include "utils.h"

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

size_t json_serialize_readings(reading_t* first_reading, size_t num_readings, char* buffer, size_t buffer_size)
{
    UNUSED(first_reading);
    UNUSED(num_readings);
    UNUSED(buffer);
    UNUSED(buffer_size);

    return 0;
}

size_t json_deserialize_commands(char* buffer, size_t buffer_size, command_t* commands_buffer, size_t commands_buffer_size)
{
    UNUSED(buffer);
    UNUSED(buffer_size);
    UNUSED(commands_buffer);
    UNUSED(commands_buffer_size);

    return 0;
}
