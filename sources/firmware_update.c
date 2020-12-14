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

#include "firmware_update.h"
#include "firmware_update_command.h"
#include "firmware_update_packet.h"
#include "sha256.h"
#include "size_definitions.h"
#include "wolk_connector.h"
#include "wolk_utils.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

typedef enum {
    STATE_IDLE = 0,
    STATE_PACKET_FILE_TRANSFER,
    STATE_URL_DOWNLOAD,
    STATE_FILE_OBTAINED,
    STATE_INSTALL
} state_t;

enum { MAX_RETRIES = 3 };

enum { FILE_VERIFICATION_CHUNK_SIZE = 1024 };

static void _handle_file_upload(file_management_t* file_management, file_management_command_t* command);
static void _handle_url_download(file_management_t* file_management, file_management_command_t* command);
static void _handle_install(file_management_t* file_management);
static void _handle_abort(file_management_t* file_management);

static bool _update_sequence_init(file_management_t* file_management, const char* file_name, size_t file_size);
static bool _write_chunk(file_management_t* file_management, uint8_t* data, size_t data_size);
static size_t _read_chunk(file_management_t* file_management, size_t index, uint8_t* data, size_t data_size);
static void _update_abort(file_management_t* file_management);
static void _update_finalize(file_management_t* file_management);

static bool _persist_version(file_management_t* file_management, const char* version);
static bool _unpersist_version(file_management_t* file_management, char* version, size_t version_size);

static bool _start_url_download(file_management_t* file_management, const char* url);
static bool _is_url_download_done(file_management_t* file_management, bool* success);
static bool _has_url_download(file_management_t* file_management);

static void _report_result(file_management_t* file_management);
static void _check_url_download(file_management_t* file_management);

static void _reset_state(file_management_t* file_management);

static bool _is_file_valid(file_management_t* file_management);

static void _listener_on_status(file_management_t* file_management, file_management_status_t status);
static void _listener_on_packet_request(file_management_t* file_management, file_management_packet_request_t request);

void file_management_init(file_management_t* file_management, const char* device_key, const char* version,
                          size_t maximum_file_size, size_t chunk_size, file_management_start_t start,
                          file_management_write_chunk_t write_chunk, file_management_read_chunk_t read_chunk,
                          file_management_abort_t abort, file_management_finalize_t finalize,
                          file_management_persist_firmware_version_t persist_version,
                          file_management_unpersist_firmware_version_t unpersist_version,
                          file_management_start_url_download_t start_url_download,
                          file_management_is_url_download_done_t is_url_download_done, void* wolk_ctx)
{
    /* Sanity check */
    WOLK_ASSERT(file_management);
    WOLK_ASSERT(device_key);
    WOLK_ASSERT(version);
    WOLK_ASSERT(wolk_ctx);

    strcpy(file_management->version, version);

    file_management->device_key = device_key;

    file_management->maximum_file_size = maximum_file_size;
    file_management->chunk_size = chunk_size;

    file_management->start = start;
    file_management->write_chunk = write_chunk;
    file_management->read_chunk = read_chunk;
    file_management->abort = abort;
    file_management->finalize = finalize;

    file_management->persist_version = persist_version;
    file_management->unpersist_version = unpersist_version;

    file_management->start_url_download = start_url_download;
    file_management->is_url_download_done = is_url_download_done;

    file_management->state = STATE_IDLE;
    memset(file_management->last_packet_hash, 0, WOLK_ARRAY_LENGTH(file_management->last_packet_hash));
    file_management->next_chunk_index = 0;
    file_management->expected_number_of_chunks = 0;
    file_management->retry_count = 0;

    memset(file_management->file_name, '\0', WOLK_ARRAY_LENGTH(file_management->file_name));
    memset(file_management->file_hash, 0, WOLK_ARRAY_LENGTH(file_management->file_hash));
    file_management->file_size = 0;

    file_management->wolk_ctx = wolk_ctx;

    file_management->has_valid_configuration = true;
    if (device_key == NULL || version == NULL || maximum_file_size == 0 || chunk_size == 0 || start == NULL
        || write_chunk == NULL || read_chunk == NULL || abort == NULL || finalize == NULL || persist_version == NULL
        || unpersist_version == NULL || wolk_ctx == NULL) {
        file_management->has_valid_configuration = false;
    }
}

static void _report_result(file_management_t* file_management)
{
    /* Sanity check */
    WOLK_ASSERT(file_management);

    char persisted_version[FILE_MANAGEMENT_FILE_NAME_SIZE];
    if (!_unpersist_version(file_management, persisted_version, WOLK_ARRAY_LENGTH(persisted_version))) {
        return;
    }

    if (strcmp(persisted_version, file_management->version) != 0) {
        _listener_on_status(file_management, file_management_status_ok(FILE_MANAGEMENT_STATE_COMPLETED));
    } else {
        _listener_on_status(file_management, file_management_status_error(FILE_MANAGEMENT_ERROR_INSTALLATION_FAILED));
    }
}

static void _check_url_download(file_management_t* file_management)
{
    /* Sanity check */
    WOLK_ASSERT(file_management);

    if (!_has_url_download(file_management)) {
        return;
    }

    bool success;
    switch (file_management->state) {
    case STATE_URL_DOWNLOAD:
        if (!_is_url_download_done(file_management, &success)) {
            return;
        }

        if (!success) {
            _reset_state(file_management);
            _listener_on_status(file_management, file_management_status_error(FILE_MANAGEMENT_ERROR_UNSPECIFIED));
            return;
        }

        file_management->state = STATE_FILE_OBTAINED;
        _listener_on_status(file_management, file_management_status_ok(FILE_MANAGEMENT_STATE_FILE_READY));

        break;

    case STATE_IDLE:
    case STATE_PACKET_FILE_TRANSFER:
    case STATE_FILE_OBTAINED:
    case STATE_INSTALL:
        break;

    default:
        /* Sanity check */
        WOLK_ASSERT(false);
    }
}

void file_management_handle_command(file_management_t* file_management,
                                    file_management_command_t* file_management_command)
{
    /* Sanity check */
    WOLK_ASSERT(file_management);
    WOLK_ASSERT(file_management_command);

    if (!file_management->has_valid_configuration) {
        _listener_on_status(file_management, file_management_status_error(FILE_MANAGEMENT_ERROR_FILE_UPLOAD_DISABLED));
        return;
    }

    switch (file_management_command_get_type(file_management_command)) {
    case FILE_MANAGEMENT_COMMAND_TYPE_FILE_UPLOAD:
        _handle_file_upload(file_management, file_management_command);
        break;

    case FILE_MANAGEMENT_COMMAND_TYPE_URL_DOWNLOAD:
        _handle_url_download(file_management, file_management_command);
        break;

    case FILE_MANAGEMENT_COMMAND_TYPE_INSTALL:
        _handle_install(file_management);
        break;

    case FILE_MANAGEMENT_COMMAND_TYPE_ABORT:
        _handle_abort(file_management);
        break;

    case FILE_MANAGEMENT_COMMAND_TYPE_UNKNOWN:
        /* Ignore */
        break;

    default:
        /* Sanity check */
        WOLK_ASSERT(false);
    }
}

void file_management_handle_packet(file_management_t* file_management, uint8_t* packet, size_t packet_size)
{
    /* Sanity check */
    WOLK_ASSERT(file_management);
    WOLK_ASSERT(packet);

    if (!file_management->has_valid_configuration) {
        _listener_on_status(file_management, file_management_status_error(FILE_MANAGEMENT_ERROR_FILE_UPLOAD_DISABLED));
        return;
    }

    if (!file_management_packet_is_valid(packet, packet_size)) {
        file_management->retry_count += 1;
        if (file_management->retry_count >= MAX_RETRIES) {
            _update_abort(file_management);
            _reset_state(file_management);

            _listener_on_status(file_management,
                                file_management_status_error(FILE_MANAGEMENT_ERROR_RETRY_COUNT_EXCEEDED));
            return;
        }

        file_management_packet_request_t packet_request;
        file_management_packet_request_init(&packet_request, file_management->file_name,
                                            file_management->next_chunk_index,
                                            file_management->chunk_size + (2 * FILE_MANAGEMENT_HASH_SIZE));
        _listener_on_packet_request(file_management, packet_request);
        return;
    }

    if (memcmp(file_management->last_packet_hash, file_management_packet_get_previous_packet_hash(packet, packet_size),
               WOLK_ARRAY_LENGTH(file_management->last_packet_hash))
        != 0) {
        file_management_packet_request_t packet_request;
        file_management_packet_request_init(&packet_request, file_management->file_name,
                                            file_management->next_chunk_index,
                                            file_management->chunk_size + (2 * FILE_MANAGEMENT_HASH_SIZE));
        _listener_on_packet_request(file_management, packet_request);
        return;
    }

    memcpy(file_management->last_packet_hash, file_management_packet_get_hash(packet, packet_size),
           WOLK_ARRAY_LENGTH(file_management->last_packet_hash));

    if (!_write_chunk(file_management, file_management_packet_get_data(packet, packet_size),
                      file_management_packet_get_data_size(packet, packet_size))) {
        _reset_state(file_management);

        _listener_on_status(file_management, file_management_status_error(FILE_MANAGEMENT_ERROR_FILE_SYSTEM));
        return;
    }

    file_management->next_chunk_index += 1;
    /* Chunks are zero indexed */
    if (file_management->next_chunk_index < file_management->expected_number_of_chunks) {
        file_management_packet_request_t packet_request;
        file_management_packet_request_init(&packet_request, file_management->file_name,
                                            file_management->next_chunk_index,
                                            file_management->chunk_size + (2 * FILE_MANAGEMENT_HASH_SIZE));
        _listener_on_packet_request(file_management, packet_request);
        return;
    }

    if (!_is_file_valid(file_management)) {
        _update_abort(file_management);
        _reset_state(file_management);

        _listener_on_status(file_management, file_management_status_error(FILE_MANAGEMENT_ERROR_FILE_SYSTEM));
        return;
    }

    file_management->state = STATE_FILE_OBTAINED;
    _listener_on_status(file_management, file_management_status_ok(FILE_MANAGEMENT_STATE_FILE_READY));
}

const char* file_management_get_current_version(file_management_t* file_management)
{
    /* Sanity check */
    WOLK_ASSERT(file_management);

    return file_management->version;
}

void file_management_process(file_management_t* file_management)
{
    /* Sanity check */
    WOLK_ASSERT(file_management);

    if (!file_management->has_valid_configuration) {
        return;
    }

    _report_result(file_management);
    _check_url_download(file_management);
}

void file_management_set_on_status_listener(file_management_t* file_management,
                                            file_management_on_status_listener on_status)
{
    /* Sanity check */
    WOLK_ASSERT(file_management);
    WOLK_ASSERT(on_status);

    file_management->on_status = on_status;
}

void file_management_set_on_packet_request_listener(file_management_t* file_management,
                                                    file_management_on_packet_request_listener on_packet_request)
{
    /* Sanity check */
    WOLK_ASSERT(file_management);
    WOLK_ASSERT(on_packet_request);

    file_management->on_packet_request = on_packet_request;
}

static void _handle_file_upload(file_management_t* file_management, file_management_command_t* command)
{
    /* Sanity check */
    WOLK_ASSERT(file_management);
    WOLK_ASSERT(command);

    switch (file_management->state) {
    case STATE_IDLE:
        if (file_management->maximum_file_size == 0 || file_management->chunk_size == 0) {
            _listener_on_status(file_management,
                                file_management_status_error(FILE_MANAGEMENT_ERROR_FILE_UPLOAD_DISABLED));
            return;
        }

        if (file_management->maximum_file_size < file_management_command_get_file_size(command)) {
            _listener_on_status(file_management,
                                file_management_status_error(FILE_MANAGEMENT_ERROR_UNSUPPORTED_FILE_SIZE));
            return;
        }

        if (!_update_sequence_init(file_management, file_management_command_get_file_name(command),
                                   file_management_command_get_file_size(command))) {
            _listener_on_status(file_management, file_management_status_error(FILE_MANAGEMENT_ERROR_UNSPECIFIED));
            return;
        }

        WOLK_ASSERT(strlen(file_management_command_get_file_name(command))
                    <= WOLK_ARRAY_LENGTH(file_management->file_name));
        strcpy(file_management->file_name, file_management_command_get_file_name(command));

        WOLK_ASSERT(file_management_command_get_file_hash_size(command)
                    <= WOLK_ARRAY_LENGTH(file_management->file_hash));
        memcpy(file_management->file_hash, file_management_command_get_file_hash(command),
               file_management_command_get_file_hash_size(command));

        file_management->file_size = file_management_command_get_file_size(command);

        file_management->state = STATE_PACKET_FILE_TRANSFER;

        file_management->next_chunk_index = 0;

        file_management->expected_number_of_chunks =
            (size_t)WOLK_CEIL((double)file_management_command_get_file_size(command) / file_management->chunk_size);
        file_management->retry_count = 0;

        _listener_on_status(file_management, file_management_status_ok(FILE_MANAGEMENT_STATE_FILE_TRANSFER));

        file_management_packet_request_t packet_request;
        file_management_packet_request_init(&packet_request, file_management->file_name,
                                            file_management->next_chunk_index,
                                            file_management->chunk_size + (2 * FILE_MANAGEMENT_HASH_SIZE));
        _listener_on_packet_request(file_management, packet_request);
        break;

    /* File Management already in progress - Ignore command */
    case STATE_PACKET_FILE_TRANSFER:
    case STATE_URL_DOWNLOAD:
    case STATE_FILE_OBTAINED:
    case STATE_INSTALL:
        break;

    default:
        /* Sanity check */
        WOLK_ASSERT(false);
    }
}

static void _handle_url_download(file_management_t* file_management, file_management_command_t* command)
{
    /* Sanity check */
    WOLK_ASSERT(file_management);
    WOLK_ASSERT(command);

    switch (file_management->state) {
    case STATE_IDLE:

        if (!_has_url_download(file_management)) {
            _listener_on_status(file_management,
                                file_management_status_error(FILE_MANAGEMENT_ERROR_FILE_UPLOAD_DISABLED));
            return;
        }

        if (!_start_url_download(file_management, file_management_command_get_file_url(command))) {
            _listener_on_status(file_management, file_management_status_error(FILE_MANAGEMENT_ERROR_UNSPECIFIED));
            return;
        }

        file_management->state = STATE_URL_DOWNLOAD;
        _listener_on_status(file_management, file_management_status_ok(FILE_MANAGEMENT_STATE_FILE_TRANSFER));
        break;

    /* File Management already in progress - Ignore command */
    case STATE_PACKET_FILE_TRANSFER:
    case STATE_URL_DOWNLOAD:
    case STATE_FILE_OBTAINED:
    case STATE_INSTALL:
        break;

    default:
        /* Sanity check */
        WOLK_ASSERT(false);
    }
}

static void _handle_install(file_management_t* file_management)
{
    /* Sanity check */
    WOLK_ASSERT(file_management);

    switch (file_management->state) {
    case STATE_FILE_OBTAINED:
        _listener_on_status(file_management, file_management_status_ok(FILE_MANAGEMENT_STATE_INSTALLATION));
        if (!_persist_version(file_management, file_management->version)) {
            _update_abort(file_management);
            _reset_state(file_management);
            _listener_on_status(file_management,
                                file_management_status_error(FILE_MANAGEMENT_ERROR_INSTALLATION_FAILED));
            return;
        }

        _update_finalize(file_management);
        break;

        //    /* Firmware installation already started */
        //    case STATE_INSTALL:
        //        /* Ignore install command */
        //        break;

    /* File not ready - Ignore command */
    case STATE_IDLE:
    case STATE_PACKET_FILE_TRANSFER:
    case STATE_URL_DOWNLOAD:
        _reset_state(file_management);
        _listener_on_status(file_management, file_management_status_error(FILE_MANAGEMENT_ERROR_UNSPECIFIED));
        break;

    default:
        /* Sanity check */
        WOLK_ASSERT(false);
    }
}

static void _handle_abort(file_management_t* file_management)
{
    /* Sanity check */
    WOLK_UNUSED(file_management);

    switch (file_management->state) {
    case STATE_FILE_OBTAINED:
    case STATE_PACKET_FILE_TRANSFER:
    case STATE_URL_DOWNLOAD:
    case STATE_INSTALL:
        _update_abort(file_management);
        _reset_state(file_management);
        _listener_on_status(file_management, file_management_status_ok(FILE_MANAGEMENT_STATE_ABORTED));
        break;

    /* File Management not in progress - Ignore command */
    case STATE_IDLE:
        _listener_on_status(file_management, file_management_status_error(FILE_MANAGEMENT_ERROR_UNSPECIFIED));
        break;
    default:
        /* Sanity check */
        WOLK_ASSERT(false);
    }
}

static bool _update_sequence_init(file_management_t* file_management, const char* file_name, size_t file_size)
{
    /* Sanity check */
    WOLK_ASSERT(file_management);

    return file_management->start(file_name, file_size);
}

static bool _write_chunk(file_management_t* file_management, uint8_t* data, size_t data_size)
{
    /* Sanity check */
    WOLK_ASSERT(file_management);
    WOLK_ASSERT(data);

    return file_management->write_chunk(data, data_size);
}

static size_t _read_chunk(file_management_t* file_management, size_t index, uint8_t* data, size_t data_size)
{
    /* Sanity check */
    WOLK_ASSERT(file_management);
    WOLK_ASSERT(data);

    return file_management->read_chunk(index, data, data_size);
}

static void _update_abort(file_management_t* file_management)
{
    /* Sanity check */
    WOLK_ASSERT(file_management);

    file_management->abort();
}

static void _update_finalize(file_management_t* file_management)
{
    /* Sanity check */
    WOLK_ASSERT(file_management);

    file_management->finalize();
}

static void _reset_state(file_management_t* file_management)
{
    /* Sanity check */
    WOLK_ASSERT(file_management);

    file_management->state = STATE_IDLE;
    memset(file_management->last_packet_hash, 0, WOLK_ARRAY_LENGTH(file_management->last_packet_hash));
    file_management->next_chunk_index = 0;
    file_management->expected_number_of_chunks = 0;

    memset(file_management->file_name, '\0', WOLK_ARRAY_LENGTH(file_management->file_name));
    memset(file_management->file_hash, 0, WOLK_ARRAY_LENGTH(file_management->file_hash));
    file_management->file_size = 0;
}

static bool _is_file_valid(file_management_t* file_management)
{
    /* Sanity check */
    WOLK_ASSERT(file_management);

    sha256_context sha256_ctx;
    sha256_init(&sha256_ctx);

    for (size_t i = 0; i < (size_t)WOLK_CEIL((double)file_management->file_size / FILE_VERIFICATION_CHUNK_SIZE); ++i) {
        uint8_t read_data[FILE_VERIFICATION_CHUNK_SIZE];
        const size_t read_data_size = _read_chunk(file_management, i, read_data, WOLK_ARRAY_LENGTH(read_data));
        sha256_hash(&sha256_ctx, read_data, read_data_size);
    }

    uint8_t calculated_hash[FILE_MANAGEMENT_HASH_SIZE];
    sha256_done(&sha256_ctx, calculated_hash);

    return memcmp(calculated_hash, file_management->file_hash, WOLK_ARRAY_LENGTH(calculated_hash)) == 0;
}

static void _listener_on_status(file_management_t* file_management, file_management_status_t status)
{
    /* Sanity check */
    WOLK_ASSERT(file_management);

    if (file_management->on_status != NULL) {
        file_management->on_status(file_management, status);
    }
}

static void _listener_on_packet_request(file_management_t* file_management, file_management_packet_request_t request)
{
    /* Sanity check */
    WOLK_ASSERT(file_management);

    if (file_management->on_packet_request != NULL) {
        file_management->on_packet_request(file_management, request);
    }
}

static bool _persist_version(file_management_t* file_management, const char* version)
{
    /* Sanity check */
    WOLK_ASSERT(file_management);
    WOLK_ASSERT(version);

    return file_management->persist_version(version);
}

static bool _unpersist_version(file_management_t* file_management, char* version, size_t version_size)
{
    /* Sanity check */
    WOLK_ASSERT(file_management);
    WOLK_ASSERT(version);

    return file_management->unpersist_version(version, version_size);
}

static bool _start_url_download(file_management_t* file_management, const char* url)
{
    /* Sanity check */
    WOLK_ASSERT(file_management);
    WOLK_ASSERT(url);

    return file_management->start_url_download(url);
}

static bool _is_url_download_done(file_management_t* file_management, bool* success)
{
    /* Sanity check */
    WOLK_ASSERT(file_management);
    WOLK_ASSERT(success);

    return file_management->is_url_download_done(success);
}

static bool _has_url_download(file_management_t* file_management)
{
    /* Sanity check */
    WOLK_ASSERT(file_management);

    return file_management->start_url_download != NULL;
}
