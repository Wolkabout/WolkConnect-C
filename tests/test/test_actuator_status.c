#include "unity.h"
#include "string.h"

#include "actuator_status.h"

void setUp(void)
{
}

void tearDown(void)
{
}

void test_actuator_status_actuator_status_init(void)
{
    actuator_status_t actuator_status;
    char value[PAYLOAD_SIZE];

    strncpy(value, "0", strlen("0"));
    actuator_status_init(&actuator_status, &value, ACTUATOR_STATE_READY);

    TEST_ASSERT_EQUAL_INT(ACTUATOR_STATE_READY, actuator_status.state);
    TEST_ASSERT_EQUAL_STRING(value, actuator_status.value);
}
