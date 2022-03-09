#include "unity.h"

#include "actuator_status.h"
#include "manifest_item.h"


reading_t reading;

void setUp(void)
{
}

void tearDown(void)
{
}

void test_reading_readings_init(void)
{
    manifest_item_t string_sensor;
    manifest_item_init(&string_sensor, "T", READING_TYPE_SENSOR, DATA_TYPE_STRING);

    feed_init(&reading, &string_sensor);

    TEST_ASSERT_EQUAL_INT(reading.actuator_status, ACTUATOR_STATE_READY);
    TEST_ASSERT_EQUAL_INT(reading.manifest_item.reading_type, READING_TYPE_SENSOR);
    TEST_ASSERT_EQUAL_INT(reading.manifest_item.data_type, DATA_TYPE_STRING);
    TEST_ASSERT_EQUAL_STRING("T", reading.manifest_item.reference);

    feed_clear(&reading);
    TEST_ASSERT_EQUAL_STRING("", reading.reading_data);
}

void test_reading_readings_clear()
{
    manifest_item_t string_sensor;
    manifest_item_init(&string_sensor, "reference", READING_TYPE_SENSOR, DATA_TYPE_STRING);

    feed_init(&reading, &string_sensor);
    feed_set_data(&reading, "TEST READINGS");
    TEST_ASSERT_EQUAL_STRING("TEST READINGS", reading.reading_data);

    feed_clear(&reading);

    TEST_ASSERT_EQUAL_INT(ACTUATOR_STATE_READY, reading.actuator_status);
    TEST_ASSERT_EQUAL_INT(0, reading.utc);
    TEST_ASSERT_EQUAL_STRING("", reading.reading_data);
}

void test_reading_reading_set_data(void)
{
    manifest_item_t string_sensor;
    manifest_item_init(&string_sensor, "reference", READING_TYPE_SENSOR, DATA_TYPE_STRING);

    feed_init(&reading, &string_sensor);
    feed_set_data(&reading, "TEST READINGS");

    TEST_ASSERT_EQUAL_STRING("TEST READINGS", reading.reading_data);

    feed_clear(&reading);
    TEST_ASSERT_EQUAL_STRING("", reading.reading_data);
}

void test_reading_reading_get_delimited_data(void)
{
    char data_buffer[PARSER_INTERNAL_BUFFER_SIZE];
    int8_t value_size = 2;
    manifest_item_t string_sensor;
    manifest_item_init(&string_sensor, "reference", READING_TYPE_SENSOR, DATA_TYPE_STRING);
    manifest_item_set_reading_size_and_delimiter(&string_sensor, value_size, DATA_DELIMITER);

    feed_init(&reading, &string_sensor);
    feed_set_utc(&reading, 1591621716);

    for (size_t i = 0; i < value_size; ++i) {
        feed_set_data_at(&reading, "TEST READINGS", i);
    }

    TEST_ASSERT(reading_get_delimited_data(&reading, data_buffer, PARSER_INTERNAL_BUFFER_SIZE));

    feed_clear(&reading);
    TEST_ASSERT_EQUAL_STRING("", reading.reading_data);
}

void test_reading_reading_set_data_at(void)
{
    feed_set_data_at(&reading, "TEST READING", 0);

    TEST_ASSERT_EQUAL_STRING("TEST READING", reading.reading_data);

    feed_clear(&reading);
    TEST_ASSERT_EQUAL_STRING("", reading.reading_data);
}

void test_reading_reading_get_data_at(void)
{
    manifest_item_t string_sensor;
    manifest_item_init(&string_sensor, "reference", READING_TYPE_SENSOR, DATA_TYPE_STRING);

    feed_init(&reading, &string_sensor);
    feed_set_data_at(&reading, "TEST READINGS", 0);

    TEST_ASSERT_EQUAL_STRING("TEST READINGS", feed_get_data_at(&reading, 0));

    feed_clear(&reading);
    TEST_ASSERT_EQUAL_STRING("", reading.reading_data);
}

void test_reading_reading_get_manifest_item(void)
{
    manifest_item_t string_sensor;
    manifest_item_init(&string_sensor, "reference", READING_TYPE_SENSOR, DATA_TYPE_NUMERIC);
    manifest_item_set_reading_size_and_delimiter(&string_sensor, 1, DATA_DELIMITER);

    feed_init(&reading, &string_sensor);
    feed_set_data_at(&reading, "TEST READINGS", 0);

    TEST_ASSERT_EQUAL_STRING("reference", reading_get_manifest_item(&reading)->reference);
    TEST_ASSERT_EQUAL_INT(READING_TYPE_SENSOR, reading_get_manifest_item(&reading)->reading_type);
    TEST_ASSERT_EQUAL_INT(DATA_TYPE_NUMERIC, reading_get_manifest_item(&reading)->data_type);
    TEST_ASSERT_EQUAL_INT(1, reading_get_manifest_item(&reading)->data_dimensions);
    TEST_ASSERT_EQUAL_STRING(DATA_DELIMITER, reading_get_manifest_item(&reading)->data_delimiter);
}

void test_reading_set_utc(void)
{
    uint64_t utc = 1591621716;
    feed_set_utc(&reading, utc);

    TEST_ASSERT_EQUAL_INT64(utc, reading.utc);
}

void test_reading_reading_get_utc(void)
{
    uint64_t utc = 1591621716;
    feed_set_utc(&reading, utc);

    TEST_ASSERT_EQUAL_INT64(utc, feed_get_utc(&reading));
}

void test_reading_reading_set_actuator_state(void)
{
    reading_set_actuator_state(&reading, ACTUATOR_STATE_READY);

    TEST_ASSERT_EQUAL_INT(ACTUATOR_STATE_READY, reading.actuator_status);

    feed_clear(&reading);
    TEST_ASSERT_EQUAL_STRING("", reading.reading_data);
}

void test_reading_reading_get_actuator_state(void)
{
    reading_set_actuator_state(&reading, ACTUATOR_STATE_BUSY);

    TEST_ASSERT_EQUAL_INT(ACTUATOR_STATE_BUSY, reading_get_actuator_state(&reading));

    feed_clear(&reading);
    TEST_ASSERT_EQUAL_STRING("", reading.reading_data);
}
