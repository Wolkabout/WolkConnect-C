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
    char data[FEEDS_MAX_NUMBER][FEED_ELEMENT_SIZE];
    uint16_t size;

    char reference[REFERENCE_SIZE];

    uint64_t utc;
} feed_t;

void feed_init(feed_t* feed, uint16_t feed_size, char* reference);

void feed_clear(feed_t* feed);

void feed_set_data(feed_t* feed, const char** data);
char** feed_get_data(feed_t* feed);

void feed_set_data_at(feed_t* feed, const char* data, size_t data_position);
char* feed_get_data_at(feed_t* feed, size_t data_position);

void feed_set_utc(feed_t* feed, uint64_t utc);
uint64_t feed_get_utc(feed_t* feed);

#ifdef __cplusplus
}
#endif

#endif
