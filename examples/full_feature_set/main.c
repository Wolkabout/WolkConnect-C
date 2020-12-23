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
#include "wolk_connector.h"
#include "wolk_utils.h"

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
#include "sensor_readings.h"

#define DEFAULT_PUBLISH_PERIOD_SECONDS 15

static SSL_CTX* ctx;
static BIO* sockfd;

/* WolkAbout Platform device connection parameters */
static const char* device_key = "some_key";
static const char* device_password = "some_password";
static const char* hostname = "api-demo.wolkabout.com";
static int portno = 8883;
static char certs[] = "../ca.crt";

/* Sample in-memory persistence storage - size 1MB */
static uint8_t persistence_storage[1024 * 1024];

/* WolkConnect-C Connector context */
static wolk_ctx_t wolk;

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

/* Actuation setup and call-backs */
static const char* actuator_references[] = {"SW", "SL"};
static char actuator_value[READING_SIZE] = {"0"};
static const uint32_t num_actuator_references = 2;

static void actuation_handler(const char* reference, const char* value)
{
    printf("Actuation handler - Reference: %s Value: %s\n", reference, value);

    if ((strcmp(reference, actuator_references[0]) == 0) || (strcmp(reference, actuator_references[1]) == 0)) {
        strcpy(actuator_value, value);
    } else {
        printf("Actuation handler - Wrong Reference\n");
    }
}

static actuator_status_t actuator_status_provider(const char* reference)
{
    printf("Actuator status provider - Reference: %s\n", reference);

    actuator_status_t actuator_status;
    actuator_status_init(&actuator_status, "", ACTUATOR_STATE_ERROR);

    if (strcmp(reference, "SW") == 0) {
        actuator_status_init(&actuator_status, actuator_value, ACTUATOR_STATE_READY);
    } else if (strcmp(reference, "SL") == 0) {
        actuator_status_init(&actuator_status, actuator_value, ACTUATOR_STATE_READY);
    }

    return actuator_status;
}

/* Configuration setup and call-backs */
static int publish_period_seconds = DEFAULT_PUBLISH_PERIOD_SECONDS;
static char device_configuration_references[CONFIGURATION_ITEMS_SIZE][CONFIGURATION_REFERENCE_SIZE] = {"HB", "LL",
                                                                                                       "EF"};
static char device_configuration_values[CONFIGURATION_ITEMS_SIZE][CONFIGURATION_VALUE_SIZE] = {"", "INFO", "T,H,P,ACL"};

int update_default_device_configuration_values(char* default_device_configuration_values[], int default_value)
{
    char default_publish_period[10];

    int number_size = snprintf(default_publish_period, 10, "%d", default_value);
    strncpy(&default_device_configuration_values[0], default_publish_period, number_size);
}


static void configuration_handler(char (*reference)[CONFIGURATION_REFERENCE_SIZE],
                                  char (*value)[CONFIGURATION_VALUE_SIZE], size_t num_configuration_items)
{
    char value_str[READING_SIZE];
    memset(value_str, 0, sizeof(value_str));
    sprintf(value_str, "%f", value);

    for (size_t i = 0; i < num_configuration_items; ++i) {
        size_t iteration_counter = 0;

        for (size_t j = 0; j < CONFIGURATION_ITEMS_SIZE; ++j) {
            if (!strcmp(reference[i], device_configuration_references[j])) {
                strcpy(device_configuration_values[j], value[i]);
                printf("Configuration handler - Reference: %s | Value: %s\n", reference[i], value[i]);

                if (!strcmp(reference[i], device_configuration_references[0])) {
                    publish_period_seconds = atoi(value[i]);
                } else if (!strcmp(reference[i], device_configuration_references[2])) {
                    enable_feeds(&value[i]);
                }
            } else
                iteration_counter++;

            if (iteration_counter == CONFIGURATION_ITEMS_SIZE) {
                printf("Unrecognised Reference received!\n");
            }
        }
    }
}

static size_t configuration_provider(char (*reference)[CONFIGURATION_REFERENCE_SIZE],
                                     char (*value)[CONFIGURATION_VALUE_SIZE], size_t max_num_configuration_items)
{
    WOLK_UNUSED(max_num_configuration_items);
    WOLK_ASSERT(max_num_configuration_items >= NUMBER_OF_CONFIGURATION);

    for (size_t i = 0; i < CONFIGURATION_ITEMS_SIZE; ++i) {
        strcpy(reference[i], device_configuration_references[i]);
        strcpy(value[i], device_configuration_values[i]);
    }

    return CONFIGURATION_ITEMS_SIZE;
}


int main(int argc, char* argv[])
{
    WOLK_UNUSED(argc);
    WOLK_UNUSED(argv);

    signal(SIGINT, int_handler);
    signal(SIGTERM, int_handler);

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

    if (wolk_init(&wolk, send_buffer, receive_buffer, actuation_handler, actuator_status_provider,
                  configuration_handler, configuration_provider, device_key, device_password, PROTOCOL_WOLKABOUT,
                  actuator_references, num_actuator_references)
        != W_FALSE) {
        printf("Error initializing WolkConnect-C\n");
        return 1;
    }

    if (wolk_init_in_memory_persistence(&wolk, persistence_storage, sizeof(persistence_storage), false) != W_FALSE) {
        printf("Error initializing in-memory persistence\n");
        return 1;
    }

    if (wolk_init_file_management(&wolk, 128 * 1024 * 1024, 500, file_management_start, file_management_chunk_write,
                                  file_management_chunk_read, file_management_abort, file_management_finalize,
                                  file_management_start_url_download, file_management_is_url_download_done,
                                  file_management_get_file_list, file_management_remove_file,
                                  file_management_purge_files)
        != W_FALSE) {
        printf("Error initializing File Management");
        return 1;
    }

    // TODO: separate init for firmware_installation()
    update_default_device_configuration_values(&device_configuration_values, DEFAULT_PUBLISH_PERIOD_SECONDS);

    printf("Wolk client - Connecting to server\n");
    if (wolk_connect(&wolk) != W_FALSE) {
        printf("Wolk client - Error connecting to server\n");
        return -1;
    }
    printf("Wolk client - Connected to server\n");

    wolk_add_alarm(&wolk, "HH", true, 0);
    wolk_publish(&wolk);

    while (keep_running) {
        // MANDATORY: sleep(currently 1000us) and number of tick(currently 1) when are multiplied needs to give 1ms.
        // you can change this parameters, but keep it's multiplication
        usleep(1000);
        wolk_process(&wolk, 1);

        sensor_readings_process(&wolk, &publish_period_seconds, DEFAULT_PUBLISH_PERIOD_SECONDS);
    }

    printf("Wolk client - Diconnecting\n");
    wolk_disconnect(&wolk);
    BIO_free_all(sockfd);

    return 0;
}
