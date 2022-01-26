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

#ifndef FEED_VALUE_MESSAGE_H
#define FEED_VALUE_MESSAGE_H

#include "size_definitions.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    char reference[REFERENCE_SIZE];
    char value[COMMAND_ARGUMENT_SIZE];
} feed_value_message_t;

void feed_value_message_init(feed_value_message_t* message, const char* reference, const char* value);

char* feed_value_message_get_reference(feed_value_message_t* message);
void feed_value_message_set_reference(feed_value_message_t* message, const char* reference);

char* feed_value_message_get_value(feed_value_message_t* message);

#ifdef __cplusplus
}
#endif

#endif
