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

#include "WolkConn.h"
#include "MQTTPacket.h"
#include "parser.h"
#include "persistence.h"
#include "in_memory_persistence.h"
#include "outbound_message.h"
#include "outbound_message_factory.h"
#include "wolk_utils.h"

#include <stdarg.h>
#include <string.h>
#include <unistd.h>

#define TIMEOUT_STEP 500000

#define STR_64 64

#define NON_EXISTING "N/A"

#define BOOL_FALSE "false"
#define BOOL_TRUE "true"

#define KEEPALIVE_INTERVAL 60
#define CONFIG_PATH "config/"

#define ACTUATORS_COMMANDS "actuators/commands/"

#define LASWILL_STRING "lastwill/"
#define GONE_OFFLINE "Gone offline"

#define SET_COMMAND "SET"
#define STATUS_COMMAND "STATUS"

static WOLK_ERR_T _wolk_subscribe(wolk_ctx_t *ctx, const char *topic);
static WOLK_ERR_T _wolk_set_parser(wolk_ctx_t *ctx, parser_type_t parser_type);
static WOLK_ERR_T _wolk_publish(wolk_ctx_t *ctx, outbound_message_t* outbound_message);

WOLK_ERR_T wolk_connect (wolk_ctx_t *ctx, send_func snd_func, recv_func rcv_func, const char *device_key, const char *password)
{
    /* Sanity check */
    WOLK_ASSERT(persistence_is_initialized(&ctx->persistence));
    WOLK_ASSERT(parser_is_initialized(&ctx->parser));

    int rc = 0;
    char lastwill_topic[TOPIC_SIZE];
    char sub_topic[TOPIC_SIZE];
    MQTTString lastwill_topic_string = MQTTString_initializer;
    MQTTString lastwill_message_string = MQTTString_initializer;
    wolk_queue_init (&ctx->actuator_queue);
    wolk_queue_init (&ctx->config_queue);

    memset (lastwill_topic, 0, TOPIC_SIZE);
    strcpy (lastwill_topic, LASWILL_STRING);
    strcat (lastwill_topic, device_key);

    lastwill_topic_string.cstring = lastwill_topic;
    lastwill_message_string.cstring = GONE_OFFLINE;

    memset (ctx->serial, 0, SERIAL_SIZE);
    strcpy (ctx->serial, device_key);

    MQTTPacket_connectData data_temp = MQTTPacket_connectData_initializer;

    ctx->data = data_temp;
    ctx->sock = 0;
    unsigned char buf[MQTT_PACKET_SIZE];

    int len = 0;

    memset (buf, 0, MQTT_PACKET_SIZE);

    ctx->iof.send = snd_func;
    ctx->iof.recv = rcv_func;

    ctx->sock = transport_open(&ctx->iof);
    if(ctx->sock < 0)
        return ctx->sock;

    ctx->mqtt_transport.sck = &ctx->sock;
    ctx->mqtt_transport.getfn = transport_getdatanb;
    ctx->mqtt_transport.state = 0;
    ctx->data.clientID.cstring = (char *)device_key;
    ctx->data.keepAliveInterval = KEEPALIVE_INTERVAL;
    ctx->data.cleansession = 1;
    ctx->data.username.cstring = (char *)device_key;
    ctx->data.password.cstring = (char *)password;
    ctx->data.will.topicName = lastwill_topic_string;
    ctx->data.will.message = lastwill_message_string;

    len = MQTTSerialize_connect(buf, MQTT_PACKET_SIZE, &ctx->data);
    rc = transport_sendPacketBuffer(ctx->sock, buf, len);

    if (parser_get_type(&ctx->parser) == PARSER_TYPE_MQTT)
    {
        memset (sub_topic, 0, TOPIC_SIZE);
        strcpy (sub_topic, CONFIG_PATH);
        strcat (sub_topic, device_key);
        if (_wolk_subscribe(ctx, sub_topic)!= W_FALSE)
        {
            return W_TRUE;
        }
    }

    return W_FALSE;
}

WOLK_ERR_T wolk_disconnect(wolk_ctx_t *ctx)
{
    int rc = 0;
    int len = 0;
    unsigned char buf[200];
    int buflen = sizeof(buf);
    len = MQTTSerialize_disconnect(buf, buflen);
    rc = transport_sendPacketBuffer(ctx->sock, buf, len);

    if (rc == -1)
    {
        return W_TRUE;
    }

    return W_FALSE;
}

WOLK_ERR_T wolk_initialize_in_memory_persistence(wolk_ctx_t* ctx, void* storage, uint32_t size, bool wrap)
{
    in_memory_persistence_init(storage, size, wrap);
    persistence_init(&ctx->persistence, in_memory_persistence_push, in_memory_persistence_pop, in_memory_persistence_clear,
                                        in_memory_persistence_is_empty);

    return W_FALSE;
}

WOLK_ERR_T wolk_initialize_custom_persistence(wolk_ctx_t* ctx, persistence_push_t push, persistence_pop_t pop,
                                              persistence_clear_t clear, persistence_is_empty_t is_empty)
{
    persistence_init(&ctx->persistence, push, pop, clear, is_empty);

    return W_FALSE;
}

WOLK_ERR_T wolk_set_protocol (wolk_ctx_t *ctx, protocol_type_t protocol)
{
    if (protocol == PROTOCOL_TYPE_WOLKSENSOR)
    {
        _wolk_set_parser (ctx, PARSER_TYPE_MQTT);
    } else if (protocol == PROTOCOL_TYPE_JSON)
    {
        _wolk_set_parser (ctx, PARSER_TYPE_JSON);
    }

    return W_FALSE;
}

WOLK_ERR_T wolk_set_actuator_references (wolk_ctx_t *ctx, int num_of_items,  const char *item, ...)
{
    va_list argptr;
    char pub_topic[TOPIC_SIZE];
    int i;

    memset(pub_topic, '\0', sizeof(pub_topic));

    if (parser_get_type(&ctx->parser) == PARSER_TYPE_JSON)
    {
        va_start( argptr, item );

        const char* str = item;
        for (i=0;i<num_of_items;i++)
        {
            memset (pub_topic, 0, TOPIC_SIZE);

            strcpy(pub_topic,ACTUATORS_COMMANDS);
            strcat(pub_topic,ctx->serial);
            strcat(pub_topic,"/");
            strcat(pub_topic,str);

            if (_wolk_subscribe(ctx,pub_topic)!=W_FALSE)
            {
                return W_TRUE;
            }

            str = va_arg( argptr, const char* );
        }
        va_end( argptr );
    }

    return W_FALSE;
}

WOLK_ERR_T wolk_receive (wolk_ctx_t *ctx, unsigned int timeout)
{
    int rc = 0;
    size_t i;
    unsigned char buf[200];
    int buflen = sizeof(buf);
    int timeout_counter = 0;
    int timeout_microseconds = timeout * TIMEOUT_STEP * 2;
    actuator_command_t commands_buffer[128];

    while (timeout_counter != timeout_microseconds)
    {
        if((rc = MQTTPacket_readnb(buf, buflen, &(ctx->mqtt_transport))) == PUBLISH)
        {
            unsigned char dup;
            int qos;
            unsigned char retained;
            unsigned short msgid;
            int payloadlen_in;
            unsigned char* payload_in;
            MQTTString receivedTopic;
            char reference[MANIFEST_ITEM_REFERENCE_SIZE];
            memset (reference, 0, sizeof(reference));

            rc = MQTTDeserialize_publish(&dup, &qos, &retained, &msgid, &receivedTopic,
                                         &payload_in, &payloadlen_in, buf, buflen);
            if (rc == -1)
            {
                return W_TRUE;
            }

            if (parser_get_type(&ctx->parser) == PARSER_TYPE_JSON)
            {
                char *end_ptr = strstr (receivedTopic.lenstring.data, "{");
                if (end_ptr != NULL)
                {
                    char *start_ptr = strrchr(receivedTopic.lenstring.data, '/');
                    if (start_ptr != NULL)
                    {
                        strncpy(reference, start_ptr+1, end_ptr-start_ptr-1);
                    }
                }
            }

            size_t num_deserialized_commands = parser_deserialize_commands(&ctx->parser, (char*) payload_in, payloadlen_in, &commands_buffer[0], 128);
            for (i = 0; i < num_deserialized_commands; ++i) {
                actuator_command_t* command = &commands_buffer[i];

                switch(actuator_command_get_type(command))
                {
                case ACTUATOR_COMMAND_TYPE_STATUS:
                    if (parser_get_type(&ctx->parser) == PARSER_TYPE_MQTT)
                    {
                        wolk_queue_push(&ctx->actuator_queue, actuator_command_get_reference(command), STATUS_COMMAND, NON_EXISTING);
                    } else if (parser_get_type(&ctx->parser) == PARSER_TYPE_JSON)
                    {
                        wolk_queue_push(&ctx->actuator_queue, reference, STATUS_COMMAND, NON_EXISTING);
                    }
                    break;

                case ACTUATOR_COMMAND_TYPE_SET:
                    if (parser_get_type(&ctx->parser) == PARSER_TYPE_MQTT)
                    {
                        wolk_queue_push(&ctx->actuator_queue,  actuator_command_get_reference(command), SET_COMMAND, actuator_command_get_value(command));
                    } else if (parser_get_type(&ctx->parser) == PARSER_TYPE_JSON)
                    {
                        wolk_queue_push(&ctx->actuator_queue,  reference, SET_COMMAND, actuator_command_get_value(command));
                    }
                    break;

                case ACTUATOR_COMMAND_TYPE_UNKNOWN:
                    break;
                }
            }

        }
        usleep(TIMEOUT_STEP);
        timeout_counter += TIMEOUT_STEP;
    }
    return W_FALSE;
}

WOLK_ERR_T wolk_read_actuator (wolk_ctx_t *ctx, char *command, char *reference, char *value)
{
    if (wolk_queue_pop(&ctx->actuator_queue, reference, command, value) != Q_FALSE)
    {
        return W_TRUE;
    }

    return W_FALSE;
}

WOLK_ERR_T wolk_read_config(wolk_ctx_t *ctx, char *command, char *reference, char *value)
{
    WOLK_UNUSED(ctx);
    WOLK_UNUSED(command);
    WOLK_UNUSED(reference);
    WOLK_UNUSED(value);

    return W_FALSE;
}

WOLK_ERR_T wolk_add_string_reading(wolk_ctx_t *ctx,const char *reference,const char *value, uint32_t utc_time)
{
    manifest_item_t string_sensor;
    manifest_item_init(&string_sensor, (char *) reference, READING_TYPE_SENSOR, DATA_TYPE_STRING);

    reading_t reading;
    reading_init(&reading, &string_sensor);
    reading_set_data(&reading, (char*) value);
    reading_set_rtc(&reading, utc_time);

    outbound_message_t outbound_message;
    outbound_message_make_from_readings(&ctx->parser, ctx->serial, &reading, 1, &outbound_message);

    return persistence_push(&ctx->persistence, &outbound_message) ? W_FALSE : W_TRUE;
}

WOLK_ERR_T wolk_add_numeric_reading(wolk_ctx_t *ctx,const char *reference,double value, uint32_t utc_time)
{
    manifest_item_t numeric_sensor;
    manifest_item_init(&numeric_sensor, (char *)reference, READING_TYPE_SENSOR, DATA_TYPE_NUMERIC);

    char value_str[STR_64];
    memset (value_str, 0, sizeof(value_str));
    sprintf(value_str, "%f", value);

    reading_t reading;
    reading_init(&reading, &numeric_sensor);
    reading_set_data(&reading, value_str);
    reading_set_rtc(&reading, utc_time);

    outbound_message_t outbound_message;
    outbound_message_make_from_readings(&ctx->parser, ctx->serial, &reading, 1, &outbound_message);

    return persistence_push(&ctx->persistence, &outbound_message) ? W_FALSE : W_TRUE;
}

WOLK_ERR_T wolk_add_bool_reading(wolk_ctx_t *ctx, const char *reference, bool value, uint32_t utc_time)
{
    manifest_item_t bool_sensor;
    manifest_item_init(&bool_sensor, (char *)reference, READING_TYPE_SENSOR, DATA_TYPE_BOOLEAN);

    reading_t reading;
    reading_init(&reading, &bool_sensor);
    reading_set_data(&reading, value == true ? BOOL_TRUE : BOOL_FALSE);
    reading_set_rtc(&reading, utc_time);

    outbound_message_t outbound_message;
    outbound_message_make_from_readings(&ctx->parser, ctx->serial, &reading, 1, &outbound_message);

    return persistence_push(&ctx->persistence, &outbound_message) ? W_FALSE : W_TRUE;
}


WOLK_ERR_T wolk_add_alarm(wolk_ctx_t* ctx, const char* reference, char* message, uint32_t utc_time)
{
    manifest_item_t alarm;
    manifest_item_init(&alarm, (char *)reference, READING_TYPE_ALARM, DATA_TYPE_STRING);

    reading_t alarm_reading;
    reading_init(&alarm_reading, &alarm);
    reading_set_rtc(&alarm_reading, utc_time);
    reading_set_data(&alarm_reading, message);

    outbound_message_t outbound_message;
    outbound_message_make_from_alarm(&ctx->parser, ctx->serial, &alarm_reading, 1, &outbound_message);

    return persistence_push(&ctx->persistence, &outbound_message) ? W_FALSE : W_TRUE;
}

WOLK_ERR_T wolk_clear_readings (wolk_ctx_t *ctx)
{
    persistence_clear(&ctx->persistence);

    return W_FALSE;
}

WOLK_ERR_T wolk_publish (wolk_ctx_t *ctx)
{
    while(!persistence_is_empty(&ctx->persistence))
    {
        outbound_message_t outbound_message;
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

WOLK_ERR_T wolk_publish_single (wolk_ctx_t *ctx, const char *reference, const char *value, data_type_t type, uint32_t utc_time)
{
    manifest_item_t sensor;
    reading_t reading;

    switch(type)
    {
    case DATA_TYPE_STRING:
        manifest_item_init(&sensor, (char *)reference, READING_TYPE_SENSOR, DATA_TYPE_STRING);
        break;

    case DATA_TYPE_BOOLEAN:
        manifest_item_init(&sensor, (char *)reference, READING_TYPE_SENSOR, DATA_TYPE_BOOLEAN);
        break;

    case DATA_TYPE_NUMERIC:
        manifest_item_init(&sensor, (char *)reference, READING_TYPE_SENSOR, DATA_TYPE_NUMERIC);
        break;

    default:
        /* Sanity check */
        WOLK_ASSERT(false);
    }

    reading_init(&reading, &sensor);
    reading_set_data(&reading, (char *)value);
    reading_set_rtc(&reading, utc_time);

    outbound_message_t outbound_message;
    outbound_message_make_from_readings(&ctx->parser, ctx->serial, &reading, 1, &outbound_message);

    if (_wolk_publish (ctx, &outbound_message) != W_FALSE)
    {
        return persistence_push(&ctx->persistence, &outbound_message) ? W_FALSE : W_TRUE;
    }

    return W_FALSE;
}

WOLK_ERR_T wolk_publish_string_actuator_status(wolk_ctx_t* ctx, const char* reference, char* value, actuator_state_t state, uint32_t utc_time)
{
    manifest_item_t string_actuator;
    manifest_item_init(&string_actuator, (char *)reference, READING_TYPE_ACTUATOR, DATA_TYPE_STRING);

    reading_t reading;
    reading_init(&reading, &string_actuator);
    reading_set_rtc(&reading, utc_time);
    reading_set_data(&reading, value);
    reading_set_actuator_state(&reading, state);

    outbound_message_t outbound_message;
    outbound_message_make_from_actuator_status(&ctx->parser, ctx->serial, &reading, 1, &outbound_message);

    if (_wolk_publish (ctx, &outbound_message) != W_FALSE)
    {
        return persistence_push(&ctx->persistence, &outbound_message) ? W_FALSE : W_TRUE;
    }

    return W_FALSE;
}

WOLK_ERR_T wolk_publish_num_actuator_status (wolk_ctx_t *ctx, const char *reference, double value, actuator_state_t state, uint32_t utc_time)
{
    manifest_item_t numeric_actuator;
    manifest_item_init(&numeric_actuator, (char *)reference, READING_TYPE_ACTUATOR, DATA_TYPE_NUMERIC);

    char value_str[STR_64];
    memset (value_str, 0, STR_64);
    sprintf(value_str, "%lf", value);

    reading_t reading;
    reading_init(&reading, &numeric_actuator);
    reading_set_rtc(&reading, utc_time);
    reading_set_data(&reading, value_str);
    reading_set_actuator_state(&reading, state);

    outbound_message_t outbound_message;
    outbound_message_make_from_actuator_status(&ctx->parser, ctx->serial, &reading, 1, &outbound_message);

    if (_wolk_publish (ctx, &outbound_message) != W_FALSE)
    {
        return persistence_push(&ctx->persistence, &outbound_message) ? W_FALSE : W_TRUE;
    }

    return W_FALSE;
}

WOLK_ERR_T wolk_publish_bool_actuator_status (wolk_ctx_t *ctx, const char *reference, bool value, actuator_state_t state, uint32_t utc_time)
{
    manifest_item_t bool_actuator;
    manifest_item_init(&bool_actuator, (char *)reference, READING_TYPE_ACTUATOR, DATA_TYPE_BOOLEAN);

    reading_t reading;
    reading_init(&reading, &bool_actuator);
    reading_set_rtc(&reading, utc_time);
    reading_set_data(&reading, value == true ? BOOL_TRUE : BOOL_FALSE);
    reading_set_actuator_state(&reading, state);

    outbound_message_t outbound_message;
    outbound_message_make_from_actuator_status(&ctx->parser, ctx->serial, &reading, 1, &outbound_message);

    if (_wolk_publish (ctx, &outbound_message) != W_FALSE)
    {
        return persistence_push(&ctx->persistence, &outbound_message) ? W_FALSE : W_TRUE;
    }

    return W_FALSE;
}

WOLK_ERR_T wolk_publish_alarm(wolk_ctx_t* ctx, const char* reference, const char* message, uint32_t utc_time)
{
    manifest_item_t alarm;
    manifest_item_init(&alarm, (char *)reference, READING_TYPE_ALARM, DATA_TYPE_STRING);

    reading_t alarm_reading;
    reading_init(&alarm_reading, &alarm);
    reading_set_rtc(&alarm_reading, utc_time);
    reading_set_data(&alarm_reading, message);

    outbound_message_t outbound_message;
    outbound_message_make_from_alarm(&ctx->parser, ctx->serial, &alarm_reading, 1, &outbound_message);

    if (_wolk_publish (ctx, &outbound_message) != W_FALSE)
    {
        return persistence_push(&ctx->persistence, &outbound_message) ? W_FALSE : W_TRUE;
    }

    return W_FALSE;
}

WOLK_ERR_T wolk_keep_alive (wolk_ctx_t *ctx)
{
    unsigned char buf[MQTT_PACKET_SIZE];
    memset (buf, 0, MQTT_PACKET_SIZE);

    int len = MQTTSerialize_pingreq(buf, MQTT_PACKET_SIZE);
    transport_sendPacketBuffernb_start(ctx->sock, buf, len);

    switch(transport_sendPacketBuffernb(ctx->sock)){
    case TRANSPORT_DONE:
        break;
    case TRANSPORT_ERROR:
        return W_TRUE;
    case TRANSPORT_AGAIN:
        return W_TRUE;
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

    switch(transport_sendPacketBuffernb(ctx->sock)){
    case TRANSPORT_DONE:
        break;
    case TRANSPORT_ERROR:
        return W_TRUE;
        /* Fallthrough */
    case TRANSPORT_AGAIN:
        return W_TRUE;
    }

    return W_FALSE;
}

WOLK_ERR_T _wolk_subscribe (wolk_ctx_t *ctx, const char *topic)
{
    unsigned char buf[MQTT_PACKET_SIZE];
    int msgid = 1;
    int len=0;
    int req_qos = 0;
    int rc = 0;
    MQTTString topicString = MQTTString_initializer;

    memset (buf, 0, MQTT_PACKET_SIZE);


    /* subscribe */
    topicString.cstring = (char *)topic;
    len = MQTTSerialize_subscribe(buf, MQTT_PACKET_SIZE, 0, msgid, 1, &topicString, &req_qos);

    transport_sendPacketBuffernb_start(ctx->sock, buf, len);
    while((rc=transport_sendPacketBuffernb(ctx->sock)) != TRANSPORT_DONE);

    return W_FALSE;
}

WOLK_ERR_T _wolk_set_parser (wolk_ctx_t *ctx, parser_type_t parser_type)
{
    parser_initialize(&ctx->parser, parser_type);

    return W_FALSE;

}
