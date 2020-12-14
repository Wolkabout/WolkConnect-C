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

#ifndef FIRMWARE_UPDATE_COMMAND_H
#define FIRMWARE_UPDATE_COMMAND_H

#include "size_definitions.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    FILE_MANAGEMENT_COMMAND_TYPE_FILE_UPLOAD = 0,
    FILE_MANAGEMENT_COMMAND_TYPE_URL_DOWNLOAD,
    FILE_MANAGEMENT_COMMAND_TYPE_INSTALL,
    FILE_MANAGEMENT_COMMAND_TYPE_ABORT,

    FILE_MANAGEMENT_COMMAND_TYPE_UNKNOWN
} file_management_command_type_t;

typedef struct {
    file_management_command_type_t type;

    char file_name[FILE_MANAGEMENT_FILE_NAME_SIZE];

    size_t file_size;

    uint8_t file_hash[FILE_MANAGEMENT_HASH_SIZE];

    char file_url[FILE_MANAGEMENT_URL_SIZE];
} file_management_command_t;

void file_management_command_init(file_management_command_t* command);

file_management_command_type_t file_management_command_get_type(file_management_command_t* command);
void file_management_command_set_type(file_management_command_t* command, file_management_command_type_t type);

const char* file_management_command_get_file_name(file_management_command_t* command);
void file_management_command_set_filename(file_management_command_t* command, const char* file_name);

size_t file_management_command_get_file_size(file_management_command_t* command);
void file_management_command_set_file_size(file_management_command_t* command, size_t file_size);

const uint8_t* file_management_command_get_file_hash(file_management_command_t* command);
void file_management_command_set_file_hash(file_management_command_t* command, const uint8_t* hash, size_t hash_size);
size_t file_management_command_get_file_hash_size(file_management_command_t* command);

const char* file_management_command_get_file_url(file_management_command_t* command);
void file_management_command_set_file_url(file_management_command_t* command, const char* file_url);

#ifdef __cplusplus
}
#endif

#endif
