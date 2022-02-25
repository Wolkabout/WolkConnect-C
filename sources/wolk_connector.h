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
 * @param reference parameter references defined on WolkAbout IoT Platform
 * @param value parameter values received from WolkAbout IoT Platform.
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

    feed_handler_t feed_handler; /**< Callback for handling incoming feed from WolkAbout IoT Platform.
                                              @see feed_handler_t*/

    parameter_handler_t parameter_handler; /**< Callback for handling received configuration from WolkAbout IoT
                                                      Platform. @see parameter_handler_t*/

    char device_key[DEVICE_KEY_SIZE]; /**<  Authentication parameters for WolkAbout IoT Platform. Obtained as a
 *
 * result
                                         of device creation on the platform.*/
    char device_password[DEVICE_PASSWORD_SIZE]; /**<  Authentication parameters for WolkAbout IoT Platform. Obtained as
                                                   a

                                                   result of device creation on the platform.*/

    parser_t parser;

    persistence_t persistence;

    file_management_t file_management;

    firmware_update_t firmware_update;

    uint64_t utc;

    bool is_initialized;
} wolk_ctx_t;

/**
 * @brief  WolkAbout IoT Platform numeric reading type. It is simplified reading_t type
 */
typedef struct wolk_numeric_readings_t {
    double value;
    uint64_t utc_time;
} wolk_numeric_readings_t;

/**
 * @brief  WolkAbout IoT Platform string reading type. It is simplified reading_t type
 */
typedef struct wolk_string_readings {
    char* value;
    uint64_t utc_time;
} wolk_string_readings_t;

/**
 * @brief  WolkAbout IoT Platform boolean reading type. It is simplified reading_t type
 */
typedef struct wolk_boolean_readings {
    bool* value;
    uint64_t utc_time;
} wolk_boolean_readings_t;

typedef feed_t wolk_feed_t;

/**
 * @brief Initializes WolkAbout IoT Platform connector context
 *
 * @param ctx Context
 *
 * @param snd_func Callback function that handles outgoing traffic
 * @param rcv_func Callback function that handles incoming traffic
 *
 * @param feed_handler function pointer to 'feed_handler_t' implementation
 *
 * @param parameter_handler function pointer to 'parameter_handler_t' implementation
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
 * platform, obtain and perform incoming traffic
 *
 * @param ctx Context
 * @param tick Period at which wolk_process is called
 *
 * @return Error code
 */
WOLK_ERR_T wolk_process(wolk_ctx_t* ctx, uint64_t tick);

// TODO: feed VS reading
/** @brief Add string reading
 *
 * @param ctx Context
 * @param reference Feed reference
 * @param readings Feed values, one or more values organized as value:utc pairs. Value is char pointer. Utc time has to
 * be in milliseconds.
 * @param number_of_readings Number of readings that is captured
 *
 *  @return Error code
 */
WOLK_ERR_T wolk_add_string_feed(wolk_ctx_t* ctx, const char* reference, wolk_string_readings_t* readings,
                                size_t number_of_readings);

/**
 * @brief Add numeric reading
 *
 * @param ctx Context
 * @param reference Feed reference
 * @param readings Feed values, one or more values organized as value:utc pairs. Value is double. Utc time has to be in
 * milliseconds.
 * @param number_of_readings Number of readings that is captured
 *
 * @return Error code
 */
WOLK_ERR_T wolk_add_numeric_feed(wolk_ctx_t* ctx, const char* reference, wolk_numeric_readings_t* readings,
                                 size_t number_of_readings);

/**
 * @brief Add multi-value numeric reading. For feeds that has more than one numeric number associated as value, like
 * location is. Max number of numeric values is define with READING_MAX_NUMBER from size_definition
 *
 * @param ctx Context
 * @param reference Feed reference
 * @param values Feed values
 * @param value_size Number of numeric values limited by READING_MAX_NUMBER
 * @param utc_time UTC time of feed value acquisition [miliseconds]
 *
 * @return Error code
 */
WOLK_ERR_T wolk_add_multi_value_numeric_feed(wolk_ctx_t* ctx, const char* reference, double* values,
                                             uint16_t value_size, uint64_t utc_time);

/**
 * @brief Add bool reading
 *
 * @param ctx Context
 * @param reference Feed reference
 * @param readings Feed values, one or more values organized as value:utc pairs. Value is boolean. Utc time has to be in
 * milliseconds.
 * @param number_of_readings Number of readings that is captured
 *
 * @return Error code
 */
WOLK_ERR_T wolk_add_bool_reading(wolk_ctx_t* ctx, const char* reference, wolk_boolean_readings_t* readings,
                                 size_t number_of_readings);

/**
 * @brief Publish accumulated readings
 *
 * @param ctx Context
 *
 * @return Error code
 */
WOLK_ERR_T wolk_publish(wolk_ctx_t* ctx);

/**
 * @brief Register and update attribute. The attribute name must be unique per device. All attributes created by a
 * device are always required and read-only. If an attribute with the given name already exists, the value will be
 * updated.
 *
 * @param ctx Context
 * @param attribute Attribute description consists of name, data type and value.
 *
 * @return Error code
 */
WOLK_ERR_T wolk_register_attribute(wolk_ctx_t* ctx, attribute_t* attribute);

/**
 * @brief Register feeds. If feed already exist error will be raised at the error channel. If bulk registration is
 * request one error feed is enough to block registration.
 *
 * @param ctx Context
 * @param feeds List of feeds that will be register. The feed name and reference must be unique per device, it's user
 * responsibility.
 * @param number_of_feeds Number of feeds presented into feeds list
 *
 * @return Error code
 */
WOLK_ERR_T wolk_register_feed(wolk_ctx_t* ctx, feed_t* feeds, size_t number_of_feeds);

/**
 * @brief Remove feeds.
 *
 * @param ctx Context
 * @param feeds A list of feed references to be removed. If there is no feed with a specified reference, the reference
 * will be ignored and nothing will happen
 * @param number_of_feeds Number of feeds presented into feeds list
 *
 * @return Error code
 */
WOLK_ERR_T wolk_remove_feed(wolk_ctx_t* ctx, feed_t* feeds, size_t number_of_feeds);

WOLK_ERR_T wolk_pull_feed_values(wolk_ctx_t* ctx);

/**
 * @brief Change existing parameters on the WolkAbout IoT Platform.
 * Updating parameters this way can lead to a misconfigured connection which might prevent further updates. Update
 * connectivity parameters responsibly!
 *
 * @param ctx Context
 * @param parameters List of parameters
 * @param number_of_parameters Number of readings that is captured
 *
 * @return Error code
 */
WOLK_ERR_T wolk_change_parameter(wolk_ctx_t* ctx, parameter_t* parameter, size_t number_of_parameters);

/**
 * @brief Sending a message to this topic will notify the platform that the device is ready to receive all parameter
 * updates that were queued on the platform for that device. Relevant when the device specified PULL outbound data type.
 *
 * @param ctx Context
 * @param parameters List of parameters
 * @param number_of_parameters Number of readings that is captured
 *
 * @return Error code
 */
WOLK_ERR_T wolk_pull_parameters(wolk_ctx_t* ctx);

/**
 * @brief A request from the device to the WolkAbout IoT platform for the current values of device parameters.
 * The payload should contain a list of parameter names that will be contained in the response with it's values.
 *
 * @param ctx Context
 * @param parameters List of parameters
 * @param number_of_parameters Number of readings that is captured
 *
 * @return Error code
 */
WOLK_ERR_T wolk_sync_parameters(wolk_ctx_t* ctx, parameter_t* parameters, size_t number_of_parameters);

WOLK_ERR_T wolk_sync_time_request(wolk_ctx_t* ctx);


#ifdef __cplusplus
}
#endif

#endif
