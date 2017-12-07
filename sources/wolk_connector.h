/*
 * Copyright 2017 WolkAbout Technology s.r.o.
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
#include "transport.h"
#include "parser.h"
#include "persistence.h"
#include "size_definitions.h"
#include "actuator_status.h"

#include <stdbool.h>
#include <stdint.h>

enum {
    WOLK_VERSION_MAJOR = 2,
    WOLK_VERSION_MINOR = 0,
    WOLK_VERSION_PATCH = 0
};

typedef enum {
    PROTOCOL_JSON_SINGLE = 0
} protocol_t;

/**
 * @brief WOLK_ERR_T Boolean used for error handling in WolkConnect-C library
 */
typedef unsigned char WOLK_ERR_T;

/**
 * @brief WOLK_BOOL_T Boolean used in WolkConnect-C library
 */
typedef unsigned char WOLK_BOOL_T;
enum WOLK_BOOL_T_values {
    W_FALSE = 0,
    W_TRUE = 1
};

/**
 * @brief Callback for writting bytes to socket
 */
typedef int (*send_func_t)(unsigned char* bytes, unsigned int num_bytes);

/**
 * @brief Callback for reading bytes from socket
 */
typedef int (*recv_func_t)(unsigned char* bytes, unsigned int num_bytes);

typedef void (*actuation_handler_t)(const char* reference, const char* value);
typedef actuator_status_t (*actuator_status_provider_t)(const char* reference);

typedef struct {
    int sock;
    MQTTPacket_connectData connectData;
    MQTTTransport mqtt_transport;
    transport_iofunctions_t iof;

    actuation_handler_t actuation_handler;
    actuator_status_provider_t actuator_status_provider;

    char device_key[DEVICE_KEY_SIZE];
    char device_password[DEVICE_PASSWORD_SIZE];

    protocol_t protocol;
    parser_t parser;

    persistence_t persistence;

    const char** actuator_references;
    uint32_t num_actuator_references;

    bool is_initialized;
} wolk_ctx_t;

/**
 * @brief Initializes WolkAbout IoT platform connector context
 *
 * @param ctx Context
 *
 * @param snd_func Callback function that handles outgoing traffic
 * @param rcv_func Callback function that handles incoming traffic
 *
 * @param device_key Device key provided by WolkAbout IoT Platform upon device creation
 * @param password Device password provided by WolkAbout IoT platform device upon device creation
 * @param protocol Protocol specified for device
 *
 * @param actuator_references Array of strings containing references of actuators that device possess
 * @param num_actuator_references Number of actuator references contained in actuator_references
 *
 * @return Error code
 */
WOLK_ERR_T wolk_init(wolk_ctx_t* ctx,
                     send_func_t snd_func, recv_func_t rcv_func,
                     actuation_handler_t actuation_handler, actuator_status_provider_t actuator_status_provider,
                     const char *device_key, const char *device_password, protocol_t protocol,
                     const char** actuator_references, uint32_t num_actuator_references);

/**
 * @brief Initializes persistence mechanism with in-memory implementation
 *
 * @param ctx Context
 * @param storage Address to start of the memory which will be used by persistence mechanism
 * @param size Size of memory in bytes
 * @param wrap If storage is full overwrite oldest item when pushing new item
 *
 * @return Error code
 */
WOLK_ERR_T wolk_init_in_memory_persistence(wolk_ctx_t *ctx, void* storage, uint32_t size, bool wrap);

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
 * @see persistence.h for signature of methods to be implemented, and implementation contract
 */
WOLK_ERR_T wolk_init_custom_persistence(wolk_ctx_t *ctx,
                                        persistence_push_t push, persistence_pop_t pop,
                                        persistence_is_empty_t is_empty);

/**
 * @brief Connect to WolkAbout IoT Platform
 *
 * Prior to connecting, following must be performed:
 *  1. Context must be initialized via wolk_init(wolk_ctx_t* ctx, send_func snd_func, recv_func rcv_func,
 *                                               const char *device_key, const char *password, protocol_t protocol)
 *  2. Persistence must be initialized using
 *      wolk_initialize_in_memory_persistence(wolk_ctx_t *ctx, void* storage, uint16_t num_elements, bool wrap)
 *      or
 *      wolk_initialize_custom_persistence(wolk_ctx_t *ctx,
 *                                         persistence_push_t push, persistence_pop_t pop,
 *                                         persistence_is_empty_t is_empty, persistence_size_t size)
 *
 * @param ctx Context
 *
 * @return Error code
 */
WOLK_ERR_T wolk_connect(wolk_ctx_t *ctx);

/**
 * @brief Disconnect from WolkAbout IoT Platform
 *
 * @param ctx Context
 *
 * @return Error code
 */
WOLK_ERR_T wolk_disconnect(wolk_ctx_t *ctx);

/**
 * @brief Must be called periodically to keep alive connection to WolkAbout IoT platform, obtain and perform
 * actuation requests
 *
 * @param ctx Context
 * @param timeout TODO
 *
 * @return Error code
 */
WOLK_ERR_T wolk_process(wolk_ctx_t *ctx);

/** @brief Add string reading
 *
 *  @param ctx Context
 *  @param reference Sensor reference
 *  @param value Sensor value
 *  @param utc_time UTC time of sensor value acquisition [seconds]
 *
 *  @return Error code
 */
WOLK_ERR_T wolk_add_string_sensor_reading(wolk_ctx_t *ctx,const char *reference,const char *value, uint32_t utc_time);

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
WOLK_ERR_T wolk_add_numeric_sensor_reading(wolk_ctx_t *ctx,const char *reference,double value, uint32_t utc_time);

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
WOLK_ERR_T wolk_add_bool_sensor_reading(wolk_ctx_t *ctx, const char *reference, bool value, uint32_t utc_time);

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
WOLK_ERR_T wolk_add_alarm(wolk_ctx_t *ctx, const char *reference, char* message, uint32_t utc_time);

/**
 * @brief Obtains actuator status via actuator_status_provider_t and publishes it.
 * If actuator status can not be published, it is persisted.
 *
 * @param ctx Context
 * @param reference Actuator reference
 *
 * @return Error code
 */
WOLK_ERR_T wolk_publish_actuator_status (wolk_ctx_t *ctx, const char* reference);

/**
 * @brief Publish accumulated sensor readings, and alarms
 *
 * @param ctx Context
 *
 * @return Error code
 */
WOLK_ERR_T wolk_publish (wolk_ctx_t *ctx);

#ifdef __cplusplus
}
#endif

#endif
