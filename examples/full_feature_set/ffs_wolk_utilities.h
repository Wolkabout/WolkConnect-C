//
// Created by srdjanstankovic on 26.11.21..
//

#ifndef WOLKCONNECTOR_C_FFS_WOLK_UTILITIES_H
#define WOLKCONNECTOR_C_FFS_WOLK_UTILITIES_H

#include "permanent_storage.h"
#include "wolk_connector.h"
#include "wolk_utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define DEFAULT_PUBLISH_PERIOD_SECONDS 60

/* Actuation setup and call-backs */
const char* actuator_references[3];
uint32_t number_actuator_references;

/* Configuration setup and call-backs */
int publish_period_seconds;
char device_configuration_references[CONFIGURATION_ITEMS_SIZE][CONFIGURATION_REFERENCE_SIZE];
char device_configuration_values[CONFIGURATION_ITEMS_SIZE][CONFIGURATION_VALUE_SIZE];

void actuation_handler(const char* reference, const char* value);
actuator_status_t actuator_status_provider(const char* reference);

// Configuration
void update_default_device_configuration_values(char (*default_device_configuration_values)[CONFIGURATION_VALUE_SIZE],
                                                int default_value);
void configuration_handler(char (*reference)[CONFIGURATION_REFERENCE_SIZE], char (*value)[CONFIGURATION_VALUE_SIZE],
                           size_t num_configuration_items);
size_t configuration_provider(char (*reference)[CONFIGURATION_REFERENCE_SIZE], char (*value)[CONFIGURATION_VALUE_SIZE],
                              size_t max_num_configuration_items);

#endif // WOLKCONNECTOR_C_FFS_WOLK_UTILITIES_H
