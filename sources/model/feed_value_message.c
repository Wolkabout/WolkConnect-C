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

#include "feed_value_message.h"
#include "utility/wolk_utils.h"

#include <string.h>

void feed_value_message_init(feed_value_message_t* message, const char* reference, const char* value)
{ /* Sanity check */
    WOLK_ASSERT(strlen(reference) <= REFERENCE_SIZE);
    WOLK_ASSERT(strlen(argument) <= FEED_ELEMENT_SIZE);

    strcpy(message->reference, reference);
    strcpy(message->value, value);
}

char* feed_value_message_get_reference(feed_value_message_t* message)
{
    return message->reference;
}

void feed_value_message_set_reference(feed_value_message_t* message, const char* reference)
{
    /* Sanity check */
    WOLK_ASSERT(strlen(reference) < REFERENCE_SIZE);

    strcpy(message->reference, reference);
}

char* feed_value_message_get_value(feed_value_message_t* message)
{
    return message->value;
}
