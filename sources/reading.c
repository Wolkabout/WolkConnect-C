#include "reading.h"
#include "manifest_item.h"
#include "utils.h"

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

void reading_init(reading_t* reading, manifest_item_t* item)
{
    uint8_t i;
    uint8_t reading_dimensions = manifest_item_get_data_dimensions(item);

    reading->manifest_item = item;
    reading->actuator_status = ACTUATOR_STATUS_READY;
    reading->rtc = 0;

    for (i = 0; i < reading_dimensions; ++i) {
        reading_set_data(reading, "");
    }
}

void reading_clear(reading_t* reading)
{
    reading_init(reading, reading_get_manifest_item(reading));
}

void reading_clear_array(reading_t* first_reading, size_t readings_count)
{
    size_t i;

    reading_t* current_reading = first_reading;
    for (i = 0; i < readings_count; ++i, ++current_reading) {
        reading_clear(current_reading);
    }
}

void reading_set_data(reading_t* reading, char* data)
{
    reading_set_data_at(reading, data, 0);
}

char* reading_get_data(reading_t* reading)
{
    return reading_get_data_at(reading, 0);
}

void reading_set_data_at(reading_t* reading, char* data, size_t data_position)
{
    /* Sanity check */
    ASSERT(data_position < READING_MAX_READING_SIZE);

    memcpy(reading->reading_data[data_position], data, strlen(data));
    reading->rtc = 0;
}

char* reading_get_data_at(reading_t* reading, size_t data_position)
{
    /* Sanity check */
    ASSERT(data_position < reading->manifest_item->data_dimensions);

    return reading->reading_data[data_position];
}

manifest_item_t* reading_get_manifest_item(reading_t* reading)
{
    return reading->manifest_item;
}

void reading_set_rtc(reading_t* reading, uint32_t rtc)
{
    reading->rtc = rtc;
}

uint32_t reading_get_rtc(reading_t* reading)
{
    return reading->rtc;
}

void reading_set_actuator_status(reading_t* reading, actuator_state_t actuator_status)
{
    /* Sanity check */
    ASSERT(manifest_item_get_reading_type(reading_get_manifest_item(reading)) & READING_TYPE_ACTUATOR);

    reading->actuator_status = actuator_status;
}

actuator_state_t reading_get_actuator_status(reading_t* reading)
{
    /* Sanity check */
    ASSERT(manifest_item_get_reading_type(reading_get_manifest_item(reading)) & READING_TYPE_ACTUATOR);

    return reading->actuator_status;
}
