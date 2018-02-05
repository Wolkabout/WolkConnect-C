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

#include "firmware_update_command.h"
#include "wolk_utils.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

void firmware_update_command_init(firmware_update_command_t* command)
{
    /* Sanity check */
    WOLK_ASSERT(command);

    command->type = FIRMWARE_UPDATE_COMMAND_TYPE_UNKNOWN;

    memset(command->file_name, '\0', WOLK_ARRAY_LENGTH(command->file_name));
    memset(command->file_hash, '\0', WOLK_ARRAY_LENGTH(command->file_hash));
    memset(command->file_url, '\0', WOLK_ARRAY_LENGTH(command->file_url));
    command->file_size = 0;
    command->auto_install = false;
}

firmware_update_command_type_t firmware_update_command_get_type(firmware_update_command_t* command)
{
    /* Sanity check */
    WOLK_ASSERT(command);

    return command->type;
}

void firmware_update_command_set_type(firmware_update_command_t* command, firmware_update_command_type_t type)
{
    /* Sanity check */
    WOLK_ASSERT(command);

    command->type = type;
}

const char* firmware_update_command_get_file_name(firmware_update_command_t* command)
{
    /* Sanity check */
    WOLK_ASSERT(command);

    return command->file_name;
}

void firmware_update_command_set_filename(firmware_update_command_t* command, const char* file_name)
{
    /* Sanity check */
    WOLK_ASSERT(command);
    WOLK_ASSERT(file_name);
    WOLK_ASSERT(strlen(file_name) <= FIRMWARE_UPDATE_FILE_NAME_SIZE);

    strncpy(command->file_name, file_name, FIRMWARE_UPDATE_FILE_NAME_SIZE);
}

size_t firmware_update_command_get_file_size(firmware_update_command_t* command)
{
    /* Sanity check */
    WOLK_ASSERT(command);

    return command->file_size;
}

void firmware_update_command_set_file_size(firmware_update_command_t* command, size_t file_size)
{
    /* Sanity check */
    WOLK_ASSERT(command);

    command->file_size = file_size;
}

const uint8_t* firmware_update_command_get_file_hash(firmware_update_command_t* command)
{
    /* Sanity check */
    WOLK_ASSERT(command);

    return command->file_hash;
}

void firmware_update_command_set_file_hash(firmware_update_command_t* command, const uint8_t* hash, size_t hash_size)
{
    /* Sanity check */
    WOLK_ASSERT(command);
    WOLK_ASSERT(hash);
    WOLK_ASSERT(hash_size <= FIRMWARE_UPDATE_HASH_SIZE);

    memcpy((void*)command->file_hash, (const void*)hash, hash_size);
}

size_t firmware_update_command_get_file_hash_size(firmware_update_command_t* command)
{
    /* Sanity check */
    WOLK_ASSERT(command);

    return WOLK_ARRAY_LENGTH(command->file_hash);
}

const char* firmware_update_command_get_file_url(firmware_update_command_t* command)
{
    /* Sanity check */
    WOLK_ASSERT(command);

    return command->file_url;
}

void firmware_update_command_set_file_url(firmware_update_command_t* command, const char* file_url)
{
    /* Sanity check */
    WOLK_ASSERT(command);
    WOLK_ASSERT(file_url);
    WOLK_ASSERT(strlen(file_url) <= FIRMWARE_UPDATE_URL_SIZE);

    strncpy(command->file_url, file_url, FIRMWARE_UPDATE_URL_SIZE);
}

bool firmware_update_command_get_auto_install(firmware_update_command_t* command)
{
    /* Sanity check */
    WOLK_ASSERT(command);

    return command->auto_install;
}

void firmware_update_command_set_auto_install(firmware_update_command_t* command, bool auto_install)
{
    /* Sanity check */
    WOLK_ASSERT(command);

    command->auto_install = auto_install;
}
