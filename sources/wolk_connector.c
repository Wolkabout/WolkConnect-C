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
#include "model/feed_value_message.h"
#include "model/file_management/file_management.h"
#include "model/file_management/file_management_parameter.h"
#include "model/firmware_update.h"
#include "model/outbound_message.h"
#include "model/outbound_message_factory.h"
#include "persistence/in_memory_persistence.h"
#include "persistence/persistence.h"
#include "protocol/parser.h"
#include "utility/wolk_utils.h"

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#define MQTT_KEEP_ALIVE_INTERVAL 60 // Unit: s

static WOLK_ERR_T mqtt_keep_alive(wolk_ctx_t* ctx, uint64_t tick);

static WOLK_ERR_T receive(wolk_ctx_t* ctx);

static WOLK_ERR_T publish(wolk_ctx_t* ctx, outbound_message_t* outbound_message);
static WOLK_ERR_T subscribe(wolk_ctx_t* ctx, const char* topic);

static void parser_init_parameters(wolk_ctx_t* ctx, protocol_t protocol);

static bool is_wolk_initialized(wolk_ctx_t* ctx);

static void handle_feed_value(wolk_ctx_t* ctx, feed_value_message_t* feed_value_msg);
static void handle_parameter_message(wolk_ctx_t* ctx, parameter_t* parameter_message);
static void handle_utc_command(wolk_ctx_t* ctx, utc_command_t* utc);

static void handle_file_management_parameter(file_management_t* file_management,
                                             file_management_parameter_t* file_management_parameter);
static void handle_file_management_packet(file_management_t* file_management, uint8_t* packet, size_t packet_size);
static void handle_file_management_abort(file_management_t* file_management);
static void handle_file_management_url_download(file_management_t* file_management,
                                                file_management_parameter_t* parameter);
static void handle_file_management_file_delete(file_management_t* file_management,
                                               file_management_parameter_t* parameter);
static void handle_file_management_file_purge(file_management_t* file_management);

static void listener_file_management_on_status(file_management_t* file_management, file_management_status_t status);
static void listener_file_management_on_packet_request(file_management_t* file_management,
                                                       file_management_packet_request_t request);
static void listener_file_management_on_url_download_status(file_management_t* file_management,
                                                            file_management_status_t status);

static void listener_file_management_on_file_list_status(file_management_t* file_management, char* file_list,
                                                         size_t file_list_items);

static void listener_firmware_update_on_status(firmware_update_t* firmware_update);
static void listener_firmware_update_on_version(firmware_update_t* firmware_update, char* firmware_update_version);
static void listener_firmware_update_on_verification(firmware_update_t* firmware_update);

static void handle_firmware_update_installation(firmware_update_t* firmware_update, firmware_update_t* parameter);
static void handle_firmware_update_abort(firmware_update_t* firmware_update);

static WOLK_ERR_T subscribe_to(wolk_ctx_t* ctx, char message_type[MESSAGE_TYPE_SIZE]);

WOLK_ERR_T wolk_init(wolk_ctx_t* ctx, send_func_t snd_func, recv_func_t rcv_func, feed_handler_t feed_handler,
                     parameter_handler_t parameter_handler, const char* device_key, const char* device_password,
                     outbound_mode_t outbound_mode, protocol_t protocol)
{
    /* Sanity check */
    WOLK_ASSERT(snd_func != NULL);
    WOLK_ASSERT(rcv_func != NULL);

    WOLK_ASSERT(device_key != NULL);
    WOLK_ASSERT(device_password != NULL);

    WOLK_ASSERT(strlen(device_key) < DEVICE_KEY_SIZE);
    WOLK_ASSERT(strlen(device_password) < DEVICE_PASSWORD_SIZE);

    WOLK_ASSERT(protocol == PROTOCOL_WOLKABOUT);

    /* Sanity check */

    MQTTPacket_connectData connectData = MQTTPacket_connectData_initializer;
    ctx->connectData = connectData;
    ctx->sock = 0;

    ctx->iof.send = snd_func;
    ctx->iof.recv = rcv_func;

    ctx->sock = transmission_open(&ctx->iof);
    if (ctx->sock < 0) {
        return W_TRUE;
    }

    strcpy(&ctx->device_key[0], device_key);
    strcpy(&ctx->device_password[0], device_password);

    ctx->mqtt_transport.sck = &ctx->sock;
    ctx->mqtt_transport.getfn = transmission_get_data_nb;
    ctx->mqtt_transport.state = 0;
    ctx->connectData.clientID.cstring = &ctx->device_key[0];
    ctx->connectData.keepAliveInterval = MQTT_KEEP_ALIVE_INTERVAL;
    ctx->connectData.cleansession = 1;
    ctx->connectData.username.cstring = &ctx->device_key[0];
    ctx->connectData.password.cstring = &ctx->device_password[0];

    ctx->feed_handler = feed_handler;
    ctx->parameter_handler = parameter_handler;

    ctx->outbound_mode = outbound_mode;

    ctx->protocol = protocol;
    parser_init_parameters(ctx, protocol);

    ctx->utc = 0;

    ctx->is_initialized = true;

    wolk_init_file_management(ctx, 0, 0, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
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

WOLK_ERR_T wolk_init_file_management(
    wolk_ctx_t* ctx, size_t maximum_file_size, size_t chunk_size, file_management_start_t start,
    file_management_write_chunk_t write_chunk, file_management_read_chunk_t read_chunk, file_management_abort_t abort,
    file_management_finalize_t finalize, file_management_start_url_download_t start_url_download,
    file_management_is_url_download_done_t is_url_download_done, file_management_get_file_list_t get_file_list,
    file_management_remove_file_t remove_file, file_management_purge_files_t purge_files)
{
    if (chunk_size > (PAYLOAD_SIZE - 4 * FILE_MANAGEMENT_HASH_SIZE)) {
        chunk_size = PAYLOAD_SIZE - 4 * FILE_MANAGEMENT_HASH_SIZE;
    }

    file_management_init(&ctx->file_management, ctx->device_key, maximum_file_size, chunk_size, start, write_chunk,
                         read_chunk, abort, finalize, start_url_download, is_url_download_done, get_file_list,
                         remove_file, purge_files, ctx);

    file_management_set_on_status_listener(&ctx->file_management, listener_file_management_on_status);
    file_management_set_on_packet_request_listener(&ctx->file_management, listener_file_management_on_packet_request);
    file_management_set_on_url_download_status_listener(&ctx->file_management,
                                                        listener_file_management_on_url_download_status);
    file_management_set_on_file_list_listener(&ctx->file_management, listener_file_management_on_file_list_status);
    return W_FALSE;
}

WOLK_ERR_T wolk_init_firmware_update(wolk_ctx_t* ctx, firmware_update_start_installation_t start_installation,
                                     firmware_update_is_installation_completed_t is_installation_completed,
                                     firmware_update_verification_store_t verification_store,
                                     firmware_update_verification_read_t verification_read,
                                     firmware_update_get_version_t get_version,
                                     firmware_update_abort_t abort_installation)
{
    firmware_update_init(&ctx->firmware_update, start_installation, is_installation_completed, verification_store,
                         verification_read, get_version, abort_installation, ctx);

    firmware_update_set_on_status_listener(&ctx->firmware_update, listener_firmware_update_on_status);
    firmware_update_set_on_version_listener(&ctx->firmware_update, listener_firmware_update_on_version);
    firmware_update_set_on_verification_listener(&ctx->firmware_update, listener_firmware_update_on_verification);

    return W_FALSE;
}

WOLK_ERR_T wolk_connect(wolk_ctx_t* ctx)
{
    /* Sanity check */
    WOLK_ASSERT(is_wolk_initialized(ctx));

    uint64_t i;
    char buf[MQTT_PACKET_SIZE];
    char topic_buf[TOPIC_SIZE];

    /* Setup and Connect to MQTT */

    int len = MQTTSerialize_connect((unsigned char*)buf, sizeof(buf), &ctx->connectData);
    if (transmission_buffer(ctx->sock, (unsigned char*)buf, len) == TRANSPORT_DONE) {
        return W_TRUE;
    }

    /* Subscribe to topics */
    subscribe_to(ctx, ctx->parser.FEED_VALUES_MESSAGE_TOPIC);
    subscribe_to(ctx, ctx->parser.PARAMETERS_TOPIC);
    subscribe_to(ctx, ctx->parser.SYNC_TIME_TOPIC);

    subscribe_to(ctx, ctx->parser.FILE_MANAGEMENT_UPLOAD_INITIATE_TOPIC);
    subscribe_to(ctx, ctx->parser.FILE_MANAGEMENT_CHUNK_UPLOAD_TOPIC);
    subscribe_to(ctx, ctx->parser.FILE_MANAGEMENT_UPLOAD_ABORT_TOPIC);
    subscribe_to(ctx, ctx->parser.FILE_MANAGEMENT_URL_DOWNLOAD_INITIATE_TOPIC);
    subscribe_to(ctx, ctx->parser.FILE_MANAGEMENT_URL_DOWNLOAD_ABORT_TOPIC);
    subscribe_to(ctx, ctx->parser.FILE_MANAGEMENT_FILE_DELETE_TOPIC);
    subscribe_to(ctx, ctx->parser.FILE_MANAGEMENT_FILE_PURGE_TOPIC);

    subscribe_to(ctx, ctx->parser.FIRMWARE_UPDATE_INSTALL_TOPIC);
    subscribe_to(ctx, ctx->parser.FIRMWARE_UPDATE_ABORT_TOPIC);

    if (subscribe(ctx, topic_buf) != W_FALSE) {
        return W_TRUE;
    }

    char file_list[FILE_MANAGEMENT_FILE_LIST_SIZE][FILE_MANAGEMENT_FILE_NAME_SIZE] = {0};
    if (ctx->file_management.has_valid_configuration) {

        listener_file_management_on_file_list_status(&ctx->file_management, file_list,
                                                     ctx->file_management.get_file_list(file_list));
    }

    char firmware_update_version[FIRMWARE_UPDATE_VERSION_SIZE] = {0};
    if (ctx->firmware_update.is_initialized) {
        listener_firmware_update_on_verification(&ctx->firmware_update);

        ctx->firmware_update.get_version(firmware_update_version);
        listener_firmware_update_on_version(&ctx->firmware_update, firmware_update_version);
    }

    return W_FALSE;
}

WOLK_ERR_T wolk_disconnect(wolk_ctx_t* ctx)
{
    /* Sanity check */
    WOLK_ASSERT(is_wolk_initialized(ctx));

    unsigned char buf[MQTT_PACKET_SIZE];
    memset(buf, 0, MQTT_PACKET_SIZE);

    /* lastwill message */
    MQTTString lastwill_topic_string = MQTTString_initializer;
    MQTTString lastwill_message_string = MQTTString_initializer;

    char lastwill_topic[TOPIC_SIZE];
    memset(lastwill_topic, 0, TOPIC_SIZE);
    strcat(lastwill_topic, ctx->device_key);

    lastwill_topic_string.cstring = lastwill_topic;

    int len =
        MQTTSerialize_publish(buf, MQTT_PACKET_SIZE, 0, 1, 0, 0, lastwill_topic_string, lastwill_message_string.cstring,
                              (int)strlen((const char*)lastwill_message_string.cstring));
    if (transmission_buffer(ctx->sock, (unsigned char*)buf, len) == TRANSPORT_DONE) {
        return W_TRUE;
    }

    memset(buf, 0, MQTT_PACKET_SIZE);

    /* disconnect message */
    len = MQTTSerialize_disconnect(buf, sizeof(buf));
    if (transmission_buffer(ctx->sock, buf, len) == TRANSPORT_DONE) {
        return W_TRUE;
    }

    return W_FALSE;
}

WOLK_ERR_T wolk_process(wolk_ctx_t* ctx, uint64_t tick)
{
    /* Sanity check */
    WOLK_ASSERT(is_wolk_initialized(ctx));

    if (mqtt_keep_alive(ctx, tick) != W_FALSE) {
        return W_TRUE;
    }

    if (receive(ctx) != W_FALSE) {
        return W_TRUE;
    }

    file_management_process(&ctx->file_management);
    firmware_update_process(&ctx->firmware_update);

    return W_FALSE;
}
// TODO Maybe make a separate function - add {type}readings with timestamp to send in bulk

WOLK_ERR_T wolk_add_string_reading(wolk_ctx_t* ctx, const char* reference, const char* value, uint64_t utc_time)
{
    /* Sanity check */
    WOLK_ASSERT(is_wolk_initialized(ctx));

    reading_t reading;
    reading_init(&reading, 1, reference);
    reading_set_data(&reading, value);
    reading_set_rtc(&reading, utc_time);

    outbound_message_t outbound_message;
    outbound_message_make_from_readings(&ctx->parser, ctx->device_key, &reading, 1, &outbound_message);

    return persistence_push(&ctx->persistence, &outbound_message) ? W_FALSE : W_TRUE;
}

WOLK_ERR_T wolk_add_multi_value_string_reading(wolk_ctx_t* ctx, const char* reference, const char (*values)[READING_SIZE],
                                               uint16_t values_size, uint64_t utc_time)
{
    /* Sanity check */
    WOLK_ASSERT(is_wolk_initialized(ctx));
    WOLK_ASSERT(READING_DIMENSIONS > 1);

    reading_t reading;
    reading_init(&reading, values_size, reference);
    reading_set_rtc(&reading, utc_time);

    reading_set_data(&reading, values);

    outbound_message_t outbound_message;
    outbound_message_make_from_readings(&ctx->parser, ctx->device_key, &reading, 1, &outbound_message);

    return persistence_push(&ctx->persistence, &outbound_message) ? W_FALSE : W_TRUE;
}

WOLK_ERR_T wolk_add_numeric_reading(wolk_ctx_t* ctx, const char* reference, double value, uint64_t utc_time)
{
    /* Sanity check */
    WOLK_ASSERT(is_wolk_initialized(ctx));

    char value_str[READING_SIZE];
    memset(value_str, 0, sizeof(value_str));
    if (!snprintf(value_str, READING_SIZE, "%f", value)) {
        return W_TRUE;
    }

    reading_t reading;
    reading_init(&reading, 1, reference);
    reading_set_data(&reading, value_str);
    reading_set_rtc(&reading, utc_time);

    outbound_message_t outbound_message;
    outbound_message_make_from_readings(&ctx->parser, ctx->device_key, &reading, 1, &outbound_message);

    return persistence_push(&ctx->persistence, &outbound_message) ? W_FALSE : W_TRUE;
}

WOLK_ERR_T wolk_add_multi_value_numeric_reading(wolk_ctx_t* ctx, const char* reference, double* values,
                                                uint16_t values_size, uint64_t utc_time)
{
    /* Sanity check */
    WOLK_ASSERT(is_wolk_initialized(ctx));
    WOLK_ASSERT(READING_DIMENSIONS > 1);

    reading_t reading;
    reading_init(&reading, values_size, reference);
    reading_set_rtc(&reading, utc_time);

    for (uint32_t i = 0; i < values_size; ++i) {
        char value_str[READING_SIZE];
        memset(value_str, 0, sizeof(value_str));
        if (!snprintf(value_str, READING_SIZE, "%f", values[i])) {
            return W_TRUE;
        }

        reading_set_data_at(&reading, value_str, i);
    }

    outbound_message_t outbound_message;
    outbound_message_make_from_readings(&ctx->parser, ctx->device_key, &reading, 1, &outbound_message);

    return persistence_push(&ctx->persistence, &outbound_message) ? W_FALSE : W_TRUE;
}

WOLK_ERR_T wolk_add_bool_reading(wolk_ctx_t* ctx, const char* reference, bool value, uint64_t utc_time)
{
    /* Sanity check */
    WOLK_ASSERT(is_wolk_initialized(ctx));

    reading_t reading;
    reading_init(&reading, 1, reference);
    reading_set_data(&reading, BOOL_TO_STR(value));
    reading_set_rtc(&reading, utc_time);

    outbound_message_t outbound_message;
    outbound_message_make_from_readings(&ctx->parser, ctx->device_key, &reading, 1, &outbound_message);

    return persistence_push(&ctx->persistence, &outbound_message) ? W_FALSE : W_TRUE;
}

WOLK_ERR_T wolk_add_multi_value_bool_reading(wolk_ctx_t* ctx, const char* reference, bool* values, uint16_t values_size,
                                             uint64_t utc_time)
{
    /* Sanity check */
    WOLK_ASSERT(is_wolk_initialized(ctx));
    WOLK_ASSERT(READING_DIMENSIONS > 1);

    reading_t reading;
    reading_init(&reading, values_size, reference);
    reading_set_rtc(&reading, utc_time);

    for (uint32_t i = 0; i < values_size; ++i) {
        reading_set_data_at(&reading, BOOL_TO_STR(values[i]), i);
    }

    outbound_message_t outbound_message;
    outbound_message_make_from_readings(&ctx->parser, ctx->device_key, &reading, 1, &outbound_message);

    return persistence_push(&ctx->persistence, &outbound_message) ? W_FALSE : W_TRUE;
}

WOLK_ERR_T wolk_publish(wolk_ctx_t* ctx)
{
    /* Sanity check */
    WOLK_ASSERT(is_wolk_initialized(ctx));

    uint16_t i;
    uint16_t batch_size = 50;
    outbound_message_t outbound_message;

    for (i = 0; i < batch_size; ++i) {
        if (persistence_is_empty(&ctx->persistence)) {
            return W_FALSE;
        }

        if (!persistence_peek(&ctx->persistence, &outbound_message)) {
            continue;
        }

        if (publish(ctx, &outbound_message) != W_FALSE) {
            return W_TRUE;
        }

        persistence_pop(&ctx->persistence, &outbound_message);
    }

    return W_FALSE;
}
WOLK_ERR_T wolk_register_feed(wolk_ctx_t* ctx, feed_t* feed)
{
    outbound_message_t outbound_message;
    outbound_message_feed_registration(&ctx->parser, ctx->device_key, feed, &outbound_message);

    return persistence_push(&ctx->persistence, &outbound_message) ? W_FALSE : W_TRUE;
}
WOLK_ERR_T wolk_remove_feed(wolk_ctx_t* ctx, feed_t* feed)
{
    outbound_message_t outbound_message;
    outbound_message_feed_removal(&ctx->parser, ctx->device_key, feed, &outbound_message);

    return persistence_push(&ctx->persistence, &outbound_message) ? W_FALSE : W_TRUE;
}
WOLK_ERR_T wolk_pull_feed_values(wolk_ctx_t* ctx)
{
    if(ctx->outbound_mode == PULL) {
        outbound_message_t outbound_message;
        outbound_message_pull_feed_values(&ctx->parser, ctx->device_key, &outbound_message);

        return persistence_push(&ctx->persistence, &outbound_message) ? W_FALSE : W_TRUE;
    }
    return W_TRUE;
}
WOLK_ERR_T wolk_pull_parameters(wolk_ctx_t* ctx)
{
    if(ctx->outbound_mode == PULL) {
        outbound_message_t outbound_message;
        outbound_message_pull_parameters(&ctx->parser, ctx->device_key, &outbound_message);

        return persistence_push(&ctx->persistence, &outbound_message) ? W_FALSE : W_TRUE;
    }
    return W_TRUE;
}
WOLK_ERR_T wolk_sync_parameters(wolk_ctx_t* ctx)
{
    outbound_message_t outbound_message;
    outbound_message_synchronize_parameters(&ctx->parser, ctx->device_key, &outbound_message);

    return persistence_push(&ctx->persistence, &outbound_message) ? W_FALSE : W_TRUE;
}
WOLK_ERR_T wolk_sync_time_request(wolk_ctx_t* ctx)
{
    outbound_message_t outbound_message;
    outbound_message_synchronize_time(&ctx->parser, ctx->device_key, &outbound_message);

    return persistence_push(&ctx->persistence, &outbound_message) ? W_FALSE : W_TRUE;
}
WOLK_ERR_T wolk_register_attribute(wolk_ctx_t* ctx, attribute_t* attribute)
{
    outbound_message_t outbound_message;
    outbound_message_attribute_registration(&ctx->parser, ctx->device_key, attribute, &outbound_message);

    return persistence_push(&ctx->persistence, &outbound_message) ? W_FALSE : W_TRUE;
}
WOLK_ERR_T wolk_change_parameter(wolk_ctx_t* ctx, parameter_t* parameter)
{
    outbound_message_t outbound_message;
    outbound_message_update_parameters(&ctx->parser, ctx->device_key, parameter, &outbound_message);

    return persistence_push(&ctx->persistence, &outbound_message) ? W_FALSE : W_TRUE;
}

/* Local function definitions */

static WOLK_ERR_T mqtt_keep_alive(wolk_ctx_t* ctx, uint64_t tick)
{
    unsigned char buf[MQTT_PACKET_SIZE];
    memset(buf, 0, MQTT_PACKET_SIZE);

    if (ctx->connectData.keepAliveInterval < (MQTT_KEEP_ALIVE_INTERVAL * 1000)) { // Convert to Unit: ms
        ctx->connectData.keepAliveInterval += tick;
        return W_FALSE;
    }

    int len = MQTTSerialize_pingreq(buf, MQTT_PACKET_SIZE);
    transmission_buffer_nb_start(ctx->sock, buf, len);

    do {
        switch (transmission_buffer_nb(ctx->sock)) {
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

static WOLK_ERR_T receive(wolk_ctx_t* ctx)
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

        if (strstr(topic_str, ctx->parser.FEED_VALUES_MESSAGE_TOPIC) != NULL) {
            feed_value_message_t feed_message;
            const size_t num_deserialized_commands = parser_deserialize_feed_value_message(
                &ctx->parser, (char*)payload, (size_t)payload_len, &feed_message, 1);
            // TODO make this a for loop and the feed_message a buffer
            if (num_deserialized_commands != 0) {
                handle_feed_value(ctx, &feed_message);
            }
        } else if (strstr(topic_str, ctx->parser.PARAMETERS_TOPIC)) {
            parameter_t parameter_message;
            const size_t num_deserialized_commands = parser_deserialize_parameter_message(
                &ctx->parser, (char*)payload, (size_t)payload_len, &parameter_message, 1);
            // TODO make this a for loop and the parameter_message a buffer
            if (num_deserialized_commands != 0) {
                handle_parameter_message(ctx, &parameter_message);
            }
        } else if (strstr(topic_str, ctx->parser.SYNC_TIME_TOPIC)) {
            utc_command_t utc_command;
            const size_t num_deserialized_commands =
                parser_deserialize_time(&ctx->parser, (char*)payload, (size_t)payload_len, &utc_command);
            if (num_deserialized_commands != 0) {
                handle_utc_command(ctx, &utc_command);
            }
        } else if (strstr(topic_str, ctx->parser.FILE_MANAGEMENT_UPLOAD_INITIATE_TOPIC)) {
            file_management_parameter_t file_management_parameter;
            const size_t num_deserialized_parameter = parser_deserialize_file_management_parameter(
                &ctx->parser, (char*)payload, (size_t)payload_len, &file_management_parameter);
            if (num_deserialized_parameter != 0) {
                handle_file_management_parameter(&ctx->file_management, &file_management_parameter);
            }
        } else if (strstr(topic_str, ctx->parser.FILE_MANAGEMENT_CHUNK_UPLOAD_TOPIC)) {
            handle_file_management_packet(&ctx->file_management, (uint8_t*)payload, (size_t)payload_len);
        } else if ((strstr(topic_str, ctx->parser.FILE_MANAGEMENT_UPLOAD_ABORT_TOPIC))
                   || (strstr(topic_str, ctx->parser.FILE_MANAGEMENT_URL_DOWNLOAD_ABORT_TOPIC))) {
            file_management_parameter_t file_management_parameter;
            const size_t num_deserialized_parameter = parser_deserialize_file_management_parameter(
                &ctx->parser, (char*)payload, (size_t)payload_len, &file_management_parameter);
            if (num_deserialized_parameter != 0) {
                handle_file_management_abort(&ctx->file_management);
            }
        } else if (strstr(topic_str, ctx->parser.FILE_MANAGEMENT_URL_DOWNLOAD_INITIATE_TOPIC)) {
            file_management_parameter_t file_management_parameter;
            const size_t num_deserialized = parser_deserialize_file_management_parameter(
                &ctx->parser, (char*)payload, (size_t)payload_len, &file_management_parameter);
            if (num_deserialized != 0) {
                handle_file_management_url_download(&ctx->file_management, &file_management_parameter);
            }
        } else if (strstr(topic_str, ctx->parser.FILE_MANAGEMENT_FILE_DELETE_TOPIC)) {
            file_management_parameter_t file_management_parameter;
            const size_t num_deserialized = parser_deserialize_file_management_parameter(
                &ctx->parser, (char*)payload, (size_t)payload_len, &file_management_parameter);
            if (num_deserialized != 0) {
                handle_file_management_file_delete(&ctx->file_management, &file_management_parameter);
            }
        } else if (strstr(topic_str, ctx->parser.FILE_MANAGEMENT_FILE_PURGE_TOPIC)) {
            handle_file_management_file_purge(&ctx->file_management);
        } else if (strstr(topic_str, ctx->parser.FIRMWARE_UPDATE_INSTALL_TOPIC)) {
            firmware_update_t firmware_update_parameter;
            const size_t num_deserialized = parse_deserialize_firmware_update_parameter(
                &ctx->parser, (char*)ctx->device_key, (char*)payload, (size_t)payload_len, &firmware_update_parameter);
            if (num_deserialized != 0) {
                handle_firmware_update_installation(&ctx->firmware_update, &firmware_update_parameter);
            }
        } else if (strstr(topic_str, ctx->parser.FIRMWARE_UPDATE_ABORT_TOPIC)) {
            handle_firmware_update_abort(&ctx->firmware_update);
        }
    }

    return W_FALSE;
}

static WOLK_ERR_T publish(wolk_ctx_t* ctx, outbound_message_t* outbound_message)
{
    int len;
    unsigned char buf[MQTT_PACKET_SIZE];
    memset(buf, 0, MQTT_PACKET_SIZE);

    MQTTString mqtt_topic = MQTTString_initializer;
    mqtt_topic.cstring = outbound_message_get_topic(outbound_message);

    unsigned char* payload = (unsigned char*)outbound_message_get_payload(outbound_message);
    len = MQTTSerialize_publish(buf, MQTT_PACKET_SIZE, 0, 0, 0, 0, mqtt_topic, payload,
                                (int)strlen((const char*)payload));
    transmission_buffer_nb_start(ctx->sock, buf, len);

    do {
        switch (transmission_buffer_nb(ctx->sock)) {
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

static WOLK_ERR_T subscribe(wolk_ctx_t* ctx, const char* topic)
{
    unsigned char buf[MQTT_PACKET_SIZE];
    memset(buf, 0, MQTT_PACKET_SIZE);

    int req_qos = 0;

    MQTTString topicString = MQTTString_initializer;
    topicString.cstring = topic;

    const int len = MQTTSerialize_subscribe(buf, MQTT_PACKET_SIZE, 0, 1, 1, &topicString, &req_qos);
    transmission_buffer_nb_start(ctx->sock, buf, len);

    do {
        switch (transmission_buffer_nb(ctx->sock)) {
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

static void parser_init_parameters(wolk_ctx_t* ctx, protocol_t protocol)
{
    switch (protocol) {
    case PROTOCOL_WOLKABOUT:
        parser_init(&ctx->parser, PARSER_TYPE_WOLKABOUT);
        break;

    default:
        /* Sanity check */
        WOLK_ASSERT(false);
    }
}

static bool is_wolk_initialized(wolk_ctx_t* ctx)
{
    /* Sanity Check */
    WOLK_ASSERT(ctx);

    return ctx->is_initialized && persistence_is_initialized(&ctx->persistence);
}

static void handle_feed_value(wolk_ctx_t* ctx, feed_value_message_t* feed_value_msg)
{
    const char* reference = feed_value_msg->reference;
    const char* value = feed_value_msg->value;

    if (ctx->feed_handler != NULL) {
        ctx->feed_handler(reference, value);
    }
}

static void handle_parameter_message(wolk_ctx_t* ctx, parameter_t* parameter_message)
{
    const char* name = parameter_message->name;
    const char* value = parameter_message->value;

    if (ctx->parameter_handler != NULL) {
        ctx->parameter_handler(name, value);
    }
}

static void handle_utc_command(wolk_ctx_t* ctx, utc_command_t* utc)
{
    /* Sanity check */
    WOLK_ASSERT(ctx);
    WOLK_ASSERT(utc);

    ctx->utc = utc_command_get(utc);
}

static void handle_file_management_parameter(file_management_t* file_management,
                                             file_management_parameter_t* file_management_parameter)
{
    /* Sanity check */
    WOLK_ASSERT(file_management);
    WOLK_ASSERT(file_management_parameter);

    file_management_handle_parameter(file_management, file_management_parameter);
}

static void handle_file_management_packet(file_management_t* file_management, uint8_t* packet, size_t packet_size)
{
    /* Sanity check */
    WOLK_ASSERT(file_management);
    WOLK_ASSERT(packet_size);

    file_management_handle_packet(file_management, packet, packet_size);
}

static void handle_file_management_abort(file_management_t* file_management)
{
    /* Sanity check */
    WOLK_ASSERT(file_management);

    file_management_handle_abort(file_management);
}

static void handle_file_management_url_download(file_management_t* file_management,
                                                file_management_parameter_t* parameter)
{
    /* Sanity check */
    WOLK_ASSERT(file_management);
    WOLK_ASSERT(parameter);

    file_management_handle_url_download(file_management, parameter);
}

static void handle_file_management_file_delete(file_management_t* file_management,
                                               file_management_parameter_t* parameter)
{
    /* Sanity Check*/
    WOLK_ASSERT(file_management);
    WOLK_ASSERT(parameter);

    file_management_handle_file_delete(file_management, parameter);
}

static void handle_file_management_file_purge(file_management_t* file_management)
{
    /* Sanity Check */
    WOLK_ASSERT(file_management);

    file_management_handle_file_purge(file_management);
}

static void listener_file_management_on_status(file_management_t* file_management, file_management_status_t status)
{
    /* Sanity check */
    WOLK_ASSERT(file_management);
    WOLK_ASSERT(status);

    wolk_ctx_t* wolk_ctx = (wolk_ctx_t*)file_management->wolk_ctx;
    outbound_message_t outbound_message;
    outbound_message_make_from_file_management_status(&wolk_ctx->parser, wolk_ctx->device_key,
                                                      file_management->file_name, &status, &outbound_message);

    publish(wolk_ctx, &outbound_message);
}

static void listener_file_management_on_packet_request(file_management_t* file_management,
                                                       file_management_packet_request_t request)
{
    /* Sanity check */
    WOLK_ASSERT(file_management);
    WOLK_ASSERT(request);

    wolk_ctx_t* wolk_ctx = (wolk_ctx_t*)file_management->wolk_ctx;

    outbound_message_t outbound_message;
    outbound_message_make_from_file_management_packet_request(&wolk_ctx->parser, wolk_ctx->device_key, &request,
                                                              &outbound_message);

    publish(wolk_ctx, &outbound_message);
}

static void listener_file_management_on_url_download_status(file_management_t* file_management,
                                                            file_management_status_t status)
{
    /* Sanity check */
    WOLK_ASSERT(file_management);
    WOLK_ASSERT(status);

    wolk_ctx_t* wolk_ctx = (wolk_ctx_t*)file_management->wolk_ctx;
    outbound_message_t outbound_message;
    file_management_parameter_t file_management_parameter;

    file_management_parameter_set_file_url(&file_management_parameter, file_management->file_url);
    file_management_parameter_set_filename(&file_management_parameter, file_management->file_name);

    outbound_message_make_from_file_management_url_download_status(
        &wolk_ctx->parser, wolk_ctx->device_key, &file_management_parameter, &status, &outbound_message);

    publish(wolk_ctx, &outbound_message);
}

static void listener_file_management_on_file_list_status(file_management_t* file_management, char* file_list,
                                                         size_t file_list_items)
{
    /* Sanity check */
    WOLK_ASSERT(file_management);
    WOLK_ASSERT(file_list);
    WOLK_ASSERT(file_list_items);

    wolk_ctx_t* wolk_ctx = (wolk_ctx_t*)file_management->wolk_ctx;
    outbound_message_t outbound_message;

    outbound_message_make_from_file_management_file_list(&wolk_ctx->parser, wolk_ctx->device_key, file_list,
                                                         file_list_items, &outbound_message);

    publish(wolk_ctx, &outbound_message);
}

static void listener_firmware_update_on_status(firmware_update_t* firmware_update)
{
    /* Sanity check */
    WOLK_ASSERT(firmware_update);
    WOLK_ASSERT(status);

    wolk_ctx_t* wolk_ctx = (wolk_ctx_t*)firmware_update->wolk_ctx;
    outbound_message_t outbound_message;

    outbound_message_make_from_firmware_update_status(&wolk_ctx->parser, wolk_ctx->device_key, firmware_update,
                                                      &outbound_message);

    publish(wolk_ctx, &outbound_message);
}

static void listener_firmware_update_on_version(firmware_update_t* firmware_update, char* firmware_update_version)
{
    /* Sanity check */
    WOLK_ASSERT(firmware_update);
    WOLK_ASSERT(firmware_update_version);

    wolk_ctx_t* wolk_ctx = (wolk_ctx_t*)firmware_update->wolk_ctx;
    outbound_message_t outbound_message;

    outbound_message_make_from_firmware_update_version(&wolk_ctx->parser, wolk_ctx->device_key, firmware_update_version,
                                                       &outbound_message);

    publish(wolk_ctx, &outbound_message);
}

static void listener_firmware_update_on_verification(firmware_update_t* firmware_update)
{
    /* Sanity Check */
    WOLK_ASSERT(firmware_update);

    firmware_update_handle_verification(firmware_update);
}

static void handle_firmware_update_installation(firmware_update_t* firmware_update, firmware_update_t* parameter)
{
    /* Sanity check */
    WOLK_ASSERT(firmware_update);
    WOLK_ASSERT(parameter);

    firmware_update_handle_parameter(firmware_update, parameter);
}

static void handle_firmware_update_abort(firmware_update_t* firmware_update)
{
    /* Sanity check */
    WOLK_ASSERT(firmware_update);

    firmware_update_handle_abort(firmware_update);
}

static WOLK_ERR_T subscribe_to(wolk_ctx_t* ctx, char message_type[MESSAGE_TYPE_SIZE])
{
    char topic[TOPIC_SIZE];
    parser_create_topic(&ctx->parser, ctx->parser.P2D_TOPIC, ctx->device_key, message_type, topic);
    if (subscribe(ctx, topic) != W_FALSE) {
        return W_TRUE;
    }
    return W_FALSE;
}
