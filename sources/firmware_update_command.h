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
    FIRMWARE_UPDATE_COMMAND_TYPE_FILE_UPLOAD = 0,
    FIRMWARE_UPDATE_COMMAND_TYPE_URL_DOWNLOAD,
    FIRMWARE_UPDATE_COMMAND_TYPE_INSTALL,
    FIRMWARE_UPDATE_COMMAND_TYPE_ABORT,

    FIRMWARE_UPDATE_COMMAND_TYPE_UNKNOWN
} firmware_update_command_type_t;

typedef struct {
    firmware_update_command_type_t type;

    char file_name[FIRMWARE_UPDATE_FILE_NAME_SIZE];

    size_t file_size;

    uint8_t file_hash[FIRMWARE_UPDATE_HASH_SIZE];

    char file_url[FIRMWARE_UPDATE_URL_SIZE];

    bool auto_install;
} firmware_update_command_t;

void firmware_update_command_init(firmware_update_command_t* command);

firmware_update_command_type_t firmware_update_command_get_type(firmware_update_command_t* command);
void firmware_update_command_set_type(firmware_update_command_t* command, firmware_update_command_type_t type);

const char* firmware_update_command_get_file_name(firmware_update_command_t* command);
void firmware_update_command_set_filename(firmware_update_command_t* command, const char* file_name);

size_t firmware_update_command_get_file_size(firmware_update_command_t* command);
void firmware_update_command_set_file_size(firmware_update_command_t* command, size_t file_size);

const uint8_t* firmware_update_command_get_file_hash(firmware_update_command_t* command);
void firmware_update_command_set_file_hash(firmware_update_command_t* command, const uint8_t* hash, size_t hash_size);
size_t firmware_update_command_get_file_hash_size(firmware_update_command_t* command);

const char* firmware_update_command_get_file_url(firmware_update_command_t* command);
void firmware_update_command_set_file_url(firmware_update_command_t* command, const char* file_url);

bool firmware_update_command_get_auto_install(firmware_update_command_t* command);
void firmware_update_command_set_auto_install(firmware_update_command_t* command, bool auto_install);

#ifdef __cplusplus
}
#endif

#endif
