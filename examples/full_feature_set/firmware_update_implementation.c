/*
 * Copyright 2021 WolkAbout Technology s.r.o.
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

#include <stdint.h>
#include <stdio.h>

#include "firmware_update_implementation.h"
#include "size_definitions.h"

enum { VERSION_MAJOR = 1, VERSION_MINOR = 0, VERSION_PATCH = 0 };
static int8_t version_major = 0;
static int8_t version_minor = 0;
static int8_t version_patch = 0;

static bool _get_version(char* version)
{
    if (snprintf(version, FIRMWARE_UPDATE_VERSION_SIZE, "v%d.%d.%d", version_major, version_minor, version_patch)
        > FIRMWARE_UPDATE_VERSION_SIZE) {
        return false;
    }

    return true;
}

bool firmware_update_start_installation(const char* file_name)
{
    printf("Starting installation of file with name: %s \n", file_name);

    return true;
}

bool firmware_update_is_installation_completed(bool* success)
{
    printf("Installation is completed\n");

    version_patch++;
    *success = true;
    return true;
}

bool firmware_update_get_version(const char* version)
{
    if (!_get_version(version)) {
        return false;
    }
    printf("Current Firmware Version is: %s\n", version);

    return true;
}

bool firmware_update_abort_installation(void)
{
    printf("Trying to abort current installation\n");

    return true;
}
