#ifndef WOLK_H
#define WOLK_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include "MQTTPacket.h"
#include "transport.h"
#include "WolkQueue.h"
#include "parser.h"

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

#define PAYLOAD_SIZE 256
#define TOPIC_SIZE 64
#define SERIAL_SIZE 32

#define READINGS_SIZE 10
#define READINGS_BUFFER_SIZE 192
#define READINGS_MQTT_SIZE 256

#define STR_64 64

typedef struct _wolk_ctx_t wolk_ctx_t;

struct _wolk_ctx_t {
    char serial[SERIAL_SIZE];
    MQTTPacket_connectData data;
    transport_iofunctions_t iof;
    MQTTTransport mqtt_transport;
    int sock;
    parser_t wolk_parser;
    wolk_queue actuator_queue;
    wolk_queue config_queue;
    reading_t readings[READINGS_SIZE];
    int readings_index;
    parser_type_t parser_type;
};

typedef enum {
    PROTOCOL_TYPE_WOLKSENSOR = 0,
    PROTOCOL_TYPE_JSON
} protocol_type_t;

typedef int (*send_func)(unsigned char *, unsigned int);
typedef int (*recv_func)(unsigned char *, unsigned int);



WOLK_ERR_T wolk_connect (wolk_ctx_t *ctx, send_func snd_func, recv_func rcv_func, const char *device_key, const char *password);
WOLK_ERR_T wolk_disconnect (wolk_ctx_t *ctx);
WOLK_ERR_T wolk_set_protocol (wolk_ctx_t *ctx, protocol_type_t protocol);
WOLK_ERR_T wolk_set_actuator_references (wolk_ctx_t *ctx, int num_of_items, const char *item, ...);

//Receiving data
WOLK_ERR_T wolk_receive (wolk_ctx_t *ctx, unsigned int timeout);
WOLK_ERR_T wolk_read_actuator (wolk_ctx_t *ctx, char *command, char *reference, char *value);
WOLK_ERR_T wolk_read_config (wolk_ctx_t *ctx, char *command, char *reference, char *value);

//Sending data
WOLK_ERR_T wolk_add_string_reading(wolk_ctx_t *ctx,const char *reference,const char *value, uint32_t utc_time);
WOLK_ERR_T wolk_add_numeric_reading(wolk_ctx_t *ctx,const char *reference,double value, uint32_t utc_time);
WOLK_ERR_T wolk_add_bool_reading(wolk_ctx_t *ctx,const char *reference,bool value, uint32_t utc_time);
WOLK_ERR_T wolk_clear_readings (wolk_ctx_t *ctx);
WOLK_ERR_T wolk_publish (wolk_ctx_t *ctx);
WOLK_ERR_T wolk_publish_single (wolk_ctx_t *ctx,const char *reference,const char *value, data_type_t type, uint32_t utc_time);
WOLK_ERR_T wolk_publish_num_actuator_status (wolk_ctx_t *ctx,const char *reference,double value, actuator_status_t state, uint32_t utc_time);
WOLK_ERR_T wolk_publish_bool_actuator_status (wolk_ctx_t *ctx,const char *reference,bool value, actuator_status_t state, uint32_t utc_time);
WOLK_ERR_T wolk_keep_alive (wolk_ctx_t *ctx);



#ifdef __cplusplus
}
#endif

#endif

