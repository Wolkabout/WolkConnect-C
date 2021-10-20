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
#include "utc_command.h"

#include "persistence.h"
#include "in_memory_persistence.h"

#include "file_management.h"
#include "file_management_packet.h"
#include "file_management_parameter.h"
#include "file_management_status.h"
#include "file_management_packet_request.h"
#include "file_management_implementation.h"

#include "firmware_update.h"


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
    TEST_ASSERT_EQUAL_INT(ACTUATOR_STATE_READY, wolk.num_actuator_references);
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
    bool file_management_start(const char* file_name, size_t file_size){};
    bool file_management_chunk_write(uint8_t* data, size_t data_size){};
    size_t file_management_chunk_read(size_t index, uint8_t* data, size_t data_size){};
    void file_management_abort(void){};
    void file_management_finalize(void){};
    bool file_management_start_url_download(const char* url){};
    bool file_management_is_url_download_done(bool* success){};
    int8_t file_management_get_file_list(char* file_list){return 0;};
    bool file_management_remove_file(const char* file_name){};
    bool file_management_purge_files(void){};
    bool firmware_update_start_installation(const char* file_name){};
    bool firmware_update_is_installation_completed(bool* success){};
    bool firmware_update_verification_store(uint8_t parameter){};
    uint8_t firmware_update_verification_read(void){};
    bool firmware_update_get_version(const char* version){};
    bool firmware_update_abort_installation(void){};

    TEST_ASSERT_EQUAL_INT(W_FALSE, wolk_init(&wolk, send_buffer, receive_buffer, actuation_handler, actuator_status_provider, configuration_handler, configuration_provider, "device_key", "device_password", PROTOCOL_WOLKABOUT, actuator_references, num_actuator_references));
    TEST_ASSERT_EQUAL_INT(W_FALSE, wolk_init_file_management(&wolk, 128 * 1024 * 1024, 500, file_management_start, file_management_chunk_write, file_management_chunk_read, file_management_abort, file_management_finalize, file_management_start_url_download, file_management_is_url_download_done, file_management_get_file_list, file_management_remove_file, file_management_purge_files));
    TEST_ASSERT_EQUAL_INT(W_FALSE, wolk_init_firmware_update(&wolk, firmware_update_start_installation, firmware_update_is_installation_completed, firmware_update_verification_store, firmware_update_verification_read, firmware_update_get_version, firmware_update_abort_installation));

    TEST_ASSERT_EQUAL_INT(W_FALSE, wolk_connect(&wolk));
}

void test_wolk_connector_wolk_disconnect(void)
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

    TEST_ASSERT_EQUAL_INT(W_FALSE, wolk_disconnect(&wolk));
}

void test_wolk_connector_wolk_process(void)
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

    TEST_ASSERT_EQUAL_INT(W_FALSE, wolk_process(&wolk, 5));
}

void test_wolk_connector_wolk_add_string_sensor_reading(void)
{
    wolk_ctx_t wolk;
    char reference[MANIFEST_ITEM_REFERENCE_SIZE];
    char value[READING_SIZE];
    uint64_t utc_time = 1592574949;
    uint8_t persistence_storage[1024 * 1024];
    static outbound_message_t outbound_message;

    strncpy(reference, "reference", strlen("reference"));
    strncpy(value, "string", strlen("string"));

    wolk_init(&wolk, NULL, NULL, NULL, ACTUATOR_STATE_READY, NULL, NULL, "device_key", "device_password", PROTOCOL_WOLKABOUT, NULL, 0);
    wolk_init_in_memory_persistence(&wolk, persistence_storage, sizeof(persistence_storage), false);

    TEST_ASSERT_EQUAL_INT(W_FALSE, wolk_add_string_sensor_reading(&wolk, reference, value, utc_time));
    TEST_ASSERT_TRUE(persistence_pop(&wolk.persistence, &outbound_message));
    TEST_ASSERT_EQUAL_STRING("{\"utc\":1592574949,\"data\":\"string\"}", outbound_message.payload);
}

void test_wolkconnector_wolk_add_multi_value_string_sensor_reading(void)
{
    wolk_ctx_t wolk;
    char reference[MANIFEST_ITEM_REFERENCE_SIZE];
    char value[READING_DIMENSIONS][READING_SIZE];
    uint64_t utc_time = 1592574949;
    uint8_t persistence_storage[1024 * 1024];
    static outbound_message_t outbound_message;

    strncpy(reference, "reference", strlen("reference"));
    for (int i = 0; i < READING_DIMENSIONS; ++i) {
        strncpy(value[i], "string", strlen("string"));
    }

    wolk_init(&wolk, NULL, NULL, NULL, ACTUATOR_STATE_READY, NULL, NULL, "device_key", "device_password", PROTOCOL_WOLKABOUT, NULL, 0);
    wolk_init_in_memory_persistence(&wolk, persistence_storage, sizeof(persistence_storage), false);

    TEST_ASSERT_EQUAL_INT(W_FALSE, wolk_add_multi_value_string_sensor_reading(&wolk, reference, value, READING_DIMENSIONS, utc_time));
    TEST_ASSERT_TRUE(persistence_pop(&wolk.persistence, &outbound_message));
    TEST_ASSERT_EQUAL_STRING("{\"utc\":1592574949,\"data\":\"string,string,string\"}", outbound_message.payload);
}

void test_wolk_connector_wolk_add_numeric_sensor_reading(void)
{
    wolk_ctx_t wolk;
    char reference[MANIFEST_ITEM_REFERENCE_SIZE];
    double value = 32.1;
    uint64_t utc_time = 1592574949;
    uint8_t persistence_storage[1024 * 1024];
    static outbound_message_t outbound_message;

    strncpy(reference, "reference", strlen("reference"));

    wolk_init(&wolk, NULL, NULL, NULL, ACTUATOR_STATE_READY, NULL, NULL, "device_key", "device_password", PROTOCOL_WOLKABOUT, NULL, 0);
    wolk_init_in_memory_persistence(&wolk, persistence_storage, sizeof(persistence_storage), false);

    TEST_ASSERT_EQUAL_INT(W_FALSE, wolk_add_numeric_sensor_reading(&wolk, reference, value, utc_time));
    TEST_ASSERT_TRUE(persistence_pop(&wolk.persistence, &outbound_message));
    TEST_ASSERT_EQUAL_STRING("{\"utc\":1592574949,\"data\":\"32.100000\"}", outbound_message.payload);
}

void test_wolk_connector_wolk_add_multi_value_numeric_sensor_reading(void)
{
    wolk_ctx_t wolk;
    char reference[MANIFEST_ITEM_REFERENCE_SIZE];
    double value[READING_SIZE];
    uint64_t utc_time = 1592574949;
    uint8_t persistence_storage[1024 * 1024];
    static outbound_message_t outbound_message;

    strncpy(reference, "reference", strlen("reference"));
    for (int i = 0; i < READING_DIMENSIONS; ++i) {
        value[i] = i;
    }

    wolk_init(&wolk, NULL, NULL, NULL, ACTUATOR_STATE_READY, NULL, NULL, "device_key", "device_password", PROTOCOL_WOLKABOUT, NULL, 0);
    wolk_init_in_memory_persistence(&wolk, persistence_storage, sizeof(persistence_storage), false);

    TEST_ASSERT_EQUAL_INT(W_FALSE, wolk_add_multi_value_numeric_sensor_reading(&wolk, reference, value, READING_DIMENSIONS, utc_time));
    TEST_ASSERT_TRUE(persistence_pop(&wolk.persistence, &outbound_message));
    TEST_ASSERT_EQUAL_STRING("{\"utc\":1592574949,\"data\":\"0.000000,1.000000,2.000000\"}", outbound_message.payload);
}

void test_wolk_connector_wolk_add_bool_sensor_reading(void)
{
    wolk_ctx_t wolk;
    char reference[MANIFEST_ITEM_REFERENCE_SIZE];
    bool value = 1;
    uint64_t utc_time = 1592574949;
    uint8_t persistence_storage[1024 * 1024];
    static outbound_message_t outbound_message;

    strncpy(reference, "reference", strlen("reference"));

    wolk_init(&wolk, NULL, NULL, NULL, ACTUATOR_STATE_READY, NULL, NULL, "device_key", "device_password", PROTOCOL_WOLKABOUT, NULL, 0);
    wolk_init_in_memory_persistence(&wolk, persistence_storage, sizeof(persistence_storage), false);

    TEST_ASSERT_EQUAL_INT(W_FALSE, wolk_add_bool_sensor_reading(&wolk, reference, value, utc_time));
    TEST_ASSERT_TRUE(persistence_pop(&wolk.persistence, &outbound_message));
    TEST_ASSERT_EQUAL_STRING("{\"utc\":1592574949,\"data\":\"true\"}", outbound_message.payload);
}

void test_wolk_connector_wolk_add_multi_value_bool_sensor_reading(void)
{
    wolk_ctx_t wolk;
    char reference[MANIFEST_ITEM_REFERENCE_SIZE];
    bool value[READING_SIZE];
    uint64_t utc_time = 1592574949;
    uint8_t persistence_storage[1024 * 1024];
    static outbound_message_t outbound_message;

    strncpy(reference, "reference", strlen("reference"));
    for (int i = 0; i < READING_DIMENSIONS; ++i) {
        value[i] = true;
    }

    wolk_init(&wolk, NULL, NULL, NULL, ACTUATOR_STATE_READY, NULL, NULL, "device_key", "device_password", PROTOCOL_WOLKABOUT, NULL, 0);
    wolk_init_in_memory_persistence(&wolk, persistence_storage, sizeof(persistence_storage), false);

    TEST_ASSERT_EQUAL_INT(W_FALSE, wolk_add_multi_value_bool_sensor_reading(&wolk, reference, value, READING_DIMENSIONS, utc_time));
    TEST_ASSERT_TRUE(persistence_pop(&wolk.persistence, &outbound_message));
    TEST_ASSERT_EQUAL_STRING("{\"utc\":1592574949,\"data\":\"true,true,true\"}", outbound_message.payload);
}

void  test_wolk_connector_wolk_add_alarm(void)
{
    wolk_ctx_t wolk;
    char reference[MANIFEST_ITEM_REFERENCE_SIZE];
    bool value;
    uint64_t utc_time = 1592574949;
    uint8_t persistence_storage[1024 * 1024];
    static outbound_message_t outbound_message;

    strncpy(reference, "reference", strlen("reference"));
    value = false;

    wolk_init(&wolk, NULL, NULL, NULL, ACTUATOR_STATE_READY, NULL, NULL, "device_key", "device_password", PROTOCOL_WOLKABOUT, NULL, 0);
    wolk_init_in_memory_persistence(&wolk, persistence_storage, sizeof(persistence_storage), false);


    TEST_ASSERT_EQUAL_INT(W_FALSE, wolk_add_alarm(&wolk, reference, value, utc_time));
    TEST_ASSERT_TRUE(persistence_pop(&wolk.persistence, &outbound_message));
    TEST_ASSERT_EQUAL_STRING("{\"utc\":1592574949,\"data\":\"false\"}", outbound_message.payload);
}

void test_wolkconnector_wolk_publish_actuator_status(void)
{
    wolk_ctx_t wolk;
    int send_buffer(unsigned char* buffer, unsigned int len){return 1;}
    int receive_buffer(unsigned char* buffer, unsigned int max_bytes){return 1;}
    static const char* actuator_references[] = {"SW"};
    static const uint32_t num_actuator_references = 1;
    void actuation_handler(const char* reference, const char* value){}
    actuator_status_t actuator_status_provider(const char* reference){
        actuator_status_t actuator_status;
        actuator_status_init(&actuator_status, "1", ACTUATOR_STATE_READY);
        return actuator_status;
    }

    wolk_init(&wolk, send_buffer, receive_buffer, actuation_handler, actuator_status_provider, NULL, NULL, "device_key", "device_password", PROTOCOL_WOLKABOUT, actuator_references, num_actuator_references);

    TEST_ASSERT_EQUAL_INT(W_FALSE, wolk_publish_actuator_status(&wolk, actuator_references[0]));
}

void test_wolkconnector_wolk_publish(void)
{
    wolk_ctx_t wolk;
    int send_buffer(unsigned char* buffer, unsigned int len){return 1;}
    int receive_buffer(unsigned char* buffer, unsigned int max_bytes){return 1;}
    uint8_t persistence_storage[1024 * 1024];
    static outbound_message_t outbound_message;

    TEST_ASSERT_EQUAL_INT(W_FALSE, wolk_init(&wolk, send_buffer, receive_buffer, NULL, NULL, NULL, NULL, "device_key", "device_password", PROTOCOL_WOLKABOUT, NULL, 0));
    TEST_ASSERT_EQUAL_INT(W_FALSE, wolk_init_in_memory_persistence(&wolk, persistence_storage, sizeof(persistence_storage), false));

    //NOTE: This call fails at RPI OS for some reason. It will be commented, anyway this test will be refactored with new WA Protocol
//    TEST_ASSERT_EQUAL_INT(W_FALSE, wolk_connect(&wolk));
//
//    TEST_ASSERT_EQUAL_INT(W_FALSE, wolk_publish(&wolk));
}
