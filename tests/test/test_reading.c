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

void test_readings_init(void)
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
}

void test_reading_set_rtc(void)
{
    uint32_t rtc = 1591621716;
    reading_set_rtc(&reading, rtc);

    TEST_ASSERT_EQUAL_INT32(rtc, reading.rtc);
}