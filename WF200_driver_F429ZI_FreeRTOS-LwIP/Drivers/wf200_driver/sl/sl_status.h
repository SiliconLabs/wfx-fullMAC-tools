/*
* Copyright 2018, Silicon Laboratories Inc.  All rights reserved.
* 
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
* 
*    http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*/

#pragma once

#include "sl_wifi_status.h"

typedef enum
{
    SL_SUCCESS          = 0,
    SL_PENDING          = 1,
    SL_TIMEOUT          = 2,
    SL_PARTIAL_RESULTS  = 3,
    SL_ERROR            = 4,
    SL_BAD_ARG          = 5,
    SL_BAD_OPTION       = 6,
    SL_UNSUPPORTED      = 7,
    SL_ERROR_OUT_OF_BUFFERS = 8,
    SL_ERROR_OUT_OF_HEAP = 9,
    SL_COMMAND_ARGUMENT_ERROR = 10,
    SL_WIFI_FRAME_RECEIVED = 11,
    SL_WIFI_STATUS_LIST( SL_WIFI )

    SL_FORCE_ENUM_TO_32_BIT = 0xFFFFFFFF,
} sl_status_t;

