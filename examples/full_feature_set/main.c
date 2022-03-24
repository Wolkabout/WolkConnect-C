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
#include "utility/wolk_utils.h"
#include "wolk_connector.h"

#include <signal.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include <openssl/ssl.h>

#include "file_management_implementation.h"
#include "firmware_update_implementation.h"

#define DEFAULT_PUBLISH_PERIOD_SECONDS 15

static SSL_CTX* ctx;
static BIO* sockfd;

/* WolkAbout Platform device connection parameters */
static const char* device_key = "device_key";
static const char* device_password = "some_password";
static const char* hostname = "api-demo.wolkabout.com";
static int portno = 8883;
static char certs[] = "../ca.crt";

/* Sample in-memory persistence storage - size 1MB */
static uint8_t persistence_storage[1024 * 1024];

/* WolkConnect-C Connector context */
static wolk_ctx_t wolk;

wolk_numeric_feeds_t temperature_value = {0};
wolk_numeric_feeds_t heartbeat_value = {0};
wolk_boolean_feeds_t switch_value = {0};
void feed_value_handler(wolk_feed_t* feeds, size_t number_of_feeds);
void parameter_value_handler(wolk_parameter_t* parameter_message, size_t number_of_parameters);

/* System dependencies */
static volatile bool keep_running = true;
static void int_handler(int dummy)
{
    WOLK_UNUSED(dummy);

    keep_running = false;
}

static int send_buffer(unsigned char* buffer, unsigned int len)
{
    int n;

    n = (int)BIO_write(sockfd, buffer, (int)len);
    if (n < 0) {
        return -1;
    }

    return n;
}

static int receive_buffer(unsigned char* buffer, unsigned int max_bytes)
{
    bzero(buffer, max_bytes);
    int n;

    n = (int)BIO_read(sockfd, buffer, (int)max_bytes);
    if (n < 0) {
        return -1;
    }

    return n;
}

static void open_socket(BIO** bio, SSL_CTX** ssl_ctx, const char* addr, const int port_number, const char* ca_file,
                        const char* ca_path)
{
    SSL* ssl;
    char port[5];
    snprintf(port, 5, "%d", port_number);

    /* Load OpenSSL */
    SSL_load_error_strings();
    ERR_load_BIO_strings();
    OpenSSL_add_all_algorithms();
    SSL_library_init();

    *ssl_ctx = SSL_CTX_new(SSLv23_client_method());

    /* Load Certificate */
    if (!SSL_CTX_load_verify_locations(*ssl_ctx, ca_file, ca_path)) {
        printf("Wolk client - Error, failed to load certificate\n");
        exit(1);
    }

    /* Open BIO Socket */
    *bio = BIO_new_ssl_connect(*ssl_ctx);
    BIO_get_ssl(*bio, &ssl);
    SSL_set_mode(ssl, SSL_MODE_AUTO_RETRY);
    BIO_set_conn_hostname(*bio, addr);
    BIO_set_nbio(*bio, 1);
    BIO_set_conn_port(*bio, port);

    /* Wait for connection in 15 second timeout */
    int start_time = (int)time(NULL);
    while (BIO_do_connect(*bio) <= 0 && (int)time(NULL) - start_time < 15)
        ;
    if (BIO_do_connect(*bio) <= 0) {
        printf("Wolk client - Error, do connect\n");
        BIO_free_all(*bio);
        SSL_CTX_free(*ssl_ctx);
        *bio = NULL;
        *ssl_ctx = NULL;
        return;
    }

    /* Verify Certificate */
    if (SSL_get_verify_result(ssl) != X509_V_OK) {
        printf("Wolk client - Error, x509 certificate verification failed\n");
        exit(1);
    }
}

void feed_value_handler(wolk_feed_t* feeds, size_t number_of_feeds)
{
    for (int i = 0; i < (int)number_of_feeds; ++i) {
        printf("Received is feed with reference %s and value %s\n", feeds->reference, feeds->data[0]);
        if (strstr(feeds->reference, "SW")) {
            if (strstr(feeds->data[0], "true"))
                switch_value.value = (bool*)true;
            else
                switch_value.value = (bool*)false;
        } else if (strstr(feeds->reference, "HB")) {
            heartbeat_value.value = (double)atol(feeds->data[0]);
        }
        feeds++;
    }
}

void parameter_value_handler(wolk_parameter_t* parameter_message, size_t number_of_parameters)
{
    for (int i = 0; i < (int)number_of_parameters; ++i) {
        printf("Received is parameter with name: %s and value: %s\n", parameter_message->name,
               parameter_message->value);

        parameter_message++;
    }
}

int main(int argc, char* argv[])
{
    WOLK_UNUSED(argc);
    WOLK_UNUSED(argv);

    signal(SIGINT, int_handler);
    signal(SIGTERM, int_handler);

    heartbeat_value.value = DEFAULT_PUBLISH_PERIOD_SECONDS;

    if (strcmp(device_key, "device_key") == 0) {
        printf("Wolk client - Error, device key not provided\n");
        return 1;
    }

    if (strcmp(device_password, "password") == 0) {
        printf("Wolk client - Error, password key not provided\n");
        return 1;
    }

    printf("Wolk client - Establishing connection to WolkAbout IoT platform\n");
    open_socket(&sockfd, &ctx, hostname, portno, certs, NULL);
    if (sockfd == NULL) {
        printf("Wolk client - Error establishing connection to WolkAbout IoT platform\n");
        return 1;
    }

    if (wolk_init(&wolk, send_buffer, receive_buffer, device_key, device_password, PUSH, feed_value_handler,
                  parameter_value_handler, NULL)
        != W_FALSE) {
        printf("Wolk client - Error initializing WolkConnect-C\n");
        return 1;
    }

    if (wolk_init_in_memory_persistence(&wolk, persistence_storage, sizeof(persistence_storage), false) != W_FALSE) {
        printf("Wolk client - Error initializing in-memory persistence\n");
        return 1;
    }

    if (wolk_init_file_management(&wolk, 128 * 1024 * 1024, 1024, file_management_start, file_management_chunk_write,
                                  file_management_chunk_read, file_management_abort, file_management_finalize,
                                  file_management_start_url_download, file_management_is_url_download_done,
                                  file_management_get_file_list, file_management_remove_file,
                                  file_management_purge_files)
        != W_FALSE) {
        printf("Error initializing File Management");
        return 1;
    }

    if (wolk_init_firmware_update(&wolk, firmware_update_start_installation, firmware_update_is_installation_completed,
                                  firmware_update_verification_store, firmware_update_verification_read,
                                  firmware_update_abort_installation)
        != W_FALSE) {
        printf("Error initializing Firmware Update");
        return 1;
    }

    printf("Wolk client - Connecting to server\n");
    if (wolk_connect(&wolk) != W_FALSE) {
        printf("Wolk client - Error connecting to server\n");
        return 1;
    }
    printf("Wolk client - Connected to server\n");

    wolk_sync_time_request(&wolk);
    wolk_publish(&wolk);

    int32_t tick_count = 0;

    while (keep_running) {
        // Sending feeds on heartbeat
        if (tick_count > (heartbeat_value.value) * 100000) {
            printf("Wolk client - Sending feed readings:\n");

            temperature_value.value = rand() % 100 - 20;
            printf("Temperature feed with value: %lf °C\n", temperature_value.value);
            wolk_add_numeric_feed(&wolk, "T", &temperature_value, 1);

            printf("Heartbeat feed with value: %lf seconds\n", heartbeat_value.value);
            wolk_add_numeric_feed(&wolk, "HB", &heartbeat_value, 1);

            printf("Switch feed with value: %s\n", switch_value.value == (bool*)true ? "true" : "false");
            wolk_add_bool_feeds(&wolk, "SW", &switch_value, 1);

            if (wolk_publish(&wolk)) {
                printf("Wolk client - Error publishing feeds!\n");
                return 1;
            }

            tick_count = 0;
        }

        // MANDATORY: sleep(currently 1000us) and number of tick(currently 1) when are multiplied needs to give 1ms.
        // you can change this parameters, but keep it's multiplication
        usleep(10);
        wolk_process(&wolk, 1);

        tick_count++;
    }

    printf("Wolk client - Disconnecting\n");
    wolk_disconnect(&wolk);
    BIO_free_all(sockfd);

    return 0;
}
