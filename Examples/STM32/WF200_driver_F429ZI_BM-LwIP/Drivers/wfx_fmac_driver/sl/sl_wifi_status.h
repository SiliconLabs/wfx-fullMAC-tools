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

#ifndef __SL_WIFI_STATUS_H
#define __SL_WIFI_STATUS_H

#define SL_WIFI_ENUM_OFFSET        1000
#define SL_STATUS_ENUM( prefix, name, value )  prefix ## _ ## name = (prefix##_ENUM_OFFSET + value)

#define SL_WIFI_STATUS_LIST( prefix )  \
    SL_STATUS_ENUM( prefix, INVALID_KEY,                   4 ),   /**< Invalid key */                       \
    SL_STATUS_ENUM( prefix, DOES_NOT_EXIST,                5 ),   /**< Does not exist */                    \
    SL_STATUS_ENUM( prefix, NOT_AUTHENTICATED,             6 ),   /**< Not authenticated */                 \
    SL_STATUS_ENUM( prefix, NOT_KEYED,                     7 ),   /**< Not keyed */                         \
    SL_STATUS_ENUM( prefix, IOCTL_FAIL,                    8 ),   /**< IOCTL fail */                        \
    SL_STATUS_ENUM( prefix, BUFFER_UNAVAILABLE_TEMPORARY,  9 ),   /**< Buffer unavailable temporarily */    \
    SL_STATUS_ENUM( prefix, BUFFER_UNAVAILABLE_PERMANENT, 10 ),   /**< Buffer unavailable permanently */    \
    SL_STATUS_ENUM( prefix, WPS_PBC_OVERLAP,              11 ),   /**< WPS PBC overlap */                   \
    SL_STATUS_ENUM( prefix, CONNECTION_LOST,              12 ),   /**< Connection lost */                   \
    SL_STATUS_ENUM( prefix, OUT_OF_EVENT_HANDLER_SPACE,   13 ),   /**< Cannot add extra event handler */    \
    SL_STATUS_ENUM( prefix, SEMAPHORE_ERROR,              14 ),   /**< Error manipulating a semaphore */    \
    SL_STATUS_ENUM( prefix, FLOW_CONTROLLED,              15 ),   /**< Packet retrieval cancelled due to flow control */ \
    SL_STATUS_ENUM( prefix, NO_CREDITS,                   16 ),   /**< Packet retrieval cancelled due to lack of bus credits */ \
    SL_STATUS_ENUM( prefix, NO_PACKET_TO_SEND,            17 ),   /**< Packet retrieval cancelled due to no pending packets */ \
    SL_STATUS_ENUM( prefix, CORE_CLOCK_NOT_ENABLED,       18 ),   /**< Core disabled due to no clock */    \
    SL_STATUS_ENUM( prefix, CORE_IN_RESET,                19 ),   /**< Core disabled - in reset */         \
    SL_STATUS_ENUM( prefix, UNSUPPORTED,                  20 ),   /**< Unsupported function */             \
    SL_STATUS_ENUM( prefix, BUS_WRITE_REGISTER_ERROR,     21 ),   /**< Error writing to WLAN register */   \
    SL_STATUS_ENUM( prefix, SDIO_BUS_UP_FAIL,             22 ),   /**< SDIO bus failed to come up */       \
    SL_STATUS_ENUM( prefix, JOIN_IN_PROGRESS,             23 ),   /**< Join not finished yet */   \
    SL_STATUS_ENUM( prefix, NETWORK_NOT_FOUND,            24 ),   /**< Specified network was not found */   \
    SL_STATUS_ENUM( prefix, INVALID_JOIN_STATUS,          25 ),   /**< Join status error */   \
    SL_STATUS_ENUM( prefix, UNKNOWN_INTERFACE,            26 ),   /**< Unknown interface specified */ \
    SL_STATUS_ENUM( prefix, SDIO_RX_FAIL,                 27 ),   /**< Error during SDIO receive */   \
    SL_STATUS_ENUM( prefix, HWTAG_MISMATCH,               28 ),   /**< Hardware tag header corrupt */   \
    SL_STATUS_ENUM( prefix, RX_BUFFER_ALLOC_FAIL,         29 ),   /**< Failed to allocate a buffer to receive into */   \
    SL_STATUS_ENUM( prefix, BUS_READ_REGISTER_ERROR,      30 ),   /**< Error reading a bus hardware register */   \
    SL_STATUS_ENUM( prefix, THREAD_CREATE_FAILED,         31 ),   /**< Failed to create a new thread */   \
    SL_STATUS_ENUM( prefix, QUEUE_ERROR,                  32 ),   /**< Error manipulating a queue */   \
    SL_STATUS_ENUM( prefix, BUFFER_POINTER_MOVE_ERROR,    33 ),   /**< Error moving the current pointer of a packet buffer  */   \
    SL_STATUS_ENUM( prefix, BUFFER_SIZE_SET_ERROR,        34 ),   /**< Error setting size of packet buffer */   \
    SL_STATUS_ENUM( prefix, THREAD_STACK_NULL,            35 ),   /**< Null stack pointer passed when non null was reqired */   \
    SL_STATUS_ENUM( prefix, THREAD_DELETE_FAIL,           36 ),   /**< Error deleting a thread */   \
    SL_STATUS_ENUM( prefix, SLEEP_ERROR,                  37 ),   /**< Error sleeping a thread */ \
    SL_STATUS_ENUM( prefix, BUFFER_ALLOC_FAIL,            38 ),   /**< Failed to allocate a packet buffer */ \
    SL_STATUS_ENUM( prefix, NO_PACKET_TO_RECEIVE,         39 ),   /**< No Packets waiting to be received */ \
    SL_STATUS_ENUM( prefix, INTERFACE_NOT_UP,             40 ),   /**< Requested interface is not active */ \
    SL_STATUS_ENUM( prefix, DELAY_TOO_LONG,               41 ),   /**< Requested delay is too long */ \
    SL_STATUS_ENUM( prefix, INVALID_DUTY_CYCLE,           42 ),   /**< Duty cycle is outside limit 0 to 0 */ \
    SL_STATUS_ENUM( prefix, PMK_WRONG_LENGTH,             43 ),   /**< Returned pmk was the wrong length */ \
    SL_STATUS_ENUM( prefix, UNKNOWN_SECURITY_TYPE,        44 ),   /**< AP security type was unknown */ \
    SL_STATUS_ENUM( prefix, WEP_NOT_ALLOWED,              45 ),   /**< AP not allowed to use WEP - it is not secure - use Open instead */ \
    SL_STATUS_ENUM( prefix, WPA_KEYLEN_BAD,               46 ),   /**< WPA / WPA2 key length must be between 8 & 64 bytes */ \
    SL_STATUS_ENUM( prefix, FILTER_NOT_FOUND,             47 ),   /**< Specified filter id not found */ \
    SL_STATUS_ENUM( prefix, SPI_ID_READ_FAIL,             48 ),   /**< Failed to read 0xfeedbead SPI id from chip */ \
    SL_STATUS_ENUM( prefix, SPI_SIZE_MISMATCH,            49 ),   /**< Mismatch in sizes between SPI header and SDPCM header */ \
    SL_STATUS_ENUM( prefix, ADDRESS_ALREADY_REGISTERED,   50 ),   /**< Attempt to register a multicast address twice */ \
    SL_STATUS_ENUM( prefix, SDIO_RETRIES_EXCEEDED,        51 ),   /**< SDIO transfer failed too many times. */ \
    SL_STATUS_ENUM( prefix, NULL_PTR_ARG,                 52 ),   /**< Null Pointer argument passed to function. */ \
    SL_STATUS_ENUM( prefix, THREAD_FINISH_FAIL,           53 ),   /**< Error deleting a thread */ \
    SL_STATUS_ENUM( prefix, WAIT_ABORTED,                 54 ),   /**< Semaphore/mutex wait has been aborted */ \
    SL_STATUS_ENUM( prefix, QUEUE_MESSAGE_UNALIGNED,      55 ), \
    SL_STATUS_ENUM( prefix, MUTEX_ERROR,                  56 ), \
    SL_STATUS_ENUM( prefix, FIRMWARE_DOWNLOAD_TIMEOUT,    57 ),

#endif // __SL_WIFI_STATUS_H
