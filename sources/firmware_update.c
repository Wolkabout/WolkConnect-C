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
#include <string.h>

#include "firmware_update.h"
#include "wolk_utils.h"

void firmware_update_parameter_init(firmware_update_t* parameter)
{
    /* Sanity Check */
    WOLK_ASSERT(parameter);

    memset(parameter->file_name, '\0', WOLK_ARRAY_LENGTH(parameter->file_name));
}
void firmware_update_parameter_set_filename(firmware_update_t* parameter, const char* file_name)
{
    WOLK_ASSERT(parameter);
    WOLK_ASSERT(buffer);
    WOLK_ASSERT(strlen(file_name) <= FILE_MANAGEMENT_FILE_NAME_SIZE);

    strncpy(parameter->file_name, file_name, FILE_MANAGEMENT_FILE_NAME_SIZE);
}
