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


#ifndef WOLKCONNECTOR_C_FIRMWARE_UPDATE_IMPLEMENTATION_H
#define WOLKCONNECTOR_C_FIRMWARE_UPDATE_IMPLEMENTATION_H

#include <stdbool.h>

bool firmware_update_start_installation(const char* file_name);
bool firmware_update_is_installation_completed(bool* success);
bool firmware_update_verification_store(uint8_t parameter);
uint8_t firmware_update_verification_read(void);
bool firmware_update_abort_installation(void);

#endif // WOLKCONNECTOR_C_FIRMWARE_UPDATE_IMPLEMENTATION_H
