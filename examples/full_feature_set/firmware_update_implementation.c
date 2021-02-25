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
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "firmware_update_implementation.h"
#include "size_definitions.h"

#define FIRMWARE_UPDATE_VERIFICATION_FILE_NAME "firmware_update_verification.txt"
#define FIRMWARE_UPDATE_VERIFICATION_FILE_MODE_READ "r+"
#define FIRMWARE_UPDATE_VERIFICATION_FILE_MODE_WRITE "w+"
#define FIRMWARE_UPDATE_VERIFICATION_FILE_DELIMITER ","
#define FIRMWARE_UPDATE_VERSION_FORMAT_DELIMITER "."
#define FIRMWARE_UPDATE_VERSION_NUMBER_OF_ELEMENTS 3

static int8_t version_major = 0;
static int8_t version_minor = 0;
static int8_t version_patch = 1;

static bool _get_version(char* version);
static bool _set_version(char* version, int8_t* firmware_update_version);
static uint8_t _set_status(char* source);
static bool _file_write(uint8_t firmware_update_installation_status, int8_t* firmware_update_version);
static bool _file_read(uint8_t* firmware_update_installation_status, int8_t* firmware_update_version);


static bool _get_version(char* version)
{
    if (snprintf(version, FIRMWARE_UPDATE_VERSION_SIZE, "%d%s%d%s%d", version_major,
                 FIRMWARE_UPDATE_VERSION_FORMAT_DELIMITER, version_minor, FIRMWARE_UPDATE_VERSION_FORMAT_DELIMITER,
                 version_patch)
        > FIRMWARE_UPDATE_VERSION_SIZE) {
        return false;
    }

    return true;
}

static bool _set_version(char* source, int8_t* version)
{
    /* From source extract version */
    char* version_string = strtok(source, FIRMWARE_UPDATE_VERIFICATION_FILE_DELIMITER);
    version_string = strtok(NULL, FIRMWARE_UPDATE_VERSION_FORMAT_DELIMITER);

    *version = atoi(version_string);
    while (version_string != NULL) {
        version_string = strtok(NULL, FIRMWARE_UPDATE_VERSION_FORMAT_DELIMITER);
        if (version_string != NULL) {
            version++;
            *version = atoi(version_string);
        }
    }

    return true;
}

static uint8_t _set_status(char* source)
{
    /* From source extract status */
    char* element = strtok(source, FIRMWARE_UPDATE_VERIFICATION_FILE_DELIMITER);
    if (element == NULL) {
        return -1;
    }

    uint8_t status = atoi(element);
    if (!status) {
        return -1;
    }

    return status;
}

static bool _file_write(uint8_t status, int8_t* version)
{
    char file_content[FIRMWARE_UPDATE_VERSION_SIZE];
    memset(file_content, '\0', FIRMWARE_UPDATE_VERSION_SIZE);
    int8_t tmp_status, tmp_version[FIRMWARE_UPDATE_VERSION_NUMBER_OF_ELEMENTS];
    tmp_status = status;
    *tmp_version = version;

    if (status == NULL && version == NULL) {
        return false;
    }

    if (status == NULL) {
        _file_read(&tmp_status, NULL);
    }
    if (version == NULL) {
        _file_read(NULL, &tmp_version);
    } else {
        for (int i = 0; i < FIRMWARE_UPDATE_VERSION_NUMBER_OF_ELEMENTS; i++) {
            tmp_version[i] = version[i];
        }
    }

    FILE* file_pointer = fopen(FIRMWARE_UPDATE_VERIFICATION_FILE_NAME, FIRMWARE_UPDATE_VERIFICATION_FILE_MODE_WRITE);
    if (file_pointer == NULL) {
        return false;
    }

    if (tmp_status != NULL) {
        if (snprintf(file_content, FIRMWARE_UPDATE_VERSION_SIZE, "%d", tmp_status) > FIRMWARE_UPDATE_VERSION_SIZE) {
            return false;
        }
    }
    if (tmp_version != NULL) {
        if (snprintf(file_content + strlen(file_content), FIRMWARE_UPDATE_VERSION_SIZE, "%s%d%s%d%s%d",
                     FIRMWARE_UPDATE_VERIFICATION_FILE_DELIMITER, tmp_version[0],
                     FIRMWARE_UPDATE_VERSION_FORMAT_DELIMITER, tmp_version[1], FIRMWARE_UPDATE_VERSION_FORMAT_DELIMITER,
                     tmp_version[2])
            > FIRMWARE_UPDATE_VERSION_SIZE) {
            return false;
        }
    }

    if (fwrite(file_content, 1, strlen(file_content), file_pointer) < strlen(file_content)) {
        return false;
    }
    if (fclose(file_pointer)) {
        return false;
    }

    return true;
}

static bool _file_read(uint8_t* status, int8_t* version)
{
    char file_content[FIRMWARE_UPDATE_VERSION_SIZE];
    memset(file_content, '\0', FIRMWARE_UPDATE_VERSION_SIZE);

    FILE* file_pointer = fopen(FIRMWARE_UPDATE_VERIFICATION_FILE_NAME, FIRMWARE_UPDATE_VERIFICATION_FILE_MODE_READ);
    if (file_pointer == NULL) {
        return false;
    }

    if (fread(file_content, FIRMWARE_UPDATE_VERSION_SIZE, 1, file_pointer) < 0) {
        printf("Firmware Update verification file is empty!\n");
        return false;
    }

    if (version != NULL) {
        _set_version(file_content, version);
    }
    if (status != NULL) {
        *status = _set_status(file_content);
    }

    if (fclose(file_pointer)) {
        return false;
    }

    return true;
}

bool firmware_update_start_installation(const char* file_name)
{
    printf("Starting installation of file with name: %s \n", file_name);

    char* env_argv[] = {NULL};
    char* env_args[] = {NULL};
    char file_name_with_path[FILE_MANAGEMENT_FILE_NAME_SIZE];
    memset(file_name_with_path, '\0', FILE_MANAGEMENT_FILE_NAME_SIZE);

    int8_t firmware_update_version[FIRMWARE_UPDATE_VERSION_NUMBER_OF_ELEMENTS] = {version_major, version_minor,
                                                                                  version_patch};

    if (!_file_write(NULL, &firmware_update_version)) {
        return false;
    }

    snprintf(file_name_with_path, FILE_MANAGEMENT_FILE_NAME_SIZE, "files/%s", file_name);
    if (chmod(file_name_with_path, S_IRUSR | S_IWUSR | S_IXUSR) == -1) {
        return false;
    }

    if (execve(file_name_with_path, env_argv, env_args) == -1) {
        return false;
    }

    return true;
}

bool firmware_update_is_installation_completed(bool* success)
{
    int8_t firmware_update_version[FIRMWARE_UPDATE_VERSION_NUMBER_OF_ELEMENTS];

    if (!_file_read(NULL, &firmware_update_version)) {
        return false;
    }

    if (version_major >= firmware_update_version[0] || version_minor >= firmware_update_version[1]
        || version_patch >= firmware_update_version[2]) {
        *success = true;
        int8_t firmware_update_version[FIRMWARE_UPDATE_VERSION_NUMBER_OF_ELEMENTS] = {version_major, version_minor,
                                                                                      version_patch};

        if (!_file_write(NULL, &firmware_update_version)) {
            return false;
        }
        printf("Installation is completed\n");
    } else {
        *success = false;
        printf("Installation is Failed or Downgrade\n");
    }

    return true;
}

bool firmware_update_verification_store(uint8_t parameter)
{
    return _file_write(parameter, NULL);
}

uint8_t firmware_update_verification_read(void)
{
    uint8_t parameter = 0;

    if (!_file_read(&parameter, NULL)) {
        return false;
    }

    return parameter;
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
