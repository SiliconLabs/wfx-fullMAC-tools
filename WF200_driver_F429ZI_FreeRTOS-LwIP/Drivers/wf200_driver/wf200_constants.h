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
 * \file wf200_constants.h
 * \brief wf200_constants.h contains defines, enums, structures and functions used by wf200 driver
 *
 */

#pragma once

#include "sl_status.h"
#include <stdint.h>

/******************************************************
 *                      Macros
 ******************************************************/

#define WF200_WAIT_FOREVER  0xFFFFFFF

#ifndef ROUND_UP
#define ROUND_UP(x,y)    ((x) % (y) ? (x) + (y)-((x)%(y)) : (x))
#endif /* ifndef ROUND_UP */
   
#define ERROR_CHECK(__status__) \
    do {\
        if(((sl_status_t)__status__) != SL_SUCCESS){\
            goto error_handler;\
        }\
    } while(0)


// little endian has Least Significant Byte First
#define PACK_32BIT_LITTLE_ENDIAN(_ptr_, _val_) do{\
    *((uint8_t*)(_ptr_)    ) = ((_val_) & 0x000000FF);\
    *((uint8_t*)(_ptr_) + 1) = ((_val_) & 0x0000FF00) >> 8;\
    *((uint8_t*)(_ptr_) + 2) = ((_val_) & 0x00FF0000) >> 16;\
    *((uint8_t*)(_ptr_) + 3) = ((_val_) & 0xFF000000) >> 24;\
}while(0);

#define UNPACK_32BIT_LITTLE_ENDIAN(_ptr_)\
   (((uint32_t)(*((uint8_t*)(_ptr_)    )))       |\
    ((uint32_t)(*((uint8_t*)(_ptr_) + 1))) << 8  |\
    ((uint32_t)(*((uint8_t*)(_ptr_) + 2))) << 16 |\
    ((uint32_t)(*((uint8_t*)(_ptr_) + 3))) << 24)

#define PACK_24BIT_LITTLE_ENDIAN(_ptr_, _val_) do{\
    *((uint8_t*)(_ptr_)    ) = ((_val_) & 0x000000FF);\
    *((uint8_t*)(_ptr_) + 1) = ((_val_) & 0x0000FF00) >> 8;\
    *((uint8_t*)(_ptr_) + 2) = ((_val_) & 0x00FF0000) >> 16;\
}while(0);

#define UNPACK_24BIT_LITTLE_ENDIAN(_ptr_)\
   (((uint32_t)(*((uint8_t*)(_ptr_)    )))       |\
    ((uint32_t)(*((uint8_t*)(_ptr_) + 1))) << 8  |\
    ((uint32_t)(*((uint8_t*)(_ptr_) + 2))) << 16)

#define PACK_16BIT_LITTLE_ENDIAN(_ptr_, _val_) do{\
    *((uint8_t*)(_ptr_)    ) = ((_val_) & 0x000000FF);\
    *((uint8_t*)(_ptr_) + 1) = ((_val_) & 0x0000FF00) >> 8;\
}while(0);

#define UNPACK_16BIT_LITTLE_ENDIAN(_ptr_)\
   (((uint32_t)(*((uint8_t*)(_ptr_)    )))       |\
    ((uint32_t)(*((uint8_t*)(_ptr_) + 1))) << 8)

// big endian has Most Significant Byte First
#define PACK_32BIT_BIG_ENDIAN(_ptr_, _val_) do{\
    *((uint8_t*)(_ptr_)    ) = ((_val_) & 0xFF000000) >> 24;\
    *((uint8_t*)(_ptr_) + 1) = ((_val_) & 0x00FF0000) >> 16;\
    *((uint8_t*)(_ptr_) + 2) = ((_val_) & 0x0000FF00) >> 8;\
    *((uint8_t*)(_ptr_) + 3) = ((_val_) & 0x000000FF);\
}while(0);

#define UNPACK_32BIT_BIG_ENDIAN(_ptr_)\
   (((uint32_t)(*((uint8_t*)(_ptr_)    ))) << 24 |\
    ((uint32_t)(*((uint8_t*)(_ptr_) + 1))) << 16 |\
    ((uint32_t)(*((uint8_t*)(_ptr_) + 2))) << 8  |\
    ((uint32_t)(*((uint8_t*)(_ptr_) + 3))))

#define PACK_24BIT_BIG_ENDIAN(_ptr_, _val_) do{\
    *((uint8_t*)(_ptr_)    ) = ((_val_) & 0x00FF0000) >> 16;\
    *((uint8_t*)(_ptr_) + 1) = ((_val_) & 0x0000FF00) >> 8;\
    *((uint8_t*)(_ptr_) + 2) = ((_val_) & 0x000000FF);\
}while(0);

#define UNPACK_24BIT_BIG_ENDIAN(_ptr_)\
   (((uint32_t)(*((uint8_t*)(_ptr_)    ))) << 16 |\
    ((uint32_t)(*((uint8_t*)(_ptr_) + 1))) << 8 |\
    ((uint32_t)(*((uint8_t*)(_ptr_) + 2))))

#define PACK_16BIT_BIG_ENDIAN(_ptr_, _val_) do{\
    *((uint8_t*)(_ptr_)    ) = ((_val_) & 0x0000FF00) >> 8;\
    *((uint8_t*)(_ptr_) + 1) = ((_val_) & 0x000000FF);\
}while(0);

#define UNPACK_16BIT_BIG_ENDIAN(_ptr_)\
   (((uint16_t)(*((uint8_t*)(_ptr_)    ))) << 8 |\
    ((uint16_t)(*((uint8_t*)(_ptr_) + 1))))

#define UNPACK_8BIT(_ptr_)\
   ((uint8_t)(*((uint8_t*)(_ptr_))))

/******************************************************
 *                    Constants
 ******************************************************/

#define WF200_BUS_NOTIFICATION                 (1 << 0)
#define WF200_IRQ_NOTIFICATION                 (1 << 1)
#define WF200_FRAME_CONFIRMATION_NOTIFICATION  (1 << 2)

#define IE_RSNE_ID                48
#define IE_RSNE_CIPHER_SUITE_TKIP 0x02AC0F00
#define IE_RSNE_CIPHER_SUITE_CCMP 0x04AC0F00

#define IE_VENDOR_SPECIFIC_ID 221
#define IE_WPA_OUI            0x0050F2

/* Below are copied from wf200 firmware pds.h */
#define PDS_HF_CLK_KEY            'e'
#define PDS_POWER_CONFIG_KEY      'h'
#define PDS_ANTENNA_SEL_KEY       'j'

/* ------------------------------------ */
/* JSON key indices */
#define PDS_KEY_A       'a'
#define PDS_KEY_B       'b'
#define PDS_KEY_C       'c'
#define PDS_KEY_D       'd'
#define PDS_KEY_E       'e'
#define PDS_KEY_F       'f'

#define WFX_PTE_INFO              0x0900C0C0
#define PTE_INFO_KEYSET_IDX       0x0D
#define PTE_INFO_SIZE             0x10

#define WF200_CONFIG_REVISION_OFFSET    24
#define WF200_CONFIG_REVISION_MASK      0x7
#define WF200_CONFIG_TYPE_OFFSET        31
#define WF200_CONFIG_TYPE_MASK          0x1

#define HI_EXCEPTION_IND_ID             0xe0

#define WF200_OPN_SIZE                  14

/******************************************************
 *                   Enumerations
 ******************************************************/

typedef enum
{
    WF200_CONFIG_REG_ID         = 0x0000,
    WF200_CONTROL_REG_ID        = 0x0001,
    WF200_IN_OUT_QUEUE_REG_ID   = 0x0002,
    WF200_AHB_DPORT_REG_ID      = 0x0003,
    WF200_SRAM_BASE_ADDR_REG_ID = 0x0004,
    WF200_SRAM_DPORT_REG_ID     = 0x0005,
    WF200_TSET_GEN_R_W_REG_ID   = 0x0006,
    WF200_FRAME_OUT_REG_ID      = 0x0007,
} wf200_register_address_t;

typedef enum
{
    WF200_STARTED                 = (1 << 0),
    WF200_STA_INTERFACE_CONNECTED = (1 << 1),
    WF200_AP_INTERFACE_UP         = (1 << 2),
} wf200_status_t;

/**
 * \enum    wf200_interface_t
 * \brief   Enum for available interface in wf200.
 * \details For convenience, interface 0 is associated with the station interface and interface 1 is associated with the softap interface.
 */
typedef enum
{
    WF200_STA_INTERFACE    = 0, /*!< Interface 0, linked to the station */
    WF200_SOFTAP_INTERFACE = 1, /*!< Interface 1, linked to the softap */
} wf200_interface_t;

/**
 * \enum   wf200_antenna_config_t
 * \brief  Enum describing antenna configuration to be sent in the PDS configuration (Platform data set)
 */
typedef enum
{
    WF200_ANTENNA_1_ONLY = 0, /*!< RF output 1 is used */
    WF200_ANTENNA_2_ONLY,     /*!< RF output 2 is used */
    WF200_ANTENNA_TX1_RX2,    /*!< RF output 1 is used for TX, RF 2 for RX */
    WF200_ANTENNA_TX2_RX1,    /*!< RF output 2 is used for TX, RF 1 for RX */
    WF200_ANTENNA_DIVERSITY   /*!< wf200 uses an antenna diversity algorithm */
} wf200_antenna_config_t;

/******************************************************
 *                    Structures
 ******************************************************/

/**
 * \struct wf200_basic_frame_t
 * \brief  Structure used to describe the header of messages exchanged with wf200
 */
typedef struct
{
    uint16_t wsm_len;
    uint16_t wsm_id;
    uint8_t  data[0];
} wf200_basic_frame_t;

/**
 * \struct wf200_buffer_t
 * \brief  Structure used to describe the header of messages exchanged with wf200
 */
typedef struct
{
    uint16_t msg_len;
    uint8_t  msg_id;
    uint8_t  msg_info;
    uint8_t  data[];
} wf200_buffer_t;

/**
 * \struct wf200_frame_t
 * \brief  Structure used to describe the frame exchanged with wf200
 */
typedef struct
{
    uint16_t msg_len;
    uint16_t msg_id;
    uint8_t  FrameType;
    uint8_t  Priority;
    uint16_t PacketId;
    uint32_t PacketDataLength;
    uint8_t  data[];
} wf200_frame_t;

/**
 * \struct wf200_ethernet_frame_t
 * \brief  Structure used to describe the ethernet frame exchanged with wf200
 */
typedef struct
{
    uint16_t wsm_len;
    uint16_t wsm_id;

    // Frame management
    uint8_t  frame_type;
    uint8_t  frame_padding;
    uint16_t frame_length;
    uint8_t  padding_and_data[0];
} wf200_ethernet_frame_t;

typedef struct
{
    uint8_t octet[ 6 ];
} wf200_mac_address_t;

typedef struct
{
    uint32_t hp_packet_count;
    uint32_t rx_packet_count;
    uint32_t tx_packet_count;
} wf200_nonce_t;

/**
 * \struct wf200_context_t
 * \brief  Structure used to maintain wf200 context on the host side
 */
typedef struct
{
    uint16_t data_frame_id;               /*!< Frame id incremented by ::wf200_send_ethernet_frame*/
    uint32_t last_command_id;             /*!< Last command id issued to wf200*/
    uint32_t waited_event_id;             /*!< Host waited event*/
    uint32_t posted_event_id;             /*!< Last event posted by wf200*/
    uint32_t events;
    uint16_t used_buffer_number;
    wf200_mac_address_t mac_addr_0;       /*!< Mac address used by wf200 interface 0, station*/
    wf200_mac_address_t mac_addr_1;       /*!< Mac address used by wf200 interface 1, softap*/
    uint8_t  event_payload_buffer[512];   /*!< Event payload associated with the last posted event*/
    uint8_t  ineo_opn[WF200_OPN_SIZE];    /*!< Required for PTE (Only ?)*/
    wf200_nonce_t secure_link_nonce;
} wf200_context_t;

typedef struct __attribute__((__packed__))
{
    uint32_t ssid_length;
    uint8_t  ssid[32];
    uint8_t  mac[6]; /*MAC address*/
    uint16_t channel; /*Channel*/
    uint32_t flags; /*Scan result flags*/
    uint16_t rcpi; /*RCPI*/
    uint16_t ie_data_length; /*Length of the IE data*/
    /* uint32_t IeData;              *//*IE data added to association request; only vendor-specific IEs*/
} wf200_scan_result_t;

typedef struct __attribute__((__packed__))
{
    uint32_t IndicationId;                     /*Identify the indication data.*/
    uint32_t NbRxFrame;                        /*Total number of frame received*/
    uint32_t NbCrcFrame;                       /*Number of frame received with bad CRC*/
    uint32_t PerTotal;                         /*PER on the total number of frame*/
    uint32_t Throughput;                       /*Throughput calculated on correct frames received*/
    uint32_t NbRxByRate[22];   /*Number of frame received by rate*/
    uint32_t Per[22];                /*PER*10000 by frame rate*/
    int32_t  Snr[22];                /*SNR in Db*100 by frame rate*/
    int32_t  Rssi[22];              /*RSSI in Dbm*100 by frame rate*/
} wf200_generic_status_t;

/******************************************************
 *             Static Inline Functions
 ******************************************************/

static inline uint8_t ie_get_element_id(uint8_t* base_pointer)
{
    // Element ID is the first octet in IE
    return *base_pointer;
}

static inline uint8_t ie_get_element_length(uint8_t* base_pointer)
{
    return *(base_pointer + 1);
}

static inline uint8_t* ie_get_next_element(uint8_t* base_pointer)
{
    return base_pointer + ie_get_element_length(base_pointer) + 2;
}

static inline uint16_t ie_get_rsne_version(uint8_t* base_pointer)
{
    return UNPACK_16BIT_LITTLE_ENDIAN(base_pointer + 2);
}

static inline uint32_t ie_get_rsne_group_data_cipher_suite(uint8_t* base_pointer)
{
    return UNPACK_32BIT_LITTLE_ENDIAN(base_pointer + 4);
}

static inline uint16_t ie_get_rsne_pairwise_data_cipher_suite_count(uint8_t* base_pointer)
{
    return UNPACK_16BIT_LITTLE_ENDIAN(base_pointer + 8);
}

static inline uint32_t ie_get_rsne_pairwise_data_cipher_suite(uint8_t* base_pointer, uint32_t index)
{
    uint8_t* item_index = base_pointer + 10 + 4 * index;
    return UNPACK_32BIT_LITTLE_ENDIAN(item_index);
}

static inline uint32_t ie_get_vendor_specific_oui(uint8_t* base_pointer)
{
    return UNPACK_24BIT_LITTLE_ENDIAN(base_pointer + 2);
}

static inline uint32_t ie_get_wpa_group_data_cipher_suite(uint8_t* base_pointer)
{
    return UNPACK_32BIT_LITTLE_ENDIAN(base_pointer + 8);
}

static inline uint16_t ie_get_wpa_pairwise_data_cipher_suite_count(uint8_t* base_pointer)
{
    return UNPACK_16BIT_LITTLE_ENDIAN(base_pointer + 12);
}

static inline uint32_t ie_get_wpa_pairwise_data_cipher_suite(uint8_t* base_pointer, uint32_t index)
{
    uint8_t* item_index = base_pointer + 14 + 4 * index;
    return UNPACK_32BIT_LITTLE_ENDIAN(item_index);
}
