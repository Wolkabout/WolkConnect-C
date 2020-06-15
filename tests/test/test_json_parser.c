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


reading_t first_reading;

static const char* UNIT_TEST_READINGS_TOPIC = "d2p/sensor_reading/d/";
static const char* UNIT_TEST_EVENTS_TOPIC = "d2p/events/d/";

static const char* UNIT_TEST_ACTUATOR_SET_TOPIC = "p2d/actuator_set/d/";
static const char* UNIT_TEST_ACTUATOR_STATUS_TOPIC = "d2p/actuator_status/d/";

static const char* UNIT_TEST_CONFIGURATION_SET = "p2d/configuration_set/d/";
static const char* UNIT_TEST_CONFIGURATION_GET = "d2p/configuration_get/d/";

void setUp(void)
{
}

void tearDown(void)
{
}

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
    char topic[TOPIC_SIZE];
    char payload[PAYLOAD_SIZE];
    int payload_len = 0;
    actuator_command_t actuator_command;

    strncpy(topic, UNIT_TEST_ACTUATOR_SET_TOPIC, strlen(UNIT_TEST_ACTUATOR_SET_TOPIC));
    strcat(topic, "/r/reference");
    strncpy(payload, "{\"value\":\"32.1\"}", strlen("{\"value\":\"32.1\"}"));
    payload_len = strlen(payload);

    TEST_ASSERT_EQUAL_INT(1, json_deserialize_actuator_commands(topic, strlen(topic), payload, (size_t)payload_len, &actuator_command, 1));
    TEST_ASSERT_EQUAL_STRING("32.1", actuator_command.argument);
}

void test_json_json_serialize_readings_topic(void)
{
    char device_key[DEVICE_KEY_SIZE];
    char buffer[PAYLOAD_SIZE];
    char topic[TOPIC_SIZE];
    char reference[MANIFEST_ITEM_REFERENCE_SIZE] = {"reference"};
    manifest_item_t string_sensor;

    //Data type String
    manifest_item_init(&string_sensor, reference, READING_TYPE_SENSOR, DATA_TYPE_STRING);
    manifest_item_set_reading_dimensions_and_delimiter(&string_sensor, 1, DATA_DELIMITER);

    reading_init(&first_reading, &string_sensor);
    reading_set_data_at(&first_reading, "TEST SENSOR", 0);

    strncpy(device_key, "some_device_key", strlen("some_device_key"));

    strncpy(topic, UNIT_TEST_READINGS_TOPIC, strlen(UNIT_TEST_READINGS_TOPIC)+1);
    strcat(topic, device_key);
    strcat(topic, "/r/");
    strcat(topic, reference);

    TEST_ASSERT_TRUE(json_serialize_readings_topic(&first_reading, 1, device_key, buffer, PAYLOAD_SIZE));
    TEST_ASSERT_EQUAL_STRING(topic, buffer);
}

void test_json_parser_json_serialize_configuration_single_item(void)
{
    char device_key[DEVICE_KEY_SIZE] = {"some_device_key"};
    char references[CONFIGURATION_ITEMS_SIZE][CONFIGURATION_REFERENCE_SIZE] = {"reference"};
    char values[CONFIGURATION_ITEMS_SIZE][CONFIGURATION_VALUE_SIZE] = {"string"};
    char topic[TOPIC_SIZE];
    int8_t num_configuration_items = 1;
    outbound_message_t outbound_message;

    strncpy(topic, UNIT_TEST_CONFIGURATION_GET, strlen(UNIT_TEST_CONFIGURATION_GET)+1);
    strcat(topic, device_key);

    TEST_ASSERT_TRUE(json_serialize_configuration(device_key, references, values, num_configuration_items, &outbound_message));
    TEST_ASSERT_EQUAL_STRING(topic, outbound_message.topic);
    TEST_ASSERT_EQUAL_STRING("{\"values\":{\"{reference}\":\"string\"}}", outbound_message.payload);
}

void test_json_parser_json_serialize_configuration_multi_item(void)
{
    char device_key[DEVICE_KEY_SIZE] = {"some_device_key"};
    char references[CONFIGURATION_ITEMS_SIZE][CONFIGURATION_REFERENCE_SIZE] = {"reference1", "reference2", "referenceN"};
    char values[CONFIGURATION_ITEMS_SIZE][CONFIGURATION_VALUE_SIZE] = {"string1", "string2", "stringN"};
    char topic[TOPIC_SIZE];
    int8_t num_configuration_items = 3;
    outbound_message_t outbound_message;

    strncpy(topic, UNIT_TEST_CONFIGURATION_GET, strlen(UNIT_TEST_CONFIGURATION_GET)+1);
    strcat(topic, device_key);

    TEST_ASSERT_TRUE(json_serialize_configuration(device_key, references, values, num_configuration_items, &outbound_message));
    TEST_ASSERT_EQUAL_STRING(topic, outbound_message.topic);
    TEST_ASSERT_EQUAL_STRING("{\"values\":{\"{reference1}\":\"string1\",\"{reference2}\":\"string2\",\"{referenceN}\":\"stringN\"}}", outbound_message.payload);
}
