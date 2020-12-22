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

#ifndef FILE_MANAGEMENT_IMPLEMENTATION_H
#define FILE_MANAGEMENT_IMPLEMENTATION_H

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#include "wolk_connector.h"

bool file_management_start(const char* file_name, size_t file_size);
bool file_management_chunk_write(uint8_t* data, size_t data_size);
size_t file_management_chunk_read(size_t index, uint8_t* data, size_t data_size);
void file_management_abort(void);
void file_management_finalize(void);
bool file_management_start_url_download(const char* url);
bool file_management_is_url_download_done(bool* success);
int8_t file_management_get_file_list(char* file_list[]);
bool file_management_remove_file(const char* file_name);

#endif // FILE_MANAGEMENT_IMPLEMENTATION_H
