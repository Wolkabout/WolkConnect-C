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

#ifndef FILE_MANAGEMENT_PARAMETER_H
#define FILE_MANAGEMENT_PARAMETER_H

#include "size_definitions.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    char file_name[FILE_MANAGEMENT_FILE_NAME_SIZE];

    size_t file_size;

    uint8_t file_hash[FILE_MANAGEMENT_HASH_SIZE];

    char file_url[FILE_MANAGEMENT_URL_SIZE];

    char file_list[FILE_MANAGEMENT_FILE_LIST_SIZE];
} file_management_parameter_t;

void file_management_parameter_init(file_management_parameter_t* parameter);

const char* file_management_parameter_get_file_name(file_management_parameter_t* parameter);
void file_management_parameter_set_filename(file_management_parameter_t* parameter, const char* file_name);

size_t file_management_parameter_get_file_size(file_management_parameter_t* parameter);
void file_management_parameter_set_file_size(file_management_parameter_t* parameter, size_t file_size);

const uint8_t* file_management_parameter_get_file_hash(file_management_parameter_t* parameter);
void file_management_parameter_set_file_hash(file_management_parameter_t* parameter, const uint8_t* hash,
                                             size_t hash_size);
size_t file_management_parameter_get_file_hash_size(file_management_parameter_t* parameter);

const char* file_management_parameter_get_file_url(file_management_parameter_t* parameter);
void file_management_parameter_set_file_url(file_management_parameter_t* parameter, const char* file_url);

#ifdef __cplusplus
}
#endif

#endif
