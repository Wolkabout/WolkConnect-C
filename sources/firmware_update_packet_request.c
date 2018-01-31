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

#include "firmware_update_packet_request.h"
#include "wolk_utils.h"

#include <stddef.h>
#include <string.h>

void firmware_update_packet_request_init(firmware_update_packet_request_t* request, const char* file_name,
                                         size_t chunk_index, size_t chunk_size)
{
    /* Sanity check */
    WOLK_ASSERT(request);
    WOLK_ASSERT(file_name);
    WOLK_ASSERT(strlen(file_name) <= FIRMWARE_UPDATE_FILE_NAME_SIZE);

    strcpy(request->file_name, file_name);
    request->chunk_index = chunk_index;
    request->chunk_size = chunk_size;
}

const char* firmware_update_packet_request_get_file_name(firmware_update_packet_request_t* request)
{
    /* Sanity check */
    WOLK_ASSERT(request);

    return request->file_name;
}

size_t firmware_update_packet_request_get_chunk_index(firmware_update_packet_request_t* request)
{
    /* Sanity check */
    WOLK_ASSERT(request);

    return request->chunk_index;
}

size_t firmware_update_packet_request_get_chunk_size(firmware_update_packet_request_t* request)
{
    /* Sanity check */
    WOLK_ASSERT(request);

    return request->chunk_size;
}
