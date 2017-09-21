#include "mqtt_parser.h"
#include "size_definitions.h"
#include "manifest_item.h"
#include "reading.h"
#include "utils.h"

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

enum {
    COMMAND_MAX_SIZE = 10,

    COMMAND_MAX_ARGUMENT_PART_SIZE = MANIFEST_ITEM_MAX_REFERENCE_SIZE + 1 + READING_MAX_READING_SIZE /* +1 for ':' delimiter */
};

static bool append_to_buffer(char* buffer, size_t buffer_size, char* apendee)
{
    if (strlen(buffer) + strlen(apendee) > buffer_size) {
        return false;
    }

    strcpy(buffer + strlen(buffer), apendee);
    return true;
}

static bool append_actuator_status(char* buffer, size_t buffer_size, actuator_state_t actuator_status)
{
    char reading_buffer[SERIALIZER_MAX_PARSER_INTERNAL_BUFFER_SIZE];

    switch (actuator_status) {
    case ACTUATOR_STATUS_READY:
        sprintf(reading_buffer, ":READY");
        break;

    case ACTUATOR_STATUS_BUSY:
        sprintf(reading_buffer, ":BUSY");
        break;

    case ACTUATOR_STATUS_ERROR:
        sprintf(reading_buffer, ":ERROR");
        break;

    default:
        ASSERT(false);
    }

    return append_to_buffer(buffer, buffer_size, reading_buffer);
}

static bool append_reading_prefix(reading_t* reading, char* buffer, size_t buffer_size)
{
    int signed_buffer_size = buffer_size;
    if (manifest_item_get_reading_type(reading_get_manifest_item(reading)) == READING_TYPE_ACTUATOR) {
        if (snprintf(buffer, buffer_size, "STATUS ") >= signed_buffer_size) {
            return false;
        }
    } else if (manifest_item_get_reading_type(reading_get_manifest_item(reading)) == READING_TYPE_SENSOR) {
        if (snprintf(buffer, buffer_size, "READINGS R:%u,", reading_get_rtc(reading)) >= signed_buffer_size) {
            return false;
        }
    } else {
        return false;
    }

    return true;
}

static bool append_reading(reading_t* reading, char* buffer, size_t buffer_size)
{
    int signed_buffer_size = buffer_size;

    if (snprintf(buffer + strlen(buffer), buffer_size - strlen(buffer), "%s:%s",
                 manifest_item_get_reference(reading_get_manifest_item(reading)),
                 reading_get_data_at(reading, 0)) >= signed_buffer_size) {
        return false;
    }

    return true;
}

static bool serialize_reading(reading_t* reading, char* buffer, size_t buffer_size)
{
    size_t reading_buffer_size = SERIALIZER_MAX_PARSER_INTERNAL_BUFFER_SIZE;
    char reading_buffer[SERIALIZER_MAX_PARSER_INTERNAL_BUFFER_SIZE];

    uint8_t i;
    uint8_t data_size;
    manifest_item_t* manifest_item = reading_get_manifest_item(reading);

    if (!append_reading_prefix(reading, reading_buffer, reading_buffer_size)) {
        return false;
    }

    if (!append_reading(reading, reading_buffer, reading_buffer_size)) {
        return false;
    }

    data_size = manifest_item_get_data_dimensions(manifest_item);
    if (data_size == 1) {
        if ((manifest_item_get_reading_type(reading_get_manifest_item(reading)) == READING_TYPE_ACTUATOR) &&
             !append_actuator_status(reading_buffer, reading_buffer_size, reading_get_actuator_status(reading))) {
            return false;
        }

        return append_to_buffer(buffer, buffer_size, reading_buffer);
    }

    char* delimiter = manifest_item_get_data_delimiter(manifest_item);
    for (i = 1; i < manifest_item_get_data_dimensions(manifest_item); ++i) {
        uint8_t numb_bytes_to_write = reading_buffer_size - strlen(reading_buffer);
        if (snprintf(reading_buffer + strlen(reading_buffer), numb_bytes_to_write,
                     "%s%s",
                     delimiter,
                     reading_get_data_at(reading, i)) >= numb_bytes_to_write) {
            return false;
        }
    }

    if ((manifest_item_get_reading_type(reading_get_manifest_item(reading)) == READING_TYPE_ACTUATOR) &&
         !append_actuator_status(reading_buffer, reading_buffer_size, reading_get_actuator_status(reading))) {
        return false;
    }

    return append_to_buffer(buffer, buffer_size, reading_buffer);
}

static bool serialize_readings_delimiter(char* buffer, size_t buffer_size)
{
    int delimiter_buffer_size = MANIFEST_ITEM_MAX_DATA_DELIMITER_SIZE;
    char delimiter_buffer[MANIFEST_ITEM_MAX_DATA_DELIMITER_SIZE];

    if(snprintf(delimiter_buffer, delimiter_buffer_size, ";") >= delimiter_buffer_size) {
        return false;
    }

    return append_to_buffer(buffer, buffer_size, delimiter_buffer);
}

static bool deserialize_command(char* buffer, command_t* command)
{
    char type_part[COMMAND_MAX_SIZE];
    char argument_part[COMMAND_MAX_ARGUMENT_PART_SIZE];

    if (sscanf(buffer, "%s %s", type_part, argument_part) != 2) {
        return false;
    }

    if (strcmp(type_part, "STATUS") == 0) {
        command_init(command, COMMAND_TYPE_STATUS, argument_part, "");
        return true;
    } else if (strcmp(type_part, "SET") == 0){
        char reference[MANIFEST_ITEM_MAX_REFERENCE_SIZE];
        char argument[COMMAND_MAX_ARGUMENT_SIZE];

        if (sscanf(argument_part, "%[^:]:%s", reference, argument) != 2) {
            return false;
        }

        command_init(command, COMMAND_TYPE_SET, reference, argument);
        return true;
    }

    return false;
}

size_t mqtt_serialize_readings(reading_t* first_reading, size_t num_readings, char* buffer, size_t buffer_size)
{
    size_t num_serialized_readings;
    reading_t* current_reading = first_reading;

    for(num_serialized_readings = 0; num_serialized_readings < num_readings; ++num_serialized_readings, ++current_reading) {
        if(!serialize_reading(current_reading, buffer, buffer_size)) {
            break;
        }

        serialize_readings_delimiter(buffer, buffer_size);
    }

    return num_serialized_readings;
}

size_t mqtt_deserialize_commands(char* buffer, size_t buffer_size, command_t* commands_buffer, size_t commands_buffer_size)
{
    UNUSED(buffer_size);

    /* Sanity check */
    ASSERT(SERIALIZER_MAX_PARSER_INTERNAL_BUFFER_SIZE >= buffer_size);

    size_t num_serialized_commands = 0;
    command_t* current_command = commands_buffer;
    char tmp_buffer[SERIALIZER_MAX_PARSER_INTERNAL_BUFFER_SIZE];
    strcpy(tmp_buffer, buffer);

    char* p = strtok (tmp_buffer, ";");
    while (p != NULL) {
        if (num_serialized_commands == commands_buffer_size) {
            break;
        }

        if (deserialize_command(p, current_command)) {
            ++num_serialized_commands;
            ++current_command;
        }

        p = strtok(NULL, ";");
    }

    return num_serialized_commands;
}
