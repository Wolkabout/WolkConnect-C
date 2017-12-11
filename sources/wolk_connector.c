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

#include "wolk_connector.h"
#include "MQTTPacket.h"
#include "parser.h"
#include "persistence.h"
#include "in_memory_persistence.h"
#include "outbound_message.h"
#include "outbound_message_factory.h"
#include "wolk_utils.h"

#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>

#define TIMEOUT_STEP 100000

#define NON_EXISTING "N/A"

#define BOOL_FALSE "false"
#define BOOL_TRUE "true"

#define KEEPALIVE_INTERVAL 60

#define CONFIG_TOPIC "config/"

#define ACTUATOR_COMMANDS_TOPIC_JSON "actuators/commands/"
#define ACTUATOR_COMMANDS_TOPIC_WOLKSENSE "config/"

#define LASTWILL_TOPIC "lastwill/"
#define LASTWILL_MESSAGE "Gone offline"

#define COMMAND_SET "SET"
#define COMMAND_STATUS "STATUS"

static WOLK_ERR_T _wolk_keep_alive (wolk_ctx_t *ctx);
static WOLK_ERR_T _wolk_receive (wolk_ctx_t *ctx);

static WOLK_ERR_T _wolk_publish(wolk_ctx_t *ctx, outbound_message_t* outbound_message);
static WOLK_ERR_T _wolk_subscribe(wolk_ctx_t *ctx, const char *topic);

static void _wolk_parser_init(wolk_ctx_t* ctx, protocol_t protocol);

static bool _wolk_is_initialized(wolk_ctx_t* ctx);

WOLK_ERR_T wolk_init(wolk_ctx_t* ctx,
                     send_func_t snd_func, recv_func_t rcv_func,
                     actuation_handler_t actuation_handler, actuator_status_provider_t actuator_status_provider,
                     const char* device_key, const char* device_password, protocol_t protocol,
                     const char** actuator_references, uint32_t num_actuator_references)
{
    /* Sanity check */
    WOLK_ASSERT(snd_func != NULL);
    WOLK_ASSERT(rcv_func != NULL);

    WOLK_ASSERT(device_key != NULL);
    WOLK_ASSERT(device_password != NULL);

    WOLK_ASSERT(strlen(device_key) < DEVICE_KEY_SIZE);
    WOLK_ASSERT(strlen(device_password) < DEVICE_PASSWORD_SIZE);

    WOLK_ASSERT(protocol == PROTOCOL_JSON_SINGLE);

    if (num_actuator_references > 0 && (actuation_handler == NULL || actuator_status_provider == NULL))
    {
        WOLK_ASSERT(false);
        return W_TRUE;
    }
    /* Sanity check */

    MQTTPacket_connectData connectData = MQTTPacket_connectData_initializer;
    ctx->connectData = connectData;
    ctx->sock = 0;

    ctx->iof.send = snd_func;
    ctx->iof.recv = rcv_func;

    ctx->sock = transport_open(&ctx->iof);
    if(ctx->sock < 0)
    {
        return W_TRUE;
    }

    strcpy(&ctx->device_key[0], device_key);
    strcpy(&ctx->device_password[0], device_password);

    ctx->mqtt_transport.sck = &ctx->sock;
    ctx->mqtt_transport.getfn = transport_getdatanb;
    ctx->mqtt_transport.state = 0;
    ctx->connectData.clientID.cstring = &ctx->device_key[0];
    ctx->connectData.keepAliveInterval = KEEPALIVE_INTERVAL;
    ctx->connectData.cleansession = 1;
    ctx->connectData.username.cstring = &ctx->device_key[0];
    ctx->connectData.password.cstring = &ctx->device_password[0];

    ctx->actuation_handler = actuation_handler;
    ctx->actuator_status_provider = actuator_status_provider;

    ctx->protocol = protocol;
    _wolk_parser_init(ctx, protocol);

    ctx->actuator_references = actuator_references;
    ctx->num_actuator_references = num_actuator_references;

    ctx->is_initialized = true;

    return W_FALSE;
}

WOLK_ERR_T wolk_init_in_memory_persistence(wolk_ctx_t* ctx, void* storage, uint32_t size, bool wrap)
{
    in_memory_persistence_init(storage, size, wrap);
    persistence_init(&ctx->persistence, in_memory_persistence_push, in_memory_persistence_pop,
                                        in_memory_persistence_is_empty);

    return W_FALSE;
}

WOLK_ERR_T wolk_init_custom_persistence(wolk_ctx_t* ctx, persistence_push_t push, persistence_pop_t pop, persistence_is_empty_t is_empty)
{
    persistence_init(&ctx->persistence, push, pop, is_empty);

    return W_FALSE;
}

WOLK_ERR_T wolk_connect (wolk_ctx_t *ctx)
{
    /* Sanity check */
    WOLK_ASSERT(_wolk_is_initialized(ctx));

    uint64_t i;
    unsigned char buf[MQTT_PACKET_SIZE];
    unsigned char topic_buf[TOPIC_SIZE];

    char lastwill_topic[TOPIC_SIZE];
    MQTTString lastwill_topic_string = MQTTString_initializer;
    MQTTString lastwill_message_string = MQTTString_initializer;

    memset (lastwill_topic, 0, TOPIC_SIZE);
    strcpy (lastwill_topic, LASTWILL_TOPIC);
    strcat (lastwill_topic, ctx->device_key);

    lastwill_topic_string.cstring = lastwill_topic;
    lastwill_message_string.cstring = LASTWILL_MESSAGE;

    ctx->connectData.will.topicName = lastwill_topic_string;
    ctx->connectData.will.message = lastwill_message_string;

    int len = MQTTSerialize_connect(buf, sizeof(buf), &ctx->connectData);
    if(transport_sendPacketBuffer(ctx->sock, buf, len) == TRANSPORT_DONE)
    {
        return W_TRUE;
    }

    for (i = 0; i < ctx->num_actuator_references; ++i)
    {
        const char* reference = ctx->actuator_references[i];
        memset (topic_buf, '\0', sizeof(topic_buf));

        strcpy(topic_buf, ACTUATOR_COMMANDS_TOPIC_JSON);
        strcat(topic_buf, ctx->device_key);
        strcat(topic_buf, "/");
        strcat(topic_buf, reference);

        if (_wolk_subscribe(ctx, topic_buf) != W_FALSE)
        {
            return W_TRUE;
        }
    }

    for (i = 0; i < ctx->num_actuator_references; ++i)
    {
        const char* reference = ctx->actuator_references[i];

        actuator_status_t actuator_status = ctx->actuator_status_provider(reference);
        outbound_message_t outbound_message;
        if (outbound_message_make_from_actuator_status(&ctx->parser, ctx->device_key, &actuator_status, reference, &outbound_message) == 0)
        {
            continue;
        }

        if (_wolk_publish(ctx, &outbound_message) != W_FALSE)
        {
            persistence_push(&ctx->persistence, &outbound_message);
        }
    }

    return W_FALSE;
}

WOLK_ERR_T wolk_disconnect(wolk_ctx_t *ctx)
{
    /* Sanity check */
    WOLK_ASSERT(_wolk_is_initialized(ctx));

    unsigned char buf[MQTT_PACKET_SIZE];

    int len = MQTTSerialize_disconnect(buf, sizeof(buf));
    if(transport_sendPacketBuffer(ctx->sock, buf, len) != TRANSPORT_DONE)
    {
        return W_TRUE;
    }

    return W_FALSE;
}

WOLK_ERR_T wolk_process(wolk_ctx_t* ctx)
{
    /* Sanity check */
    WOLK_ASSERT(_wolk_is_initialized(ctx));

    if (_wolk_keep_alive(ctx) != W_FALSE)
    {
        return W_TRUE;
    }

    if (_wolk_receive(ctx) != W_FALSE)
    {
        return W_TRUE;
    }

    return W_FALSE;
}

WOLK_ERR_T wolk_add_string_sensor_reading(wolk_ctx_t *ctx, const char *reference,const char *value, uint32_t utc_time)
{
    /* Sanity check */
    WOLK_ASSERT(_wolk_is_initialized(ctx));

    manifest_item_t string_sensor;
    manifest_item_init(&string_sensor, (char *) reference, READING_TYPE_SENSOR, DATA_TYPE_STRING);

    reading_t reading;
    reading_init(&reading, &string_sensor);
    reading_set_data(&reading, (char*) value);
    reading_set_rtc(&reading, utc_time);

    outbound_message_t outbound_message;
    outbound_message_make_from_readings(&ctx->parser, ctx->device_key, &reading, 1, &outbound_message);

    return persistence_push(&ctx->persistence, &outbound_message) ? W_FALSE : W_TRUE;
}

WOLK_ERR_T wolk_add_numeric_sensor_reading(wolk_ctx_t *ctx, const char *reference,double value, uint32_t utc_time)
{
    /* Sanity check */
    WOLK_ASSERT(_wolk_is_initialized(ctx));

    manifest_item_t numeric_sensor;
    manifest_item_init(&numeric_sensor, (char *)reference, READING_TYPE_SENSOR, DATA_TYPE_NUMERIC);

    char value_str[READING_SIZE];
    memset (value_str, 0, sizeof(value_str));
    sprintf(value_str, "%f", value);

    reading_t reading;
    reading_init(&reading, &numeric_sensor);
    reading_set_data(&reading, value_str);
    reading_set_rtc(&reading, utc_time);

    outbound_message_t outbound_message;
    outbound_message_make_from_readings(&ctx->parser, ctx->device_key, &reading, 1, &outbound_message);

    return persistence_push(&ctx->persistence, &outbound_message) ? W_FALSE : W_TRUE;
}

WOLK_ERR_T wolk_add_bool_sensor_reading(wolk_ctx_t *ctx, const char *reference, bool value, uint32_t utc_time)
{
    /* Sanity check */
    WOLK_ASSERT(_wolk_is_initialized(ctx));

    manifest_item_t bool_sensor;
    manifest_item_init(&bool_sensor, (char *)reference, READING_TYPE_SENSOR, DATA_TYPE_BOOLEAN);

    reading_t reading;
    reading_init(&reading, &bool_sensor);
    reading_set_data(&reading, value == true ? BOOL_TRUE : BOOL_FALSE);
    reading_set_rtc(&reading, utc_time);

    outbound_message_t outbound_message;
    outbound_message_make_from_readings(&ctx->parser, ctx->device_key, &reading, 1, &outbound_message);

    return persistence_push(&ctx->persistence, &outbound_message) ? W_FALSE : W_TRUE;
}


WOLK_ERR_T wolk_add_alarm(wolk_ctx_t* ctx, const char* reference, char* message, uint32_t utc_time)
{
    /* Sanity check */
    WOLK_ASSERT(_wolk_is_initialized(ctx));

    manifest_item_t alarm;
    manifest_item_init(&alarm, (char *)reference, READING_TYPE_ALARM, DATA_TYPE_STRING);

    reading_t alarm_reading;
    reading_init(&alarm_reading, &alarm);
    reading_set_rtc(&alarm_reading, utc_time);
    reading_set_data(&alarm_reading, message);

    outbound_message_t outbound_message;
    outbound_message_make_from_readings(&ctx->parser, ctx->device_key, &alarm_reading, 1, &outbound_message);

    return persistence_push(&ctx->persistence, &outbound_message) ? W_FALSE : W_TRUE;
}

WOLK_ERR_T wolk_publish_actuator_status (wolk_ctx_t *ctx, const char* reference)
{
    /* Sanity check */
    WOLK_ASSERT(_wolk_is_initialized(ctx));

    if (ctx->actuator_status_provider != NULL)
    {
        actuator_status_t actuator_status = ctx->actuator_status_provider(reference);

        outbound_message_t outbound_message;
        if (outbound_message_make_from_actuator_status(&ctx->parser, ctx->device_key, &actuator_status, reference, &outbound_message) == 0)
        {
            return W_TRUE;
        }

        if (_wolk_publish(ctx, &outbound_message) != W_FALSE)
        {
            persistence_push(&ctx->persistence, &outbound_message);
        }
    }

    return W_FALSE;
}

WOLK_ERR_T wolk_publish (wolk_ctx_t *ctx)
{
    /* Sanity check */
    WOLK_ASSERT(_wolk_is_initialized(ctx));

    uint8_t i;
    uint16_t batch_size = 50;
    outbound_message_t outbound_message;

    for (i = 0; i < batch_size; ++i)
    {
        if (persistence_is_empty(&ctx->persistence))
        {
            return W_FALSE;
        }

        if (!persistence_pop(&ctx->persistence, &outbound_message))
        {
            continue;
        }

        if (_wolk_publish (ctx, &outbound_message) != W_FALSE)
        {
            return W_TRUE;
        }
    }

    return W_FALSE;
}

WOLK_ERR_T _wolk_keep_alive (wolk_ctx_t *ctx)
{
    unsigned char buf[MQTT_PACKET_SIZE];
    memset (buf, 0, MQTT_PACKET_SIZE);

    int len = MQTTSerialize_pingreq(buf, MQTT_PACKET_SIZE);
    transport_sendPacketBuffernb_start(ctx->sock, buf, len);

    do
    {
        switch (transport_sendPacketBuffernb(ctx->sock)) {
        case TRANSPORT_DONE:
            return W_FALSE;

        case TRANSPORT_ERROR:
            return W_TRUE;

        case TRANSPORT_AGAIN:
            continue;

        default:
            /* Sanity check */
            WOLK_ASSERT(false);
            return W_TRUE;
        }
    } while(true);
}

WOLK_ERR_T _wolk_receive (wolk_ctx_t *ctx)
{
    unsigned char buf[PAYLOAD_SIZE];
    int buflen = sizeof(buf);

    if(MQTTPacket_readnb(buf, buflen, &(ctx->mqtt_transport)) == PUBLISH)
    {
        unsigned char dup;
        int qos;
        unsigned char retained;
        unsigned short msgid;
        int payloadlen_in;
        unsigned char* payload_in;
        MQTTString receivedTopic;

        actuator_command_t actuator_command;
        char* reference;
        char* value;

        if (MQTTDeserialize_publish(&dup, &qos, &retained, &msgid, &receivedTopic,
                                    &payload_in, &payloadlen_in, buf, buflen) != 1)
        {
            return W_TRUE;
        }

        size_t num_deserialized_commands = parser_deserialize_commands(&ctx->parser, (char*) payload_in, payloadlen_in, &actuator_command, 1);
        if (num_deserialized_commands == 0)
        {
            return W_FALSE;
        }

        if (ctx->protocol == PROTOCOL_JSON_SINGLE)
        {
            char *end_ptr = strstr (receivedTopic.lenstring.data, "{");
            if (end_ptr != NULL)
            {
                char *start_ptr = strrchr(receivedTopic.lenstring.data, '/');
                if (start_ptr != NULL)
                {
                    strncpy(&actuator_command.reference[0], start_ptr+1, end_ptr-start_ptr-1);
                }
            }
        }

        reference = actuator_command_get_reference(&actuator_command);
        value = actuator_command_get_value(&actuator_command);
        switch(actuator_command_get_type(&actuator_command))
        {
        case ACTUATOR_COMMAND_TYPE_SET:
            if (ctx->actuation_handler != NULL)
            {
                ctx->actuation_handler(reference, value);
            }

            /* Fallthrough */
            /* break; */

        case ACTUATOR_COMMAND_TYPE_STATUS:
            if (ctx->actuator_status_provider != NULL)
            {
                actuator_status_t actuator_status = ctx->actuator_status_provider(reference);

                outbound_message_t outbound_message;
                if (outbound_message_make_from_actuator_status(&ctx->parser, ctx->device_key, &actuator_status, reference, &outbound_message) > 0)
                {
                    _wolk_publish(ctx, &outbound_message);
                }
            }
            break;

        case ACTUATOR_COMMAND_TYPE_UNKNOWN:
            break;
        }
    }

    return W_FALSE;
}

WOLK_ERR_T _wolk_publish (wolk_ctx_t *ctx, outbound_message_t* outbound_message)
{
    int len;
    unsigned char buf[MQTT_PACKET_SIZE];
    memset (buf, 0, MQTT_PACKET_SIZE);

    MQTTString mqtt_topic = MQTTString_initializer;
    mqtt_topic.cstring = outbound_message_get_topic(outbound_message);

    unsigned char* payload = outbound_message_get_payload(outbound_message);
    len = MQTTSerialize_publish(buf, MQTT_PACKET_SIZE, 0, 0, 0, 0, mqtt_topic, payload, strlen(payload));
    transport_sendPacketBuffernb_start(ctx->sock, buf, len);

    do
    {
        switch (transport_sendPacketBuffernb(ctx->sock)) {
        case TRANSPORT_DONE:
            return W_FALSE;
            break;

        case TRANSPORT_ERROR:
            return W_TRUE;
            break;

        case TRANSPORT_AGAIN:
            continue;
            break;

        default:
            /* Sanity check */
            WOLK_ASSERT(false);
            return W_TRUE;
        }
    } while(true);
}

WOLK_ERR_T _wolk_subscribe (wolk_ctx_t *ctx, const char *topic)
{
    unsigned char buf[MQTT_PACKET_SIZE];
    int msgid = 1;
    int len=0;
    int req_qos = 0;
    MQTTString topicString = MQTTString_initializer;

    memset (buf, 0, MQTT_PACKET_SIZE);


    /* subscribe */
    topicString.cstring = (char *)topic;
    len = MQTTSerialize_subscribe(buf, MQTT_PACKET_SIZE, 0, msgid, 1, &topicString, &req_qos);

    transport_sendPacketBuffernb_start(ctx->sock, buf, len);

    do
    {
        switch (transport_sendPacketBuffernb(ctx->sock)) {
        case TRANSPORT_DONE:
            return W_FALSE;
            break;

        case TRANSPORT_ERROR:
            return W_TRUE;
            break;

        case TRANSPORT_AGAIN:
            continue;
            break;

        default:
            /* Sanity check */
            WOLK_ASSERT(false);
            return W_TRUE;
        }
    } while(true);

    return W_FALSE;
}

void _wolk_parser_init(wolk_ctx_t* ctx, protocol_t protocol)
{
    switch(protocol)
    {
    case PROTOCOL_JSON_SINGLE:
        parser_init(&ctx->parser, PARSER_TYPE_JSON);
        break;

    default:
        /* Sanity check */
        WOLK_ASSERT(false);
    }
}

bool _wolk_is_initialized(wolk_ctx_t* ctx)
{
    return ctx->is_initialized && persistence_is_initialized(&ctx->persistence);
}
