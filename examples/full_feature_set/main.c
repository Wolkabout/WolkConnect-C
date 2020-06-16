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

#include <memory.h>
#include <netdb.h>
#include <netinet/in.h>
#include <signal.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#include <openssl/ssl.h>

#define DEFAULT_PUBLISH_PERIOD_SECONDS 5

static SSL_CTX* ctx;
static BIO* sockfd;

static const char* device_key = "device_key";
static const char* device_password = "some_password";
static const char* hostname = "api-demo.wolkabout.com";
static int portno = 8883;
static char certs[] = "../ca.crt";

/* Sample in-memory persistence storage - size 1MB */
static uint8_t persistence_storage[1024 * 1024];

/* WolkConnect-C Connector context */
static wolk_ctx_t wolk;

static int publish_period_seconds = DEFAULT_PUBLISH_PERIOD_SECONDS;
static const char* actuator_references[] = {"SW", "SL"};
static const uint32_t num_actuator_references = 2;

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


static char actuator_value[READING_SIZE] = {"0"};

static void actuation_handler(const char* reference, const char* value)
{
    printf("Actuation handler - Reference: %s Value: %s\n", reference, value);

    strcpy(actuator_value, value);
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

static char device_configuration_references[CONFIGURATION_ITEMS_SIZE][CONFIGURATION_REFERENCE_SIZE] = {
    "HB", "LL", "EF"};
static char device_configuration_values[CONFIGURATION_ITEMS_SIZE][CONFIGURATION_VALUE_SIZE] = {
    "5", "INFO", "T,H,P"};

static void configuration_handler(char (*reference)[CONFIGURATION_REFERENCE_SIZE],
                                  char (*value)[CONFIGURATION_VALUE_SIZE], size_t num_configuration_items)
{
    for (size_t i = 0; i < num_configuration_items; ++i) {
        size_t iteration_counter = 0;

        for (size_t j = 0; j < CONFIGURATION_ITEMS_SIZE; ++j) {
            if(!strcmp(reference[i], device_configuration_references[j])) {
                strcpy(device_configuration_values[j], value[i]);
                printf("Configuration handler - Reference: %s | Value: %s\n", reference[i], value[i]);

                if(!strcmp(reference[i], device_configuration_references[0])) {
                    publish_period_seconds=atoi(value[i]);
                }
            } else
                iteration_counter++;

            if(iteration_counter==CONFIGURATION_ITEMS_SIZE){
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

static FILE* firmware_file;
char firmware_file_name[FIRMWARE_UPDATE_FILE_NAME_SIZE];
static size_t firmware_file_size = 0;
static bool firmware_update_start(const char* file_name, size_t file_size)
{
    printf("Starting firmware update. File name: %s. File size:%zu\n", file_name, file_size);
    firmware_file_size = file_size;

    firmware_file = fopen(file_name, "w+b");

    if (firmware_file == NULL) {
        return false;
    }

    strcpy(firmware_file_name, file_name);
    return true;
}

static bool firmware_chunk_write(uint8_t* data, size_t data_size)
{
    printf("Firmware update chunk write\n");

    const size_t items_written = fwrite(data, data_size, 1, firmware_file);
    fflush(firmware_file);
    return items_written == 1;
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

    remove(firmware_file_name);
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
    printf("Starting firmware download from url %s\n", url);
    return true;
}

static bool firmware_update_is_url_download_done(bool* success)
{
    *success = true;
    return true;
}

static void sending_pressure_reading(void);
static void sending_temperature_reading(void);
static void sending_humidity_reading(void);
static void sending_accl_readings(void);
static bool sensor_readings_process(int* publish_period);

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

    if (wolk_init_firmware_update(&wolk, "1.0.0", 128 * 1024 * 1024, 256, firmware_update_start, firmware_chunk_write,
                                  firmware_chunk_read, firmware_update_abort, firmware_update_finalize,
                                  firmware_update_persist_firmware_version, firmware_update_unpersist_firmware_version,
                                  firmware_update_start_url_download, firmware_update_is_url_download_done)
        != W_FALSE) {
        printf("Error initializing firmware update");
        return 1;
    }

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

        sensor_readings_process(&publish_period_seconds);
    }

    printf("Wolk client - Diconnecting\n");
    wolk_disconnect(&wolk);
    BIO_free_all(sockfd);

    return 0;
}

static bool sensor_readings_process(int* publish_period_seconds) {
    static double publish_period_counter = 0;

    if(publish_period_seconds < DEFAULT_PUBLISH_PERIOD_SECONDS)
        return 1;

    publish_period_counter++;
    if (publish_period_counter > (*publish_period_seconds)*1000)
    {
        printf("Sending sensor readings:\n");
        sending_pressure_reading();
        sending_temperature_reading();
        sending_humidity_reading();
        sending_accl_readings();

        publish_period_counter = 0;
    }
    return 0;
}

static void sending_pressure_reading(void){
    int8_t pressure = 0;

    pressure = (rand() % 800)+300;
    wolk_add_numeric_sensor_reading(&wolk, "P", pressure, 0);
    wolk_publish(&wolk);
    printf("\tPressure\t: %dmb\n", pressure);
}

static void sending_temperature_reading(void){
    int8_t temperature = 0;

    temperature = (rand() % 125)-40;
    wolk_add_numeric_sensor_reading(&wolk, "T", temperature, 0);
    wolk_publish(&wolk);
    printf("\tTemperature\t: %d°C\n", temperature);
}

static void sending_humidity_reading(void){
    int8_t humidity = 0;

    humidity = rand() % 100;
    wolk_add_numeric_sensor_reading(&wolk, "H", humidity, 0);
    wolk_publish(&wolk);
    printf("\tHumidity\t: %d%\n", humidity);
}

static void sending_accl_readings(void)
{
    double accl_readings[3] = {1, 0, 0};

    wolk_add_multi_value_numeric_sensor_reading(&wolk, "ACL", &accl_readings[0], 3, 0);
    printf("\tAcceleration on XYZ\t: %fg, %fg, %fg\n", accl_readings[0],  accl_readings[1],  accl_readings[2]);
    wolk_publish(&wolk);
}
