/*
 * Copyright 2021 WolkAbout Technology s.r.o.
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

#ifndef WOLKCONNECTOR_C_FIRMWARE_UPDATE_H
#define WOLKCONNECTOR_C_FIRMWARE_UPDATE_H

#include <stdbool.h>

#include "size_definitions.h"


typedef struct firmware_update firmware_update_t;

struct firmware_update {
    bool is_initialized;

    char file_name[FILE_MANAGEMENT_FILE_NAME_SIZE];
};

void firmware_update_parameter_init(firmware_update_t* parameter);
void firmware_update_parameter_set_filename(firmware_update_t* parameter, const char* file_name);

#endif // WOLKCONNECTOR_C_FIRMWARE_UPDATE_H
