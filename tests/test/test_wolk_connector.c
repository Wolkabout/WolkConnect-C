#include "unity.h"

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

TEST_FILE("MQTTSerializePublish.c")
TEST_FILE("MQTTConnectClient.c")
TEST_FILE("MQTTDeserializePublish.c")
TEST_FILE("MQTTSubscribeClient.c")

#include "wolk_connector.h"
#include "wolk_utils.h"

#include "MQTTPacket.h"

#include "sha256.h"
#include "base64.h"
#include "data_transmission.h"
#include "size_definitions.h"
#include "parser.h"
#include "json_parser.h"
#include "jsmn.h"
#include "manifest_item.h"
#include "circular_buffer.h"
#include "outbound_message.h"
#include "outbound_message_factory.h"

#include "reading.h"
#include "actuator_command.h"
#include "actuator_status.h"
#include "configuration_command.h"

#include "persistence.h"
#include "in_memory_persistence.h"

#include "firmware_update.h"
#include "firmware_update_packet.h"
#include "firmware_update_command.h"
#include "firmware_update_status.h"
#include "firmware_update_packet_request.h"


void setUp(void)
{
}

void tearDown(void)
{
}

void test_wolk_connector_wolk_init_simple(void)
{
    wolk_ctx_t wolk;

    TEST_ASSERT_EQUAL_INT(W_FALSE, wolk_init(&wolk, NULL, NULL, NULL, ACTUATOR_STATE_READY, NULL, NULL, "device_key", "device_password", PROTOCOL_WOLKABOUT, NULL, 0));

    TEST_ASSERT_EQUAL_INT(NULL, wolk.actuation_handler);
    TEST_ASSERT_EQUAL_INT(ACTUATOR_STATE_READY, wolk.actuator_status_provider);
    TEST_ASSERT_EQUAL_INT(NULL, wolk.configuration_handler);
    TEST_ASSERT_EQUAL_INT(NULL, wolk.configuration_provider);
    TEST_ASSERT_EQUAL_STRING("device_key", wolk.device_key);
    TEST_ASSERT_EQUAL_STRING("device_password", wolk.device_password);
    TEST_ASSERT_EQUAL_INT(PROTOCOL_WOLKABOUT, wolk.protocol);
    TEST_ASSERT_EQUAL_INT(NULL, wolk.actuator_references);
    TEST_ASSERT_EQUAL_INT(0, wolk.num_actuator_references);
}

void test_wolk_connector_wolk_init_ffs(void)
{
    wolk_ctx_t wolk;
    int send_buffer(unsigned char* buffer, unsigned int len){return 1;}
    int receive_buffer(unsigned char* buffer, unsigned int max_bytes){return 1;}
    void actuation_handler(const char* reference, const char* value){}
    actuator_status_t actuator_status_provider(const char* reference){
        actuator_status_t actuator_status;
        actuator_status_init(&actuator_status, "1", ACTUATOR_STATE_READY);
        return actuator_status;
    }
    void configuration_handler(char (*reference)[CONFIGURATION_REFERENCE_SIZE], char (*value)[CONFIGURATION_VALUE_SIZE], size_t num_configuration_items){}
    size_t configuration_provider(char (*reference)[CONFIGURATION_REFERENCE_SIZE], char (*value)[CONFIGURATION_VALUE_SIZE], size_t max_num_configuration_items){return CONFIGURATION_ITEMS_SIZE;}
    static const char* actuator_references[] = {"SW"};
    static const uint32_t num_actuator_references = 1;

    TEST_ASSERT_EQUAL_INT(W_FALSE, wolk_init(&wolk, send_buffer, receive_buffer, actuation_handler, actuator_status_provider, configuration_handler, configuration_provider, "device_key", "device_password", PROTOCOL_WOLKABOUT, actuator_references, num_actuator_references));

    TEST_ASSERT_EQUAL_INT(actuation_handler, wolk.actuation_handler);
    TEST_ASSERT_EQUAL_INT(actuator_status_provider, wolk.actuator_status_provider);
    TEST_ASSERT_EQUAL_INT(configuration_handler, wolk.configuration_handler);
    TEST_ASSERT_EQUAL_INT(configuration_provider, wolk.configuration_provider);
    TEST_ASSERT_EQUAL_STRING("device_key", wolk.device_key);
    TEST_ASSERT_EQUAL_STRING("device_password", wolk.device_password);
    TEST_ASSERT_EQUAL_INT(PROTOCOL_WOLKABOUT, wolk.protocol);
    TEST_ASSERT_EQUAL_INT(actuator_references, wolk.actuator_references);
    TEST_ASSERT_EQUAL_INT(num_actuator_references, wolk.num_actuator_references);
}

void test_wolk_connector_wolk_connect(void)
{
    wolk_ctx_t wolk;

    wolk_init(&wolk, NULL, NULL, NULL, ACTUATOR_STATE_READY, NULL, NULL, "device_key", "device_password", PROTOCOL_WOLKABOUT, NULL, 0);

    //TODO: mock MQTT calls
//    TEST_ASSERT_EQUAL_INT(1, wolk_connect(&wolk));
}

void test_wolk_connector_wolk_disconnect(void)
{
    wolk_ctx_t wolk;

    wolk_init(&wolk, NULL, NULL, NULL, ACTUATOR_STATE_READY, NULL, NULL, "device_key", "device_password", PROTOCOL_WOLKABOUT, NULL, 0);

    //TODO: mock MQTT calls
//    TEST_ASSERT_EQUAL_INT(1, wolk_disconnect(&wolk));
}

void test_wolk_connector_wolk_process(void)
{
    wolk_ctx_t wolk;

    wolk_init(&wolk, NULL, NULL, NULL, ACTUATOR_STATE_READY, NULL, NULL, "device_key", "device_password", PROTOCOL_WOLKABOUT, NULL, 0);

    //TODO: mock MQTT calls
//    TEST_ASSERT_EQUAL_INT(1, wolk_process(&wolk, 5));
}

void test_wolk_connector_wolk_add_string_sensor_reading(void)
{
    wolk_ctx_t wolk;
    char reference[MANIFEST_ITEM_REFERENCE_SIZE];
    char value[READING_SIZE];
    uint32_t utc_time = 1592574949;
    uint8_t persistence_storage[1024 * 1024];

    strncpy(reference, "reference", strlen("reference"));
    strncpy(value, "string", strlen("string"));

    wolk_init(&wolk, NULL, NULL, NULL, ACTUATOR_STATE_READY, NULL, NULL, "device_key", "device_password", PROTOCOL_WOLKABOUT, NULL, 0);
    wolk_init_in_memory_persistence(&wolk, persistence_storage, sizeof(persistence_storage), false);

    TEST_ASSERT_EQUAL_INT(W_FALSE, wolk_add_string_sensor_reading(&wolk, reference, value, utc_time));
}

void test_wolkconnector_wolk_add_multi_value_string_sensor_reading(void)
{
    wolk_ctx_t wolk;
    char reference[MANIFEST_ITEM_REFERENCE_SIZE];
    char value[READING_DIMENSIONS][READING_SIZE];
    uint32_t utc_time = 1592574949;
    uint8_t persistence_storage[1024 * 1024];

    strncpy(reference, "reference", strlen("reference"));
    for (int i = 0; i < READING_DIMENSIONS; ++i) {
        strncpy(value[i], "string", strlen("string"));
    }

    wolk_init(&wolk, NULL, NULL, NULL, ACTUATOR_STATE_READY, NULL, NULL, "device_key", "device_password", PROTOCOL_WOLKABOUT, NULL, 0);
    wolk_init_in_memory_persistence(&wolk, persistence_storage, sizeof(persistence_storage), false);

    TEST_ASSERT_EQUAL_INT(W_FALSE, wolk_add_multi_value_string_sensor_reading(&wolk, reference, value, READING_DIMENSIONS, utc_time));
}
