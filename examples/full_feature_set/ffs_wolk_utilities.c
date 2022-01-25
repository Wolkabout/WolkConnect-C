//
// Created by srdjanstankovic on 26.11.21..
//
#include "ffs_wolk_utilities.h"

void actuation_handler(const char* reference, const char* value)
{
    bool existing_reference = false;

    for (int i = 0; i < number_actuator_references; ++i) {
        if (strcmp(reference, actuator_references[i]) == 0) {
            if (!permanent_storage_write(reference, value))
                printf("Actuation handler - Fail to write reference: %s with value: %s to permanent storage\n",
                       reference, value);
            else
                printf("Actuation handler - Reference: %s Value: %s\n", reference, value);
            existing_reference = true;
        }
    }

    if (!existing_reference)
        printf("Actuation handler - Wrong Reference\n");
}

actuator_status_t actuator_status_provider(const char* reference)
{
    char value[READING_SIZE];
    actuator_status_t actuator_status;
    actuator_status_init(&actuator_status, "", ACTUATOR_STATE_ERROR);

    for (int i = 0; i < number_actuator_references; ++i) {
        if (strcmp(reference, actuator_references[i]) == 0) {
            if (!permanent_storage_read(reference, value)) {
                printf("Actuator status provider - Fail to read reference: %s from permanent storage\n", reference);
                actuator_status_init(&actuator_status, "", ACTUATOR_STATE_ERROR);
            } else {
                printf("Actuator status provider - Reference: %s with value: %s\n", reference, value);
                actuator_status_init(&actuator_status, value, ACTUATOR_STATE_READY);
            }
        }
    }

    return actuator_status;
}

void update_default_device_configuration_values(char (*default_device_configuration_values)[CONFIGURATION_VALUE_SIZE],
                                                int default_value)
{
    char default_publish_period[10];

    int number_size = snprintf(default_publish_period, 10, "%d", default_value);
    strncpy(&default_device_configuration_values[0], default_publish_period, (unsigned long)number_size);
}

void configuration_handler(char (*reference)[CONFIGURATION_REFERENCE_SIZE], char (*value)[CONFIGURATION_VALUE_SIZE],
                           size_t num_configuration_items)
{
    for (size_t i = 0; i < num_configuration_items; ++i) {
        size_t iteration_counter = 0;

        for (size_t j = 0; j < CONFIGURATION_ITEMS_SIZE; ++j) {
            if (!strcmp(reference[i], device_configuration_references[j])) {
                strcpy(device_configuration_values[j], value[i]);
                printf("Configuration handler - Reference: %s | Value: %s\n", reference[i], value[i]);

                if (!strcmp(reference[i], device_configuration_references[0])) {
                    publish_period_seconds = atoi(value[i]);
                }
            } else
                iteration_counter++;

            if (iteration_counter == CONFIGURATION_ITEMS_SIZE) {
                printf("Unrecognised Reference received!\n");
            }
        }
    }
}

size_t configuration_provider(char (*reference)[CONFIGURATION_REFERENCE_SIZE], char (*value)[CONFIGURATION_VALUE_SIZE],
                              size_t max_num_configuration_items)
{
    WOLK_UNUSED(max_num_configuration_items);
    WOLK_ASSERT(max_num_configuration_items >= NUMBER_OF_CONFIGURATION);

    for (size_t i = 0; i < CONFIGURATION_ITEMS_SIZE; ++i) {
        strcpy(reference[i], device_configuration_references[i]);
        strncpy(value[i], device_configuration_values[i], CONFIGURATION_VALUE_SIZE);
        printf("Configuration provider - Reference: %s | Value: %s\n", reference[i], value[i]);
    }

    return CONFIGURATION_ITEMS_SIZE;
}
