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

#ifndef FILE_MANAGEMENT_PACKET_H
#define FILE_MANAGEMENT_PACKET_H

#include "size_definitions.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

bool file_management_packet_is_valid(uint8_t* packet, size_t packet_size);

uint8_t* file_management_packet_get_hash(uint8_t* packet, size_t packet_size);

uint8_t* file_management_packet_get_previous_packet_hash(uint8_t* packet, size_t packet_size);

uint8_t* file_management_packet_get_data(uint8_t* packet, size_t packet_size);
size_t file_management_packet_get_data_size(uint8_t* packet, size_t packet_size);

#ifdef __cplusplus
}
#endif

#endif
