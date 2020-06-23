/*
 * Copyright 2020 WolkAbout Technology s.r.o.
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

#include "firmware_implementation.h"

static FILE* firmware_file;
char firmware_file_name[FIRMWARE_UPDATE_FILE_NAME_SIZE];
static size_t firmware_file_size = 0;

bool firmware_update_start(const char* file_name, size_t file_size)
{
    printf("Starting firmware update. File name: %s. File size:%zu\n", file_name, file_size);
    firmware_file_size = file_size;

    firmware_file = fopen(file_name, "w+b");

    if (firmware_file == NULL) {
        return false;
    }

    strcpy(firmware_file_name, file_name);
    return true;
}

bool firmware_chunk_write(uint8_t* data, size_t data_size)
{
    printf("Firmware update chunk write\n");

    const size_t items_written = fwrite(data, data_size, 1, firmware_file);
    fflush(firmware_file);
    return items_written == 1;
}

size_t firmware_chunk_read(size_t index, uint8_t* data, size_t data_size)
{
    printf("Firmware update chunk read\n");

    fseek(firmware_file, (long)index * (long)data_size, SEEK_SET);

    /* When firmware size is not multiple of 'data_size' */
    /* last chunk will be less than 'data_size' */
    if (firmware_file_size < (index + 1) * data_size) {
        data_size = firmware_file_size % data_size;
    }
    return fread(data, data_size, 1, firmware_file) == 1 ? data_size : 0;
}

void firmware_update_abort(void)
{
    printf("Aborting firmware update\n");

    if (firmware_file != NULL) {
        fclose(firmware_file);
    }

    remove(firmware_file_name);
}

void firmware_update_finalize(void)
{
    printf("Finalizing firmware update\n");

    if (firmware_file != NULL) {
        fclose(firmware_file);
    }

    exit(0);
}

bool firmware_update_persist_firmware_version(const char* version)
{
    FILE* firmware_version = fopen(".firmware_version", "w");
    if (firmware_version == NULL) {
        return false;
    }

    if (fputs(version, firmware_version) <= 0) {
        fclose(firmware_version);
        return false;
    }

    fclose(firmware_version);
    return true;
}

bool firmware_update_unpersist_firmware_version(char* version, size_t version_size)
{
    FILE* firmware_version = fopen(".firmware_version", "r");
    if (firmware_version == NULL) {
        return false;
    }

    if (fgets(version, (int)version_size, firmware_version) == NULL) {
        fclose(firmware_version);
        return false;
    }

    fclose(firmware_version);
    remove(".firmware_version");
    return true;
}

bool firmware_update_start_url_download(const char* url)
{
    /* Dummy firmware downloader */
    printf("Starting firmware download from url %s\n", url);
    return true;
}

bool firmware_update_is_url_download_done(bool* success)
{
    *success = true;
    return true;
}
