/**************************************************************************//**
 * Copyright 2018, Silicon Laboratories Inc.
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
 *****************************************************************************/

#ifndef SL_WFX_CONSTANTS_H
#define SL_WFX_CONSTANTS_H

#include "sl/sl_status.h"
#include <stdint.h>

/******************************************************
*                      Macros
******************************************************/
#define SL_WFX_UNUSED_VARIABLE(x) (void)(x)
#define SL_WFX_UNUSED_PARAMETER(x) (void)(x)

#ifndef SL_WFX_ARRAY_COUNT
#define SL_WFX_ARRAY_COUNT(x) (sizeof (x) / sizeof *(x))
#endif /* ifndef SL_WFX_ARRAY_COUNT */

#ifndef SL_WFX_ROUND_UP
#define SL_WFX_ROUND_UP(x, y)    ((x) % (y) ? (x) + (y) - ((x) % (y)) : (x))
#endif /* ifndef SL_WFX_ROUND_UP */

#ifndef SL_WFX_ROUND_UP_EVEN
#define SL_WFX_ROUND_UP_EVEN(x)    ((x) + ((x) & 1))
#endif /* ifndef SL_WFX_ROUND_UP_EVEN */

#define SL_WAIT_FOREVER  0xFFFFFFFF

#define SL_WFX_ERROR_CHECK(__status__)             \
  do {                                             \
    if (((sl_status_t)__status__) != SL_SUCCESS) { \
      goto error_handler;                          \
    }                                              \
  } while (0)

// little endian has Least Significant Byte First
#define sl_wfx_pack_32bit_little_endian(_ptr_, _val_) do {    \
    *((uint8_t *)(_ptr_)    ) = ((_val_) & 0x000000FF);       \
    *((uint8_t *)(_ptr_) + 1) = ((_val_) & 0x0000FF00) >> 8;  \
    *((uint8_t *)(_ptr_) + 2) = ((_val_) & 0x00FF0000) >> 16; \
    *((uint8_t *)(_ptr_) + 3) = ((_val_) & 0xFF000000) >> 24; \
} while (0)

#define sl_wfx_unpack_32bit_little_endian(_ptr_)    \
  (((uint32_t)(*((uint8_t *)(_ptr_)      )))        \
   | ((uint32_t)(*((uint8_t *)(_ptr_) + 1))) << 8   \
    | ((uint32_t)(*((uint8_t *)(_ptr_) + 2))) << 16 \
    | ((uint32_t)(*((uint8_t *)(_ptr_) + 3))) << 24)

#define sl_wfx_pack_24bit_little_endian(_ptr_, _val_) do {    \
    *((uint8_t *)(_ptr_)    ) = ((_val_) & 0x000000FF);       \
    *((uint8_t *)(_ptr_) + 1) = ((_val_) & 0x0000FF00) >> 8;  \
    *((uint8_t *)(_ptr_) + 2) = ((_val_) & 0x00FF0000) >> 16; \
} while (0)

#define sl_wfx_unpack_24bit_little_endian(_ptr_)  \
  (((uint32_t)(*((uint8_t *)(_ptr_)      )))      \
   | ((uint32_t)(*((uint8_t *)(_ptr_) + 1))) << 8 \
    | ((uint32_t)(*((uint8_t *)(_ptr_) + 2))) << 16)

#define sl_wfx_pack_16bit_little_endian(_ptr_, _val_) do {   \
    *((uint8_t *)(_ptr_)    ) = ((_val_) & 0x000000FF);      \
    *((uint8_t *)(_ptr_) + 1) = ((_val_) & 0x0000FF00) >> 8; \
} while (0)

#define sl_wfx_unpack_16bit_little_endian(_ptr_) \
  (((uint32_t)(*((uint8_t *)(_ptr_)      )))     \
   | ((uint32_t)(*((uint8_t *)(_ptr_) + 1))) << 8)

// big endian has Most Significant Byte First
#define sl_wfx_pack_32bit_big_endian(_ptr_, _val_) do {       \
    *((uint8_t *)(_ptr_)    ) = ((_val_) & 0xFF000000) >> 24; \
    *((uint8_t *)(_ptr_) + 1) = ((_val_) & 0x00FF0000) >> 16; \
    *((uint8_t *)(_ptr_) + 2) = ((_val_) & 0x0000FF00) >> 8;  \
    *((uint8_t *)(_ptr_) + 3) = ((_val_) & 0x000000FF);       \
} while (0)

#define sl_wfx_unpack_32bit_big_endian(_ptr_)       \
  (((uint32_t)(*((uint8_t *)(_ptr_)       ))) << 24 \
    | ((uint32_t)(*((uint8_t *)(_ptr_) + 1))) << 16 \
    | ((uint32_t)(*((uint8_t *)(_ptr_) + 2))) << 8  \
    | ((uint32_t)(*((uint8_t *)(_ptr_) + 3))))

#define sl_wfx_pack_24bit_big_endian(_ptr_, _val_) do {       \
    *((uint8_t *)(_ptr_)    ) = ((_val_) & 0x00FF0000) >> 16; \
    *((uint8_t *)(_ptr_) + 1) = ((_val_) & 0x0000FF00) >> 8;  \
    *((uint8_t *)(_ptr_) + 2) = ((_val_) & 0x000000FF);       \
} while (0)

#define sl_wfx_unpack_24bit_big_endian(_ptr_)       \
  (((uint32_t)(*((uint8_t *)(_ptr_)       ))) << 16 \
    | ((uint32_t)(*((uint8_t *)(_ptr_) + 1))) << 8  \
    | ((uint32_t)(*((uint8_t *)(_ptr_) + 2))))

#define sl_wfx_pack_16bit_big_endian(_ptr_, _val_) do {      \
    *((uint8_t *)(_ptr_)    ) = ((_val_) & 0x0000FF00) >> 8; \
    *((uint8_t *)(_ptr_) + 1) = ((_val_) & 0x000000FF);      \
} while (0)

#define sl_wfx_unpack_16bit_big_endian(_ptr_)     \
  (((uint16_t)(*((uint8_t *)(_ptr_)      ))) << 8 \
    | ((uint16_t)(*((uint8_t *)(_ptr_) + 1))))

#define sl_wfx_unpack_8bit(_ptr_) \
  ((uint8_t)(*((uint8_t *)(_ptr_))))

#define sl_wfx_swap_16(x) ((uint16_t) ((((x) >> 8) & 0xff) | (((x) & 0xff) << 8)))
#define sl_wfx_swap_32(x) ((((x) & 0xff000000u) >> 24) | (((x) & 0x00ff0000u) >> 8) | (((x) & 0x0000ff00u) << 8) | (((x) & 0x000000ffu) << 24))

static inline uint16_t bswap_16(uint16_t bsx)
{
  return sl_wfx_swap_16(bsx);
}

static inline uint32_t bswap_32(uint32_t bsx)
{
  return sl_wfx_swap_32(bsx);
}

static inline uint16_t uint16_identity(uint16_t x)
{
  return x;
}

static inline uint32_t uint32_identity(uint32_t x)
{
  return x;
}

#ifdef BIG_ENDIAN
#define sl_wfx_htobe16(x) uint16_identity(x)
#define sl_wfx_htole16(x) bswap_16(x)
#define sl_wfx_htobe32(x) uint32_identity(x)
#define sl_wfx_htole32(x) bswap_32(x)
#else
#define sl_wfx_htobe16(x) bswap_16(x)
#define sl_wfx_htole16(x) uint16_identity(x)
#define sl_wfx_htobe32(x) bswap_32(x)
#define sl_wfx_htole32(x) uint32_identity(x)
#endif

/******************************************************
*                    Constants
******************************************************/

#define SL_WAIT_FOREVER  0xFFFFFFFF
#define SL_WFX_BUS_NOTIFICATION                 (1 << 0)
#define SL_WFX_IRQ_NOTIFICATION                 (1 << 1)
#define SL_WFX_FRAME_CONFIRMATION_NOTIFICATION  (1 << 2)

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

#define SL_WFX_CONT_FRAME_TYPE_OFFSET    14

#define SL_WFX_CONFIG_REVISION_OFFSET    24
#define SL_WFX_CONFIG_REVISION_MASK      0x7
#define SL_WFX_CONFIG_TYPE_OFFSET        31
#define SL_WFX_CONFIG_TYPE_MASK          0x1

#define SL_WFX_OPN_SIZE                  14

#define SL_WFX_CTRL_REGISTER_SIZE        (2)

/* Secure link constants*/
#define SECURE_LINK_MAC_KEY_LENGTH      32

#define SL_WFX_SECURE_LINK_SESSION_KEY_LENGTH          (16)

#define SL_WFX_SECURE_LINK_ENCRYPTION_BITMAP_SIZE      (32)
#define SL_WFX_SECURE_LINK_ENCRYPTION_NOT_REQUIRED     (0)
#define SL_WFX_SECURE_LINK_ENCRYPTION_REQUIRED         (1)

#define SL_WFX_SECURE_LINK_HEADER_SIZE                 (4)
#define SL_WFX_SECURE_LINK_CCM_TAG_SIZE                (16)
#define SL_WFX_SECURE_LINK_NONCE_SIZE_BYTES            (12)
#define SL_WFX_SECURE_LINK_NONCE_COUNTER_MAX           (0x3FFFFFFFUL)  // Top two bits are used to indicate which counter
#define SL_WFX_SECURE_LINK_OVERHEAD                    (SL_WFX_SECURE_LINK_HEADER_SIZE + SL_WFX_SECURE_LINK_CCM_TAG_SIZE)

#define SL_WFX_SECURE_LINK_SESSION_KEY_BIT_COUNT       (SL_WFX_SECURE_LINK_SESSION_KEY_LENGTH * 8)

// Maximum nonce value is 2 ^ 30, watermark is chosen as 2 ^ 29
#define SL_WFX_SECURE_LINK_NONCE_WATERMARK             536870912

/******************************************************
*                   Enumerations
******************************************************/

typedef enum {
  SL_WFX_CONFIG_REG_ID         = 0x0000,
  SL_WFX_CONTROL_REG_ID        = 0x0001,
  SL_WFX_IN_OUT_QUEUE_REG_ID   = 0x0002,
  SL_WFX_AHB_DPORT_REG_ID      = 0x0003,
  SL_WFX_SRAM_BASE_ADDR_REG_ID = 0x0004,
  SL_WFX_SRAM_DPORT_REG_ID     = 0x0005,
  SL_WFX_TSET_GEN_R_W_REG_ID   = 0x0006,
  SL_WFX_FRAME_OUT_REG_ID      = 0x0007,
} sl_wfx_register_address_t;

typedef enum {
  SL_WFX_LINK_MODE_RESERVED      = 0,
  SL_WFX_LINK_MODE_UNTRUSTED     = 1,
  SL_WFX_LINK_MODE_TRUSTED_EVAL  = 2,
  SL_WFX_LINK_MODE_ACTIVE        = 3,
} sl_wfx_secure_link_mode_t;

typedef enum {
  SL_WFX_STARTED                 = (1 << 0),
  SL_WFX_STA_INTERFACE_CONNECTED = (1 << 1),
  SL_WFX_AP_INTERFACE_UP         = (1 << 2),
} sl_wfx_interface_status_t;

/**
 * \enum    sl_wfx_interface_t
 * \brief   Enum for available interface in wf200.
 * \details For convenience, interface 0 is associated with the station interface and interface 1 is associated with the softap interface.
 */
typedef enum {
  SL_WFX_STA_INTERFACE    = 0x00,   /*!< Interface 0, linked to the station */
  SL_WFX_SOFTAP_INTERFACE = 0x02,   /*!< Interface 1, linked to the softap */
} sl_wfx_interface_t;

/**
 * \enum   sl_wfx_antenna_config_t
 * \brief  Enum describing antenna configuration to be sent in the PDS configuration (Platform data set)
 */
typedef enum {
  SL_WFX_ANTENNA_1_ONLY = 0,   /*!< RF output 1 is used */
  SL_WFX_ANTENNA_2_ONLY,       /*!< RF output 2 is used */
  SL_WFX_ANTENNA_TX1_RX2,      /*!< RF output 1 is used for TX, RF 2 for RX */
  SL_WFX_ANTENNA_TX2_RX1,      /*!< RF output 2 is used for TX, RF 1 for RX */
  SL_WFX_ANTENNA_DIVERSITY     /*!< wf200 uses an antenna diversity algorithm */
} sl_wfx_antenna_config_t;

/**
 * \enum   sl_wfx_received_message_type_t
 * \brief  Enum listing different message types received from WF200. The information is found in the control register using SL_WFX_CONT_FRAME_TYPE_INFO mask.
 */
typedef enum {
  SL_WFX_CONFIRMATION_MESSAGE  = 0,   /*!< Frame type indicating a confirmation message is available */
  SL_WFX_INDICATION_MESSAGE    = 1,   /*!< Frame type indicating an indication message is available */
  SL_WFX_MANAGEMENT_MESSAGE    = 2,   /*!< Reserved from Low MAC interface */
  SL_WFX_ETHERNET_DATA_MESSAGE = 3,   /*!< Frame type indicating message encapsulating a data frame is available */
} sl_wfx_received_message_type_t;

/******************************************************
*                    Structures
******************************************************/

typedef struct {
  uint8_t octet[6];
} sl_wfx_mac_address_t;

typedef struct {
  uint32_t hp_packet_count;
  uint32_t rx_packet_count;
  uint32_t tx_packet_count;
} sl_wfx_nonce_t;

/**
 * \struct sl_wfx_context_t
 * \brief  Structure used to maintain wf200 context on the host side
 */
typedef struct {
  uint8_t  event_payload_buffer[512];     /*!< Event payload associated with the last posted event*/
  uint16_t data_frame_id;                 /*!< Frame id incremented by ::sl_wfx_send_ethernet_frame*/
  uint32_t waited_event_id;               /*!< Host waited event*/
  uint32_t posted_event_id;               /*!< Last event posted by wf200*/
  uint16_t used_buffer_number;
  sl_wfx_mac_address_t mac_addr_0;         /*!< Mac address used by wf200 interface 0, station*/
  sl_wfx_mac_address_t mac_addr_1;         /*!< Mac address used by wf200 interface 1, softap*/
  uint8_t  ineo_opn[SL_WFX_OPN_SIZE];      /*!< Required for PTE (Only ?)*/
} sl_wfx_context_t;

#endif // SL_WFX_CONSTANTS_H
