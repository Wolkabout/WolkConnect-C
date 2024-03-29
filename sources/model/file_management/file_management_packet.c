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

#include "file_management_packet.h"
#include "size_definitions.h"
#include "utility/sha256.h"
#include "utility/wolk_utils.h"

#include <stdbool.h>
#include <stddef.h>
#include <string.h>

/*  File Management packet:               */
/*  32 bytes   - Previous packet SHA-256  */
/*  N  byte(s) - Data                     */
/*  32 bytes   - Packet SHA-256           */

bool file_management_packet_is_valid(uint8_t* packet, size_t packet_size)
{
    /* Sanity check */
    WOLK_ASSERT(packet);

    if (packet_size <= 2 * FILE_MANAGEMENT_HASH_SIZE) {
        return false;
    }

    uint8_t* received_sha256 = packet + packet_size - FILE_MANAGEMENT_HASH_SIZE;

    const uint8_t* data = packet + FILE_MANAGEMENT_HASH_SIZE;
    const size_t data_size = packet_size - (2 * FILE_MANAGEMENT_HASH_SIZE);

    uint8_t calculated_sha256[FILE_MANAGEMENT_HASH_SIZE];
    sha256(data, data_size, &calculated_sha256[0]);

    return memcmp(received_sha256, &calculated_sha256, FILE_MANAGEMENT_HASH_SIZE) == 0;
}

uint8_t* file_management_packet_get_hash(uint8_t* packet, size_t packet_size)
{
    /* Sanity check */
    WOLK_ASSERT(packet);
    WOLK_ASSERT(packet_size > 2 * FILE_MANAGEMENT_HASH_SIZE);

    return packet + packet_size - FILE_MANAGEMENT_HASH_SIZE;
}

uint8_t* file_management_packet_get_previous_packet_hash(uint8_t* packet, size_t packet_size)
{
    /* Sanity check */
    WOLK_ASSERT(packet);
    WOLK_ASSERT(packet_size > 2 * FILE_MANAGEMENT_HASH_SIZE);
    WOLK_UNUSED(packet_size);

    // previous sha256 hash is located at the first 32 bytes
    return packet;
}

uint8_t* file_management_packet_get_data(uint8_t* packet, size_t packet_size)
{
    /* Sanity check */
    WOLK_ASSERT(packet);
    WOLK_ASSERT(packet_size > 2 * FILE_MANAGEMENT_HASH_SIZE);

    WOLK_UNUSED(packet_size);

    return packet + FILE_MANAGEMENT_HASH_SIZE;
}

size_t file_management_packet_get_data_size(uint8_t* packet, size_t packet_size)
{
    /* Sanity check */
    WOLK_ASSERT(packet);
    WOLK_ASSERT(packet_size > 2 * FILE_MANAGEMENT_HASH_SIZE);

    WOLK_UNUSED(packet);

    return packet_size - (2 * FILE_MANAGEMENT_HASH_SIZE);
}
