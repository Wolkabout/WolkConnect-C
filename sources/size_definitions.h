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

#ifndef SIZEDEFINITIONS_H
#define SIZEDEFINITIONS_H

#ifdef __cplusplus
extern "C" {
#endif

enum {
    /* Maximum number of characters in device key string */
    DEVICE_KEY_SIZE = 64,
    /* Maximum number of characters in device password string */
    DEVICE_PASSWORD_SIZE = 64,

    /* Topic root path size */
    TOPIC_DIRECTION_SIZE = 4,
    /* Topic branch size */
    TOPIC_MESSAGE_TYPE_SIZE = 64,
    /* Maximum number of characters in topic string */
    TOPIC_SIZE = TOPIC_DIRECTION_SIZE + DEVICE_KEY_SIZE + TOPIC_MESSAGE_TYPE_SIZE,

    /* Maximum number of bytes in payload string */
    PAYLOAD_SIZE = 2048,

    /* Maximum size of MQTT header, inherit from dependency */
    MQTT_HEADER_SIZE = 72,
    /* Maximum size of MQTT packet in bytes */
    MQTT_PACKET_SIZE = PAYLOAD_SIZE + TOPIC_SIZE + MQTT_HEADER_SIZE,

    /* Maximum number of characters in a single feed */
    FEED_ELEMENT_SIZE = 64,
    /* Maximum number of feeds*/
    FEEDS_MAX_NUMBER = 32,

    /* Maximum number of characters in reference string */
    REFERENCE_SIZE = 64,
    /* Maximum number of characters in name string */
    ITEM_NAME_SIZE = 64,
    /* Maximum number of characters in unit string */
    ITEM_UNIT_SIZE = 25,
    /* Maximum number of characters in feed type string */
    ITEM_FEED_TYPE_SIZE = 5,
    /* Maximum number of characters in type string */
    ITEM_DATA_TYPE_SIZE = 32,

    /* Maximum number of characters in parameter type string */
    PARAMETER_TYPE_SIZE = 32,
    /* Maximum number of characters in parameter value */
    PARAMETER_VALUE_SIZE = FEED_ELEMENT_SIZE,

    /* Maximum number of characters in attribute value */
    ATTRIBUTE_VALUE_SIZE = FEED_ELEMENT_SIZE,

    /* Maximum number of files in list */
    FILE_MANAGEMENT_FILE_LIST_SIZE = 32,
    /* Maximum number of characters in file management filename */
    FILE_MANAGEMENT_FILE_NAME_SIZE = 64,
    /* Maximum number of characters in file management file url */
    FILE_MANAGEMENT_URL_SIZE = 64,
    /* Size of hash used for file management file transfer (SHA-256) */
    FILE_MANAGEMENT_HASH_SIZE = 32,
    /* Size of the chunks read from file during verification phase. Don't change it */
    FILE_MANAGEMENT_VERIFICATION_CHUNK_SIZE = 1024,

    /* Maximum number of characters in firmware update version */
    FIRMWARE_UPDATE_VERSION_SIZE = 16,

    /* Number of batches in which data will be published */
    PUBLISH_BATCH_SIZE = 50,
};

#ifdef __cplusplus
}
#endif

#endif
