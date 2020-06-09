#include "unity.h"

#include "reading.h"
#include "manifest_item.h"
#include "actuator_status.h"


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

    reading_t reading;
    reading_init(&reading, &string_sensor);

    TEST_ASSERT_EQUAL_INT(reading.actuator_status, ACTUATOR_STATE_READY);
    TEST_ASSERT_EQUAL_INT(reading.manifest_item.reading_type, READING_TYPE_SENSOR);
    TEST_ASSERT_EQUAL_INT(reading.manifest_item.data_type, DATA_TYPE_STRING);
    TEST_ASSERT_EQUAL_STRING("T", reading.manifest_item.reference);

    reading_clear(&reading);
    TEST_ASSERT_EQUAL_STRING("", reading.reading_data);
}

void test_reading_readings_clear()
{
    manifest_item_t string_sensor;
    manifest_item_init(&string_sensor, "reference", READING_TYPE_SENSOR, DATA_TYPE_STRING);

    reading_init(&reading, &string_sensor);
    reading_set_data(&reading, "SOME TEST READINGS");
    TEST_ASSERT_EQUAL_STRING("SOME TEST READINGS", reading.reading_data);

    reading_clear(&reading);

    TEST_ASSERT_EQUAL_INT(ACTUATOR_STATE_READY, reading.actuator_status);
    TEST_ASSERT_EQUAL_INT(0, reading.rtc);
    TEST_ASSERT_EQUAL_STRING("", reading.reading_data);
}

void test_reading_reading_set_data(void)
{
    manifest_item_t string_sensor;
    manifest_item_init(&string_sensor, "reference", READING_TYPE_SENSOR, DATA_TYPE_STRING);

    reading_init(&reading, &string_sensor);
    reading_set_data(&reading, "SOME TEST READINGS");

    TEST_ASSERT_EQUAL_STRING("SOME TEST READINGS", reading.reading_data);

    reading_clear(&reading);
    TEST_ASSERT_EQUAL_STRING("", reading.reading_data);
}

void test_reading_reading_get_delimited_data(void)
{
    char data_buffer[PARSER_INTERNAL_BUFFER_SIZE];
    int8_t values_size = 2;
    manifest_item_t string_sensor;
    manifest_item_init(&string_sensor, "reference", READING_TYPE_SENSOR, DATA_TYPE_STRING);
    manifest_item_set_reading_dimensions_and_delimiter(&string_sensor, values_size, DATA_DELIMITER);

    reading_init(&reading, &string_sensor);
    reading_set_rtc(&reading, 1591621716);

    for (uint32_t i = 0; i < values_size; ++i) {
        reading_set_data_at(&reading, "SOME TEST READINGS", i);
    }

    TEST_ASSERT(reading_get_delimited_data(&reading, data_buffer, PARSER_INTERNAL_BUFFER_SIZE));

    reading_clear(&reading);
    TEST_ASSERT_EQUAL_STRING("", reading.reading_data);
}

void test_reading_reading_set_data_at(void)
{
    reading_set_data_at(&reading, "SOME TEST READING", 0);

    TEST_ASSERT_EQUAL_STRING("SOME TEST READING", reading.reading_data);

    reading_clear(&reading);
    TEST_ASSERT_EQUAL_STRING("", reading.reading_data);
}

void test_reading_set_rtc(void)
{
    uint32_t rtc = 1591621716;
    reading_set_rtc(&reading, rtc);

    TEST_ASSERT_EQUAL_INT32(rtc, reading.rtc);
}