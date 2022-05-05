#ifdef TEST

#include "unity.h"

#include "MQTTPacket.h"
#include "size_definitions.h"
#include "string.h"
#include "types.h"
//#include "wolk_connector.h"

#include "model/attribute.h"
#include "model/feed.h"
#include "model/file_management/file_management.h"
#include "model/file_management/file_management_packet.h"
#include "model/file_management/file_management_packet_request.h"
#include "model/file_management/file_management_parameter.h"
#include "model/file_management/file_management_status.h"
#include "model/firmware_update.h"
#include "model/outbound_message.h"
#include "model/outbound_message_factory.h"
#include "model/utc_command.h"

#include "connectivity/data_transmission.h"

#include "persistence/in_memory_persistence.h"
#include "persistence/persistence.h"

#include "protocol/json_parser.h"
#include "protocol/parser.h"

#include "utility/base64.h"
#include "utility/circular_buffer.h"
#include "utility/jsmn.h"
#include "utility/md5.h"
#include "utility/sha256.h"
#include "utility/wolk_utils.h"


void setUp(void)
{
}

void tearDown(void)
{
}


void test_json_parser_json_serialize_feed_all_types(void)
{
    feed_t first_feed;
    char buffer[PAYLOAD_SIZE] = "";
    uint64_t utc = 1646815080000; // in milliseconds

    /* Data type String */
    feed_initialize(&first_feed, 1, "FS");
    feed_set_data_at(&first_feed, "FEED STRING", 0);
    feed_set_utc(&first_feed, utc);

    TEST_ASSERT_TRUE(json_serialize_feeds(&first_feed, STRING, 1, 1, buffer, sizeof(buffer)));
    TEST_ASSERT_EQUAL_STRING("[{\"FS\":\"FEED STRING\",\"timestamp\":1646815080000}]", buffer);

    /* Data type Numeric */
    feed_initialize(&first_feed, 1, "FN");
    feed_set_data_at(&first_feed, "32.1", 0);
    feed_set_utc(&first_feed, utc);

    TEST_ASSERT_TRUE(json_serialize_feeds(&first_feed, NUMERIC, 1, 1, buffer, sizeof(buffer)));
    TEST_ASSERT_EQUAL_STRING("[{\"FN\":32.1,\"timestamp\":1646815080000}]", buffer);

    /* Data type Boolean */
    feed_initialize(&first_feed, 1, "FB");
    feed_set_data_at(&first_feed, "true", 0);
    feed_set_utc(&first_feed, utc);

    TEST_ASSERT_TRUE(json_serialize_feeds(&first_feed, BOOLEAN, 1, 1, buffer, sizeof(buffer)));
    TEST_ASSERT_EQUAL_STRING("[{\"FB\":\"true\",\"timestamp\":1646815080000}]", buffer);
}

void test_json_parser_json_serialize_feeds_all_types_different_timestamp(void)
{
    feed_t first_feed[3];
    char buffer[PAYLOAD_SIZE] = "";
    uint64_t utc = 1646815080000; // in milliseconds

    /* Data type String */
    feed_initialize(&first_feed[0], 1, "FS");
    feed_set_data_at(&first_feed[0], "STRING1", 0);
    feed_set_utc(&first_feed[0], utc);
    feed_initialize(&first_feed[1], 1, "FS");
    feed_set_data_at(&first_feed[1], "STRING2", 0);
    feed_set_utc(&first_feed[1], utc + 100);

    TEST_ASSERT_TRUE(json_serialize_feeds(&first_feed, STRING, 2, 1, buffer, sizeof(buffer)));
    TEST_ASSERT_EQUAL_STRING(
        "[{\"FS\":\"STRING1\",\"timestamp\":1646815080000},{\"FS\":\"STRING2\",\"timestamp\":1646815080100}]", buffer);

    memset(buffer, '\0', PAYLOAD_SIZE);
    /* Data type Numeric */
    feed_initialize(&first_feed[0], 1, "FN");
    feed_set_data_at(&first_feed[0], "3", 0);
    feed_set_utc(&first_feed[0], utc);

    feed_initialize(&first_feed[1], 1, "FN");
    feed_set_data_at(&first_feed[1], "32", 0);
    feed_set_utc(&first_feed[1], utc + 100);

    feed_initialize(&first_feed[2], 1, "FN");
    feed_set_data_at(&first_feed[2], "32.1", 0);
    feed_set_utc(&first_feed[2], utc + 200);

    TEST_ASSERT_TRUE(json_serialize_feeds(&first_feed, NUMERIC, 3, 1, buffer, sizeof(buffer)));
    TEST_ASSERT_EQUAL_STRING(
        "[{\"FN\":3,\"timestamp\":1646815080000},{\"FN\":32,\"timestamp\":1646815080100},{\"FN\":32.1,\"timestamp\":1646815080200}]",
        buffer);

    memset(buffer, '\0', PAYLOAD_SIZE);
    /* Data type Boolean */
    feed_initialize(&first_feed[0], 1, "FB");
    feed_set_data_at(&first_feed[0], "true", 0);
    feed_set_utc(&first_feed[0], utc);

    feed_initialize(&first_feed[1], 1, "FB");
    feed_set_data_at(&first_feed[1], "false", 0);
    feed_set_utc(&first_feed[1], utc + 100);

    TEST_ASSERT_TRUE(json_serialize_feeds(&first_feed, BOOLEAN, 2, 1, buffer, sizeof(buffer)));
    TEST_ASSERT_EQUAL_STRING(
        "[{\"FB\":\"true\",\"timestamp\":1646815080000},{\"FB\":\"false\",\"timestamp\":1646815080100}]", buffer);
}

void test_json_parser_json_serialize_feed_multivalue(void)
{
    feed_t first_feed;
    char buffer[PAYLOAD_SIZE] = "";
    uint64_t utc = 1646815080000; // in milliseconds

    /* Data type String */
    feed_initialize(&first_feed, 3, "FS");
    feed_set_data_at(&first_feed, "STRING1", 0);
    feed_set_data_at(&first_feed, "STRING2", 1);
    feed_set_data_at(&first_feed, "STRING3", 2);
    feed_set_utc(&first_feed, utc);

    TEST_ASSERT_TRUE(json_serialize_feeds(&first_feed, STRING, 1, 3, buffer, sizeof(buffer)));
    TEST_ASSERT_EQUAL_STRING("[{\"FS\":\"STRING1,STRING2,STRING3\",\"timestamp\":1646815080000}]", buffer);

    memset(buffer, '\0', PAYLOAD_SIZE);
    /* Data type Numeric */
    feed_initialize(&first_feed, 3, "FN");
    feed_set_data_at(&first_feed, "3", 0);
    feed_set_data_at(&first_feed, "32", 1);
    feed_set_data_at(&first_feed, "32.1", 2);
    feed_set_utc(&first_feed, utc);

    TEST_ASSERT_TRUE(json_serialize_feeds(&first_feed, NUMERIC, 1, 3, buffer, sizeof(buffer)));
    TEST_ASSERT_EQUAL_STRING("[{\"FN\":3,32,32.1,\"timestamp\":1646815080000}]", buffer);

    memset(buffer, '\0', PAYLOAD_SIZE);
    /* Data type Boolean */
    feed_initialize(&first_feed, 2, "FB");
    feed_set_data_at(&first_feed, "true", 0);
    feed_set_data_at(&first_feed, "false", 1);
    feed_set_utc(&first_feed, utc);

    TEST_ASSERT_TRUE(json_serialize_feeds(&first_feed, BOOLEAN, 1, 2, buffer, sizeof(buffer)));
    TEST_ASSERT_EQUAL_STRING("[{\"FB\":\"true,false\",\"timestamp\":1646815080000}]", buffer);
}

void test_json_deserialize_file_delete(void)
{
    char received_payload[100];
    strcpy(received_payload, "[\"My file\", \"My other file\", \"firmware_1.0.0.firmware\"]");
    size_t buffer_size = 40;
    file_list_t file_list[10];

    TEST_ASSERT_EQUAL_INT(3, json_deserialize_file_delete((char*)received_payload, buffer_size, file_list));
    TEST_ASSERT_EQUAL_STRING("My file", file_list[0].file_name);
    TEST_ASSERT_EQUAL_STRING("My other file", file_list[1].file_name);
    TEST_ASSERT_EQUAL_STRING("firmware_1.0.0.firmware", file_list[2].file_name);
}

void test_json_deserialize_url_download(void)
{
    char* buffer = "\"https://www.modbusdriver.com/downloads/modpoll.tgz\"";
    char url_download[100] = {0};

    TEST_ASSERT_TRUE(json_deserialize_url_download(buffer, strlen(buffer), url_download));
    TEST_ASSERT_EQUAL_STRING("https://www.modbusdriver.com/downloads/modpoll.tgz", url_download);

    TEST_ASSERT_FALSE(json_deserialize_url_download("https://www.modbusdriver.com/downloads/modpoll.tgz\"",
                                                    strlen("https://www.modbusdriver.com/downloads/modpoll.tgz\""),
                                                    url_download));
    TEST_ASSERT_FALSE(json_deserialize_url_download("\"https://www.modbusdriver.com/downloads/modpoll.tgz\"",
                                                    strlen("\"https://www.modbusdriver.com/downloads/modpoll.tgz"),
                                                    url_download));
    TEST_ASSERT_FALSE(json_deserialize_url_download("", 0, url_download));
}


#endif // TEST
