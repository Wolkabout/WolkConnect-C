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

#include "wolk_connector.h"
#include "MQTTPacket.h"
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

static bool is_wolk_initialized(wolk_ctx_t* ctx);

static void handle_feeds(wolk_ctx_t* ctx, feed_t* feeds, size_t number_of_feeds);
static void handle_parameter_message(wolk_ctx_t* ctx, parameter_t* parameter_message, size_t number_of_parameters);
static void handle_utc_command(wolk_ctx_t* ctx, utc_command_t* utc);
static void handle_details_synchronization_message(wolk_ctx_t* ctx, feed_registration_t* feeds, size_t number_of_feeds,
                                                   attribute_t* attributes, size_t number_of_attributes);
static void handle_error_message(wolk_ctx_t* ctx, char* error);

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
static void listener_firmware_update_on_verification(firmware_update_t* firmware_update);

static void handle_firmware_update_installation(firmware_update_t* firmware_update, firmware_update_t* parameter);
static void handle_firmware_update_abort(firmware_update_t* firmware_update);

static WOLK_ERR_T subscribe_to(wolk_ctx_t* ctx, char message_type[TOPIC_MESSAGE_TYPE_SIZE]);

WOLK_ERR_T wolk_init(wolk_ctx_t* ctx, send_func_t snd_func, recv_func_t rcv_func, const char* device_key,
                     const char* device_password, outbound_mode_t outbound_mode, feed_handler_t feed_handler,
                     parameter_handler_t parameter_handler,
                     details_synchronization_handler_t details_synchronization_handler)
{
    /* Sanity check */
    WOLK_ASSERT(snd_func != NULL);
    WOLK_ASSERT(rcv_func != NULL);

    WOLK_ASSERT(device_key != NULL);
    WOLK_ASSERT(device_password != NULL);

    WOLK_ASSERT(strlen(device_key) < DEVICE_KEY_SIZE);
    WOLK_ASSERT(strlen(device_password) < DEVICE_PASSWORD_SIZE);

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
    ctx->details_synchronization_handler = details_synchronization_handler;

    ctx->outbound_mode = outbound_mode;

    parser_init(&ctx->parser);

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
    if (chunk_size > (PAYLOAD_SIZE - 4 * FILE_MANAGEMENT_HASH_SIZE)) { // TODO: unit is Kbytes
        chunk_size = PAYLOAD_SIZE - 4 * FILE_MANAGEMENT_HASH_SIZE;
    }

    //    char parameter_value[PARAMETER_VALUE_SIZE];
    //    sprintf(parameter_value, "%d", chunk_size);
    //    parameter_t parameter;
    //    parameter_init(&parameter, PARAMETER_MAXIMUM_MESSAGE_SIZE, parameter_value);
    //    wolk_change_parameter(&ctx, &parameter, 1);

    if (!file_management_init(ctx, &ctx->file_management, ctx->device_key, maximum_file_size, chunk_size, start,
                              write_chunk, read_chunk, abort, finalize, start_url_download, is_url_download_done,
                              get_file_list, remove_file, purge_files))
        return W_TRUE;

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
    subscribe_to(ctx, ctx->parser.ERROR_TOPIC);
    subscribe_to(ctx, ctx->parser.DETAILS_SYNCHRONIZATION_TOPIC);

    subscribe_to(ctx, ctx->parser.FILE_MANAGEMENT_UPLOAD_INITIATE_TOPIC);
    subscribe_to(ctx, ctx->parser.FILE_MANAGEMENT_BINARY_RESPONSE_TOPIC);
    subscribe_to(ctx, ctx->parser.FILE_MANAGEMENT_UPLOAD_ABORT_TOPIC);
    subscribe_to(ctx, ctx->parser.FILE_MANAGEMENT_URL_DOWNLOAD_INITIATE_TOPIC);
    subscribe_to(ctx, ctx->parser.FILE_MANAGEMENT_URL_DOWNLOAD_ABORT_TOPIC);
    subscribe_to(ctx, ctx->parser.FILE_MANAGEMENT_FILE_LIST_TOPIC);
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

    return W_FALSE;
}

WOLK_ERR_T wolk_disconnect(wolk_ctx_t* ctx)
{
    /* Sanity check */
    WOLK_ASSERT(is_wolk_initialized(ctx));

    unsigned char buf[MQTT_PACKET_SIZE] = "";

    /* disconnect message */
    int length = MQTTSerialize_disconnect(buf, sizeof(buf));
    if (transmission_buffer(ctx->sock, buf, length) == TRANSPORT_DONE) {
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

WOLK_ERR_T wolk_init_feed(feed_registration_t* feed, char* name, const char* reference, const char* unit,
                          const feed_type_t feedType)
{
    feed_initialize_registration(feed, name, reference, unit, feedType);

    return W_FALSE;
}

WOLK_ERR_T wolk_add_string_feed(wolk_ctx_t* ctx, const char* reference, wolk_string_feeds_t* feeds,
                                size_t number_of_feeds)
{
    /* Sanity check */
    WOLK_ASSERT(is_wolk_initialized(ctx));
    WOLK_ASSERT(is_wolk_initialized(reference));
    WOLK_ASSERT(is_wolk_initialized(feeds));
    WOLK_ASSERT(is_wolk_initialized(number_of_feeds));
    WOLK_ASSERT(number_of_feeds > FEEDS_MAX_NUMBER);

    feed_t feed;
    feed_initialize(&feed, number_of_feeds, reference);

    for (size_t i = 0; i < number_of_feeds; ++i) {
        if (feeds->utc_time < 1000000000000 && feeds->utc_time != 0) // Unit ms and zero is valid value
        {
            printf("Failed UTC attached to feed with reference %s. It has to be in ms!\n", reference);
            return W_TRUE;
        }

        feed_set_data_at(&feed, feeds->value, i);
        feed_set_utc(&feed, feeds->utc_time);

        feeds++;
    }

    outbound_message_t outbound_message = {0};
    outbound_message_make_from_feeds(&ctx->parser, ctx->device_key, &feed, STRING, number_of_feeds, 1,
                                     &outbound_message);

    return persistence_push(&ctx->persistence, &outbound_message) ? W_FALSE : W_TRUE;
}

WOLK_ERR_T wolk_add_numeric_feed(wolk_ctx_t* ctx, const char* reference, wolk_numeric_feeds_t* feeds,
                                 size_t number_of_feeds)
{
    /* Sanity check */
    WOLK_ASSERT(is_wolk_initialized(ctx));
    WOLK_ASSERT(is_wolk_initialized(reference));
    WOLK_ASSERT(is_wolk_initialized(feeds));
    WOLK_ASSERT(is_wolk_initialized(number_of_feeds));
    WOLK_ASSERT(number_of_feeds > FEEDS_MAX_NUMBER);

    char value_string[FEED_ELEMENT_SIZE] = "";
    feed_t feed;
    feed_initialize(&feed, number_of_feeds, reference);

    for (size_t i = 0; i < number_of_feeds; ++i) {
        if (feeds->utc_time < 1000000000000 && feeds->utc_time != 0) // Unit ms and zero is valid value
        {
            printf("Failed UTC attached to feed with reference %s. It has to be in ms!\n", reference);
            return W_TRUE;
        }

        if (!snprintf(value_string, FEED_ELEMENT_SIZE, "%2f", feeds->value)) {
            return W_TRUE;
        }

        feed_set_data_at(&feed, value_string, i);
        feed_set_utc(&feed, feeds->utc_time);

        feeds++;
    }

    outbound_message_t outbound_message = {0};
    outbound_message_make_from_feeds(&ctx->parser, ctx->device_key, &feed, NUMERIC, number_of_feeds, 1,
                                     &outbound_message);

    return persistence_push(&ctx->persistence, &outbound_message) ? W_FALSE : W_TRUE;
}

WOLK_ERR_T wolk_add_multi_value_numeric_feed(wolk_ctx_t* ctx, const char* reference, double* values,
                                             uint16_t value_size, uint64_t utc_time)
{
    /* Sanity check */
    WOLK_ASSERT(is_wolk_initialized(ctx));
    WOLK_ASSERT(value_size > FEEDS_MAX_NUMBER);

    if (utc_time < 1000000000000 && utc_time != 0) // Unit ms and zero is valid value
    {
        printf("Failed UTC attached to feeds. It has to be in ms!\n");
        return W_TRUE;
    }

    feed_t feed;
    feed_initialize(&feed, 1, reference); // one feed consisting of N numeric values
    feed_set_utc(&feed, utc_time);

    char value_string_representation[FEED_ELEMENT_SIZE] = "";
    for (size_t i = 0; i < value_size; ++i) {
        if (!snprintf(value_string_representation, FEED_ELEMENT_SIZE, "%f", values[i])) {
            return W_TRUE;
        }

        feed_set_data_at(&feed, value_string_representation, i);
    }

    outbound_message_t outbound_message = {0};
    outbound_message_make_from_feeds(&ctx->parser, ctx->device_key, &feed, VECTOR, 1, value_size, &outbound_message);

    return persistence_push(&ctx->persistence, &outbound_message) ? W_FALSE : W_TRUE;
}

WOLK_ERR_T wolk_add_bool_feeds(wolk_ctx_t* ctx, const char* reference, wolk_boolean_feeds_t* feeds,
                               size_t number_of_feeds)
{
    /* Sanity check */
    WOLK_ASSERT(is_wolk_initialized(ctx));

    feed_t feed;
    feed_initialize(&feed, number_of_feeds, reference);

    for (size_t i = 0; i < number_of_feeds; ++i) {
        if (feeds->utc_time < 1000000000000 && feeds->utc_time != 0) // Unit ms and zero is valid value
        {
            printf("Failed UTC attached to feed with reference %s. It has to be in ms!\n", reference);
            return W_TRUE;
        }

        feed_set_data_at(&feed, BOOL_TO_STR(feeds->value), i);
        feed_set_utc(&feed, feeds->utc_time);

        feeds++;
    }

    outbound_message_t outbound_message = {0};
    outbound_message_make_from_feeds(&ctx->parser, ctx->device_key, &feed, BOOLEAN, number_of_feeds, 1,
                                     &outbound_message);

    return persistence_push(&ctx->persistence, &outbound_message) ? W_FALSE : W_TRUE;
}


WOLK_ERR_T wolk_publish(wolk_ctx_t* ctx)
{
    /* Sanity check */
    WOLK_ASSERT(is_wolk_initialized(ctx));

    uint16_t i;
    outbound_message_t outbound_message = {0};

    for (i = 0; i < PUBLISH_BATCH_SIZE; ++i) {
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

WOLK_ERR_T wolk_register_feed(wolk_ctx_t* ctx, feed_registration_t* feeds, size_t number_of_feeds)
{
    outbound_message_t outbound_message = {0};
    if (!outbound_message_feed_registration(&ctx->parser, ctx->device_key, feeds, number_of_feeds, &outbound_message))
        return W_TRUE;

    return persistence_push(&ctx->persistence, &outbound_message) ? W_FALSE : W_TRUE;
}

WOLK_ERR_T wolk_remove_feed(wolk_ctx_t* ctx, feed_registration_t* feeds, size_t number_of_feeds)
{
    outbound_message_t outbound_message = {0};
    if (!outbound_message_feed_removal(&ctx->parser, ctx->device_key, feeds, number_of_feeds, &outbound_message))
        return W_TRUE;

    return persistence_push(&ctx->persistence, &outbound_message) ? W_FALSE : W_TRUE;
}

WOLK_ERR_T wolk_pull_feed_values(wolk_ctx_t* ctx)
{
    if (ctx->outbound_mode == PULL) {
        outbound_message_t outbound_message = {0};
        outbound_message_pull_feed_values(&ctx->parser, ctx->device_key, &outbound_message);

        return persistence_push(&ctx->persistence, &outbound_message) ? W_FALSE : W_TRUE;
    }

    return W_TRUE;
}

WOLK_ERR_T wolk_init_parameter(parameter_t* parameter, char* name, char* value)
{
    parameter_init(parameter, name, value);

    return W_FALSE;
}

WOLK_ERR_T wolk_set_value_parameter(parameter_t* parameter, char* value)
{
    parameter_set_value(parameter, value);

    return W_FALSE;
}

WOLK_ERR_T wolk_change_parameter(wolk_ctx_t* ctx, parameter_t* parameter, size_t number_of_parameters)
{
    outbound_message_t outbound_message = {0};
    outbound_message_update_parameters(&ctx->parser, ctx->device_key, parameter, number_of_parameters,
                                       &outbound_message);

    return persistence_push(&ctx->persistence, &outbound_message) ? W_FALSE : W_TRUE;
}

WOLK_ERR_T wolk_pull_parameters(wolk_ctx_t* ctx)
{
    if (ctx->outbound_mode == PULL) {
        outbound_message_t outbound_message = {0};
        outbound_message_pull_parameters(&ctx->parser, ctx->device_key, &outbound_message);

        return persistence_push(&ctx->persistence, &outbound_message) ? W_FALSE : W_TRUE;
    }

    return W_TRUE;
}

WOLK_ERR_T wolk_sync_parameters(wolk_ctx_t* ctx, parameter_t* parameters, size_t number_of_parameters)
{
    outbound_message_t outbound_message = {0};
    outbound_message_synchronize_parameters(&ctx->parser, ctx->device_key, parameters, number_of_parameters,
                                            &outbound_message);

    return persistence_push(&ctx->persistence, &outbound_message) ? W_FALSE : W_TRUE;
}

WOLK_ERR_T wolk_sync_time_request(wolk_ctx_t* ctx)
{
    outbound_message_t outbound_message = {0};
    outbound_message_synchronize_time(&ctx->parser, ctx->device_key, &outbound_message);

    return persistence_push(&ctx->persistence, &outbound_message) ? W_FALSE : W_TRUE;
}

WOLK_ERR_T wolk_details_synchronization(wolk_ctx_t* ctx)
{
    outbound_message_t outbound_message = {0};
    outbound_message_details_synchronize(&ctx->parser, ctx->device_key, &outbound_message);

    return persistence_push(&ctx->persistence, &outbound_message) ? W_FALSE : W_TRUE;
}

WOLK_ERR_T wolk_init_attribute(wolk_attribute_t* attribute, char* name, char* data_type, char* value)
{
    attribute_init(attribute, name, data_type, value);

    return W_FALSE;
}

WOLK_ERR_T wolk_register_attribute(wolk_ctx_t* ctx, attribute_t* attributes, size_t number_of_attributes)
{
    outbound_message_t outbound_message = {0};
    outbound_message_attribute_registration(&ctx->parser, ctx->device_key, attributes, number_of_attributes,
                                            &outbound_message);

    return persistence_push(&ctx->persistence, &outbound_message) ? W_FALSE : W_TRUE;
}

/* Local function definitions */

static WOLK_ERR_T mqtt_keep_alive(wolk_ctx_t* ctx, uint64_t tick)
{
    unsigned char buf[MQTT_PACKET_SIZE] = "";

    if (ctx->connectData.keepAliveInterval < (MQTT_KEEP_ALIVE_INTERVAL * 1000)) { // Convert to ms
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

        MQTTString topic_mqtt_str = {0};
        unsigned char* payload;
        int payload_len = 0;

        if (MQTTDeserialize_publish(&dup, &qos, &retained, &msgid, &topic_mqtt_str, &payload, &payload_len, mqtt_packet,
                                    mqtt_packet_len)
            != 1) {
            return W_TRUE;
        }

        WOLK_ASSERT(TOPIC_SIZE > topic_mqtt_str.lenstring.len);

        char topic_str[TOPIC_SIZE] = "";
        strncpy(&topic_str, topic_mqtt_str.lenstring.data, topic_mqtt_str.lenstring.len);

        if (strstr(topic_str, ctx->parser.FEED_VALUES_MESSAGE_TOPIC) != NULL) {
            feed_t feeds_received[FEED_ELEMENT_SIZE];
            const size_t number_of_deserialized_feeds =
                parser_deserialize_feeds_message(&ctx->parser, (char*)payload, (size_t)payload_len, &feeds_received);
            if (number_of_deserialized_feeds != 0) {
                handle_feeds(ctx, &feeds_received, number_of_deserialized_feeds);
            }
        } else if (strstr(topic_str, ctx->parser.PARAMETERS_TOPIC)) {
            parameter_t parameter_message[FEED_ELEMENT_SIZE];
            const size_t number_of_deserialized_parameters = parser_deserialize_parameter_message(
                &ctx->parser, (char*)payload, (size_t)payload_len, &parameter_message);
            if (number_of_deserialized_parameters != 0) {
                handle_parameter_message(ctx, &parameter_message, number_of_deserialized_parameters);
            }
        } else if (strstr(topic_str, ctx->parser.SYNC_TIME_TOPIC)) {
            utc_command_t utc_command;
            const size_t num_deserialized_commands =
                parser_deserialize_time(&ctx->parser, (char*)payload, (size_t)payload_len, &utc_command);
            if (num_deserialized_commands != 0) {
                handle_utc_command(ctx, &utc_command);
            }
        } else if (strstr(topic_str, ctx->parser.ERROR_TOPIC)) {
            if (payload_len != 0) {
                handle_error_message(ctx, &payload);
            }
        } else if (strstr(topic_str, ctx->parser.DETAILS_SYNCHRONIZATION_TOPIC)) {
            feed_registration_t feeds[FEED_ELEMENT_SIZE];
            attribute_t attributes[FEED_ELEMENT_SIZE];
            size_t number_of_feeds = 0;
            size_t number_of_attributes = 0;

            if (parser_deserialize_details_synchronization(&ctx->parser, (char*)payload, (size_t)payload_len, &feeds,
                                                           &number_of_feeds, &attributes, &number_of_attributes)) {
                handle_details_synchronization_message(ctx, &feeds, number_of_feeds, &attributes, number_of_attributes);
            }
        } else if (strstr(topic_str, ctx->parser.FILE_MANAGEMENT_UPLOAD_INITIATE_TOPIC)) {
            file_management_parameter_t file_management_parameter;
            const size_t num_deserialized_parameter = parser_deserialize_file_management_parameter(
                &ctx->parser, (char*)payload, (size_t)payload_len, &file_management_parameter);
            if (num_deserialized_parameter != 0) {
                handle_file_management_parameter(&ctx->file_management, &file_management_parameter);
            }
        } else if (strstr(topic_str, ctx->parser.FILE_MANAGEMENT_BINARY_RESPONSE_TOPIC)) {
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
    unsigned char buf[MQTT_PACKET_SIZE] = "";

    MQTTString mqtt_topic = MQTTString_initializer;
    mqtt_topic.cstring = outbound_message_get_topic(outbound_message);

    unsigned char* payload = (unsigned char*)outbound_message_get_payload(outbound_message);
    int len = MQTTSerialize_publish(buf, MQTT_PACKET_SIZE, 0, 0, 0, 0, mqtt_topic, payload,
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
    unsigned char buf[MQTT_PACKET_SIZE] = "";

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

static bool is_wolk_initialized(wolk_ctx_t* ctx)
{
    /* Sanity Check */
    WOLK_ASSERT(ctx);

    return ctx->is_initialized && persistence_is_initialized(&ctx->persistence);
}

static void handle_feeds(wolk_ctx_t* ctx, feed_t* feeds, size_t number_of_feeds)
{
    /* Sanity Check */
    WOLK_ASSERT(ctx);

    if (ctx->feed_handler != NULL) {
        ctx->feed_handler(feeds, number_of_feeds);
    }
}

static void handle_parameter_message(wolk_ctx_t* ctx, parameter_t* parameter_message, size_t number_of_parameters)
{
    /* Sanity Check */
    WOLK_ASSERT(ctx);

    if (ctx->parameter_handler != NULL) {
        ctx->parameter_handler(parameter_message, number_of_parameters);
    }
}

static void handle_utc_command(wolk_ctx_t* ctx, utc_command_t* utc)
{
    /* Sanity check */
    WOLK_ASSERT(ctx);
    WOLK_ASSERT(utc);

    ctx->utc = utc_command_get(utc);
}

static void handle_details_synchronization_message(wolk_ctx_t* ctx, feed_registration_t* feeds, size_t number_of_feeds,
                                                   attribute_t* attributes, size_t number_of_attributes)
{
    /* Sanity Check */
    WOLK_ASSERT(ctx);

    if (ctx->details_synchronization_handler != NULL) {
        ctx->details_synchronization_handler(feeds, number_of_feeds, attributes, number_of_attributes);
    }
}

static void handle_error_message(wolk_ctx_t* ctx, char* error)
{
    WOLK_UNUSED(ctx);
    WOLK_UNUSED(error);
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
    outbound_message_t outbound_message = {0};
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

    outbound_message_t outbound_message = {0};
    outbound_message_make_from_file_management_packet_request(&wolk_ctx->parser, wolk_ctx->device_key, &request,
                                                              &outbound_message);
    // TODO topic is not good
    publish(wolk_ctx, &outbound_message);
}

static void listener_file_management_on_url_download_status(file_management_t* file_management,
                                                            file_management_status_t status)
{
    /* Sanity check */
    WOLK_ASSERT(file_management);
    WOLK_ASSERT(status);

    wolk_ctx_t* wolk_ctx = (wolk_ctx_t*)file_management->wolk_ctx;
    outbound_message_t outbound_message = {0};
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
    outbound_message_t outbound_message = {0};

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
    outbound_message_t outbound_message = {0};

    outbound_message_make_from_firmware_update_status(&wolk_ctx->parser, wolk_ctx->device_key, firmware_update,
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

static WOLK_ERR_T subscribe_to(wolk_ctx_t* ctx, char message_type[TOPIC_MESSAGE_TYPE_SIZE])
{
    char topic[TOPIC_SIZE] = "";
    parser_create_topic(&ctx->parser, ctx->parser.P2D_TOPIC, ctx->device_key, message_type, topic);
    if (subscribe(ctx, topic) != W_FALSE) {
        return W_TRUE;
    }
    return W_FALSE;
}
