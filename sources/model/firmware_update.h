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
#include <stdint.h>

#include "size_definitions.h"

/**
 * @brief firmware_update_start_installation signature.
 * Start Firmware update procedure with file named 'file_name'.
 *
 * @return true if Firmware update is able to perform installation, false otherwise
 */
typedef bool (*firmware_update_start_installation_t)(const char* file_name);

/**
 * @brief firmware_update_start_installation signature.
 * Start Firmware update procedure with file named 'file_name'.
 *
 * @return true if Firmware update is able to perform installation, false otherwise
 */
typedef bool (*firmware_update_is_installation_completed_t)(bool* success);

/**
 * @brief firmware_update_version signature.
 * Get current firmware version.
 *
 * @return true if Firmware version is available, false otherwise
 */
typedef bool (*firmware_update_get_version_t)(const char* version);

/**
 * @brief firmware_update_abort signature.
 * Abort current Firmware update procedure.
 *
 * @return true if Firmware update can be aborted, false otherwise
 */
typedef bool (*firmware_update_abort_t)(void);

/**
 * @brief firmware_update_verification_store signature.
 * Verification includes store Firmware update progress parameters.
 * WolkConnect-C user have to store parameters into permanent memory (flash memory or file system on OS implementations)
 *
 * @return true if parameters are written, false otherwise
 */
typedef bool (*firmware_update_verification_store_t)(uint8_t parameter);

/**
 * @brief firmware_update_verification_read signature.
 * Read to verify Firmware update progress parameters.
 *
 * @return value of the particular parameter
 */
typedef uint8_t (*firmware_update_verification_read_t)(void);


typedef enum {
    FIRMWARE_UPDATE_STATUS_AWAITING_DEVICE = 0,
    FIRMWARE_UPDATE_STATUS_INSTALLING = 1,
    FIRMWARE_UPDATE_STATUS_SUCCESS = 2,
    FIRMWARE_UPDATE_STATUS_ERROR = 3,
    FIRMWARE_UPDATE_STATUS_ABORTED = 4,
    FIRMWARE_UPDATE_STATUS_UNKNOWN = 5
} firmware_update_status_t;

typedef enum {
    FIRMWARE_UPDATE_ERROR_NONE = -1,
    FIRMWARE_UPDATE_ERROR_UNKNOWN = 0,
    FIRMWARE_UPDATE_ERROR_UNKNOWN_FILE = 1,
    FIRMWARE_UPDATE_ERROR_INSTALLATION_FAILED = 2
} firmware_update_error_t;

typedef struct firmware_update firmware_update_t;
typedef void (*firmware_update_on_status_listener)(firmware_update_t* firmware_update);
typedef void (*firmware_update_on_version_listener)(firmware_update_t* firmware_update,
                                                    firmware_update_get_version_t version);
typedef void (*firmware_update_on_verification_listener)(firmware_update_t* firmware_update);

struct firmware_update {
    bool is_initialized;
    char file_name[FILE_MANAGEMENT_FILE_NAME_SIZE];

    int8_t state;
    int8_t status;
    int8_t error;

    firmware_update_start_installation_t start_installation;
    firmware_update_is_installation_completed_t is_installation_completed;
    firmware_update_verification_store_t verification_store;
    firmware_update_verification_read_t verification_read;
    firmware_update_abort_t abort_installation;

    /* Listeners */
    firmware_update_on_status_listener get_status;
    firmware_update_on_verification_listener on_verification;
    /* Listeners */

    void* wolk_ctx;
};

void firmware_update_init(firmware_update_t* firmware_update, firmware_update_start_installation_t start_installation,
                          firmware_update_is_installation_completed_t is_installation_completed,
                          firmware_update_verification_store_t verification_store,
                          firmware_update_verification_read_t verification_read,
                          firmware_update_abort_t abort_installation, void* wolk_ctx);
void firmware_update_parameter_init(firmware_update_t* parameter);
void firmware_update_parameter_set_filename(firmware_update_t* parameter, const char* file_name);
void firmware_update_handle_parameter(firmware_update_t* firmware_update, firmware_update_t* parameter);
void firmware_update_handle_verification(firmware_update_t* firmware_update);
void firmware_update_handle_abort(firmware_update_t* firmware_update);
void firmware_update_set_on_status_listener(firmware_update_t* firmware_update,
                                            firmware_update_on_status_listener status);
void firmware_update_set_on_verification_listener(firmware_update_t* firmware_update,
                                                  firmware_update_on_verification_listener verification);
void firmware_update_process(firmware_update_t* firmware_update);

const char* firmware_update_status_as_str(firmware_update_t* firmware_update);
const char* firmware_update_error_as_str(firmware_update_t* firmware_update);

#endif // WOLKCONNECTOR_C_FIRMWARE_UPDATE_H
