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

static FILE* file_management_file;
char file_management_file_name[FILE_MANAGEMENT_FILE_NAME_SIZE];
static size_t file_management_file_size = 0;

bool file_management_start(const char* file_name, size_t file_size)
{
    printf("Starting File Management. File name: %s. File size:%zu\n", file_name, file_size);
    file_management_file_size = file_size;

    file_management_file = fopen(file_name, "w+b");

    if (file_management_file == NULL) {
        return false;
    }

    strcpy(file_management_file_name, file_name);
    return true;
}

bool file_management_chunk_write(uint8_t* data, size_t data_size)
{
    printf("File Management chunk write\n");

    const size_t items_written = fwrite(data, data_size, 1, file_management_file);
    fflush(file_management_file);
    return items_written == 1;
}

size_t file_management_chunk_read(size_t index, uint8_t* data, size_t data_size)
{
    printf("File Management chunk read\n");

    fseek(file_management_file, (long)index * (long)data_size, SEEK_SET);

    /* When file size is not multiple of 'data_size' */
    /* last chunk will be less than 'data_size' */
    if (file_management_file_size < (index + 1) * data_size) {
        data_size = file_management_file_size % data_size;
    }
    return fread(data, data_size, 1, file_management_file) == 1 ? data_size : 0;
}

void file_management_abort(void)
{
    printf("Aborting File Management\n");

    if (file_management_file != NULL) {
        fclose(file_management_file);
    }

    remove(file_management_file_name);
}

void file_management_finalize(void)
{
    printf("Finalizing file update\n");

    if (file_management_file != NULL) {
        fclose(file_management_file);
    }

    exit(0);
}

bool file_management_persist_file_version(const char* version)
{
    // TODO: remove firmware version
    //    FILE* firmware_version = fopen(".file_version", "w");
    //    if (firmware_version == NULL) {
    //        return false;
    //    }
    //
    //    if (fputs(version, firmware_version) <= 0) {
    //        fclose(firmware_version);
    //        return false;
    //    }
    //
    //    fclose(firmware_version);
    return true;
}

bool file_management_unpersist_file_version(char* version, size_t version_size)
{
    // TODO: remove firmware version
    //    FILE* firmware_version = fopen(".file_version", "r");
    //    if (firmware_version == NULL) {
    //        return false;
    //    }
    //
    //    if (fgets(version, (int)version_size, firmware_version) == NULL) {
    //        fclose(firmware_version);
    //        return false;
    //    }
    //
    //    fclose(firmware_version);
    //    remove(".firmware_version");
    return true;
}

bool file_management_start_url_download(const char* url)
{
    /* Dummy file downloader */
    printf("Starting file download from url %s\n", url);
    return true;
}

bool file_management_is_url_download_done(bool* success)
{
    *success = true;
    return true;
}
