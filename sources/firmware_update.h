/*
 * Copyright 2017-2018 WolkAbout Technology s.r.o.
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

#ifndef FIRMWARE_UPDATE_H
#define FIRMWARE_UPDATE_H

#ifdef __cplusplus
extern "C" {
#endif

#include "firmware_update_command.h"
#include "firmware_update_packet_request.h"
#include "firmware_update_status.h"
#include "size_definitions.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/**
 * @brief firmware_update_start signature.
 * Initializes firmware update procedure with firmware file named 'file_name' of size 'file_size'.
 *
 * Prepares device for writting received firmware file chunks (via 'firmware_update_write_chunk')
 * to appropriate location.
 *
 * @return true if firmware is able to receive firmware file and perform firmware upgrade at this time, false otherwise
 */
typedef bool (*firmware_update_start_t)(const char* file_name, size_t file_size);

/**
 * @brief firmware_update_write_chunk signature.
 * Writes firmware file chunk pointed to by 'data' and of size 'data_size'.
 *
 * @return true if firmware file chunk is successfuly written, false otherwise
 */
typedef bool (*firmware_update_write_chunk_t)(uint8_t* data, size_t data_size);

/**
 * @brief firmware_update_read_chunk signature.
 * Reads 'n'-th firmware file chunk of size up to 'data_size' to destination pointed to by 'data'.
 *
 * @return number of bytes that are written to destination pointed to by 'data'
 */
typedef size_t (*firmware_update_read_chunk_t)(size_t n, uint8_t* data, size_t data_size);

/**
 * @brief firmware_update_abort signature.
 * Aborts initialized firmware update procedure.
 */
typedef void (*firmware_update_abort_t)(void);

/**
 * @brief firmware_update_finalize signature.
 * Finalizes firmware update procedure.
 *
 * Reboots device in order to finish firmware update procedure
 */
typedef void (*firmware_update_finalize_t)(void);

/**
 * @brief firmware_update_persist_firmware_version signature.
 * Saves given 'version' to persistent storage.
 *
 * @return true if firmware version was successfully persisted, false otherwise
 */
typedef bool (*firmware_update_persist_firmware_version_t)(const char* version);

/**
 * @brief firmware_update_read_firmware_version signature.
 * Reads 'version' from persistent storage.
 *
 * @return true if firmware version was succesfully read, false otherwise
 */
typedef bool (*firmware_update_unpersist_firmware_version_t)(char* version, size_t version_size);

/**
 * @brief firmware_update_start_url_download signature.
 * Starts download of firmware from given URL, in background.
 *
 * @return true if firmware url download is started, false otherwise
 */
typedef bool (*firmware_update_start_url_download_t)(const char* url);

/**
 * @brief firmware_update_is_url_download_done signature.
 * Sets 'success' argument to true if firmware download successfully completed, false otherwise
 *
 * @return ture if firmware url download is completed, false otherwise
 */
typedef bool (*firmware_update_is_url_download_done_t)(bool* success);

typedef struct firmware_update firmware_update_t;

typedef void (*firmware_update_on_status_listener)(firmware_update_t* firmware_update, firmware_update_status_t status);
typedef void (*firmware_update_on_packet_request_listener)(firmware_update_t* firmware_update,
                                                           firmware_update_packet_request_t request);

struct firmware_update {
    const char* device_key;

    char version[FIRMWARE_UPDATE_VERSION_SIZE];

    size_t maximum_firmware_size;
    size_t chunk_size;

    firmware_update_start_t start;
    firmware_update_write_chunk_t write_chunk;
    firmware_update_read_chunk_t read_chunk;
    firmware_update_abort_t abort;
    firmware_update_finalize_t finalize;

    firmware_update_persist_firmware_version_t persist_version;
    firmware_update_unpersist_firmware_version_t unpersist_version;

    firmware_update_start_url_download_t start_url_download;
    firmware_update_is_url_download_done_t is_url_download_done;

    uint8_t state;
    uint8_t last_packet_hash[FIRMWARE_UPDATE_HASH_SIZE];
    size_t next_chunk_index;

    size_t expected_number_of_chunks;
    uint32_t retry_count;

    /* Firmware update request parameters */
    char file_name[FIRMWARE_UPDATE_FILE_NAME_SIZE];
    uint8_t file_hash[FIRMWARE_UPDATE_HASH_SIZE];
    size_t file_size;
    bool auto_install;
    /* Firmware update request parameters */

    /* Listeners */
    firmware_update_on_status_listener on_status;
    firmware_update_on_packet_request_listener on_packet_request;
    /* Listeners */

    void* wolk_ctx;

    bool has_valid_configuration;
};

void firmware_update_init(firmware_update_t* firmware_update, const char* device_key, const char* version,
                          size_t maximum_firmware_size, size_t chunk_size, firmware_update_start_t start,
                          firmware_update_write_chunk_t write_chunk, firmware_update_read_chunk_t read_chunk,
                          firmware_update_abort_t abort, firmware_update_finalize_t finalize,
                          firmware_update_persist_firmware_version_t persist_version,
                          firmware_update_unpersist_firmware_version_t unpersist_version,
                          firmware_update_start_url_download_t start_url_download,
                          firmware_update_is_url_download_done_t is_url_download_done, void* wolk_ctx);

void firmware_update_handle_command(firmware_update_t* firmware_update,
                                    firmware_update_command_t* firmware_update_command);

void firmware_update_handle_packet(firmware_update_t* firmware_update, uint8_t* packet, size_t packet_size);

const char* firmware_update_get_current_version(firmware_update_t* firmware_update);

void firmware_update_process(firmware_update_t* firmware_update);

void firmware_update_set_on_status_listener(firmware_update_t* firmware_update,
                                            firmware_update_on_status_listener on_status);
void firmware_update_set_on_packet_request_listener(firmware_update_t* firmware_update,
                                                    firmware_update_on_packet_request_listener on_packet_request);

#ifdef __cplusplus
}
#endif

#endif
