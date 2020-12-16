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

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    FILE_MANAGEMENT_STATE_FILE_TRANSFER,
    FILE_MANAGEMENT_STATE_FILE_READY,
    FILE_MANAGEMENT_STATE_ERROR,
    FILE_MANAGEMENT_STATE_ABORTED
} file_management_state_t;

typedef enum {
    FILE_MANAGEMENT_ERROR_NONE = -1,
    FILE_MANAGEMENT_ERROR_UNSPECIFIED = 0,
    FILE_MANAGEMENT_ERROR_TRANSFER_PROTOCOL_DISABLED = 1,
    FILE_MANAGEMENT_ERROR_UNSUPPORTED_FILE_SIZE = 2,
    FILE_MANAGEMENT_ERROR_MALFORMED_URL = 3,
    FILE_MANAGEMENT_ERROR_FILE_HASH_MISMATCH = 4,
    FILE_MANAGEMENT_ERROR_FILE_SYSTEM = 5,
    FILE_MANAGEMENT_ERROR_RETRY_COUNT_EXCEEDED = 10
} file_management_error_t;

typedef struct {
    file_management_state_t status;
    file_management_error_t error;
} file_management_status_t;

file_management_status_t file_management_status_ok(file_management_state_t state);
file_management_status_t file_management_status_error(file_management_error_t error);

file_management_state_t file_management_status_get_state(file_management_status_t* status);

file_management_error_t file_management_status_get_error(file_management_status_t* status);

#ifdef __cplusplus
}
#endif

#endif
