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

#include "wolk_connector.h"
#include "MQTTPacket.h"
#include "actuator_command.h"
#include "configuration_item.h"//promena
#include "configuration_command.h"
#include "firmware_update.h"
#include "firmware_update_command.h"
#include "firmware_update_packet.h"
#include "in_memory_persistence.h"
#include "outbound_message.h"
#include "outbound_message_factory.h"
#include "parser.h"
#include "persistence.h"
#include "wolk_utils.h"

#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#define NON_EXISTING "N/A"

#define MQTT_KEEP_ALIVE_INTERVAL (600 * 1000)

#define PING_KEEP_ALIVE_INTERVAL (600 * 1000)

static const char* ACTUATOR_COMMANDS_TOPIC_JSON = "actuators/commands/";

static const char* FIRMWARE_UPDATE_COMMANDS_TOPIC_JSON = "service/commands/firmware/";
static const char* FIRMWARE_UPDATE_PACKET_TOPIC_JSON = "service/binary/";

static const char* CONFIGURATION_COMMANDS = "configurations/commands/";

static const char* LASTWILL_TOPIC = "lastwill/";
static const char* LASTWILL_MESSAGE = "Gone offline";

static WOLK_ERR_T _mqtt_keep_alive(wolk_ctx_t* ctx, uint32_t tick);
static WOLK_ERR_T _ping_keep_alive(wolk_ctx_t* ctx, uint32_t tick);

static WOLK_ERR_T _receive(wolk_ctx_t* ctx);

static WOLK_ERR_T _publish(wolk_ctx_t* ctx, outbound_message_t* outbound_message);
static WOLK_ERR_T _subscribe(wolk_ctx_t* ctx, const char* topic);

static void _parser_init(wolk_ctx_t* ctx, protocol_t protocol);

static bool _is_wolk_initialized(wolk_ctx_t* ctx);

static void _handle_actuator_command(wolk_ctx_t* ctx, actuator_command_t* actuator_command);
static void _handle_configuration_command(wolk_ctx_t* ctx, configuration_command_t* configuration_command);
static void _handle_firmware_update_command(firmware_update_t* firmware_update,
                                            firmware_update_command_t* firmware_update_command);
static void _handle_firmware_update_packet(firmware_update_t* firmware_update, uint8_t* packet, size_t packet_size);

static void _listener_on_status(firmware_update_t* firmware_update, firmware_update_status_t status);
static void _listener_on_packet_request(firmware_update_t* firmware_update, firmware_update_packet_request_t request);

WOLK_ERR_T wolk_init(wolk_ctx_t* ctx, send_func_t snd_func, recv_func_t rcv_func, actuation_handler_t actuation_handler,
                     actuator_status_provider_t actuator_status_provider, configuration_handler_t configuration_handler,
                     configuration_provider_t configuration_provider, const char* device_key,
                     const char* device_password, protocol_t protocol, const char** actuator_references,
                     uint32_t num_actuator_references)
{
    /* Sanity check */
    WOLK_ASSERT(snd_func != NULL);
    WOLK_ASSERT(rcv_func != NULL);

    WOLK_ASSERT(device_key != NULL);
    WOLK_ASSERT(device_password != NULL);

    WOLK_ASSERT(strlen(device_key) < DEVICE_KEY_SIZE);
    WOLK_ASSERT(strlen(device_password) < DEVICE_PASSWORD_SIZE);

    WOLK_ASSERT(protocol == PROTOCOL_JSON_SINGLE);

    if (num_actuator_references > 0 && (actuation_handler == NULL || actuator_status_provider == NULL)) {
        WOLK_ASSERT(false);
        return W_TRUE;
    }

    if ((configuration_handler != NULL && configuration_provider == NULL)
        || (configuration_handler == NULL && configuration_provider != NULL)) {
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
    if (ctx->sock < 0) {
        return W_TRUE;
    }

    strcpy(&ctx->device_key[0], device_key);
    strcpy(&ctx->device_password[0], device_password);

    ctx->mqtt_transport.sck = &ctx->sock;
    ctx->mqtt_transport.getfn = transport_getdatanb;
    ctx->mqtt_transport.state = 0;
    ctx->connectData.clientID.cstring = &ctx->device_key[0];
    ctx->connectData.keepAliveInterval = MQTT_KEEP_ALIVE_INTERVAL;
    ctx->connectData.cleansession = 1;
    ctx->connectData.username.cstring = &ctx->device_key[0];
    ctx->connectData.password.cstring = &ctx->device_password[0];

    ctx->actuation_handler = actuation_handler;
    ctx->actuator_status_provider = actuator_status_provider;

    ctx->configuration_handler = configuration_handler;
    ctx->configuration_provider = configuration_provider;

    ctx->protocol = protocol;
    _parser_init(ctx, protocol);

    ctx->actuator_references = actuator_references;
    ctx->num_actuator_references = num_actuator_references;

	



    ctx->is_keep_alive_enabled = true;
    ctx->milliseconds_since_last_ping_keep_alive = PING_KEEP_ALIVE_INTERVAL;

    ctx->is_initialized = true;

    wolk_init_firmware_update(ctx, "", 0, 0, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
    return W_FALSE;
}

WOLK_ERR_T wolk_init_in_memory_persistence(wolk_ctx_t* ctx, void* storage, uint32_t size, bool wrap)
{
    in_memory_persistence_init(storage, size, wrap);
    persistence_init(&ctx->persistence, in_memory_persistence_push, in_memory_persistence_peek,
                     in_memory_persistence_pop, in_memory_persistence_is_empty);

    return W_FALSE;
}

WOLK_ERR_T wolk_init_custom_persistence(wolk_ctx_t* ctx, persistence_push_t push, persistence_peek_t peek,
                                        persistence_pop_t pop, persistence_is_empty_t is_empty)
{
    persistence_init(&ctx->persistence, push, peek, pop, is_empty);

    return W_FALSE;
}

WOLK_ERR_T wolk_init_firmware_update(wolk_ctx_t* ctx, const char* version, size_t maximum_firmware_size,
                                     size_t chunk_size, firmware_update_start_t start,
                                     firmware_update_write_chunk_t write_chunk, firmware_update_read_chunk_t read_chunk,
                                     firmware_update_abort_t abort, firmware_update_finalize_t finalize,
                                     firmware_update_persist_firmware_version_t persist_version,
                                     firmware_update_unpersist_firmware_version_t unpersist_version,
                                     firmware_update_start_url_download_t start_url_download,
                                     firmware_update_is_url_download_done_t is_url_download_done)
{
    firmware_update_init(&ctx->firmware_update, ctx->device_key, version, maximum_firmware_size, chunk_size, start,
                         write_chunk, read_chunk, abort, finalize, persist_version, unpersist_version,
                         start_url_download, is_url_download_done, ctx);

    firmware_update_set_on_status_listener(&ctx->firmware_update, _listener_on_status);
    firmware_update_set_on_packet_request_listener(&ctx->firmware_update, _listener_on_packet_request);
    return W_FALSE;
}

WOLK_ERR_T wolk_disable_keep_alive(wolk_ctx_t* ctx)
{
    /* Sanity check */
    WOLK_ASSERT(ctx);

    ctx->is_keep_alive_enabled = false;
    return W_FALSE;
}

WOLK_ERR_T wolk_connect(wolk_ctx_t* ctx)
{
    /* Sanity check */
    WOLK_ASSERT(_is_wolk_initialized(ctx));

    // TODO: Extract this
    uint64_t i;
    char buf[MQTT_PACKET_SIZE];
    char topic_buf[TOPIC_SIZE];

    char lastwill_topic[TOPIC_SIZE];
    MQTTString lastwill_topic_string = MQTTString_initializer;
    MQTTString lastwill_message_string = MQTTString_initializer;

    memset(lastwill_topic, 0, TOPIC_SIZE);
    strcpy(lastwill_topic, LASTWILL_TOPIC);
    strcat(lastwill_topic, ctx->device_key);

    lastwill_topic_string.cstring = lastwill_topic;
    lastwill_message_string.cstring = LASTWILL_MESSAGE;

    ctx->connectData.will.topicName = lastwill_topic_string;
    ctx->connectData.will.message = lastwill_message_string;
    ctx->connectData.willFlag = 1;

    int len = MQTTSerialize_connect((unsigned char*)buf, sizeof(buf), &ctx->connectData);
    if (transport_sendPacketBuffer(ctx->sock, (unsigned char*)buf, len) == TRANSPORT_DONE) {
        return W_TRUE;
    }

    // TODO: Extract this
    memset(topic_buf, '\0', sizeof(topic_buf));
    strcpy(&topic_buf[0], FIRMWARE_UPDATE_COMMANDS_TOPIC_JSON);
    strcat(&topic_buf[0], ctx->device_key);

    if (_subscribe(ctx, topic_buf) != W_FALSE) {
        return W_TRUE;
    }

    // TODO: Extract this
    memset(topic_buf, '\0', sizeof(topic_buf));
    strcpy(&topic_buf[0], FIRMWARE_UPDATE_PACKET_TOPIC_JSON);
    strcat(&topic_buf[0], ctx->device_key);

    if (_subscribe(ctx, topic_buf) != W_FALSE) {
        return W_TRUE;
    }

    // TODO: Extract this
    memset(topic_buf, '\0', sizeof(topic_buf));
    strcpy(&topic_buf[0], CONFIGURATION_COMMANDS);
    strcat(&topic_buf[0], ctx->device_key);

    if (_subscribe(ctx, topic_buf) != W_FALSE) {
        return W_TRUE;
    }

    // TODO: Extract this
    for (i = 0; i < ctx->num_actuator_references; ++i) {
        const char* reference = ctx->actuator_references[i];
        memset(topic_buf, '\0', sizeof(topic_buf));

        strcpy(&topic_buf[0], ACTUATOR_COMMANDS_TOPIC_JSON);
        strcat(&topic_buf[0], ctx->device_key);
        strcat(&topic_buf[0], "/");
        strcat(&topic_buf[0], reference);

        if (_subscribe(ctx, topic_buf) != W_FALSE) {
            return W_TRUE;
        }
    }

    // TODO: Extract this
    for (i = 0; i < ctx->num_actuator_references; ++i) {
        const char* reference = ctx->actuator_references[i];

        actuator_status_t actuator_status = ctx->actuator_status_provider(reference);
        outbound_message_t outbound_message;
        if (!outbound_message_make_from_actuator_status(&ctx->parser, ctx->device_key, &actuator_status, reference,
                                                        &outbound_message)) {
            continue;
        }

        if (_publish(ctx, &outbound_message) != W_FALSE) {
            persistence_push(&ctx->persistence, &outbound_message);
        }
    }

    // TODO: Extract this
    configuration_command_t configuration_command;
    configuration_command_init(&configuration_command, CONFIGURATION_COMMAND_TYPE_CURRENT);
    _handle_configuration_command(ctx, &configuration_command);

    // TODO: Extract this
    outbound_message_t firmware_version_message;
    if (outbound_message_make_from_firmware_version(&ctx->parser, ctx->device_key,
                                                    firmware_update_get_current_version(&ctx->firmware_update),
                                                    &firmware_version_message)) {
        if (_publish(ctx, &firmware_version_message) != W_FALSE) {
            persistence_push(&ctx->persistence, &firmware_version_message);
        }
    }

    return W_FALSE;
}

WOLK_ERR_T wolk_disconnect(wolk_ctx_t* ctx)
{
    /* Sanity check */
    WOLK_ASSERT(_is_wolk_initialized(ctx));

    unsigned char buf[MQTT_PACKET_SIZE];
    memset(buf, 0, MQTT_PACKET_SIZE);

    //lastwill message
    MQTTString lastwill_topic_string = MQTTString_initializer;

    char lastwill_topic[TOPIC_SIZE];
    memset(lastwill_topic, 0, TOPIC_SIZE);
    strcpy(lastwill_topic, LASTWILL_TOPIC);
    strcat(lastwill_topic, ctx->device_key);

    lastwill_topic_string.cstring = lastwill_topic;

    unsigned char* lastwill_msg = (unsigned char*)LASTWILL_MESSAGE;

    int len = MQTTSerialize_publish(buf, MQTT_PACKET_SIZE, 0, 2, 0, 0, lastwill_topic_string, lastwill_msg,
                                              (int)strlen((const char*)lastwill_msg));
    if (transport_sendPacketBuffer(ctx->sock, (unsigned char*)buf, len) == TRANSPORT_DONE) {
        return W_TRUE;
    }

    memset(buf, 0, MQTT_PACKET_SIZE);

    //disconnect message
    len = MQTTSerialize_disconnect(buf, sizeof(buf));
    if (transport_sendPacketBuffer(ctx->sock, buf, len) == TRANSPORT_DONE) {
        return W_TRUE;
    }

    return W_FALSE;
}

WOLK_ERR_T wolk_process(wolk_ctx_t* ctx, uint32_t tick)
{
    /* Sanity check */
    WOLK_ASSERT(_is_wolk_initialized(ctx));

    if (_mqtt_keep_alive(ctx, tick) != W_FALSE) {
        return W_TRUE;
    }

    if (_ping_keep_alive(ctx, tick) != W_FALSE) {
        return W_TRUE;
    }

    if (_receive(ctx) != W_FALSE) {
        return W_TRUE;
    }

    firmware_update_process(&ctx->firmware_update);

    return W_FALSE;
}

WOLK_ERR_T wolk_add_string_sensor_reading(wolk_ctx_t* ctx, const char* reference, const char* value, uint32_t utc_time)
{
    /* Sanity check */
    WOLK_ASSERT(_is_wolk_initialized(ctx));

    manifest_item_t string_sensor;
    manifest_item_init(&string_sensor, reference, READING_TYPE_SENSOR, DATA_TYPE_STRING);

    reading_t reading;
    reading_init(&reading, &string_sensor);
    reading_set_data(&reading, value);
    reading_set_rtc(&reading, utc_time);

    outbound_message_t outbound_message;
    outbound_message_make_from_readings(&ctx->parser, ctx->device_key, &reading, 1, &outbound_message);

    return persistence_push(&ctx->persistence, &outbound_message) ? W_FALSE : W_TRUE;
}

WOLK_ERR_T wolk_add_multi_value_string_sensor_reading(wolk_ctx_t* ctx, const char* reference,
                                                      const char (*values)[READING_SIZE], uint16_t values_size,
                                                      uint32_t utc_time)
{
    /* Sanity check */
    WOLK_ASSERT(_is_wolk_initialized(ctx));
    WOLK_ASSERT(READING_DIMENSIONS > 1);

    manifest_item_t string_sensor;
    manifest_item_init(&string_sensor, reference, READING_TYPE_SENSOR, DATA_TYPE_STRING);
    manifest_item_set_reading_dimensions_and_delimiter(&string_sensor, values_size, DATA_DELIMITER);

    reading_t reading;
    reading_init(&reading, &string_sensor);
    reading_set_rtc(&reading, utc_time);

    for (uint32_t i = 0; i < values_size; ++i) {
        reading_set_data_at(&reading, values[i], i);
    }

    outbound_message_t outbound_message;
    outbound_message_make_from_readings(&ctx->parser, ctx->device_key, &reading, 1, &outbound_message);

    return persistence_push(&ctx->persistence, &outbound_message) ? W_FALSE : W_TRUE;
}

WOLK_ERR_T wolk_add_numeric_sensor_reading(wolk_ctx_t* ctx, const char* reference, double value, uint32_t utc_time)
{
    /* Sanity check */
    WOLK_ASSERT(_is_wolk_initialized(ctx));

    manifest_item_t numeric_sensor;
    manifest_item_init(&numeric_sensor, reference, READING_TYPE_SENSOR, DATA_TYPE_NUMERIC);

    char value_str[READING_SIZE];
    memset(value_str, 0, sizeof(value_str));
    sprintf(value_str, "%f", value);

    reading_t reading;
    reading_init(&reading, &numeric_sensor);
    reading_set_data(&reading, value_str);
    reading_set_rtc(&reading, utc_time);

    outbound_message_t outbound_message;
    outbound_message_make_from_readings(&ctx->parser, ctx->device_key, &reading, 1, &outbound_message);

    return persistence_push(&ctx->persistence, &outbound_message) ? W_FALSE : W_TRUE;
}

WOLK_ERR_T wolk_add_multi_value_numeric_sensor_reading(wolk_ctx_t* ctx, const char* reference, double* values,
                                                       uint16_t values_size, uint32_t utc_time)
{
    /* Sanity check */
    WOLK_ASSERT(_is_wolk_initialized(ctx));
    WOLK_ASSERT(READING_DIMENSIONS > 1);

    manifest_item_t numeric_sensor;
    manifest_item_init(&numeric_sensor, reference, READING_TYPE_SENSOR, DATA_TYPE_NUMERIC);
    manifest_item_set_reading_dimensions_and_delimiter(&numeric_sensor, values_size, DATA_DELIMITER);

    reading_t reading;
    reading_init(&reading, &numeric_sensor);
    reading_set_rtc(&reading, utc_time);

    for (uint32_t i = 0; i < values_size; ++i) {
        char value_str[READING_SIZE];
        memset(value_str, 0, sizeof(value_str));
        sprintf(value_str, "%f", values[i]);

        reading_set_data_at(&reading, value_str, i);
    }

    outbound_message_t outbound_message;
    outbound_message_make_from_readings(&ctx->parser, ctx->device_key, &reading, 1, &outbound_message);

    return persistence_push(&ctx->persistence, &outbound_message) ? W_FALSE : W_TRUE;
}

WOLK_ERR_T wolk_add_bool_sensor_reading(wolk_ctx_t* ctx, const char* reference, bool value, uint32_t utc_time)
{
    /* Sanity check */
    WOLK_ASSERT(_is_wolk_initialized(ctx));

    manifest_item_t bool_sensor;
    manifest_item_init(&bool_sensor, reference, READING_TYPE_SENSOR, DATA_TYPE_BOOLEAN);

    reading_t reading;
    reading_init(&reading, &bool_sensor);
    reading_set_data(&reading, BOOL_TO_STR(value));
    reading_set_rtc(&reading, utc_time);

    outbound_message_t outbound_message;
    outbound_message_make_from_readings(&ctx->parser, ctx->device_key, &reading, 1, &outbound_message);

    return persistence_push(&ctx->persistence, &outbound_message) ? W_FALSE : W_TRUE;
}

WOLK_ERR_T wolk_add_multi_value_bool_sensor_reading(wolk_ctx_t* ctx, const char* reference, bool* values,
                                                    uint16_t values_size, uint32_t utc_time)
{
    /* Sanity check */
    WOLK_ASSERT(_is_wolk_initialized(ctx));
    WOLK_ASSERT(READING_DIMENSIONS > 1);

    manifest_item_t string_sensor;
    manifest_item_init(&string_sensor, reference, READING_TYPE_SENSOR, DATA_TYPE_STRING);
    manifest_item_set_reading_dimensions_and_delimiter(&string_sensor, values_size, DATA_DELIMITER);

    reading_t reading;
    reading_init(&reading, &string_sensor);
    reading_set_rtc(&reading, utc_time);

    for (uint32_t i = 0; i < values_size; ++i) {
        reading_set_data_at(&reading, BOOL_TO_STR(values[i]), i);
    }

    outbound_message_t outbound_message;
    outbound_message_make_from_readings(&ctx->parser, ctx->device_key, &reading, 1, &outbound_message);

    return persistence_push(&ctx->persistence, &outbound_message) ? W_FALSE : W_TRUE;
}

WOLK_ERR_T wolk_add_alarm(wolk_ctx_t* ctx, const char* reference, bool state, uint32_t utc_time)
{
    /* Sanity check */
    WOLK_ASSERT(_is_wolk_initialized(ctx));

    manifest_item_t alarm;
    manifest_item_init(&alarm, reference, READING_TYPE_ALARM, DATA_TYPE_STRING);

    reading_t alarm_reading;
    reading_init(&alarm_reading, &alarm);
    reading_set_rtc(&alarm_reading, utc_time);
    reading_set_data(&alarm_reading, (state==true ? "ON" : "OFF"));

    outbound_message_t outbound_message;
    outbound_message_make_from_readings(&ctx->parser, ctx->device_key, &alarm_reading, 1, &outbound_message);

    return persistence_push(&ctx->persistence, &outbound_message) ? W_FALSE : W_TRUE;
}

WOLK_ERR_T wolk_publish_actuator_status(wolk_ctx_t* ctx, const char* reference)
{
    /* Sanity check */
    WOLK_ASSERT(_is_wolk_initialized(ctx));

    if (ctx->actuator_status_provider != NULL) {
        actuator_status_t actuator_status = ctx->actuator_status_provider(reference);

        outbound_message_t outbound_message;
        if (!outbound_message_make_from_actuator_status(&ctx->parser, ctx->device_key, &actuator_status, reference,
                                                        &outbound_message)) {
            return W_TRUE;
        }

        if (_publish(ctx, &outbound_message) != W_FALSE) {
            persistence_push(&ctx->persistence, &outbound_message);
        }
    }

    return W_FALSE;
}

WOLK_ERR_T wolk_publish_configuration_status(wolk_ctx_t* ctx, char (*p)[CONFIGURATION_REFERENCE_SIZE],char (*p1)[CONFIGURATION_VALUE_SIZE],int number)		//	P
{														//	R
    /* Sanity check */												//	O
    WOLK_ASSERT(_is_wolk_initialized(ctx));									//	M
	puts("Ulaz");										//	E			
    if (ctx->configuration_provider != NULL) {						//	N
        size_t configuration_provider = ctx->configuration_provider(p,p1,number);		//	A

        outbound_message_t outbound_message;
        if (!outbound_message_make_from_configuration(&ctx->parser, ctx->device_key, p, p1,number,
                                                        &outbound_message)) {
     puts("true");       return W_TRUE;
        }

        if (_publish(ctx, &outbound_message) != W_FALSE) {
            persistence_push(&ctx->persistence, &outbound_message);
        }
    }
 puts("false");
    return W_FALSE;
}

WOLK_ERR_T wolk_publish(wolk_ctx_t* ctx)
{
    /* Sanity check */
    WOLK_ASSERT(_is_wolk_initialized(ctx));

    uint8_t i;
    uint16_t batch_size = 50;
    outbound_message_t outbound_message;

    for (i = 0; i < batch_size; ++i) {
        if (persistence_is_empty(&ctx->persistence)) {
            return W_FALSE;
        }

        if (!persistence_peek(&ctx->persistence, &outbound_message)) {
            continue;
        }

        if (_publish(ctx, &outbound_message) != W_FALSE) {
            return W_TRUE;
        }

        persistence_pop(&ctx->persistence, &outbound_message);
    }

    return W_FALSE;
}

static WOLK_ERR_T _mqtt_keep_alive(wolk_ctx_t* ctx, uint32_t tick)
{
    unsigned char buf[MQTT_PACKET_SIZE];
    memset(buf, 0, MQTT_PACKET_SIZE);

    if (ctx->connectData.keepAliveInterval < MQTT_KEEP_ALIVE_INTERVAL) {
        ctx->connectData.keepAliveInterval += tick;
        return W_FALSE;
    }

    int len = MQTTSerialize_pingreq(buf, MQTT_PACKET_SIZE);
    transport_sendPacketBuffernb_start(ctx->sock, buf, len);

    do {
        switch (transport_sendPacketBuffernb(ctx->sock)) {
        case TRANSPORT_DONE:
            ctx->connectData.keepAliveInterval = 0;
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
    } while (true);
}

static WOLK_ERR_T _ping_keep_alive(wolk_ctx_t* ctx, uint32_t tick)
{
    if (!ctx->is_keep_alive_enabled) {
        return W_FALSE;
    }

    if (ctx->milliseconds_since_last_ping_keep_alive < PING_KEEP_ALIVE_INTERVAL) {
        ctx->milliseconds_since_last_ping_keep_alive += tick;
        return W_FALSE;
    }

    outbound_message_t outbound_message;
    outbound_message_make_from_keep_alive_message(&ctx->parser, ctx->device_key, &outbound_message);
    if (_publish(ctx, &outbound_message) != W_FALSE) {
        return W_TRUE;
    }

    ctx->milliseconds_since_last_ping_keep_alive = 0;
    return W_FALSE;
}

static WOLK_ERR_T _receive(wolk_ctx_t* ctx)
{
    unsigned char mqtt_packet[MQTT_PACKET_SIZE];
    int mqtt_packet_len = sizeof(mqtt_packet);

    if (MQTTPacket_readnb(mqtt_packet, mqtt_packet_len, &(ctx->mqtt_transport)) == PUBLISH) {
        unsigned char dup;
        int qos;
        unsigned char retained;
        unsigned short msgid;

        MQTTString topic_mqtt_str;
        unsigned char* payload;
        int payload_len;

        if (MQTTDeserialize_publish(&dup, &qos, &retained, &msgid, &topic_mqtt_str, &payload, &payload_len, mqtt_packet,
                                    mqtt_packet_len)
            != 1) {
            return W_TRUE;
        }

        WOLK_ASSERT(TOPIC_SIZE > topic_mqtt_str.lenstring.len);

        char topic_str[TOPIC_SIZE];
        memset(&topic_str[0], '\0', TOPIC_SIZE);
        strncpy(&topic_str[0], topic_mqtt_str.lenstring.data, topic_mqtt_str.lenstring.len);

        if (strstr(topic_str, ACTUATOR_COMMANDS_TOPIC_JSON) != NULL) {
            actuator_command_t actuator_command;
            const size_t num_deserialized_commands = parser_deserialize_actuator_commands(
                &ctx->parser, topic_str, strlen(topic_str), (char*)payload, (size_t)payload_len, &actuator_command, 1);
            if (num_deserialized_commands != 0) {
                _handle_actuator_command(ctx, &actuator_command);
            }
        } else if (strstr(topic_str, FIRMWARE_UPDATE_COMMANDS_TOPIC_JSON)) {
            firmware_update_command_t firmware_update_command;
            if (parser_deserialize_firmware_update_command(&ctx->parser, (char*)payload, (size_t)payload_len,
                                                           &firmware_update_command)) {
                _handle_firmware_update_command(&ctx->firmware_update, &firmware_update_command);
            }
        } else if (strstr(topic_str, FIRMWARE_UPDATE_PACKET_TOPIC_JSON)) {
            _handle_firmware_update_packet(&ctx->firmware_update, (uint8_t*)payload, (size_t)payload_len);
        } else if (strstr(topic_str, CONFIGURATION_COMMANDS)) {
            configuration_command_t configuration_command;
            const size_t num_deserialized_commands = parser_deserialize_configuration_commands(
                &ctx->parser, (char*)payload, (size_t)payload_len, &configuration_command, 1);
            if (num_deserialized_commands != 0) {
                _handle_configuration_command(ctx, &configuration_command);
            }
        }
    }

    return W_FALSE;
}

static WOLK_ERR_T _publish(wolk_ctx_t* ctx, outbound_message_t* outbound_message)
{
    int len;
    unsigned char buf[MQTT_PACKET_SIZE];
    memset(buf, 0, MQTT_PACKET_SIZE);

    MQTTString mqtt_topic = MQTTString_initializer;
    mqtt_topic.cstring = outbound_message_get_topic(outbound_message);

    unsigned char* payload = (unsigned char*)outbound_message_get_payload(outbound_message);
    len = MQTTSerialize_publish(buf, MQTT_PACKET_SIZE, 0, 0, 0, 0, mqtt_topic, payload,
                                (int)strlen((const char*)payload));
    transport_sendPacketBuffernb_start(ctx->sock, buf, len);

    do {
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
    } while (true);
}

static WOLK_ERR_T _subscribe(wolk_ctx_t* ctx, const char* topic)
{
    unsigned char buf[MQTT_PACKET_SIZE];
    memset(buf, 0, MQTT_PACKET_SIZE);

    int req_qos = 0;

    MQTTString topicString = MQTTString_initializer;
    topicString.cstring = topic;

    const int len = MQTTSerialize_subscribe(buf, MQTT_PACKET_SIZE, 0, 1, 1, &topicString, &req_qos);
    transport_sendPacketBuffernb_start(ctx->sock, buf, len);

    do {
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
    } while (true);
}

static void _parser_init(wolk_ctx_t* ctx, protocol_t protocol)
{
    switch (protocol) {
    case PROTOCOL_JSON_SINGLE:
        parser_init(&ctx->parser, PARSER_TYPE_JSON);
        break;

    default:
        /* Sanity check */
        WOLK_ASSERT(false);
    }
}

static bool _is_wolk_initialized(wolk_ctx_t* ctx)
{
    return ctx->is_initialized && persistence_is_initialized(&ctx->persistence);
}

static void _handle_actuator_command(wolk_ctx_t* ctx, actuator_command_t* actuator_command)
{
    const char* reference = actuator_command_get_reference(actuator_command);
    const char* value = actuator_command_get_value(actuator_command);

    switch (actuator_command_get_type(actuator_command)) {
    case ACTUATOR_COMMAND_TYPE_SET:
        if (ctx->actuation_handler != NULL) {
            ctx->actuation_handler(reference, value);
        }

        /* Fallthrough */
        /* break; */

    case ACTUATOR_COMMAND_TYPE_STATUS:
        if (ctx->actuator_status_provider != NULL) {
            actuator_status_t actuator_status = ctx->actuator_status_provider(reference);

            outbound_message_t outbound_message;
            if (!outbound_message_make_from_actuator_status(&ctx->parser, ctx->device_key, &actuator_status, reference,
                                                            &outbound_message)) {
                return;
            }

            if (_publish(ctx, &outbound_message) != W_FALSE) {
                persistence_push(&ctx->persistence, &outbound_message);
            }
        }
        break;

    case ACTUATOR_COMMAND_TYPE_UNKNOWN:
        break;
    }
}

static void _handle_configuration_command(wolk_ctx_t* ctx, configuration_command_t* configuration_command)
{
    switch (configuration_command_get_type(configuration_command)) {
    case CONFIGURATION_COMMAND_TYPE_SET:
        if (ctx->configuration_handler != NULL) {
            ctx->configuration_handler(configuration_command_get_references(configuration_command),
                                       configuration_command_get_values(configuration_command),
                                       configuration_command_get_number_of_items(configuration_command));
        }

        /* Fallthrough */
        /* break; */

    case CONFIGURATION_COMMAND_TYPE_CURRENT:
        if (ctx->configuration_provider != NULL) {
            char references[CONFIGURATION_ITEMS_SIZE][CONFIGURATION_REFERENCE_SIZE];
            char values[CONFIGURATION_ITEMS_SIZE][CONFIGURATION_VALUE_SIZE];

            const size_t num_configuration_items =
                ctx->configuration_provider(&references[0], &values[0], CONFIGURATION_ITEMS_SIZE);
            if (num_configuration_items == 0) {
                return;
            }

            outbound_message_t outbound_message;
            if (!outbound_message_make_from_configuration(&ctx->parser, ctx->device_key, references, values,
                                                          num_configuration_items, &outbound_message)) {
                return;
            }

            if (_publish(ctx, &outbound_message) != W_FALSE) {
                persistence_push(&ctx->persistence, &outbound_message);
            }
        }
        break;

    case CONFIGURATION_COMMAND_TYPE_UNKNOWN:
        break;
    }
}

static void _handle_firmware_update_command(firmware_update_t* firmware_update,
                                            firmware_update_command_t* firmware_update_command)
{
    /* Sanity check */
    WOLK_ASSERT(firmware_update);
    WOLK_ASSERT(firmware_update_command);

    firmware_update_handle_command(firmware_update, firmware_update_command);
}

static void _handle_firmware_update_packet(firmware_update_t* firmware_update, uint8_t* packet, size_t packet_size)
{
    /* Sanity check */
    WOLK_ASSERT(firmware_update);
    WOLK_ASSERT(packet);

    firmware_update_handle_packet(firmware_update, packet, packet_size);
}

static void _listener_on_status(firmware_update_t* firmware_update, firmware_update_status_t status)
{
    /* Sanity check */
    WOLK_ASSERT(firmware_update);
    wolk_ctx_t* wolk_ctx = (wolk_ctx_t*)firmware_update->wolk_ctx;

    outbound_message_t outbound_message;
    outbound_message_make_from_firmware_update_status(&wolk_ctx->parser, wolk_ctx->device_key, &status,
                                                      &outbound_message);

    _publish(wolk_ctx, &outbound_message);
}

static void _listener_on_packet_request(firmware_update_t* firmware_update, firmware_update_packet_request_t request)
{
    /* Sanity check */
    WOLK_ASSERT(firmware_update);
    wolk_ctx_t* wolk_ctx = (wolk_ctx_t*)firmware_update->wolk_ctx;

    outbound_message_t outbound_message;
    outbound_message_make_from_firmware_update_packet_request(&wolk_ctx->parser, wolk_ctx->device_key, &request,
                                                              &outbound_message);

    _publish(wolk_ctx, &outbound_message);
}
