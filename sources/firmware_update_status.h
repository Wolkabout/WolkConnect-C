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

#ifndef FIRMWARE_UPDATE_STATUS_H
#define FIRMWARE_UPDATE_STATUS_H

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    FIRMWARE_UPDATE_STATE_FILE_TRANSFER,
    FIRMWARE_UPDATE_STATE_FILE_READY,
    FIRMWARE_UPDATE_STATE_INSTALLATION,
    FIRMWARE_UPDATE_STATE_COMPLETED,
    FIRMWARE_UPDATE_STATE_ERROR,
    FIRMWARE_UPDATE_STATE_ABORTED
} firmware_update_state_t;

typedef enum {
    FIRMWARE_UPDATE_ERROR_NONE = -1,
    FIRMWARE_UPDATE_ERROR_UNSPECIFIED = 0,
    FIRMWARE_UPDATE_ERROR_FILE_UPLOAD_DISABLED = 1,
    FIRMWARE_UPDATE_ERROR_UNSUPPORTED_FILE_SIZE = 2,
    FIRMWARE_UPDATE_ERROR_INSTALLATION_FAILED = 3,
    FIRMWARE_UPDATE_ERROR_MALFORMED_URL = 4,
    FIRMWARE_UPDATE_ERROR_FILE_SYSTEM = 5,
    FIRMWARE_UPDATE_ERROR_RETRY_COUNT_EXCEEDED = 10
} firmware_update_error_t;

typedef struct {
    firmware_update_state_t status;
    firmware_update_error_t error;
} firmware_update_status_t;

firmware_update_status_t firmware_update_status_ok(firmware_update_state_t state);
firmware_update_status_t firmware_update_status_error(firmware_update_error_t error);

firmware_update_state_t firmware_update_status_get_state(firmware_update_status_t* status);

firmware_update_error_t firmware_update_status_get_error(firmware_update_status_t* status);

#ifdef __cplusplus
}
#endif

#endif
