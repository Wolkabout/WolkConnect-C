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

/**
 * @file WolkConn.h
 *
 * WolkConnect C
 *
 */
#ifndef WOLK_H
#define WOLK_H

#ifdef __cplusplus
extern "C" {
#endif

#include "MQTTPacket.h"
#include "actuator_status.h"
#include "configuration_item.h"
#include "firmware_update.h"
#include "parser.h"
#include "persistence.h"
#include "size_definitions.h"
#include "transport.h"

#include <stdbool.h>
#include <stdint.h>

/**
 * @brief Library versioning
 */
enum { WOLK_VERSION_MAJOR = 2, WOLK_VERSION_MINOR = 5, WOLK_VERSION_PATCH = 0 };

/**
 * @brief Supported protocols, WolkConnect libararies currently support only PROTOCOL_JSON_SINGLE
 */
typedef enum { PROTOCOL_JSON_SINGLE = 0 } protocol_t;

/**
 * @brief WOLK_ERR_T Boolean used for error handling in WolkConnect-C library
 */
typedef unsigned char WOLK_ERR_T;

/**
 * @brief WOLK_BOOL_T Boolean used in WolkConnect-C library
 */
typedef unsigned char WOLK_BOOL_T;
enum WOLK_BOOL_T_values { W_FALSE = 0, W_TRUE = 1 };

/**
 * @brief Callback declaration for writting bytes to socket
 */
typedef int (*send_func_t)(unsigned char* bytes, unsigned int num_bytes);

/**
 * @brief Callback declaration for reading bytes from socket
 */
typedef int (*recv_func_t)(unsigned char* bytes, unsigned int num_bytes);

/**
 * @brief Declaration of actuator handler.
 * Actuator reference and value are the pairs of data on the same place in own arrays.
 *
 * @param reference actuator references defined in manifest on WolkAbout IoT Platform.
 * @param value value received from WolkAbout IoT Platform.
 */
typedef void (*actuation_handler_t)(const char* reference, const char* value);
/**
 * @brief Declaration of actuator status
 *
 * @param reference actuator references define in manifest on WolkAbout IoT Platform
 */
typedef actuator_status_t (*actuator_status_provider_t)(const char* reference);

/**
 * @brief Declaration of configuration handler.
 * Configuration reference and value are the pairs of data on the same place in own arrays.
 *
 * @param reference actuator references define in manifest on WolkAbout IoT Platform
 * @param value actuator values received from WolkAbout IoT Platform.
 * @param num_configuration_items number of used configuration parameters
 */
typedef void (*configuration_handler_t)(char (*reference)[CONFIGURATION_REFERENCE_SIZE],
                                        char (*value)[CONFIGURATION_VALUE_SIZE], size_t num_configuration_items);
/**
 * @brief Declaration of configuration provider
 *
 * @param reference configuration references define in manifest on WolkAbout IoT Platform
 * @param value configuration values received from WolkAbout IoT Platform
 * @param num_configuration_items number of used configuration parameters
 */
typedef size_t (*configuration_provider_t)(char (*reference)[CONFIGURATION_REFERENCE_SIZE],
                                           char (*value)[CONFIGURATION_VALUE_SIZE], size_t max_num_configuration_items);
/**
 * @brief  WolkAbout IoT Platform connector context.
 * Most of the parameters are used to initialize WolkConnect library forwarding to wolk_init().
 */
typedef struct wolk_ctx {
    int sock;
    MQTTPacket_connectData connectData;
    MQTTTransport mqtt_transport;
    transport_iofunctions_t iof;

    actuation_handler_t actuation_handler;                  /**< Callback for handling received actuation from WolkAbout IoT Platform. @see actuation_handler_t*/
    actuator_status_provider_t actuator_status_provider;    /**< Callback for providing the current actuator status to WolkAbout IoT Platform. @see actuator_status_provider_t*/

    configuration_handler_t configuration_handler;          /**< Callback for handling received configuration from WolkAbout IoT Platform. @see configuration_handler_t*/
    configuration_provider_t configuration_provider;        /**< Callback for providing the current configuration status to WolkAbout IoT Platform. @see configuration_provider_t*/

    char device_key[DEVICE_KEY_SIZE];                       /**<  Authentication parameters for WolkAbout IoT Platform. Obtained as a result of device creation on the platform.*/
    char device_password[DEVICE_PASSWORD_SIZE];             /**<  Authentication parameters for WolkAbout IoT Platform. Obtained as a result of device creation on the platform.*/

    protocol_t protocol;                                    /**<  Used protocol for communication with WolkAbout IoT Platform. @see protocol_t*/
    parser_t parser;

    persistence_t persistence;

    firmware_update_t firmware_update;

    const char** actuator_references;
    uint32_t num_actuator_references;

    bool is_keep_alive_enabled;
    uint32_t milliseconds_since_last_ping_keep_alive;

    bool is_initialized;
} wolk_ctx_t;

/**
 * @brief Initializes WolkAbout IoT Platform connector context
 *
 * @param ctx Context
 *
 * @param snd_func Callback function that handles outgoing traffic
 * @param rcv_func Callback function that handles incoming traffic
 *
 * @param actuation_handler function pointer to 'actuation_handler_t' implementation
 * @param actuator_status_provider function pointer to 'actuator_status_provider_t' implementation
 *
 * @param configuration_handler function pointer to 'configuration_handler_t' implementation
 * @param configuration_provider function pointer to 'configuration_provider_t' implementation
 *
 * @param device_key Device key provided by WolkAbout IoT Platform upon device
 * creation
 * @param password Device password provided by WolkAbout IoT Platform device
 * upon device creation
 * @param protocol Protocol specified for device
 *
 * @param actuator_references Array of strings containing references of
 * actuators that device possess
 * @param num_actuator_references Number of actuator references contained in
 * actuator_references
 *
 * @return Error code
 */
WOLK_ERR_T wolk_init(wolk_ctx_t* ctx, send_func_t snd_func, recv_func_t rcv_func, actuation_handler_t actuation_handler,
                     actuator_status_provider_t actuator_status_provider, configuration_handler_t configuration_handler,
                     configuration_provider_t configuration_provider, const char* device_key,
                     const char* device_password, protocol_t protocol, const char** actuator_references,
                     uint32_t num_actuator_references);

/**
 * @brief Initializes persistence mechanism with in-memory implementation
 *
 * @param ctx Context
 * @param storage Address to start of the memory which will be used by
 * persistence mechanism
 * @param size Size of memory in bytes
 * @param wrap If storage is full overwrite oldest item when pushing new item
 *
 * @return Error code
 */
WOLK_ERR_T wolk_init_in_memory_persistence(wolk_ctx_t* ctx, void* storage, uint32_t size, bool wrap);

/**
 * @brief Initializes persistence mechanism with custom implementation
 *
 * @param ctx Context
 * @param push Function pointer to 'push' implemenation
 * @param pop Function pointer to 'pop' implementation
 * @param is_empty Function pointer to 'is empty' implementation
 *
 * @return Error code
 *
 * @see persistence.h for signatures of methods to be implemented, and
 * implementation contract
 */
WOLK_ERR_T wolk_init_custom_persistence(wolk_ctx_t* ctx, persistence_push_t push, persistence_peek_t peek,
                                        persistence_pop_t pop, persistence_is_empty_t is_empty);

/**
 * @brief Initializes firmware update
 *
 * @param ctx Context
 * @param version Current firmware version
 * @param maximum_firmware_size Maximum acceptable size of firmware file, in bytes
 * @param chunk_size Firmware file is transfered in chunks of size 'chunk_size'
 * @param start Function pointer to 'firmware_update_start' implementation
 * @param write_chunk Function pointer to 'firmware_update_write_chunk' implementation
 * @param read_chunk Function pointer to 'firmware_update_read_chunk' implementation
 * @param abort Function pointer to 'firmware_update_abort' implementation
 * @param finalize Function pointer to 'firmware_update_finalize' implementation
 *
 * @return Error code.
 */
WOLK_ERR_T wolk_init_firmware_update(wolk_ctx_t* ctx, const char* version, size_t maximum_firmware_size,
                                     size_t chunk_size, firmware_update_start_t start,
                                     firmware_update_write_chunk_t write_chunk, firmware_update_read_chunk_t read_chunk,
                                     firmware_update_abort_t abort, firmware_update_finalize_t finalize,
                                     firmware_update_persist_firmware_version_t persist_version,
                                     firmware_update_unpersist_firmware_version_t unpersist_version,
                                     firmware_update_start_url_download_t start_url_download,
                                     firmware_update_is_url_download_done_t is_url_download_done);

/**
 * @brief Disables internal keep alive mechanism
 *
 * @param ctx Context
 *
 * @return Error code
 */
WOLK_ERR_T wolk_disable_keep_alive(wolk_ctx_t* ctx);

/**
 * @brief Connect to WolkAbout IoT Platform
 *
 * Prior to connecting, following must be performed:
 *  1. Context must be initialized via wolk_init(wolk_ctx_t* ctx, send_func
 * snd_func, recv_func rcv_func, const char *device_key, const char *password,
 * protocol_t protocol)
 *  2. Persistence must be initialized using
 *      wolk_initialize_in_memory_persistence(wolk_ctx_t *ctx, void* storage,
 * uint16_t num_elements, bool wrap) or
 *      wolk_initialize_custom_persistence(wolk_ctx_t *ctx,
 *                                         persistence_push_t push,
 * persistence_pop_t pop, persistence_is_empty_t is_empty, persistence_size_t
 * size)
 *
 * @param ctx Context
 *
 * @return Error code
 */
WOLK_ERR_T wolk_connect(wolk_ctx_t* ctx);

/**
 * @brief Disconnect from WolkAbout IoT Platform
 *
 * @param ctx Context
 *
 * @return Error code
 */
WOLK_ERR_T wolk_disconnect(wolk_ctx_t* ctx);

/**
 * @brief Must be called periodically to keep alive connection to WolkAbout IoT
 * platform, obtain and perform actuation requests
 *
 * @param ctx Context
 * @param tick Perid at which wolk_process is called
 *
 * @return Error code
 */
WOLK_ERR_T wolk_process(wolk_ctx_t* ctx, uint32_t tick);

/** @brief Add string reading
 *
 *  @param ctx Context
 *  @param reference Sensor reference
 *  @param value Sensor value
 *  @param utc_time UTC time of sensor value acquisition [seconds]
 *
 *  @return Error code
 */
WOLK_ERR_T wolk_add_string_sensor_reading(wolk_ctx_t* ctx, const char* reference, const char* value, uint32_t utc_time);

/** @brief Add multi-value string reading
 *
 *  @param ctx Context
 *  @param reference Sensor reference
 *  @param values Sensor values
 *  @param values_size Number of sensor dimensions
 *  @param utc_time UTC time of sensor value acquisition [seconds]
 *
 *  @return Error code
 */
WOLK_ERR_T wolk_add_multi_value_string_sensor_reading(wolk_ctx_t* ctx, const char* reference,
                                                      const char (*values)[READING_SIZE], uint16_t values_size,
                                                      uint32_t utc_time);

/**
 * @brief Add numeric reading
 *
 * @param ctx Context
 * @param reference Sensor reference
 * @param value Sensor value
 * @param utc_time UTC time of sensor value acquisition [seconds]
 *
 * @return Error code
 */
WOLK_ERR_T wolk_add_numeric_sensor_reading(wolk_ctx_t* ctx, const char* reference, double value, uint32_t utc_time);

/**
 * @brief Add multi-value numeric reading
 *
 * @param ctx Context
 * @param reference Sensor reference
 * @param values Sensor values
 * @param values_size Number of sensor dimensions
 * @param utc_time UTC time of sensor value acquisition [seconds]
 *
 * @return Error code
 */
WOLK_ERR_T wolk_add_multi_value_numeric_sensor_reading(wolk_ctx_t* ctx, const char* reference, double* values,
                                                       uint16_t values_size, uint32_t utc_time);

/**
 * @brief Add bool reading
 *
 * @param ctx Context
 * @param reference Sensor reference
 * @param value Sensor value
 * @param utc_time UTC time of sensor value acquisition [seconds]
 *
 * @return Error code
 */
WOLK_ERR_T wolk_add_bool_sensor_reading(wolk_ctx_t* ctx, const char* reference, bool value, uint32_t utc_time);

/**
 * @brief Add multi-value bool reading
 *
 * @param ctx Context
 * @param reference Sensor reference
 * @param values Sensor values
 * @param values_size Number of sensor dimensions
 * @param utc_time UTC time of sensor value acquisition [seconds]
 *
 * @return Error code
 */
WOLK_ERR_T wolk_add_multi_value_bool_sensor_reading(wolk_ctx_t* ctx, const char* reference, bool* values,
                                                    uint16_t values_size, uint32_t utc_time);

/**
 * @brief Add alarm
 *
 * @param ctx Context
 * @param reference Alarm reference
 * @param message Alarm message
 * @param utc_time UTC time when alarm was risen [seconds]
 *
 * @return Error code
 */
WOLK_ERR_T wolk_add_alarm(wolk_ctx_t* ctx, const char* reference, bool state, uint32_t utc_time);

/**
 * @brief Obtains actuator status via actuator_status_provider_t and publishes
 * it. If actuator status can not be published, it is persisted.
 *
 * @param ctx Context
 * @param reference Actuator reference
 *
 * @return Error code
 */
WOLK_ERR_T wolk_publish_actuator_status(wolk_ctx_t* ctx, const char* reference);

/**
 * @brief Publish accumulated sensor readings, and alarms
 *
 * @param ctx Context
 *
 * @return Error code
 */
WOLK_ERR_T wolk_publish_configuration_status(wolk_ctx_t* ctx, char (*p)[CONFIGURATION_REFERENCE_SIZE],char (*p1)[CONFIGURATION_VALUE_SIZE],int number);//promena

/**
 * @brief Publish configuration status
 *
 * @param ctx Context
 *
 * @return Error code
 */
WOLK_ERR_T wolk_publish(wolk_ctx_t* ctx);

#ifdef __cplusplus
}
#endif

#endif
