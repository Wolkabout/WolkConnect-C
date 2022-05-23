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

#include "file_management_parameter.h"
#include "model/file_management/file_management_packet_request.h"
#include "model/file_management/file_management_status.h"
#include "size_definitions.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>


typedef struct file_list_t {
    char file_name[FILE_MANAGEMENT_FILE_NAME_SIZE];
    size_t file_size;
} file_list_t;

/**
 * @brief file_management_start signature.
 * Initializes File Management procedure with file named 'file_name' of size 'file_size'.
 *
 * Prepares device for writing received file chunks (via 'file_management_write_chunk')
 * to appropriate location.
 *
 * @return true if File Management is able to receive file, false otherwise
 */
typedef bool (*file_management_start_t)(const char* file_name, size_t file_size);

/**
 * @brief file_management_write_chunk signature.
 * Writes file chunk pointed to by 'data' and of size 'data_size'.
 *
 * @return true if file chunk is successfully written, false otherwise
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
 *
 * @return true if file management abort is possible, false otherwise
 */
typedef bool (*file_management_abort_t)(void);

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
 * Sets name of the downloaded file
 *
 * @return ture if file url download is completed, false otherwise
 */
typedef bool (*file_management_is_url_download_done_t)(bool* success, char* downloaded_file_name);

/**
 * @brief file_management_get_file_list_t signature.
 * Get file list in "file_list" as file list array which consist of file name and size
 *
 * @return number of file presented in the list
 */
typedef size_t (*file_management_get_file_list_t)(file_list_t* file_list);

/**
 * @brief file_management_remove_file_t signature.
 * Remove specific file
 *
 * @return True on success, false on failure
 */
typedef bool (*file_management_remove_file_t)(const char* file_name);

/**
 * @brief file_management_purge_files_t signature.
 * Remove all files
 *
 * @return True on success, false on failure
 */
typedef bool (*file_management_purge_files_t)(void);

typedef struct file_management file_management_t;

typedef void (*file_management_on_status_listener)(file_management_t* file_management, file_management_status_t status);
typedef void (*file_management_on_packet_request_listener)(file_management_t* file_management,
                                                           file_management_packet_request_t request);
typedef void (*file_management_on_url_download_status_listener)(file_management_t* file_management,
                                                                file_management_status_t status);
typedef void (*file_management_on_file_list_listener)(file_management_t* file_management,
                                                      file_management_get_file_list_t file_list,
                                                      size_t file_list_items);

struct file_management {
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

    file_management_get_file_list_t get_file_list;
    file_management_remove_file_t remove_file;
    file_management_purge_files_t purge_files;

    uint8_t state;
    uint8_t previous_packet_hash[FILE_MANAGEMENT_HASH_SIZE];
    uint8_t current_packet_hash[FILE_MANAGEMENT_HASH_SIZE];
    size_t next_chunk_index;

    size_t expected_number_of_chunks;
    uint32_t retry_count;

    /* File Management request parameters */
    char file_name[FILE_MANAGEMENT_FILE_NAME_SIZE];
    uint8_t file_hash[FILE_MANAGEMENT_HASH_SIZE];
    size_t file_size;
    /* File Management request parameters */

    /* File Management URL */
    char file_url[FILE_MANAGEMENT_URL_SIZE];
    /* File Management URL */

    /* Listeners */
    file_management_on_status_listener on_status;
    file_management_on_packet_request_listener on_packet_request;
    file_management_on_url_download_status_listener on_url_download_status;
    file_management_on_file_list_listener on_file_list;
    /* Listeners */

    void* wolk_ctx;

    bool has_valid_configuration;
};

bool file_management_init(void* wolk_ctx, file_management_t* file_management, const char* device_key,
                          size_t maximum_file_size, size_t chunk_size, file_management_start_t start,
                          file_management_write_chunk_t write_chunk, file_management_read_chunk_t read_chunk,
                          file_management_abort_t abort, file_management_finalize_t finalize,
                          file_management_start_url_download_t start_url_download,
                          file_management_is_url_download_done_t is_url_download_done,
                          file_management_get_file_list_t get_file_list, file_management_remove_file_t remove_file,
                          file_management_purge_files_t purge_files);

void file_management_handle_parameter(file_management_t* file_management,
                                      file_management_parameter_t* file_management_parameter);

void file_management_handle_packet(file_management_t* file_management, uint8_t* packet, size_t packet_size);

void file_management_handle_abort(file_management_t* file_management, uint8_t* packet, size_t packet_size);

void file_management_handle_url_download(file_management_t* file_management, char* url_download);
void file_management_handle_file_list(file_management_t* file_management);
void file_management_handle_file_delete(file_management_t* file_management, file_list_t* file_list,
                                        size_t number_of_files);
void file_management_handle_file_purge(file_management_t* file_management);

void file_management_process(file_management_t* file_management);

void file_management_set_on_status_listener(file_management_t* file_management,
                                            file_management_on_status_listener on_status);
void file_management_set_on_packet_request_listener(file_management_t* file_management,
                                                    file_management_on_packet_request_listener on_packet_request);
void file_management_set_on_url_download_status_listener(file_management_t* file_management,
                                                         file_management_on_status_listener on_status);
void file_management_set_on_file_list_listener(file_management_t* file_management,
                                               file_management_on_file_list_listener file_list);

#ifdef __cplusplus
}
#endif

#endif
