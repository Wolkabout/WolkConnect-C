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
#include "WolkQueue.h"
#include "parser.h"
#include "persistence.h"
#include "size_definitions.h"

#include <stdbool.h>

enum {
    WOLK_VERSION_MAJOR = 2,
    WOLK_VERSION_MINOR = 0,
    WOLK_VERSION_PATCH = 0
};

/**
 * @brief WOLK_ERR_T Boolean used for error handling in Wolk conenction module
 */
typedef unsigned char WOLK_ERR_T;
/**
 * @brief WOLK_ERR_T Boolean used in Wolk connection module
 */
typedef unsigned char WOLK_BOOL_T;

#define W_TRUE 1
#define W_FALSE 0

typedef struct _wolk_ctx_t wolk_ctx_t;

struct _wolk_ctx_t {
    int sock;
    char serial[SERIAL_SIZE];
    MQTTPacket_connectData data;
    transport_iofunctions_t iof;
    MQTTTransport mqtt_transport;

    parser_t parser;

    wolk_queue actuator_queue;
    wolk_queue config_queue;

    persistence_t persistence;
};

typedef enum {
    PROTOCOL_TYPE_WOLKSENSOR = 0,
    PROTOCOL_TYPE_JSON
} protocol_type_t;

typedef int (*send_func)(unsigned char *, unsigned int);
typedef int (*recv_func)(unsigned char *, unsigned int);

/**
 * @brief Connect to WolkAbout IoT Platform
 *
 * Prior to connecting, following must be performed:
 *  1. Protocol must be set using wolk_set_protocol(wolk_ctx_t *ctx, protocol_type_t protocol)
 *  2. Persistence must be initialized using wolk_initialize_in_memory_persistence(wolk_ctx_t *ctx, void* storage, uint16_t num_elements, bool wrap)
 *      or wolk_initialize_custom_persistence(wolk_ctx_t *ctx, persistence_push_t push, persistence_pop_t pop,
 *                                            persistence_clear_t clear, persistence_is_empty_t is_empty, persistence_size_t size)
 *
 * @param ctx Context
 * @param snd_func Function callback that will handle outgoing traffic
 * @param rcv_func Function callback that will handle incoming traffic
 * @param device_key Device key provided by WolkAbout IoT Platform upon device creation
 * @param password Device password provided by WolkAbout IoT platform device upon device creation
 *
 * @return Error code
 */
WOLK_ERR_T wolk_connect(wolk_ctx_t *ctx, send_func snd_func, recv_func rcv_func, const char *device_key, const char *password);

/**
 * @brief Disconnect from WolkAbout IoT Platform
 *
 * @param ctx Context
 *
 * @return Error code
 */
WOLK_ERR_T wolk_disconnect(wolk_ctx_t *ctx);

/**
 * @brief Initializes persistence mechanism with in-memory implementation
 *
 * @param ctx Context
 * @param storage Address to start of the memory which will be used by persistence mechanism
 * @param size Size of memory in bytes
 * @param wrap Overwrite oldest item if storage is full
 *
 * @return Error code
 */
WOLK_ERR_T wolk_initialize_in_memory_persistence(wolk_ctx_t *ctx, void* storage, uint32_t size, bool wrap);

/**
 * @brief Initializes persistence mechanism with custom implementation
 *
 * @param ctx Context
 * @param push Function pointer to 'push' implemenation
 * @param pop Function pointer to 'pop' implementation
 * @param clear Function pointer to 'clear' implementation
 * @param is_empty Function pointer to 'is empty' implementation
 *
 * @return Error code
 *
 * @see persistence.h for signature of methods to be implemented, and implementation contract
 */
WOLK_ERR_T wolk_initialize_custom_persistence(wolk_ctx_t *ctx, persistence_push_t push, persistence_pop_t pop,
                                              persistence_clear_t clear, persistence_is_empty_t is_empty);

/**
 * @brief Set underlying protocol
 *
 * @param ctx Context
 * @param protocol Protocol to use
 *
 * @return Error code
 */
WOLK_ERR_T wolk_set_protocol(wolk_ctx_t *ctx, protocol_type_t protocol);

/**
 * @brief Set references of actuators used by device
 *
 * @param ctx Context
 * @param num_of_items Number of references
 * @param item Actuator reference(s)
 *
 * @return Error code
 */
WOLK_ERR_T wolk_set_actuator_references (wolk_ctx_t *ctx, int num_of_items, const char *item, ...);

/**
 * @brief Receive messages.
 * Must be called periodically in order to receive messages from WolkAbout IoT Platform
 *
 * @param ctx Context
 * @param timeout Read timeout
 *
 * @return Error code
 */
WOLK_ERR_T wolk_receive (wolk_ctx_t *ctx, unsigned int timeout);

/**
 * @brief Obtain actuation request
 *
 * @param ctx Context
 * @param command Actuation request name
 * @param reference Actuation request reference
 * @param value Actuation request value
 *
 * @return Error code
 */
WOLK_ERR_T wolk_read_actuator (wolk_ctx_t *ctx, char *command, char *reference, char *value);

/**
 * @brief Under contruction
 */
WOLK_ERR_T wolk_read_config (wolk_ctx_t *ctx, char *command, char *reference, char *value);

/** @brief Add string reading
 *
 *  @param ctx Context
 *  @param reference Sensor reference
 *  @param value Sensor value
 *  @param utc_time UTC time of sensor value acquisition [seconds]
 *
 *  @return Error code
 */
WOLK_ERR_T wolk_add_string_reading(wolk_ctx_t *ctx,const char *reference,const char *value, uint32_t utc_time);

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
WOLK_ERR_T wolk_add_numeric_reading(wolk_ctx_t *ctx,const char *reference,double value, uint32_t utc_time);

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
WOLK_ERR_T wolk_add_bool_reading(wolk_ctx_t *ctx, const char *reference, bool value, uint32_t utc_time);

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
 * @brief Clear acumulated sensor readings
 *
 * @param ctx Context
 *
 * @return Error code
 */
WOLK_ERR_T wolk_clear_readings (wolk_ctx_t *ctx);

/**
 * @brief Publish accumulated readings
 *
 * @param ctx Context
 *
 * @return Error code
 */
WOLK_ERR_T wolk_publish (wolk_ctx_t *ctx);

/**
 * @brief Publish single sensor reading.
 * If sensor reading can not be published it will be persisted, and sent with next successful wolk_publish(wolk_ctx_t* ctx)
 *
 * @param ctx Context
 * @param reference Sensor reference
 * @param value Sensor value
 * @param type Parameter type. Available values are: DATA_TYPE_NUMERIC, DATA_TYPE_BOOLEAN, DATA_TYPE_STRING
 * @param utc_time UTC time of sensor value acquisition [seconds]
 *
 * @return Error code
 */
WOLK_ERR_T wolk_publish_single (wolk_ctx_t *ctx,const char *reference,const char *value, data_type_t type, uint32_t utc_time);

/**
 * @brief Publish string actuator status.
 * If actuator status can not be published it will be persisted, and sent with next successful wolk_publish(wolk_ctx_t* ctx)
 *
 * @param ctx Context
 * @param reference Actuator reference
 * @param value Actuator value
 * @param state Actuator state
 * @param utc_time UTC time of actuator value acquisition [seconds]
 *
 * @return Error code
 */
WOLK_ERR_T wolk_publish_string_actuator_status (wolk_ctx_t* ctx,const char* reference, char* value, actuator_state_t state, uint32_t utc_time);

/**
 * @brief Publish numeric actuator status.
 * If actuator status can not be published it will be persisted, and sent with next successful wolk_publish(wolk_ctx_t* ctx)
 *
 * @param ctx Context
 * @param reference Actuator reference
 * @param value Actuator value
 * @param state Actuator state
 * @param utc_time UTC time of actuator value acquisition [seconds]
 * @return Error code
 */
WOLK_ERR_T wolk_publish_num_actuator_status (wolk_ctx_t *ctx,const char *reference, double value, actuator_state_t state, uint32_t utc_time);

/**
 * @brief Publish boolean actuator status.
 * If actuator status can not be published it will be persisted, and sent with next successful wolk_publish(wolk_ctx_t* ctx)
 *
 * @param ctx Context
 * @param reference Actuator reference
 * @param value Actuator value
 * @param state Actuator state
 * @param utc_time UTC time of actuator value acquisition [seconds]
 *
 * @return Error code
 */
WOLK_ERR_T wolk_publish_bool_actuator_status (wolk_ctx_t *ctx, const char *reference, bool value, actuator_state_t state, uint32_t utc_time);

/**
 * @brief Publish alarm
 *
 * @param ctx Context
 * @param reference Alarm reference
 * @param message Alarm message
 * @param utc_time UTC time of actuator value acquisition [seconds]
 *
 * @return Error code
 */
WOLK_ERR_T wolk_publish_alarm (wolk_ctx_t *ctx, const char *reference, const char* message, uint32_t utc_time);

/**
 * @brief Keep alive connection
 *
 * Must be called periodically in order to keep alive connection to WolkAbout IoT Platform
 *
 * @param ctx Context
 * @return Error code
 */
WOLK_ERR_T wolk_keep_alive (wolk_ctx_t *ctx);

#ifdef __cplusplus
}
#endif

#endif
