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

typedef enum { STATE_IDLE = 0, STATE_INSTALLATION, STATE_COMPLETED, STATE_ERROR } state_t;

static void _handle_firmware_update(firmware_update_t* firmware_update, firmware_update_t* parameter);
static void _handle_abort(firmware_update_t* firmware_update);

static void _reset_state(firmware_update_t* firmware_update);
static bool _get_version(firmware_update_t* firmware_update, char* version);
static bool _update_abort(firmware_update_t* firmware_update);
static void _set_error(firmware_update_t* firmware_update, firmware_update_error_t error);
static void _set_status(firmware_update_t* firmware_update, firmware_update_status_t status);

static void _listener_on_firmware_update_status(firmware_update_t* firmware_update);
static void _listener_on_version_status(firmware_update_t* firmware_update, char* version);

void firmware_update_init(firmware_update_t* firmware_update, firmware_update_start_installation_t start_installation,
                          firmware_update_is_installation_completed_t is_installation_completed,
                          firmware_update_get_version_t get_version, firmware_update_abort_t abort_installation,
                          void* wolk_ctx)
{
    /* Sanity check */
    WOLK_ASSERT(firmware_update);

    firmware_update->state = STATE_IDLE;
    firmware_update->start_installation = start_installation;
    firmware_update->is_installation_completed = is_installation_completed;
    firmware_update->get_version = get_version;
    firmware_update->abort_installation = abort_installation;

    firmware_update->wolk_ctx = wolk_ctx;

    firmware_update->is_initialized = true;
    if (start_installation == NULL || is_installation_completed == NULL || get_version == NULL
        || abort_installation == NULL) {
        firmware_update->is_initialized = false;
    }
}

static void _handle_firmware_update(firmware_update_t* firmware_update, firmware_update_t* parameter)
{
    /* Sanity Check */
    WOLK_ASSERT(firmware_update);
    bool success;

    switch (firmware_update->state) {
    case STATE_IDLE:
    case STATE_INSTALLATION:
        if (firmware_update->start_installation != NULL) {
            firmware_update->start_installation(parameter->file_name);
            _set_status(firmware_update, FIRMWARE_UPDATE_STATUS_INSTALLATION);
            _listener_on_firmware_update_status(firmware_update);
        }
        /* Fallthrough */
        /* break */
    case STATE_COMPLETED:
    case STATE_ERROR:
        if (firmware_update->is_installation_completed != NULL) {
            if (!firmware_update->is_installation_completed(&success)) {
                return;
            }

            if (!success) {
                _reset_state(firmware_update);
                _set_error(firmware_update, FIRMWARE_UPDATE_UNSPECIFIED_ERROR);
                _set_status(firmware_update, FIRMWARE_UPDATE_STATUS_ERROR);
                _listener_on_firmware_update_status(firmware_update);
            }

            _set_status(firmware_update, FIRMWARE_UPDATE_STATUS_COMPLETED);
            _listener_on_firmware_update_status(firmware_update);

            char firmware_update_version[FIRMWARE_UPDATE_VERSION_SIZE];
            _get_version(firmware_update, firmware_update_version);
            _listener_on_version_status(firmware_update, firmware_update_version);
        }
        break;
    default:
        /* Sanity check */
        WOLK_ASSERT(false);
    }
}

static void _handle_abort(firmware_update_t* firmware_update)
{
    /* Sanity Check */
    WOLK_ASSERT(firmware_update);

    switch (firmware_update->state) {
    case STATE_IDLE:
    case STATE_COMPLETED:
    case STATE_ERROR:
        break;
    case STATE_INSTALLATION:
        if (!_update_abort(firmware_update)) {
            _set_error(firmware_update, FIRMWARE_UPDATE_UNSPECIFIED_ERROR);
            _set_status(firmware_update, FIRMWARE_UPDATE_STATUS_ERROR);
            _listener_on_firmware_update_status(firmware_update);
        }
        _set_status(firmware_update, FIRMWARE_UPDATE_STATUS_ABORTED);
        _listener_on_firmware_update_status(firmware_update);
        break;
    default:
        /* Sanity check */
        WOLK_ASSERT(false);
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

    _handle_firmware_update(firmware_update, parameter);
}

void firmware_update_handle_abort(firmware_update_t* firmware_update)
{
    /* Sanity check */
    WOLK_ASSERT(firmware_update);

    if (!firmware_update->is_initialized) {
        return;
    }

    _handle_abort(firmware_update);
}

static void _set_error(firmware_update_t* firmware_update, firmware_update_error_t error)
{
    /* Sanity Check */
    WOLK_ASSERT(firmware_update);
    WOLK_ASSERT(error);

    firmware_update->error = error;
}

static void _set_status(firmware_update_t* firmware_update, firmware_update_status_t status)
{
    /* Sanity Check */
    WOLK_ASSERT(firmware_update);
    WOLK_ASSERT(status);

    firmware_update->status = status;
}

static void _listener_on_firmware_update_status(firmware_update_t* firmware_update)
{
    /* Sanity Check */
    WOLK_ASSERT(firmware_update);
    WOLK_ASSERT(status);

    if (firmware_update->get_status != NULL) {
        firmware_update->get_status(firmware_update);
    }
}

static void _reset_state(firmware_update_t* firmware_update)
{
    /* Sanity Check */
    WOLK_ASSERT(firmware_update);

    firmware_update->state = STATE_IDLE;
    memset(firmware_update->file_name, '\0', WOLK_ARRAY_LENGTH(firmware_update->file_name));
}

static bool _get_version(firmware_update_t* firmware_update, char* version)
{
    /* Sanity Check */
    WOLK_ASSERT(firmware_update);
    WOLK_ASSERT(version);

    return firmware_update->get_version(version);
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

static void _listener_on_version_status(firmware_update_t* firmware_update, char* version)
{
    /* Sanity check */
    WOLK_ASSERT(firmware_update);
    WOLK_ASSERT(version);

    if (firmware_update->on_version != NULL) {
        firmware_update->on_version(firmware_update, version);
    }
}

static bool _update_abort(firmware_update_t* firmware_update)
{
    /* Sanity check */
    WOLK_ASSERT(firmware_update);

    return firmware_update->abort_installation();
}
