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
#include "utility/wolk_utils.h"

typedef enum { STATE_IDLE = 1, STATE_INSTALLATION, STATE_COMPLETED, STATE_ERROR } state_t;

static void handle(firmware_update_t* firmware_update, firmware_update_t* parameter);
static void handle_verification(firmware_update_t* firmware_update);
static void handle_abort(firmware_update_t* firmware_update);
static void check_firmware_update(firmware_update_t* firmware_update);

static void reset_state(firmware_update_t* firmware_update);
static bool get_version(firmware_update_t* firmware_update, char* version);
static bool update_abort(firmware_update_t* firmware_update);
static void set_error(firmware_update_t* firmware_update, firmware_update_error_t error);
static void set_status(firmware_update_t* firmware_update, firmware_update_status_t status);

static void listener_on_firmware_update_status(firmware_update_t* firmware_update);
static void listener_on_version_status(firmware_update_t* firmware_update, char* version);


static void handle(firmware_update_t* firmware_update, firmware_update_t* parameter)
{
    /* Sanity Check */
    WOLK_ASSERT(firmware_update);
    bool success;

    switch (firmware_update->state) {
    case STATE_IDLE:
        if (firmware_update->start_installation != NULL) {
            set_status(firmware_update, FIRMWARE_UPDATE_STATUS_INSTALLATION);
            listener_on_firmware_update_status(firmware_update);

            firmware_update->state = STATE_INSTALLATION;

            if (!firmware_update->verification_store(firmware_update->state)) {
                firmware_update->state = STATE_ERROR;
                firmware_update->error = FIRMWARE_UPDATE_FILE_SYSTEM_ERROR;
            }

            if (!firmware_update->start_installation(parameter->file_name)) {
                firmware_update->state = STATE_ERROR;
                firmware_update->error = FIRMWARE_UPDATE_FILE_SYSTEM_ERROR;
            }

        } else {
            firmware_update->state = STATE_ERROR;
            firmware_update->error = FIRMWARE_UPDATE_INSTALLATION_FAILED;
        }

        break;
    case STATE_INSTALLATION:
        set_status(firmware_update, FIRMWARE_UPDATE_STATUS_INSTALLATION);
        listener_on_firmware_update_status(firmware_update);
        break;
    case STATE_COMPLETED:
        break;
    case STATE_ERROR:
        set_status(firmware_update, FIRMWARE_UPDATE_STATUS_ERROR);
        listener_on_firmware_update_status(firmware_update);
        firmware_update->state = STATE_IDLE;
        break;
    default:
        /* Sanity check */
        WOLK_ASSERT(false);
    }
}

static void handle_verification(firmware_update_t* firmware_update)
{
    /* Sanity Check */
    WOLK_ASSERT(firmware_update);

    uint8_t parameter = firmware_update->verification_read();

    switch (parameter) {
    case STATE_IDLE:
        break;
    case STATE_INSTALLATION:
        firmware_update->state = STATE_INSTALLATION;
        break;
    default:
        /* Sanity check */
        WOLK_ASSERT(false);

        firmware_update->state = STATE_ERROR;
    }
}

static void handle_abort(firmware_update_t* firmware_update)
{
    /* Sanity Check */
    WOLK_ASSERT(firmware_update);

    switch (firmware_update->state) {
    case STATE_IDLE:
    case STATE_COMPLETED:
    case STATE_ERROR:
        listener_on_firmware_update_status(firmware_update);
        break;
    case STATE_INSTALLATION:
        if (!update_abort(firmware_update)) {
            set_error(firmware_update, FIRMWARE_UPDATE_UNSPECIFIED_ERROR);
            set_status(firmware_update, FIRMWARE_UPDATE_STATUS_ERROR);
            listener_on_firmware_update_status(firmware_update);
        }
        set_status(firmware_update, FIRMWARE_UPDATE_STATUS_ABORTED);
        listener_on_firmware_update_status(firmware_update);
        break;
    default:
        /* Sanity check */
        WOLK_ASSERT(false);
    }
}

static void check_firmware_update(firmware_update_t* firmware_update)
{
    /* Sanity Check */
    WOLK_ASSERT(firmware_update);
    bool success;

    switch (firmware_update->state) {
    case STATE_IDLE:
        break;
    case STATE_INSTALLATION:
        if (firmware_update->is_installation_completed != NULL) {
            if (!firmware_update->is_installation_completed(&success)) {
                return;
            }

            if (!success) {
                firmware_update->state = STATE_ERROR;
                firmware_update->error = FIRMWARE_UPDATE_INSTALLATION_FAILED;
            } else {
                firmware_update->state = STATE_COMPLETED;
            }
        }
        break;
    case STATE_COMPLETED:
        set_status(firmware_update, FIRMWARE_UPDATE_STATUS_COMPLETED);
        listener_on_firmware_update_status(firmware_update);

        char firmware_update_version[FIRMWARE_UPDATE_VERSION_SIZE];
        get_version(firmware_update, firmware_update_version);
        listener_on_version_status(firmware_update, firmware_update_version);

        reset_state(firmware_update);
        if (!firmware_update->verification_store(firmware_update->state)) {
            firmware_update->state = STATE_ERROR;
            firmware_update->error = FIRMWARE_UPDATE_FILE_SYSTEM_ERROR;
        }
        break;
    case STATE_ERROR:
        set_status(firmware_update, FIRMWARE_UPDATE_STATUS_ERROR);
        listener_on_firmware_update_status(firmware_update);

        reset_state(firmware_update);
        break;
    default:
        /* Sanity check */
        WOLK_ASSERT(false);
    }
}

static void reset_state(firmware_update_t* firmware_update)
{
    /* Sanity Check */
    WOLK_ASSERT(firmware_update);

    firmware_update->state = STATE_IDLE;
    memset(firmware_update->file_name, '\0', WOLK_ARRAY_LENGTH(firmware_update->file_name));
}

static bool get_version(firmware_update_t* firmware_update, char* version)
{
    /* Sanity Check */
    WOLK_ASSERT(firmware_update);
    WOLK_ASSERT(version);

    return firmware_update->get_version(version);
}

static bool update_abort(firmware_update_t* firmware_update)
{
    /* Sanity check */
    WOLK_ASSERT(firmware_update);

    return firmware_update->abort_installation();
}

static void set_error(firmware_update_t* firmware_update, firmware_update_error_t error)
{
    /* Sanity Check */
    WOLK_ASSERT(firmware_update);
    WOLK_ASSERT(error);

    firmware_update->error = error;
}

static void set_status(firmware_update_t* firmware_update, firmware_update_status_t status)
{
    /* Sanity Check */
    WOLK_ASSERT(firmware_update);
    WOLK_ASSERT(status);

    firmware_update->status = status;
}

static void listener_on_firmware_update_status(firmware_update_t* firmware_update)
{
    /* Sanity Check */
    WOLK_ASSERT(firmware_update);

    if (firmware_update->get_status != NULL) {
        firmware_update->get_status(firmware_update);
    }
}

static void listener_on_version_status(firmware_update_t* firmware_update, char* version)
{
    /* Sanity check */
    WOLK_ASSERT(firmware_update);
    WOLK_ASSERT(version);

    if (firmware_update->on_version != NULL) {
        firmware_update->on_version(firmware_update, (const)version);
    }
}


/* Public implementations */
void firmware_update_init(firmware_update_t* firmware_update, firmware_update_start_installation_t start_installation,
                          firmware_update_is_installation_completed_t is_installation_completed,
                          firmware_update_verification_store_t verification_store,
                          firmware_update_verification_read_t verification_read,
                          firmware_update_get_version_t get_version, firmware_update_abort_t abort_installation,
                          void* wolk_ctx)
{
    /* Sanity check */
    WOLK_ASSERT(firmware_update);
    WOLK_ASSERT(wolk_ctx);

    firmware_update->state = STATE_IDLE;
    firmware_update->start_installation = start_installation;
    firmware_update->is_installation_completed = is_installation_completed;
    firmware_update->verification_store = verification_store;
    firmware_update->verification_read = verification_read;
    firmware_update->get_version = get_version;
    firmware_update->abort_installation = abort_installation;

    firmware_update->wolk_ctx = wolk_ctx;

    firmware_update->is_initialized = true;
    if (start_installation == NULL || is_installation_completed == NULL || verification_store == NULL
        || verification_read == NULL || get_version == NULL || abort_installation == NULL) {
        firmware_update->is_initialized = false;
    }
}

void firmware_update_parameter_init(firmware_update_t* parameter)
{
    /* Sanity Check */
    WOLK_ASSERT(parameter);

    memset(parameter->file_name, '\0', WOLK_ARRAY_LENGTH(parameter->file_name));
    parameter->error = FIRMWARE_UPDATE_ERROR_NONE;
}
void firmware_update_parameter_set_filename(firmware_update_t* parameter, const char* file_name)
{
    /* Sanity Check */
    WOLK_ASSERT(parameter);
    WOLK_ASSERT(file_name);
    WOLK_ASSERT(strlen(file_name) <= FILE_MANAGEMENT_FILE_NAME_SIZE);

    strncpy(parameter->file_name, file_name, FILE_MANAGEMENT_FILE_NAME_SIZE);
}

void firmware_update_handle_parameter(firmware_update_t* firmware_update, firmware_update_t* parameter)
{
    /* Sanity Check */
    WOLK_ASSERT(firmware_update);
    WOLK_ASSERT(parameter);

    if (!firmware_update->is_initialized) {
        return;
    }

    handle(firmware_update, parameter);
}

void firmware_update_handle_verification(firmware_update_t* firmware_update)
{
    /* Sanity Check */
    WOLK_ASSERT(firmware_update);

    if (!firmware_update->is_initialized) {
        return;
    }

    handle_verification(firmware_update);
}

void firmware_update_handle_abort(firmware_update_t* firmware_update)
{
    /* Sanity check */
    WOLK_ASSERT(firmware_update);

    if (!firmware_update->is_initialized) {
        return;
    }

    handle_abort(firmware_update);
}

void firmware_update_set_on_status_listener(firmware_update_t* firmware_update,
                                            firmware_update_on_status_listener status)
{
    /* Sanity Check */
    WOLK_ASSERT(firmware_update);
    WOLK_ASSERT(status);

    firmware_update->get_status = status;
}

void firmware_update_set_on_version_listener(firmware_update_t* firmware_update,
                                             firmware_update_on_version_listener version)
{
    /* Sanity Check */
    WOLK_ASSERT(firmware_update);
    WOLK_ASSERT(version);

    firmware_update->on_version = version;
}

void firmware_update_set_on_verification_listener(firmware_update_t* firmware_update,
                                                  firmware_update_on_verification_listener verification)
{
    /* Sanity Check */
    WOLK_ASSERT(firmware_update);
    WOLK_ASSERT(verification);

    firmware_update->on_verification = verification;
}

void firmware_update_process(firmware_update_t* firmware_update)
{
    /* Sanity Check */
    WOLK_ASSERT(firmware_update);

    if (!firmware_update->is_initialized) {
        return;
    }

    check_firmware_update(firmware_update);
}
