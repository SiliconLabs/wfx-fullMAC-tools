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

/**
 * \file wf200_configuration.h
 * \brief wf200_configuration.h defines various values related to the bus communication between WF200 and the host MCU.
 *
 */

#pragma once

#define WF200_DEFAULT_REQUEST_TIMEOUT 5000  // Timeout period in milliseconds
#define WF200_JOIN_REQUEST_TIMEOUT    5000  // Timeout period in milliseconds

#ifndef WF200_SDIO_BLOCK_SIZE
#define WF200_SDIO_BLOCK_SIZE  64
#endif
