#include "unity.h"

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

#include "manifest_item.h"
#include "outbound_message.h"
#include "configuration_command.h"

#include "string.h"

void setUp(void)
{
}

void tearDown(void)
{
}

reading_t first_reading;

void test_json_parser_json_serialize_readings_sensor(void)
{
    char buffer[PAYLOAD_SIZE];
    uint32_t rtc = 1591621716;
    manifest_item_t string_sensor;

    //Data type String
    manifest_item_init(&string_sensor, "reference", READING_TYPE_SENSOR, DATA_TYPE_STRING);
    manifest_item_set_reading_dimensions_and_delimiter(&string_sensor, 1, DATA_DELIMITER);

    reading_init(&first_reading, &string_sensor);
    reading_set_data_at(&first_reading, "TEST SENSOR", 0);
    reading_set_rtc(&first_reading, rtc);

    TEST_ASSERT_TRUE(json_serialize_readings(&first_reading, 1, buffer, sizeof(buffer)));
    TEST_ASSERT_EQUAL_STRING("{\"utc\":1591621716,\"data\":\"TEST SENSOR\"}", buffer);

    //Data type Numeric
    manifest_item_init(&string_sensor, "reference", READING_TYPE_SENSOR, DATA_TYPE_NUMERIC);

    reading_init(&first_reading, &string_sensor);
    reading_set_data_at(&first_reading, "32.1", 0);
    reading_set_rtc(&first_reading, rtc);

    TEST_ASSERT_TRUE(json_serialize_readings(&first_reading, 1, buffer, sizeof(buffer)));
    TEST_ASSERT_EQUAL_STRING("{\"utc\":1591621716,\"data\":\"32.1\"}", buffer);

    //Data type Boolean
    manifest_item_init(&string_sensor, "reference", READING_TYPE_SENSOR, DATA_TYPE_BOOLEAN);

    reading_init(&first_reading, &string_sensor);
    reading_set_data_at(&first_reading, "ON", 0);
    reading_set_rtc(&first_reading, rtc);

    TEST_ASSERT_TRUE(json_serialize_readings(&first_reading, 1, buffer, sizeof(buffer)));
    TEST_ASSERT_EQUAL_STRING("{\"utc\":1591621716,\"data\":\"ON\"}", buffer);
}

void test_json_parser_json_serialize_readings_actuator(void)
{
    char buffer[PAYLOAD_SIZE];
    manifest_item_t string_sensor;

    //Data type String
    manifest_item_init(&string_sensor, "reference", READING_TYPE_ACTUATOR, DATA_TYPE_STRING);

    reading_init(&first_reading, &string_sensor);
    reading_set_data_at(&first_reading, "TEST ACTUATOR", 0);

    TEST_ASSERT_TRUE(json_serialize_readings(&first_reading, 1, buffer, sizeof(buffer)));
    TEST_ASSERT_EQUAL_STRING("{\"status\":\"READY\",\"value\":\"TEST ACTUATOR\"}", buffer);

    //Data type Numeric
    manifest_item_init(&string_sensor, "reference", READING_TYPE_ACTUATOR, DATA_TYPE_NUMERIC);

    reading_init(&first_reading, &string_sensor);
    reading_set_data_at(&first_reading, "32.1", 0);

    TEST_ASSERT_TRUE(json_serialize_readings(&first_reading, 1, buffer, sizeof(buffer)));
    TEST_ASSERT_EQUAL_STRING("{\"status\":\"READY\",\"value\":\"32.1\"}", buffer);

    TEST_ASSERT_EQUAL_STRING("{\"status\":\"READY\",\"value\":\"32.1\"}", buffer);

    //Data type Boolean
    manifest_item_init(&string_sensor, "reference", READING_TYPE_ACTUATOR, DATA_TYPE_BOOLEAN);

    reading_init(&first_reading, &string_sensor);
    reading_set_data_at(&first_reading, "ON", 0);

    TEST_ASSERT_TRUE(json_serialize_readings(&first_reading, 1, buffer, sizeof(buffer)));
    TEST_ASSERT_EQUAL_STRING("{\"status\":\"READY\",\"value\":\"ON\"}", buffer);
}

void test_json_parser_json_serialize_readings_alarm(void)
{
    char buffer[PAYLOAD_SIZE];
    uint32_t rtc = 1591621716;
    manifest_item_t string_sensor;

    //Data type String
    manifest_item_init(&string_sensor, "reference", READING_TYPE_ALARM, DATA_TYPE_STRING);

    reading_init(&first_reading, &string_sensor);
    reading_set_data_at(&first_reading, "ON", 0);
    reading_set_rtc(&first_reading, rtc);

    TEST_ASSERT_TRUE(json_serialize_readings(&first_reading, 1, buffer, sizeof(buffer)));
    TEST_ASSERT_EQUAL_STRING("{\"utc\":1591621716,\"data\":\"ON\"}", buffer);
}

void test_json_parser_json_deserialize_actuator_commands(void)
{
    char topic_str[TOPIC_SIZE];
    char payload[PAYLOAD_SIZE];
    int payload_len = 0;
    actuator_command_t actuator_command;

    strncpy(topic_str, "p2d/actuator_set/d/device_key/r/reference", strlen("p2d/actuator_set/d/device_key/r/reference"));
    strncpy(payload, "{\"value\":\"32.1\"}", strlen("{\"value\":\"32.1\"}"));
    payload_len = strlen(payload);

//    TEST_ASSERT_EQUAL_INT(1, json_deserialize_actuator_commands(topic_str, strlen(topic_str), payload, (size_t)payload_len, &actuator_command, 1));
//    TEST_ASSERT_EQUAL_STRING("32.1", actuator_command.argument);
}

void test_json_json_serialize_readings_topic(void)
{
    char device_key[DEVICE_KEY_SIZE];
    char buffer[PAYLOAD_SIZE];
    manifest_item_t string_sensor;

    //Data type String
    manifest_item_init(&string_sensor, "reference", READING_TYPE_SENSOR, DATA_TYPE_STRING);
    manifest_item_set_reading_dimensions_and_delimiter(&string_sensor, 1, DATA_DELIMITER);

    reading_init(&first_reading, &string_sensor);
    reading_set_data_at(&first_reading, "TEST SENSOR", 0);

    strncpy(device_key, "some_device_key", strlen("some_device_key"));

    TEST_ASSERT_TRUE(json_serialize_readings_topic(&first_reading, 1, device_key, buffer, PAYLOAD_SIZE));
    TEST_ASSERT_EQUAL_STRING("readings/some_device_key/reference", buffer);
}