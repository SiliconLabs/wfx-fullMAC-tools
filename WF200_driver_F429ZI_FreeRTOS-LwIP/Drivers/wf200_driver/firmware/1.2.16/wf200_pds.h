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
 * \file wf200_pds.c
 * \brief contains the PDS configuration specific to a hardware configuration. This particular PDS is meant for the WF200 Wi-Fi EXP Board BRD8023.
 *
 */
 
#pragma once

#define ARRAY_COUNT(x) (sizeof (x) / sizeof *(x))

typedef struct
{
    const char* data;
    uint16_t    size;
} wf200_pds_t;

static const char pds0[] = "{a:{a:1,b:3},b:{a:{a:4,b:0,c:0,d:0,e:A},b:{a:4,b:0,c:0,d:0,e:B},c:{a:4,b:0,c:0,d:0,e:C},d:{a:4,b:0,c:0,d:0,e:D},e:{a:4,b:0,c:0,d:0,e:E},f:{a:4,b:0,c:0,d:0,e:F},g:{a:4,b:0,c:0,d:0,e:G},h:{a:4,b:0,c:0,d:0,e:H},i:{a:4,b:0,c:0,d:0,e:I},j:{a:4,b:0,c:0,d:0,e:J},k:{a:4,b:0,c:0,d:0,e:K},l:{a:4,b:0,c:0,d:1,e:L},m:{a:4,b:0,c:0,d:0,e:M},n:{a:4,b:0,c:0,d:0,e:O}},c:{a:{a:6,b:0,c:0},b:{a:6,b:0,c:0},c:{a:6,b:0,c:0},d:{a:6,b:0,c:0},e:{a:6,b:0,c:0},f:{a:6,b:0,c:0}}}";
static const char pds1[] = "{e:{a:{a:3,b:6E,c:6E},b:0,c:0}}";
static const char pds2[] = "{h:{e:0,a:[{a:E,b:50,c:50}],b:0,d:0}}";
static const char pds3[] = "{j:{a:0,b:0}}";

static const wf200_pds_t wf200_config_table[] =
{
    [0] = { .data = pds0, .size = sizeof( pds0 ) - 1 },
    [1] = { .data = pds1, .size = sizeof( pds1 ) - 1 },
    [2] = { .data = pds2, .size = sizeof( pds2 ) - 1 },
    [3] = { .data = pds3, .size = sizeof( pds3 ) - 1 },
};

static const uint8_t wf200_config_count = ARRAY_COUNT( wf200_config_table );
