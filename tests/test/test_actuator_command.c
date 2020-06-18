#include "unity.h"
#include "string.h"

#include "actuator_command.h"

void setUp(void)
{
}

void tearDown(void)
{
}

void test_actuator_command_actuator_command_init(void)
{
    actuator_command_t command;
    char reference[MANIFEST_ITEM_REFERENCE_SIZE];
    char argument[PAYLOAD_SIZE];

    strncpy(reference, "reference", strlen("reference"));
    strncpy(argument, "off", strlen("off"));

    actuator_command_init(&command, ACTUATOR_COMMAND_TYPE_SET, reference, argument);
    TEST_ASSERT_EQUAL_INT(ACTUATOR_COMMAND_TYPE_SET, command.type);
    TEST_ASSERT_EQUAL_STRING(reference, command.reference);
    TEST_ASSERT_EQUAL_STRING(argument, command.argument);
}

void test_actuator_actuator_command_get_type(void)
{
    actuator_command_t command;

    actuator_command_init(&command, ACTUATOR_COMMAND_TYPE_STATUS, "", "");

    TEST_ASSERT_EQUAL_INT(ACTUATOR_COMMAND_TYPE_STATUS, actuator_command_get_type(&command));
}

void test_actuator_actuator_command_get_reference(void)
{
    actuator_command_t command;
    char reference[MANIFEST_ITEM_REFERENCE_SIZE];

    strncpy(reference, "reference", strlen("reference"));
    actuator_command_init(&command, ACTUATOR_COMMAND_TYPE_STATUS, reference, "");

    TEST_ASSERT_EQUAL_STRING(reference, actuator_command_get_reference(&command));
}

void test_actuator_actuator_command_set_reference(void)
{
    actuator_command_t command;
    char reference[MANIFEST_ITEM_REFERENCE_SIZE];

    actuator_command_init(&command, ACTUATOR_COMMAND_TYPE_STATUS, "", "");

    strncpy(reference, "reference", strlen("reference"));

    actuator_command_set_reference(&command, reference);
    TEST_ASSERT_EQUAL_STRING(reference, actuator_command_get_reference(&command));
}

void test_actuator_actuator_command_get_value(void)
{
    actuator_command_t command;
    char argument[PAYLOAD_SIZE];

    strncpy(argument, "32.1", strlen("32.1"));
    actuator_command_init(&command, ACTUATOR_COMMAND_TYPE_STATUS, "", argument);

    TEST_ASSERT_EQUAL_STRING(argument, actuator_command_get_value(&command));
}
