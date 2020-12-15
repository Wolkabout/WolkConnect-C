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

#ifndef FILE_MANAGEMENT_H
#define FILE_MANAGEMENT_H

#ifdef __cplusplus
extern "C" {
#endif

#include "file_management_command.h"
#include "file_management_packet_request.h"
#include "file_management_status.h"
#include "size_definitions.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/**
 * @brief file_management_start signature.
 * Initializes File Management procedure with file named 'file_name' of size 'file_size'.
 *
 * Prepares device for writting received file chunks (via 'file_management_write_chunk')
 * to appropriate location.
 *
 * @return true if File Management is able to receive file, false otherwise
 */
typedef bool (*file_management_start_t)(const char* file_name, size_t file_size);

/**
 * @brief file_management_write_chunk signature.
 * Writes file chunk pointed to by 'data' and of size 'data_size'.
 *
 * @return true if file chunk is successfuly written, false otherwise
 */
typedef bool (*file_management_write_chunk_t)(uint8_t* data, size_t data_size);

/**
 * @brief file_management_read_chunk signature.
 * Reads 'n'-th file chunk of size up to 'data_size' to destination pointed to by 'data'.
 *
 * @return number of bytes that are written to destination pointed to by 'data'
 */
typedef size_t (*file_management_read_chunk_t)(size_t n, uint8_t* data, size_t data_size);

/**
 * @brief file_management_abort signature.
 * Aborts initialized File Management procedure.
 */
typedef void (*file_management_abort_t)(void);

/**
 * @brief file_management_finalize signature.
 * Finalizes File Management procedure.
 *
 * Reboots device in order to finish File Management procedure
 */
typedef void (*file_management_finalize_t)(void);

/**
 * @brief file_management_start_url_download signature.
 * Starts download of file from given URL, in background.
 *
 * @return true if file url download is started, false otherwise
 */
typedef bool (*file_management_start_url_download_t)(const char* url);

/**
 * @brief file_management_is_url_download_done signature.
 * Sets 'success' argument to true if file download successfully completed, false otherwise
 *
 * @return ture if file url download is completed, false otherwise
 */
typedef bool (*file_management_is_url_download_done_t)(bool* success);

typedef struct file_management_update file_management_t;

typedef void (*file_management_on_status_listener)(file_management_t* file_management, file_management_status_t status);
typedef void (*file_management_on_packet_request_listener)(file_management_t* file_management,
                                                           file_management_packet_request_t request);

struct file_management_update {
    const char* device_key;

    size_t maximum_file_size;
    size_t chunk_size;

    file_management_start_t start;
    file_management_write_chunk_t write_chunk;
    file_management_read_chunk_t read_chunk;
    file_management_abort_t abort;
    file_management_finalize_t finalize;

    file_management_start_url_download_t start_url_download;
    file_management_is_url_download_done_t is_url_download_done;

    uint8_t state;
    uint8_t last_packet_hash[FILE_MANAGEMENT_HASH_SIZE];
    size_t next_chunk_index;

    size_t expected_number_of_chunks;
    uint32_t retry_count;

    /* File Management request parameters */
    char file_name[FILE_MANAGEMENT_FILE_NAME_SIZE];
    uint8_t file_hash[FILE_MANAGEMENT_HASH_SIZE];
    size_t file_size;
    /* File Management  request parameters */

    /* Listeners */
    file_management_on_status_listener on_status;
    file_management_on_packet_request_listener on_packet_request;
    /* Listeners */

    void* wolk_ctx;

    bool has_valid_configuration;
};

void file_management_init(file_management_t* file_management, const char* device_key, size_t maximum_file_size,
                          size_t chunk_size, file_management_start_t start, file_management_write_chunk_t write_chunk,
                          file_management_read_chunk_t read_chunk, file_management_abort_t abort,
                          file_management_finalize_t finalize, file_management_start_url_download_t start_url_download,
                          file_management_is_url_download_done_t is_url_download_done, void* wolk_ctx);

void file_management_handle_command(file_management_t* file_management,
                                    file_management_command_t* file_management_command);

void file_management_handle_packet(file_management_t* file_management, uint8_t* packet, size_t packet_size);

void file_management_process(file_management_t* file_management);

void file_management_set_on_status_listener(file_management_t* file_management,
                                            file_management_on_status_listener on_status);
void file_management_set_on_packet_request_listener(file_management_t* file_management,
                                                    file_management_on_packet_request_listener on_packet_request);

#ifdef __cplusplus
}
#endif

#endif