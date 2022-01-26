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

#ifndef READING_H
#define READING_H

#include "size_definitions.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    char reading_data[READING_DIMENSIONS][READING_SIZE];
    uint16_t reading_dimensions;

    char reference[REFERENCE_SIZE];

    uint64_t rtc;
} reading_t;

void reading_init(reading_t* reading, uint16_t reading_dimensions, char* reference);

void reading_clear(reading_t* reading);

void reading_set_data(reading_t* reading, const char** data);
char** reading_get_data(reading_t* reading);

void reading_set_data_at(reading_t* reading, const char* data, size_t data_position);
char* reading_get_data_at(reading_t* reading, size_t data_position);

void reading_set_rtc(reading_t* reading, uint64_t rtc);
uint64_t reading_get_rtc(reading_t* reading);

#ifdef __cplusplus
}
#endif

#endif
