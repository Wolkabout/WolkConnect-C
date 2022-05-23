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
#include <string.h>

#include "data_transmission.h"
#include "utility/wolk_utils.h"

static transmission_io_functions_t* io_functions = NULL;
static unsigned char* starting_add = NULL;
static int number_of_bytes;


int transmission_open(transmission_io_functions_t* trans_io)
{
    io_functions = trans_io;

    return 0;
}

int transmission_get_data_nb(void* socket, unsigned char* buffer, int count)
{
    transmission_io_functions_t* tmp_io = io_functions;
    int length = 0;

    WOLK_ASSERT((tmp_io != NULL) && (tmp_io->recv != NULL));

    if ((length = tmp_io->recv(buffer, count)) >= 0) {
        return length;
    }

    return TRANSPORT_ERROR;
}

void transmission_buffer_nb_start(int socket, unsigned char* buffer, int buffer_length)
{
    WOLK_UNUSED(socket);

    starting_add = buffer;
    number_of_bytes = buffer_length;
}

int transmission_buffer_nb(int socket)
{
    WOLK_UNUSED(socket);

    transmission_io_functions_t* tmp_io = io_functions;
    int length;

    WOLK_ASSERT((tmp_io != NULL) && (tmp_io->send != NULL) && (starting_add != NULL));

    if ((length = tmp_io->send(starting_add, number_of_bytes)) > 0) {
        starting_add += length;

        if ((number_of_bytes -= length) <= 0) {
            return TRANSPORT_DONE;
        }
    } else if (length < 0) {
        return TRANSPORT_ERROR;
    }
    return TRANSPORT_AGAIN;
}

int transmission_buffer(int socket, unsigned char* buffer, int buffer_length)
{
    int response = 0;

    transmission_buffer_nb_start(socket, buffer, buffer_length);
    while ((response = transmission_buffer_nb(socket)) == TRANSPORT_AGAIN) {
    }

    if (response == TRANSPORT_DONE) {
        return buffer_length;
    }

    return TRANSPORT_ERROR;
}