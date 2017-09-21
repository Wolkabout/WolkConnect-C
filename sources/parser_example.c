#include "utils.h"
#include "manifest_item.h"
#include "reading.h"
#include "parser.h"

#include <stdio.h>
#include <string.h>

void parser_example(void)
{
    size_t i;

    parser_t parser;
    initialize_parser(&parser, PARSER_TYPE_MQTT);

    /* Sensor(s) */
    manifest_item_t numeric_sensor;
    manifest_item_init(&numeric_sensor, "NUMERIC_REFERENCE", READING_TYPE_SENSOR, DATA_TYPE_NUMERIC);

    manifest_item_t bool_sensor;
    manifest_item_init(&bool_sensor, "BOOLEAN_REFERENCE", READING_TYPE_SENSOR, DATA_TYPE_BOOLEAN);

    manifest_item_t string_sensor;
    manifest_item_init(&string_sensor, "STRING_REFERENCE", READING_TYPE_SENSOR, DATA_TYPE_STRING);

    manifest_item_t delimited_numeric_sensor;
    manifest_item_init(&delimited_numeric_sensor, "DELIMITED_NUMERIC_REFERENCE", READING_TYPE_SENSOR, DATA_TYPE_NUMERIC);
    manifest_item_set_reading_dimensions_and_delimiter(&delimited_numeric_sensor, 3, "#");

    manifest_item_t delimited_bool_sensor;
    manifest_item_init(&delimited_bool_sensor, "DELIMITED_BOOLEAN_REFERENCE", READING_TYPE_SENSOR, DATA_TYPE_BOOLEAN);
    manifest_item_set_reading_dimensions_and_delimiter(&delimited_bool_sensor, 3, "~");

    manifest_item_t delimited_string_sensor;
    manifest_item_init(&delimited_string_sensor, "DELIMITED_STRING_REFERENCE", READING_TYPE_SENSOR, DATA_TYPE_STRING);
    manifest_item_set_reading_dimensions_and_delimiter(&delimited_string_sensor, 3, "_");

    /* Actuator(s) */
    manifest_item_t numeric_actuator;
    manifest_item_init(&numeric_actuator, "NUMERIC_ACTUATOR_REFERENCE", READING_TYPE_ACTUATOR, DATA_TYPE_NUMERIC);

    manifest_item_t bool_actuator;
    manifest_item_init(&bool_actuator, "BOOL_ACTUATOR_REFERENCE", READING_TYPE_ACTUATOR, DATA_TYPE_BOOLEAN);

    /* Sensor reading(s) */
    uint16_t readings_buffer_size = 512;
    char readings_buffer[512];

    reading_t readings[8];
    reading_init(&readings[0], &numeric_sensor);
    reading_set_data(&readings[0], "12.3");
    reading_set_rtc(&readings[0], 123456789);

    reading_init(&readings[1], &bool_sensor);
    reading_set_data(&readings[1], "false");
    reading_set_rtc(&readings[1], 456789123);

    reading_init(&readings[2], &string_sensor);
    reading_set_data(&readings[2], "string-data");
    reading_set_rtc(&readings[2], 798123456);

    reading_init(&readings[3], &delimited_numeric_sensor);
    reading_set_data_at(&readings[3], "23.4", 0);
    reading_set_data_at(&readings[3], "34.4", 1);
    reading_set_data_at(&readings[3], "45.6", 2);
    reading_set_rtc(&readings[3], 123123123);

    reading_init(&readings[4], &delimited_bool_sensor);
    reading_set_data_at(&readings[4], "false", 0);
    reading_set_data_at(&readings[4], "true", 1);
    reading_set_data_at(&readings[4], "false", 2);
    reading_set_rtc(&readings[4], 456456456);

    reading_init(&readings[5], &delimited_string_sensor);
    reading_set_data_at(&readings[5], "one", 0);
    reading_set_data_at(&readings[5], "two", 1);
    reading_set_data_at(&readings[5], "three", 2);
    reading_set_rtc(&readings[5], 789789789);

    /* Actuator reading(s) */
    reading_init(&readings[6], &numeric_actuator);
    reading_set_rtc(&readings[6], 789789789);
    reading_set_data(&readings[6], "66785.4");
    reading_set_actuator_status(&readings[6], ACTUATOR_STATUS_BUSY);

    reading_init(&readings[7], &bool_actuator);
    reading_set_rtc(&readings[7], 987654321);
    reading_set_data(&readings[7], "false");
    reading_set_actuator_status(&readings[7], ACTUATOR_STATUS_ERROR);

    /* Serialize reading(s) */
    size_t serialized_readings = parser_serialize_readings(&parser, &readings[0], 8, readings_buffer, readings_buffer_size);
    reading_clear_array(readings, 8);

    printf("%lu reading(s) serialized:\n%s\n\n", serialized_readings, readings_buffer);

    char received_commands_buffer[] = "STATUS SLIDER;SET SWITCH:true;SET actuator_reference:1123354.123;ASD;SET ref;N_COM ref;";
    command_t commands_buffer[128];

    size_t num_deserialized_commands = parser_deserialize_commands(&parser, received_commands_buffer, 128, &commands_buffer[0], 128);
    printf("Received commands buffer after command deserialization: %s\n\n", received_commands_buffer);

    printf("%lu command(s) deserialized:\n", num_deserialized_commands);
    for (i = 0; i < num_deserialized_commands; ++i) {
        command_t* command = &commands_buffer[i];

        printf("Command type: ");
        switch(command_get_type(command))
        {
        case COMMAND_TYPE_STATUS:
            printf("STATUS\n");
            printf("  Reference:  %s\n", command_get_reference(command));
            break;

        case COMMAND_TYPE_SET:
            printf("SET\n");
            printf("  Reference:  %s\n", command_get_reference(command));
            printf("  Value:      %s\n", command_get_value(command));
            break;

        case COMMAND_TYPE_UNKNOWN:
            printf("UNKNOWN\n");
            break;
        }
    }
}
