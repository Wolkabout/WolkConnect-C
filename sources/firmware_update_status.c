/*
 * Copyright 2017-2018 WolkAbout Technology s.r.o.
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

#include "firmware_update_status.h"
#include "wolk_utils.h"

firmware_update_status_t firmware_update_status_ok(firmware_update_state_t state)
{
    firmware_update_status_t status;
    status.status = state;
    status.error = FIRMWARE_UPDATE_ERROR_NONE;

    return status;
}

firmware_update_status_t firmware_update_status_error(firmware_update_error_t error)
{
    firmware_update_status_t status;
    status.status = FIRMWARE_UPDATE_STATE_ERROR;
    status.error = error;

    return status;
}

firmware_update_state_t firmware_update_status_get_state(firmware_update_status_t* status)
{
    /* Sanity check */
    WOLK_ASSERT(status);

    return status->status;
}

firmware_update_error_t firmware_update_status_get_error(firmware_update_status_t* status)
{
    /* Sanity check */
    WOLK_ASSERT(status);

    return status->error;
}
