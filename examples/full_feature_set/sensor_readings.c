/*
 * Copyright 2020 WolkAbout Technology s.r.o.
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

#include "sensor_readings.h"

static bool device_configuration_enable_feeds[4][1] = {true, true, true, true};

static void sending_pressure_reading(wolk_ctx_t* ctx)
{
    if (device_configuration_enable_feeds[2][0]) {
        int8_t pressure = 0;

        pressure = (rand() % 800) + 300;
        wolk_add_numeric_sensor_reading(ctx, "P", pressure, 0);
        wolk_publish(ctx);
        printf("\tPressure\t: %dmb\n", pressure);
    }
}

static void sending_temperature_reading(wolk_ctx_t* ctx)
{
    if (device_configuration_enable_feeds[0][0]) {
        int8_t temperature = 0;

        temperature = (rand() % 125) - 40;
        wolk_add_numeric_sensor_reading(ctx, "T", temperature, 0);
        wolk_publish(ctx);
        printf("\tTemperature\t: %d°C\n", temperature);
    }
}

static void sending_humidity_reading(wolk_ctx_t* ctx)
{
    if (device_configuration_enable_feeds[1][0]) {
        int8_t humidity = 0;

        humidity = rand() % 100;
        wolk_add_numeric_sensor_reading(ctx, "H", humidity, 0);
        wolk_publish(ctx);
        printf("\tHumidity\t: %d%%\n", humidity);
    }
}

static void sending_accl_readings(wolk_ctx_t* ctx)
{
    if (device_configuration_enable_feeds[3][0]) {
        double accl_readings[3] = {1, 0, 0};

        wolk_add_multi_value_numeric_sensor_reading(ctx, "ACL", &accl_readings[0], 3, 0);
        printf("\tAcceleration on XYZ\t: %fg, %fg, %fg\n", accl_readings[0], accl_readings[1], accl_readings[2]);
        wolk_publish(ctx);
    }
}


bool enable_feeds(char* value)
{
    int8_t elements_counter = 0;
    const char delimiter[2] = ",";
    char tmp_value[CONFIGURATION_VALUE_SIZE];
    strncpy(tmp_value, value, strlen(value));
    char* element = strtok(tmp_value, delimiter);

    for (int i = 0; i < 4; i++) {
        device_configuration_enable_feeds[i][0] = false;
    }

    while (element != NULL) {
        if (!strcmp(element, "T"))
            device_configuration_enable_feeds[0][0] = true;
        else if (!strcmp(element, "H"))
            device_configuration_enable_feeds[1][0] = true;
        else if (!strcmp(element, "P"))
            device_configuration_enable_feeds[2][0] = true;
        else if (!strcmp(element, "ACL"))
            device_configuration_enable_feeds[3][0] = true;

        element = strtok(NULL, delimiter);

        if (elements_counter++ > 4)
            break;
    }
    return 0;
}

bool sensor_readings_process(wolk_ctx_t* ctx, int* publish_period_seconds, int default_publish_value)
{
    static double publish_period_counter = 0;

    if (publish_period_seconds < default_publish_value)
        return 1;

    publish_period_counter++;
    if (publish_period_counter > (*publish_period_seconds) * 1000) {
        printf("Sending sensor readings:\n");
        sending_pressure_reading(ctx);
        sending_temperature_reading(ctx);
        sending_humidity_reading(ctx);
        sending_accl_readings(ctx);

        publish_period_counter = 0;
    }
    return 0;
}
