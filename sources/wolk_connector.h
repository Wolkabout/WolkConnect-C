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
#include "connectivity/data_transmission.h"
#include "model/attribute.h"
#include "model/feed.h"
#include "model/file_management/file_management.h"
#include "model/utc_command.h"
#include "persistence/persistence.h"
#include "protocol/parser.h"
#include "size_definitions.h"
#include "types.h"

#include <stdbool.h>
#include <stdint.h>

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
 * @brief Declaration of feed value handler.
 *
 * @param reference feed reference received from WolkAbout IoT Platform.
 * @param value value received from WolkAbout IoT Platform.
 */
typedef void (*feed_handler_t)(const char* reference, const char* value);

/**
 * @brief Declaration of parameter handler.
 *
 * @param reference actuator references define in manifest on WolkAbout IoT Platform
 * @param value actuator values received from WolkAbout IoT Platform.
 * @param num_configuration_items number of used configuration parameters
 */
typedef void (*parameter_handler_t)(char (*name)[PARAMETER_TYPE_SIZE], char (*value)[PARAMETER_VALUE_SIZE]);

/**
 * @brief  WolkAbout IoT Platform connector context.
 * Most of the parameters are used to initialize WolkConnect library forwarding to wolk_init().
 */
typedef struct wolk_ctx {
    int sock;
    MQTTPacket_connectData connectData;
    MQTTTransport mqtt_transport;
    transmission_io_functions_t iof;

    outbound_mode_t outbound_mode;

    feed_handler_t feed_handler; /**< Callback for handling received actuation from WolkAbout IoT Platform.
                                              @see actuation_handler_t*/

    parameter_handler_t parameter_handler; /**< Callback for handling received configuration from WolkAbout IoT
                                                      Platform. @see configuration_handler_t*/

    char device_key[DEVICE_KEY_SIZE]; /**<  Authentication parameters for WolkAbout IoT Platform. Obtained as a result
                                         of device creation on the platform.*/
    char device_password[DEVICE_PASSWORD_SIZE]; /**<  Authentication parameters for WolkAbout IoT Platform. Obtained as
                                                   a result of device creation on the platform.*/

    parser_t parser;

    persistence_t persistence;

    file_management_t file_management;

    firmware_update_t firmware_update;

    uint64_t utc;

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
 *
 * @param configuration_handler function pointer to 'configuration_handler_t' implementation
 * @param configuration_provider function pointer to 'configuration_provider_t' implementation
 *
 * @param device_key Device key provided by WolkAbout IoT Platform upon device
 * creation
 * @param password Device password provided by WolkAbout IoT Platform device
 * upon device creation
 *
 *
 * @return Error code
 */
WOLK_ERR_T wolk_init(wolk_ctx_t* ctx, send_func_t snd_func, recv_func_t rcv_func, feed_handler_t feed_handler,
                     parameter_handler_t parameter_handler, const char* device_key, const char* device_password,
                     outbound_mode_t outbound_mode);

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
 * @brief Initializes File Management
 *
 * @param ctx Context
 * @param maximum_file_size Maximum acceptable size of file, in bytes
 * @param chunk_size file is transferred in chunks of size 'chunk_size'
 * @param start Function pointer to 'file_management_start' implementation
 * @param write_chunk Function pointer to 'file_management_write_chunk' implementation
 * @param read_chunk Function pointer to 'file_management_read_chunk' implementation
 * @param abort Function pointer to 'file_management_abort' implementation
 * @param finalize Function pointer to 'file_management_finalize' implementation
 * @param start_url_download Function pointer to 'file_management_start_url_download' implementation
 * @param is_url_download_done Function pointer to 'file_management_is_url_download_done' implementation
 *
 *
 * @return Error code.
 */
WOLK_ERR_T wolk_init_file_management(
    wolk_ctx_t* ctx, size_t maximum_file_size, size_t chunk_size, file_management_start_t start,
    file_management_write_chunk_t write_chunk, file_management_read_chunk_t read_chunk, file_management_abort_t abort,
    file_management_finalize_t finalize, file_management_start_url_download_t start_url_download,
    file_management_is_url_download_done_t is_url_download_done, file_management_get_file_list_t get_file_list,
    file_management_remove_file_t remove_file, file_management_purge_files_t purge_files);

/**
 * @brief Initializes Firmware Update
 *
 * @param ctx Context
 * @param function pointer to 'start_installation' implementation
 * @param function pointer to 'is_installation_completed' implementation
 * @param function pointer to 'verification_store' implementation
 * @param function pointer to 'verification_read' implementation
 * @param function pointer to 'get_version' implementation
 * @param function pointer to 'abort_installation' implementation
 *
 * @return Error code
 */
WOLK_ERR_T wolk_init_firmware_update(wolk_ctx_t* ctx, firmware_update_start_installation_t start_installation,
                                     firmware_update_is_installation_completed_t is_installation_completed,
                                     firmware_update_verification_store_t verification_store,
                                     firmware_update_verification_read_t verification_read,
                                     firmware_update_get_version_t get_version,
                                     firmware_update_abort_t abort_installation);

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
 * @param tick Period at which wolk_process is called
 *
 * @return Error code
 */
WOLK_ERR_T wolk_process(wolk_ctx_t* ctx, uint64_t tick);

/** @brief Add string reading
 *
 *  @param ctx Context
 *  @param reference Sensor reference
 *  @param value Sensor value
 *  @param utc_time UTC time of sensor value acquisition [seconds]
 *
 *  @return Error code
 */
WOLK_ERR_T wolk_add_string_reading(wolk_ctx_t* ctx, const char* reference, const char* value, uint64_t utc_time);

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
WOLK_ERR_T wolk_add_multi_value_string_reading(wolk_ctx_t* ctx, const char* reference,
                                               const char (*values)[READING_SIZE], uint16_t values_size,
                                               uint64_t utc_time);

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
WOLK_ERR_T wolk_add_numeric_reading(wolk_ctx_t* ctx, const char* reference, double value, uint64_t utc_time);

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
WOLK_ERR_T wolk_add_multi_value_numeric_reading(wolk_ctx_t* ctx, const char* reference, double* values,
                                                uint16_t values_size, uint64_t utc_time);

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
WOLK_ERR_T wolk_add_bool_reading(wolk_ctx_t* ctx, const char* reference, bool value, uint64_t utc_time);

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
WOLK_ERR_T wolk_add_multi_value_bool_reading(wolk_ctx_t* ctx, const char* reference, bool* values, uint16_t values_size,
                                             uint64_t utc_time);

/**
 * @brief Publish accumulated sensor readings
 *
 * @param ctx Context
 *
 * @return Error code
 */

WOLK_ERR_T wolk_register_attribute(wolk_ctx_t* ctx, attribute_t* attribute);
WOLK_ERR_T wolk_change_parameter(wolk_ctx_t* ctx, parameter_t* parameter);

WOLK_ERR_T wolk_register_feed(wolk_ctx_t* ctx, feed_t* feed);
WOLK_ERR_T wolk_remove_feed(wolk_ctx_t* ctx, feed_t* feed);

WOLK_ERR_T wolk_pull_feed_values(wolk_ctx_t* ctx);
WOLK_ERR_T wolk_pull_parameters(wolk_ctx_t* ctx);

WOLK_ERR_T wolk_sync_parameters(wolk_ctx_t* ctx);

WOLK_ERR_T wolk_sync_time_request(wolk_ctx_t* ctx);

WOLK_ERR_T wolk_publish(wolk_ctx_t* ctx);

#ifdef __cplusplus
}
#endif

#endif
