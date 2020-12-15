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

#include "file_management_command.h"
#include "wolk_utils.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

void file_management_command_init(file_management_command_t* command)
{
    /* Sanity check */
    WOLK_ASSERT(command);

    command->type = FILE_MANAGEMENT_COMMAND_TYPE_UNKNOWN;

    memset(command->file_name, '\0', WOLK_ARRAY_LENGTH(command->file_name));
    memset(command->file_hash, '\0', WOLK_ARRAY_LENGTH(command->file_hash));
    memset(command->file_url, '\0', WOLK_ARRAY_LENGTH(command->file_url));
    command->file_size = 0;
}

file_management_command_type_t file_management_command_get_type(file_management_command_t* command)
{
    /* Sanity check */
    WOLK_ASSERT(command);

    return command->type;
}

void file_management_command_set_type(file_management_command_t* command, file_management_command_type_t type)
{
    /* Sanity check */
    WOLK_ASSERT(command);

    command->type = type;
}

const char* file_management_command_get_file_name(file_management_command_t* command)
{
    /* Sanity check */
    WOLK_ASSERT(command);

    return command->file_name;
}

void file_management_command_set_filename(file_management_command_t* command, const char* file_name)
{
    /* Sanity check */
    WOLK_ASSERT(command);
    WOLK_ASSERT(file_name);
    WOLK_ASSERT(strlen(file_name) <= FILE_MANAGEMENT_FILE_NAME_SIZE);

    strncpy(command->file_name, file_name, FILE_MANAGEMENT_FILE_NAME_SIZE);
}

size_t file_management_command_get_file_size(file_management_command_t* command)
{
    /* Sanity check */
    WOLK_ASSERT(command);

    return command->file_size;
}

void file_management_command_set_file_size(file_management_command_t* command, size_t file_size)
{
    /* Sanity check */
    WOLK_ASSERT(command);

    command->file_size = file_size;
}

const uint8_t* file_management_command_get_file_hash(file_management_command_t* command)
{
    /* Sanity check */
    WOLK_ASSERT(command);

    return command->file_hash;
}

void file_management_command_set_file_hash(file_management_command_t* command, const uint8_t* hash, size_t hash_size)
{
    /* Sanity check */
    WOLK_ASSERT(command);
    WOLK_ASSERT(hash);
    WOLK_ASSERT(hash_size <= FILE_MANAGEMENT_HASH_SIZE);

    memcpy((void*)command->file_hash, (const void*)hash, hash_size);
}

size_t file_management_command_get_file_hash_size(file_management_command_t* command)
{
    /* Sanity check */
    WOLK_ASSERT(command);

    return WOLK_ARRAY_LENGTH(command->file_hash);
}

const char* file_management_command_get_file_url(file_management_command_t* command)
{
    /* Sanity check */
    WOLK_ASSERT(command);

    return command->file_url;
}

void file_management_command_set_file_url(file_management_command_t* command, const char* file_url)
{
    /* Sanity check */
    WOLK_ASSERT(command);
    WOLK_ASSERT(file_url);
    WOLK_ASSERT(strlen(file_url) <= FILE_MANAGEMENT_URL_SIZE);

    strncpy(command->file_url, file_url, FILE_MANAGEMENT_URL_SIZE);
}