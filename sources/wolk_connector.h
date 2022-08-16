/*
 * Copyright 2022 WolkAbout Technology s.r.o.
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
 * @file wolkconnector.h
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
#include "model/file_management/file_management.h"
#include "model/utc_command.h"
#include "persistence/persistence.h"
#include "protocol/parser.h"
#include "size_definitions.h"
#include "wolk_types.h"

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
 * @brief  WolkAbout IoT Platform numeric feed type.
 */
typedef struct wolk_numeric_feeds_t {
    double value;
    uint64_t utc_time;
} wolk_numeric_feeds_t;

/**
 * @brief  WolkAbout IoT Platform string feed type.
 */
typedef struct wolk_string_feeds_t {
    char* value;
    uint64_t utc_time;
} wolk_string_feeds_t;

/**
 * @brief  WolkAbout IoT Platform boolean feed type.
 */
typedef struct wolk_boolean_feeds_t {
    bool value;
    uint64_t utc_time;
} wolk_boolean_feeds_t;

typedef feed_t wolk_feed_t;
typedef feed_registration_t wolk_feed_registration_t;
typedef parameter_t wolk_parameter_t;
typedef attribute_t wolk_attribute_t;

/**
 * @brief Callback declaration for writting bytes to socket
 */
typedef int (*send_func_t)(unsigned char* bytes, unsigned int num_bytes);

/**
 * @brief Callback declaration for bytes from socket
 */
typedef int (*recv_func_t)(unsigned char* bytes, unsigned int num_bytes);

/**
 * @brief Declaration of feed value handler.
 *
 * @param feeds feeds received as name:value pairs from WolkAbout IoT Platform.
 * @param number_of_feeds number fo received feeds.
 */
typedef void (*feed_handler_t)(wolk_feed_t* feeds, size_t number_of_feeds);

/**
 * @brief Declaration of parameter handler.
 *
 * @param parameter_message Parameters received as name:value pairs from WolkAbout IoT Platform.
 * @param number_of_parameters number of received parameters
 */
typedef void (*parameter_handler_t)(wolk_parameter_t* parameter_message, size_t number_of_parameters);

/**
 * @brief Declaration of details synchronization handler. It will be called as a response on the
 * wolk_details_synchronization() call. It will give list of the all feeds and attributes from the platform.
 *
 * @param parameter_message Parameters received as name:value pairs from WolkAbout IoT Platform.
 * @param number_of_parameters number of received parameters
 */
typedef void (*details_synchronization_handler_t)(wolk_feed_registration_t* feeds, size_t number_of_received_feeds,
                                                  wolk_attribute_t* attributes, size_t number_of_received_attributes);


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

    feed_handler_t feed_handler; /**< Callback for handling incoming feeds from WolkAbout IoT Platform.
                                              @see feed_handler_t*/

    parameter_handler_t parameter_handler; /**< Callback for handling received configuration from WolkAbout IoT
                                                      Platform. @see parameter_handler_t*/

    details_synchronization_handler_t details_synchronization_handler; /**< Callback for handling received details
configuration from WolkAbout IoT Platform. @see attribute_handler_t*/

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
 * @brief Initializes WolkAbout IoT Platform connector context
 *
 * @param ctx Context
 *
 * @param snd_func Callback function that handles outgoing traffic
 * @param rcv_func Callback function that handles incoming traffic
 *
 * @param device_key Device key provided by WolkAbout IoT Platform upon device
 * creation
 * @param password Device password provided by WolkAbout IoT Platform device
 * upon device creation
 *
 * @param feed_handler function pointer to 'feed_handler_t' implementation
 *
 * @param parameter_handler function pointer to 'parameter_handler_t' implementation
 *
 * @param details_synchronization_handler function pointer to 'details_synchronization_handler_t' implementation
 *
 * @return Error code
 */
WOLK_ERR_T wolk_init(wolk_ctx_t* ctx, send_func_t snd_func, recv_func_t rcv_func, const char* device_key,
                     const char* device_password, outbound_mode_t outbound_mode, feed_handler_t feed_handler,
                     parameter_handler_t parameter_handler,
                     details_synchronization_handler_t details_synchronization_handler);

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
 * @param push Function pointer to 'push' implementation
 * @param peek Function pointer to 'peek' implementation
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
 * @param chunk_size file is transferred in chunks of size 'chunk_size'. Unit is Kbytes defined during device creation
 * on the platform as "Maximum message size" Connectivity property
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
 * @param function pointer to 'abort_installation' implementation
 *
 * @return Error code
 */
WOLK_ERR_T wolk_init_firmware_update(wolk_ctx_t* ctx, firmware_update_start_installation_t start_installation,
                                     firmware_update_is_installation_completed_t is_installation_completed,
                                     firmware_update_verification_store_t verification_store,
                                     firmware_update_verification_read_t verification_read,
                                     firmware_update_abort_t abort_installation);

/**
 * @brief Connect to WolkAbout IoT Platform
 *
 * Prior to connecting, following must be performed:
 *  1. Context must be initialized via wolk_init(wolk_ctx_t* ctx, send_func_t snd_func, recv_func_t rcv_func, const
 * char* device_key, const char* device_password, outbound_mode_t outbound_mode, feed_handler_t feed_handler,
 *  parameter_handler_t parameter_handler, details_synchronization_handler_t details_synchronization_handler)
 *  2. Persistence must be initialized using
 *      wolk_init_in_memory_persistence(wolk_ctx_t* ctx, void* storage, uint32_t size, bool wrap) or
 *      wolk_init_custom_persistence(wolk_ctx_t* ctx, persistence_push_t push, persistence_peek_t peek,
 *      persistence_pop_t pop, persistence_is_empty_t is_empty)
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

/**
 * @brief Initialized feed
 *
 * @param feed Where it's data will be stored
 * @param name Feed name
 * @param reference Reference define on the WolkAbout IoT platform. This is mandatory filed to map feed.
 * @param unit Feed's unit, use one of the defined in the type.h file
 * @param feedType Can be IN or IN_OUT
 * @return
 */
WOLK_ERR_T wolk_init_feed(feed_registration_t* feed, char* name, const char* reference, const char* unit,
                          const feed_type_t feedType);

/** @brief Add string feed
 *
 * @param ctx Context
 * @param reference Feed reference
 * @param feeds Feed values, one or more values organized as value:utc pairs. Value is char pointer. Utc time has to
 * be in milliseconds.
 * @param number_of_feeds Number of feeds that is captured
 *
 *  @return Error code
 */
WOLK_ERR_T wolk_add_string_feed(wolk_ctx_t* ctx, const char* reference, wolk_string_feeds_t* feeds,
                                size_t number_of_feeds);

/**
 * @brief Add numeric feeds
 *
 * @param ctx Context
 * @param reference Feed reference
 * @param feeds Feed values, one or more values organized as value:utc pairs. Value is double. Utc time has to be in
 * milliseconds.
 * @param number_of_feeds Number of feeds that is captured
 *
 * @return Error code
 */
WOLK_ERR_T wolk_add_numeric_feed(wolk_ctx_t* ctx, const char* reference, wolk_numeric_feeds_t* feeds,
                                 size_t number_of_feeds);

/**
 * @brief Add multi-value numeric feed. For feeds that has more than one numeric number associated as value, like
 * location is. Max number of numeric values is define with FEEDS_MAX_NUMBER from size_definition
 *
 * @param ctx Context
 * @param reference Feed reference
 * @param values Feed values
 * @param value_size Number of numeric values limited by FEEDS_MAX_NUMBER
 * @param utc_time UTC time of feed value acquisition [miliseconds]
 *
 * @return Error code
 */
WOLK_ERR_T wolk_add_multi_value_numeric_feed(wolk_ctx_t* ctx, const char* reference, double* values,
                                             uint16_t value_size, uint64_t utc_time);

/**
 * @brief Add bool feed
 *
 * @param ctx Context
 * @param reference Feed reference
 * @param feeds Feed values, one or more values organized as value:utc pairs. Value is boolean. Utc time has to be in
 * milliseconds.
 * @param number_of_feeds Number of feeds that is captured
 *
 * @return Error code
 */
WOLK_ERR_T wolk_add_bool_feeds(wolk_ctx_t* ctx, const char* reference, wolk_boolean_feeds_t* feeds,
                               size_t number_of_feeds);

/**
 * @brief Publish all accumulated data from persistence. It can be any data(feeds, attributed or parameters) added after
 * last publish.
 *
 * @param ctx Context
 *
 * @return Error code
 */
WOLK_ERR_T wolk_publish(wolk_ctx_t* ctx);

/**
 * @brief Initialized attribute
 *
 * @param attribute Destination attribute
 * @param name String name that will be associated with the attribute
 * @param data_type String value  that will be associated with the attribute name
 * @param value String value that will be associated with the attribute name
 */
WOLK_ERR_T wolk_init_attribute(wolk_attribute_t* attribute, char* name, char* data_type, char* value);

/**
 * @brief Register and update attribute. The attribute name must be unique per device. All attributes created by a
 * device are always required and read-only. If an attribute with the given name already exists, the value will be
 * updated.
 *
 * @param ctx Context
 * @param attributes Attribute description consists of name, data type and value.
 * @param number_of_attributes Number of attributes that is captured
 *
 * @return Error code
 */
WOLK_ERR_T wolk_register_attribute(wolk_ctx_t* ctx, wolk_attribute_t* attributes, size_t number_of_attributes);

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
WOLK_ERR_T wolk_register_feed(wolk_ctx_t* ctx, feed_registration_t* feeds, size_t number_of_feeds);

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
WOLK_ERR_T wolk_remove_feed(wolk_ctx_t* ctx, feed_registration_t* feeds, size_t number_of_feeds);

/**
 * @brief This will notify the platform that the device is ready to receive all feed values that were queued on the
 * platform for it. Relevant when the device specified PULL mode.
 *
 * @param ctx Context
 *
 * @return Error code
 */
WOLK_ERR_T wolk_pull_feed_values(wolk_ctx_t* ctx);

/**
 * @brief Initialized parameter
 *
 * @param parameter Where it's data will be stored
 * @param name parameter name, one of the define at types.h or custom
 * @param value parameter value
 *
 * @return Error code
 */
WOLK_ERR_T wolk_init_parameter(parameter_t* parameter, char* name, char* value);

/**
 * @brief Set parameter value
 *
 * @param parameter Where it's data will be stored
 * @param value parameter value
 * @return
 */
WOLK_ERR_T wolk_set_value_parameter(parameter_t* parameter, char* value);

/**
 * @brief Change existing parameters on the WolkAbout IoT Platform.
 * Updating parameters this way can lead to a misconfigured connection which might prevent further updates. Update
 * connectivity parameters responsibly!
 *
 * @param ctx Context
 * @param parameters List of parameters
 * @param number_of_parameters Number of parameters that is captured
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
 * @param number_of_parameters Number of parameters that is captured
 *
 * @return Error code
 */
WOLK_ERR_T wolk_pull_parameters(wolk_ctx_t* ctx);

/**
 * @brief A request from the device to the WolkAbout IoT platform for the current values of particular device
 * parameters. The payload should contain a list of parameter names that will be contained in the response with it's
 * values.
 *
 * @param ctx Context
 * @param parameters List of parameters
 * @param number_of_parameters Number of parameters that is captured
 *
 * @return Error code
 */
WOLK_ERR_T wolk_sync_parameters(wolk_ctx_t* ctx, parameter_t* parameters, size_t number_of_parameters);

/**
 * @brief This is request for getting WolkAbout IoT Platform's timestamp define as UTC in milliseconds. It will lead to
 * updating ctx->utc to received value.
 *
 * @param ctx Context
 *
 * @return Error code
 */
WOLK_ERR_T wolk_sync_time_request(wolk_ctx_t* ctx);

/**
 * @brief Requesting list of the feeds and attributes from the WolkAbout IoT platform
 *
 * @param ctx Context
 * @return Error code
 */
WOLK_ERR_T wolk_details_synchronization(wolk_ctx_t* ctx);


#ifdef __cplusplus
}
#endif

#endif
