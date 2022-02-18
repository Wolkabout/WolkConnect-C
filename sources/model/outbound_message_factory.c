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

#include "outbound_message_factory.h"
#include "model/feed.h"
#include "model/outbound_message.h"
#include "model/reading.h"
#include "protocol/parser.h"
#include "utility/wolk_utils.h"

#include <stdbool.h>
#include <stddef.h>
#include <string.h>

size_t outbound_message_make_from_readings(parser_t* parser, const char* device_key, reading_t* readings,
                                           size_t num_readings, outbound_message_t* outbound_message)
{
    /* Sanity check */
    WOLK_ASSERT(parser);
    WOLK_ASSERT(device_key);
    WOLK_ASSERT(readings);
    WOLK_ASSERT(num_readings);
    WOLK_ASSERT(outbound_message);

    char topic[TOPIC_SIZE] = "";
    char payload[PAYLOAD_SIZE] = "";
    size_t num_serialized = 0;

    parser->create_topic(parser->D2P_TOPIC, device_key, parser->FEED_VALUES_MESSAGE_TOPIC, topic);

    num_serialized = parser_serialize_readings(parser, readings, num_readings, payload, sizeof(payload));
    if (num_serialized != 0)
        outbound_message_init(outbound_message, topic, payload);

    return num_serialized;
}

bool outbound_message_make_from_file_management_status(parser_t* parser, const char* device_key,
                                                       file_management_packet_request_t* file_management_packet_request,
                                                       file_management_status_t* file_management_status,
                                                       outbound_message_t* outbound_message)
{
    /* Sanity check */
    WOLK_ASSERT(parser);
    WOLK_ASSERT(device_key);
    WOLK_ASSERT(file_management_status);
    WOLK_ASSERT(outbound_message);

    return parser_serialize_file_management_status(parser, device_key, file_management_packet_request,
                                                   file_management_status, outbound_message);
}

bool outbound_message_make_from_file_management_packet_request(
    parser_t* parser, const char* device_key, file_management_packet_request_t* file_management_packet_request,
    outbound_message_t* outbound_message)
{
    /* Sanity check */
    WOLK_ASSERT(parser);
    WOLK_ASSERT(file_management_packet_request);
    WOLK_ASSERT(outbound_message);

    return parser_serialize_file_management_packet_request(parser, device_key, file_management_packet_request,
                                                           outbound_message);
}

bool outbound_message_make_from_file_management_url_download_status(
    parser_t* parser, const char* device_key, file_management_parameter_t* file_management_parameter,
    file_management_status_t* status, outbound_message_t* outbound_message)
{
    /* Sanity check*/
    WOLK_ASSERT(parser);
    WOLK_ASSERT(device_key);
    WOLK_ASSERT(file_management_parameter);
    WOLK_ASSERT(status);
    WOLK_ASSERT(outbound_message);

    return parser_serialize_file_management_url_download(parser, device_key, file_management_parameter, status,
                                                         outbound_message);
}

bool outbound_message_make_from_file_management_file_list(parser_t* parser, const char* device_key, char* file_list,
                                                          size_t file_list_items, outbound_message_t* outbound_message)
{
    /* Sanity check */
    WOLK_ASSERT(parser);
    WOLK_ASSERT(device_key);
    WOLK_ASSERT(file_list);
    WOLK_ASSERT(outbound_message);

    return parser_serialize_file_management_file_list(parser, device_key, file_list, file_list_items, outbound_message);
}

bool outbound_message_make_from_firmware_update_status(parser_t* parser, const char* device_key,
                                                       firmware_update_t* firmware_update,
                                                       outbound_message_t* outbound_message)
{
    /* Sanity check */
    WOLK_ASSERT(parser);
    WOLK_ASSERT(device_key);
    WOLK_ASSERT(firmware_update);
    WOLK_ASSERT(outbound_message);

    return parse_serialize_firmware_update_status(parser, device_key, firmware_update, outbound_message);
}


bool outbound_message_make_from_firmware_update_version(parser_t* parser, const char* device_key,
                                                        char* firmware_update_version,
                                                        outbound_message_t* outbound_message)
{
    /* Sanity check */
    WOLK_ASSERT(parser);
    WOLK_ASSERT(device_key);
    WOLK_ASSERT(firmware_update_version);

    WOLK_ASSERT(outbound_message);

    return parse_serialize_firmware_update_version(parser, device_key, firmware_update_version, outbound_message);
}
bool outbound_message_feed_registration(parser_t* parser, const char* device_key, feed_t* feed,
                                        outbound_message_t* outbound_message)
{
    /* Sanity check */
    WOLK_ASSERT(parser);
    WOLK_ASSERT(device_key);
    WOLK_ASSERT(feed);
    WOLK_ASSERT(outbound_message);
    return parser_serialize_feed_registration(parser, device_key, feed, outbound_message);
}
bool outbound_message_feed_removal(parser_t* parser, const char* device_key, feed_t* feed,
                                   outbound_message_t* outbound_message)
{
    /* Sanity check */
    WOLK_ASSERT(parser);
    WOLK_ASSERT(device_key);
    WOLK_ASSERT(feed);
    WOLK_ASSERT(outbound_message);
    return parser_serialize_feed_removal(parser, device_key, feed, outbound_message);
}

bool outbound_message_pull_feed_values(parser_t* parser, const char* device_key, outbound_message_t* outbound_message)
{
    WOLK_ASSERT(parser);
    WOLK_ASSERT(device_key);

    return parser_serialize_pull_feed_values(parser, device_key, outbound_message);
}
bool outbound_message_attribute_registration(parser_t* parser, const char* device_key, attribute_t* attribute,
                                             outbound_message_t* outbound_message)
{
    WOLK_ASSERT(parser);
    WOLK_ASSERT(device_key);

    return parser_serialize_attribute(parser, device_key, attribute, outbound_message);
}
bool outbound_message_update_parameters(parser_t* parser, const char* device_key, parameter_t* parameter,
                                        outbound_message_t* outbound_message)
{
    WOLK_ASSERT(parser);
    WOLK_ASSERT(device_key);

    return parser_serialize_parameter(parser, device_key, parameter, outbound_message);
}
bool outbound_message_pull_parameters(parser_t* parser, const char* device_key, outbound_message_t* outbound_message)
{
    WOLK_ASSERT(parser);
    WOLK_ASSERT(device_key);
    return parser_serialize_pull_parameters(parser, device_key, outbound_message);
}
bool outbound_message_synchronize_parameters(parser_t* parser, const char* device_key,
                                             outbound_message_t* outbound_message)
{
    WOLK_ASSERT(parser);
    WOLK_ASSERT(device_key);
    return parser_serialize_sync_parameters(parser, device_key, outbound_message);
}
bool outbound_message_synchronize_time(parser_t* parser, const char* device_key, outbound_message_t* outbound_message)
{
    WOLK_ASSERT(parser);
    WOLK_ASSERT(device_key);
    return parser_serialize_sync_time(parser, device_key, outbound_message);
}
