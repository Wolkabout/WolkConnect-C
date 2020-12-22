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

#include "file_management_parameter.h"
#include "wolk_utils.h"

#include <stddef.h>
#include <stdint.h>
#include <string.h>

void file_management_parameter_init(file_management_parameter_t* parameter)
{
    /* Sanity check */
    WOLK_ASSERT(parameter);

    memset(parameter->file_name, '\0', WOLK_ARRAY_LENGTH(parameter->file_name));
    memset(parameter->file_hash, '\0', WOLK_ARRAY_LENGTH(parameter->file_hash));
    memset(parameter->file_url, '\0', WOLK_ARRAY_LENGTH(parameter->file_url));
    parameter->file_size = 0;
    memset(parameter->file_list, '\0', WOLK_ARRAY_LENGTH(parameter->file_list));
}

const char* file_management_parameter_get_file_name(file_management_parameter_t* parameter)
{
    /* Sanity check */
    WOLK_ASSERT(parameter);

    return parameter->file_name;
}

void file_management_parameter_set_filename(file_management_parameter_t* parameter, const char* file_name)
{
    /* Sanity check */
    WOLK_ASSERT(parameter);
    WOLK_ASSERT(file_name);
    WOLK_ASSERT(strlen(file_name) <= FILE_MANAGEMENT_FILE_NAME_SIZE);

    strncpy(parameter->file_name, file_name, FILE_MANAGEMENT_FILE_NAME_SIZE);
}

size_t file_management_parameter_get_file_size(file_management_parameter_t* parameter)
{
    /* Sanity check */
    WOLK_ASSERT(parameter);

    return parameter->file_size;
}

void file_management_parameter_set_file_size(file_management_parameter_t* parameter, size_t file_size)
{
    /* Sanity check */
    WOLK_ASSERT(parameter);

    parameter->file_size = file_size;
}

const uint8_t* file_management_parameter_get_file_hash(file_management_parameter_t* parameter)
{
    /* Sanity check */
    WOLK_ASSERT(parameter);

    return parameter->file_hash;
}

void file_management_parameter_set_file_hash(file_management_parameter_t* parameter, const uint8_t* hash,
                                             size_t hash_size)
{
    /* Sanity check */
    WOLK_ASSERT(parameter);
    WOLK_ASSERT(hash);
    WOLK_ASSERT(hash_size <= FILE_MANAGEMENT_HASH_SIZE);

    memcpy((void*)parameter->file_hash, (const void*)hash, hash_size);
}

size_t file_management_parameter_get_file_hash_size(file_management_parameter_t* parameter)
{
    /* Sanity check */
    WOLK_ASSERT(parameter);

    return WOLK_ARRAY_LENGTH(parameter->file_hash);
}

const char* file_management_parameter_get_file_url(file_management_parameter_t* parameter)
{
    /* Sanity check */
    WOLK_ASSERT(parameter);

    return parameter->file_url;
}

void file_management_parameter_set_file_url(file_management_parameter_t* parameter, const char* file_url)
{
    /* Sanity check */
    WOLK_ASSERT(parameter);
    WOLK_ASSERT(file_url);
    WOLK_ASSERT(strlen(file_url) <= FILE_MANAGEMENT_URL_SIZE);

    if (strlen(file_url) >= FILE_MANAGEMENT_URL_SIZE) {
        memset(parameter->file_url, '\0', FILE_MANAGEMENT_URL_SIZE);
    }

    strncpy(parameter->file_url, file_url, FILE_MANAGEMENT_URL_SIZE);
}

const char* file_management_parameter_get_result(file_management_parameter_t* parameter)
{
    /* Sanity check */
    WOLK_ASSERT(parameter);

    return parameter->result;
}

void file_management_parameter_set_result(file_management_parameter_t* parameter, const char* result)
{
    /* Sanity check */
    WOLK_ASSERT(parameter);
    WOLK_ASSERT(result);
    WOLK_ASSERT(strlen(file_name) <= FILE_MANAGEMENT_FILE_NAME_SIZE);

    strncpy(parameter->result, result, FILE_MANAGEMENT_FILE_NAME_SIZE);
}
