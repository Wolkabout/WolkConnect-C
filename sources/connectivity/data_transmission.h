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
#ifndef WOLKCONNECTOR_C_DATA_TRANSMISSION_H
#define WOLKCONNECTOR_C_DATA_TRANSMISSION_H

typedef struct {
    int (*send)(unsigned char* address, unsigned int bytes);
    int (*recv)(unsigned char* address, unsigned int max_bytes_number);
} transmission_io_functions_t;

enum { TRANSPORT_ERROR = -1, TRANSPORT_AGAIN = 0, TRANSPORT_DONE = 1 };

int transmission_open(transmission_io_functions_t* trans_io);

int transmission_get_data_nb(void* socket, unsigned char* buffer, int count);

void transmission_buffer_nb_start(int socket, unsigned char* buffer, int buffer_length);
int transmission_buffer_nb(int socket);
int transmission_buffer(int socket, unsigned char* buffer, int buffer_length);

#endif
