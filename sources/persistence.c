/*
 * Copyright 2017 WolkAbout Technology s.r.o.
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

#include "persistence.h"

#include <stdbool.h>
#include <stddef.h>

void persistence_init(persistence_t* persistence, persistence_push_t push, persistence_pop_t pop, persistence_clear_t clear,
                      persistence_is_empty_t is_empty)
{
    persistence->push = push;
    persistence->pop = pop;
    persistence->clear = clear;
    persistence->is_empty = is_empty;

    persistence->is_initialized = true;
}

bool persistence_is_initialized(const persistence_t* persistence)
{
    return persistence->is_initialized;
}
bool persistence_push(persistence_t* persistence, outbound_message_t* item)
{
    return persistence->push(item);
}

bool persistence_pop(persistence_t* persistence, outbound_message_t* item)
{
    return persistence->pop(item);
}

void persistence_clear(persistence_t* persistence)
{
    persistence->clear();
}

bool persistence_is_empty(persistence_t* persistence)
{
    return persistence->is_empty();
}
