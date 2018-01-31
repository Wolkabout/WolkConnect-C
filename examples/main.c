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

#include "wolk_connector.h"
#include "wolk_utils.h"

#include <time.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <memory.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <signal.h>

static int sockfd;
static const char *device_key = "device_key";
static const char *device_password = "some_password";
static const char *hostname = "api-demo.wolkabout.com";
static int portno = 1883;

static const char* actuator_references[] = {"SW", "SL"};
static const uint32_t num_actuator_references = 2;

static uint8_t persistence_storage[1024*1024];

static wolk_ctx_t wolk;

static volatile bool keep_running = true;

static void int_handler(int dummy)
{
    WOLK_UNUSED(dummy);

    keep_running = false;
}

static int send_buffer(unsigned char* buffer, unsigned int len)
{
    int n;

    n = (int)write(sockfd, buffer, len);
    if (n < 0) {
        return -1;
    }

    return n;
}

static int receive_buffer(unsigned char* buffer, unsigned int max_bytes)
{
    bzero(buffer, max_bytes);
    int n;

    n = (int)read(sockfd, buffer, max_bytes);
    if (n < 0) {
        return -1;
    }

    return n;
}

static void actuation_handler(const char* reference, const char* value)
{
    printf("Actuation handler - Reference: %s Value: %s\n", reference, value);
}

static actuator_status_t actuator_status_provider(const char* reference)
{
    printf("Actuator status provider - Reference: %s\n", reference);

    actuator_status_t actuator_status;
    actuator_status_init(&actuator_status, "", ACTUATOR_STATE_ERROR);

    if (strcmp(reference, "SW") == 0) {
        actuator_status_init(&actuator_status, "true", ACTUATOR_STATE_READY);
    }
    else if (strcmp(reference, "SL") == 0) {
        actuator_status_init(&actuator_status, "88", ACTUATOR_STATE_READY);
    }

    return actuator_status;
}

static int open_socket(void)
{
    struct sockaddr_in serveraddr;
    struct hostent *server;

    /* socket: create the socket */
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        printf("ERROR opening socket\n");
        return -1;
    }

    /* gethostbyname: get the server's DNS entry */
    server = gethostbyname(hostname);
    if (server == NULL) {
        fprintf(stderr,"ERROR, no such host as %s\n", hostname);
        return -1;
    }

    struct timeval tv;
    tv.tv_sec = 0;
    tv.tv_usec = 100;
    setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv,sizeof(struct timeval));

    /* build the server's Internet address */
    bzero((char *) &serveraddr, sizeof(serveraddr));
    serveraddr.sin_family = AF_INET;
    bcopy((char *)server->h_addr,
          (char *)&serveraddr.sin_addr.s_addr, server->h_length);
    serveraddr.sin_port = htons(portno);

    /* connect: create a connection with the server */
    if (connect(sockfd, (const struct sockaddr*)&serveraddr, sizeof(serveraddr)) < 0) {
        printf("ERROR connecting\n");
        return -1;
    }

    return 0;
}

static FILE* firmware_file;
static size_t firmware_file_size = 0;
static bool firmware_update_start(const char* file_name, size_t file_size)
{
    printf("Starting firmware update. File name: %s. File size:%zu\n", file_name, file_size);
    firmware_file_size = file_size;

    firmware_file = fopen(file_name, "w+b");

    if (firmware_file == NULL) {
        return false;
    }

    return true;
}

static bool firmware_chunk_write(uint8_t* data, size_t data_size)
{
    printf("Firmware update chunk write\n");

    const size_t items_written = fwrite(data, data_size, 1, firmware_file);
    fflush(firmware_file);
    return  items_written == 1;
}

static size_t firmware_chunk_read(size_t index, uint8_t* data, size_t data_size)
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

static void firmware_update_abort(void)
{
    printf("Aborting firmware update\n");

    if (firmware_file != NULL) {
        fclose(firmware_file);
    }
}

static void firmware_update_finalize(void)
{
    printf("Finalizing firmware update\n");

    if (firmware_file != NULL) {
        fclose(firmware_file);
    }

    exit(0);
}

static bool firmware_update_persist_firmware_version(const char* version)
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

static bool firmware_update_unpersist_firmware_version(char* version, size_t version_size)
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

static bool firmware_update_start_url_download(const char* url)
{
    /* Dummy firmware downloader */
    printf("Starting dirmware download from url %s\n", url);
    return true;
}

static bool firmware_update_is_url_download_done(bool* success)
{
    *success = true;
    return true;
}

int main(int argc, char *argv[])
{
    WOLK_UNUSED(argc);
    WOLK_UNUSED(argv);

    signal(SIGINT, int_handler);

    if (strcmp(device_key, "device_key") == 0) {
        printf ("Wolk client - Error, device key not provided\n");
        return 1;
    }

    if (strcmp(device_password, "password") == 0) {
        printf ("Wolk client - Error, password key not provided\n");
        return 1;
    }

    printf ("Wolk client - Establishing connection to WolkAbout IoT platform\n");
    if (open_socket() != 0) {
        printf ("Wolk client - Error establishing connection to WolkAbout IoT platform\n");
        return 1;
    }

    if (wolk_init(&wolk,
                  send_buffer, receive_buffer,
                  actuation_handler, actuator_status_provider,
                  device_key, device_password,
                  PROTOCOL_JSON_SINGLE,
                  actuator_references, num_actuator_references) != W_FALSE) {
        printf("Error initializing WolkConnect-C\n");
        return 1;
    }

    if (wolk_init_in_memory_persistence(&wolk, persistence_storage, sizeof(persistence_storage), false) != W_FALSE) {
        printf("Error initializing in-memory persistence\n");
        return 1;
    }

    if (wolk_init_firmware_update(&wolk, "1.0.0", 128 * 1024 * 1024, 1024,
                                  firmware_update_start,
                                  firmware_chunk_write, firmware_chunk_read,
                                  firmware_update_abort,
                                  firmware_update_finalize,
                                  firmware_update_persist_firmware_version,
                                  firmware_update_unpersist_firmware_version,
                                  firmware_update_start_url_download,
                                  firmware_update_is_url_download_done) != W_FALSE) {
        printf("Error initializing firmware update");
        return 1;
    }

    printf ("Wolk client - Connecting to server\n");
    if (wolk_connect(&wolk) != W_FALSE) {
        printf ("Wolk client - Error connecting to server\n");
        return -1;
    }
    printf ("Wolk client - Connected to server\n");

    wolk_add_alarm(&wolk, "MA", "High humidity", 0);
    wolk_publish(&wolk);

    wolk_add_numeric_sensor_reading(&wolk, "T", 25.6, 0);
    wolk_add_string_sensor_reading(&wolk, "V", "1.0.0", 0);
    wolk_publish(&wolk);

    while (keep_running) {
        wolk_process(&wolk);
        usleep(1000);
    }

    printf("Wolk client - Diconnecting\n");

    wolk_disconnect(&wolk);

    close(sockfd);

    return 0;
}


