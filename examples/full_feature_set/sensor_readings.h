/*
 * Copyright 2020 WolkAbout Technology s.r.o.
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

#ifndef WOLKCONNECTOR_C_SENSOR_READINGS_H
#define WOLKCONNECTOR_C_SENSOR_READINGS_H

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "wolk_connector.h"

bool enable_feeds(char* value);
bool sensor_readings_process(wolk_ctx_t* ctx, int* publish_period_seconds, int* default_publish_value);

#endif // WOLKCONNECTOR_C_SENSOR_READINGS_H
