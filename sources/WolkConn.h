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

#define READINGS_SIZE 10
#define READINGS_BUFFER_SIZE 256

#define STR_64 64


typedef struct _wolk_ctx_t wolk_ctx_t;

struct _wolk_ctx_t {
    char payload[PAYLOAD_SIZE];
    char sub_topic[TOPIC_SIZE];
    char pub_topic[TOPIC_SIZE];
    MQTTPacket_connectData data;
    transport_iofunctions_t iof;
    MQTTTransport mytransport;
    int sock;
    parser_t wolk_parser;
    wolk_queue actuator_queue;
    wolk_queue config_queue;
    reading_t readings[READINGS_SIZE];
    int readings_index;
};

typedef int (*send_func)(unsigned char *, unsigned int);
typedef int (*recv_func)(unsigned char *, unsigned int);

//Add parametrs for mikroe
WOLK_ERR_T wolk_connect (wolk_ctx_t *ctx, send_func snd_func, recv_func rcv_func, const char *serial, const char *password);
WOLK_ERR_T wolk_disconnect (wolk_ctx_t *ctx);

//Receiving data
WOLK_ERR_T wolk_receive (wolk_ctx_t *ctx, unsigned int timeout);
WOLK_ERR_T wolk_read_actuator (wolk_ctx_t *ctx, char *command, char *reference, char *value);
WOLK_ERR_T wolk_read_config (wolk_ctx_t *ctx, char *command, char *reference, char *value); // ToDo Add this when it vecomes available
// TODO add more wolk_read if necessary

//Sending data
WOLK_ERR_T wolk_add_string_reading(wolk_ctx_t *ctx,const char *reference,const char *value);
WOLK_ERR_T wolk_add_numeric_reading(wolk_ctx_t *ctx,const char *reference,double value);
WOLK_ERR_T wolk_add_bool_reading(wolk_ctx_t *ctx,const char *reference,bool value);
WOLK_ERR_T wolk_clear_readings (wolk_ctx_t *ctx);
WOLK_ERR_T wolk_publish (wolk_ctx_t *ctx);
WOLK_ERR_T wolk_publish_single (wolk_ctx_t *ctx,const char *reference,const char *value);
// TODO Should we add single string, single numeric, single bool instead of generic psingle publish?
WOLK_ERR_T wolk_publish_num_actuator_status (wolk_ctx_t *ctx,const char *reference,double value, actuator_status_t state);
WOLK_ERR_T wolk_publish_bool_actuator_status (wolk_ctx_t *ctx,const char *reference,bool value, actuator_status_t state);


#ifdef __cplusplus
}
#endif

#endif

