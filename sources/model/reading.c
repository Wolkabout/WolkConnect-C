/*
 * Copyright 2018 WolkAbout Technology s.r.o.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "model/reading.h"
#include "utility/wolk_utils.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

void reading_init(reading_t* reading, uint16_t reading_dimensions, char* reference)
{
    uint16_t i;
    reading->reading_dimensions = reading_dimensions > READING_DIMENSIONS ? READING_DIMENSIONS : reading_dimensions;

    reading->rtc = 0;

    for (i = 0; i < reading_dimensions; ++i) {
        reading_set_data(reading, "");
    }
    strcpy(reading->reference, reference);
}

void reading_clear(reading_t* reading)
{
    for (int i = 0; i < reading->reading_dimensions; ++i) {
        reading_set_data_at(reading, "", i);
    }
}

void reading_set_data(reading_t* reading, const char** data)
{
    for(int i = 0; i < reading->reading_dimensions; ++i)
    {
        WOLK_ASSERT(strlen(data[i]) < READING_SIZE);

        strcpy(reading->reading_data[i], data[i]);
    }
}

void reading_set_data_at(reading_t* reading, const char* data, size_t data_position)
{
    /* Sanity check */
    WOLK_ASSERT(strlen(data) < READING_SIZE);
    WOLK_ASSERT(data_position < READING_DIMENSIONS);

    strcpy(reading->reading_data[data_position], data);
}

char** reading_get_data(reading_t* reading)
{
    return reading->reading_data;
}

char* reading_get_data_at(reading_t* reading, size_t data_position)
{
    /* Sanity check */
    WOLK_ASSERT(data_position < reading->manifest_item.data_dimensions);

    return reading->reading_data[data_position];
}

void reading_set_rtc(reading_t* reading, uint64_t rtc)
{
    reading->rtc = rtc;
}

uint64_t reading_get_rtc(reading_t* reading)
{
    return reading->rtc;
}
