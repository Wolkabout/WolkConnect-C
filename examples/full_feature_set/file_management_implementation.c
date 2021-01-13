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

#include "file_management_implementation.h"

#include <dirent.h>
#include <stdio.h>

enum { DIRECTORY_NAME_SIZE = 6 };

static FILE* file_management_file;
static char* directory_name = "files/";
char file_management_file_name[FILE_MANAGEMENT_FILE_NAME_SIZE + DIRECTORY_NAME_SIZE];
static size_t file_management_file_size = 0;
static char current_file_list[FILE_MANAGEMENT_FILE_LIST_SIZE][FILE_MANAGEMENT_FILE_NAME_SIZE] = {0};
static size_t file_management_current_number_of_files = 0;

static size_t get_downloaded_file_position_in_list(char* first_file_list, size_t first_file_list_number_of_files,
                                                   char* second_file_list, size_t second_file_list_number_of_files)
{
    int position_in_list = 0;
    for (int i = 0; i < (first_file_list_number_of_files); i++) {
        bool found_match = false;

        for (int y = 0; y < second_file_list_number_of_files; y++) {
            if (strcmp(&first_file_list[i * FILE_MANAGEMENT_FILE_NAME_SIZE],
                       &second_file_list[y * FILE_MANAGEMENT_FILE_NAME_SIZE])
                == 0) {
                found_match = true;
                break;
            }

            if (y == (second_file_list_number_of_files - 1) && !found_match)
                position_in_list = i;
        }
    }

    return position_in_list;
}

bool file_management_start(const char* file_name, size_t file_size)
{
    printf("Starting File Management. File name: %s. File size:%zu\n", file_name, file_size);
    memset(file_management_file_name, '\0', FILE_MANAGEMENT_FILE_NAME_SIZE + DIRECTORY_NAME_SIZE);
    file_management_file_size = file_size;

    struct stat st = {0};

    if (stat(directory_name, &st) == -1) {
        if (mkdir(directory_name, 0777) == -1) {
            return false;
        }
    }

    if (snprintf(file_management_file_name, FILE_MANAGEMENT_FILE_NAME_SIZE + DIRECTORY_NAME_SIZE, "%s%s",
                 directory_name, file_name)
        >= (int)(FILE_MANAGEMENT_FILE_NAME_SIZE + DIRECTORY_NAME_SIZE)) {
        return false;
    }

    file_management_file = fopen(file_management_file_name, "w+b");
    if (file_management_file == NULL) {
        return false;
    }

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
    printf("Aborting File Management\nFile with name: %s\n", file_management_file_name);

    if (file_management_file != NULL && strlen(file_management_file_name) != 0) {
        fclose(file_management_file);
    }

    if (remove(file_management_file_name) != 0) {
        printf("File can't be removed for FS\n");
    }
}

void file_management_finalize(void)
{
    printf("Finalizing file update\n");

    if (file_management_file != NULL) {
        fclose(file_management_file);
    }
}

bool file_management_start_url_download(const char* url)
{
    /* Dummy file downloader */
    printf("Starting file download from url %s\n", url);
    /* Get file list before download from URL */
    memset(current_file_list, '\0', FILE_MANAGEMENT_FILE_NAME_SIZE);
    file_management_current_number_of_files = file_management_get_file_list(current_file_list);

    /* Download from URL */
    /* NOTE: system() function works only on UNIX systems, for bare-metal implementation this is irrelevant */
    int8_t WGET_COMMAND_LENGTH = 9;
    char web_address[FILE_MANAGEMENT_URL_SIZE + DIRECTORY_NAME_SIZE + WGET_COMMAND_LENGTH];
    if (snprintf(web_address, FILE_MANAGEMENT_URL_SIZE + DIRECTORY_NAME_SIZE + WGET_COMMAND_LENGTH, "wget -P files/ %s",
                 url)
        >= (int)(FILE_MANAGEMENT_URL_SIZE + DIRECTORY_NAME_SIZE + WGET_COMMAND_LENGTH)) {
        return false;
    }

    if (system(web_address) == -1) {
        return false;
    }

    return true;
}

bool file_management_is_url_download_done(bool* success, char* downloaded_file_name)
{
    /* Get file list after download from URL */
    *success = false;
    memset(file_management_file_name, '\0', FILE_MANAGEMENT_FILE_NAME_SIZE + DIRECTORY_NAME_SIZE);
    char after_url_download_file_list[FILE_MANAGEMENT_FILE_NAME_SIZE][FILE_MANAGEMENT_FILE_NAME_SIZE] = {0};
    size_t file_management_after_url_download_number_of_files =
        file_management_get_file_list(after_url_download_file_list);

    if (file_management_after_url_download_number_of_files - file_management_current_number_of_files < 1) {
        return false;
    }

    /* Set downloaded file name to current file name */
    size_t downloaded_file_position_in_list = get_downloaded_file_position_in_list(
        after_url_download_file_list, file_management_after_url_download_number_of_files, current_file_list,
        file_management_current_number_of_files);
    if (snprintf(file_management_file_name, FILE_MANAGEMENT_FILE_NAME_SIZE + DIRECTORY_NAME_SIZE, "%s%s",
                 directory_name, after_url_download_file_list[downloaded_file_position_in_list])
        >= (int)(FILE_MANAGEMENT_FILE_NAME_SIZE + DIRECTORY_NAME_SIZE)) {
        return false;
    }

    strncpy(downloaded_file_name, after_url_download_file_list[downloaded_file_position_in_list],
            strlen(after_url_download_file_list[downloaded_file_position_in_list]));

    printf("File download from url done.\nDownloaded file name is: %s\n", downloaded_file_name);

    *success = true;
    return true;
}

size_t file_management_get_file_list(char* file_list)
{
    struct dirent* de;

    DIR* dr = opendir(directory_name);
    if (dr == NULL) {
        printf("Could not open current directory");
        return false;
    }

    size_t file_list_items = 0;
    while ((de = readdir(dr)) != NULL) {
        if (!strcmp(de->d_name, ".") || !strcmp(de->d_name, "..")) { // Ignore unix hidden files
            continue;
        } else {
            strncpy(file_list + (file_list_items * FILE_MANAGEMENT_FILE_NAME_SIZE), de->d_name, strlen(de->d_name));
            file_list_items++;
        }
    }

    closedir(dr);
    return file_list_items;
}

bool file_management_remove_file(const char* file_name)
{
    memset(file_management_file_name, '\0', FILE_MANAGEMENT_FILE_NAME_SIZE + DIRECTORY_NAME_SIZE);

    if (snprintf(file_management_file_name, FILE_MANAGEMENT_FILE_NAME_SIZE + DIRECTORY_NAME_SIZE, "%s%s",
                 directory_name, file_name)
        >= (int)(FILE_MANAGEMENT_FILE_NAME_SIZE + DIRECTORY_NAME_SIZE)) {
        return false;
    }

    return remove(file_management_file_name) == 0 ? true : false;
}

bool file_management_purge_files(void)
{
    struct dirent* de;

    DIR* dr = opendir(directory_name);
    if (dr == NULL) {
        printf("Could not open current directory");
        return false;
    }

    while ((de = readdir(dr)) != NULL) {
        if (!strcmp(de->d_name, ".") || !strcmp(de->d_name, "..")) { // Ignore unix hidden files
            continue;
        } else {
            if (snprintf(file_management_file_name, FILE_MANAGEMENT_FILE_NAME_SIZE + DIRECTORY_NAME_SIZE, "%s%s",
                         directory_name, de->d_name)
                >= (int)(FILE_MANAGEMENT_FILE_NAME_SIZE + DIRECTORY_NAME_SIZE)) {
                return false;
            }

            if (remove(file_management_file_name) != 0) {
                return false;
            }
        }
    }

    closedir(dr);
    return true;
}
