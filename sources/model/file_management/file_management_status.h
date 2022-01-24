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

#ifndef FILE_MANAGEMENT_STATUS_H
#define FILE_MANAGEMENT_STATUS_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

file_management_status_t file_management_status_ok(file_management_state_t state);
file_management_status_t file_management_status_error(file_management_error_t error);

file_management_state_t file_management_status_get_state(file_management_status_t* status);

file_management_error_t file_management_status_get_error(file_management_status_t* status);

#ifdef __cplusplus
}
#endif

#endif
