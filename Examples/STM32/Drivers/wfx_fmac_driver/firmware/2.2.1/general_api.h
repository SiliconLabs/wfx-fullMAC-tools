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

#ifndef _GENERAL_API_H_
#define _GENERAL_API_H_

#ifdef __KERNEL__ // Linux kernel does not accept standard headers
#include <linux/types.h>
#else
#include <stdint.h>
#endif

//< API Internal Version encoding
#define SL_WFX_API_VERSION_MINOR    0x02
#define SL_WFX_API_VERSION_MAJOR    0x01

#define SL_WFX_SSID_SIZE           32
#define SL_WFX_MAC_ADDR_SIZE       6
#define SL_WFX_BSSID_SIZE          SL_WFX_MAC_ADDR_SIZE

#define GENERAL_INTERFACE_ID    2

#define SL_WFX_MSG_ID_MASK          0x00FF
#define SL_WFX_MSG_TYPE_MASK        0x80
#define SL_WFX_MSG_SEQ_RANGE        0x0007 //range of field 'host_count' in msginfo_bitfield_t

/* Message bases */
#define SL_WFX_REQ_BASE             0x00
#define SL_WFX_CNF_BASE             0x00
#define SL_WFX_IND_BASE             SL_WFX_MSG_TYPE_MASK

/**
 * @brief List of possible transmission rates.
 *
 * Note that ERP-PBCC is not supported by the hardware. The rate indices for 22 Mbit/s and 33 Mbit/s are only provided for standard compatibility.@n
 * Data rates (in the names) are for 20 MHz channel operation. Corresponding data rates for 10 MHz channel operation are half of them.
 *
 * In this API, some parameters such as 'basic_rate_set' encode a list of rates in a bitstream format.@n
 *     for instance SUPPORTED_B_RATES_MASK = 0x0000000F @n
 *                  SUPPORTED_G_RATES_MASK = 0x00003FC0 @n
 *                  SUPPORTED_N_RATES_MASK = 0x003FC000
 */
typedef enum sl_wfx_rate_index_e {
  SL_WFX_RATE_INDEX_B_1MBPS                   = 0,             ///< Data rate 802.11b 1Mbps
  SL_WFX_RATE_INDEX_B_2MBPS                   = 1,             ///< Data rate 802.11b 2Mbps
  SL_WFX_RATE_INDEX_B_5P5MBPS                 = 2,             ///< Data rate 802.11b 5.5Mbps
  SL_WFX_RATE_INDEX_B_11MBPS                  = 3,             ///< Data rate 802.11b 11Mbps
  SL_WFX_RATE_INDEX_PBCC_22MBPS               = 4,             ///<ERP-PBCC, not supported
  SL_WFX_RATE_INDEX_PBCC_33MBPS               = 5,             ///<ERP-PBCC, not supported
  SL_WFX_RATE_INDEX_G_6MBPS                   = 6,             ///< Data rate 802.11g 6Mbps
  SL_WFX_RATE_INDEX_G_9MBPS                   = 7,             ///< Data rate 802.11g 9Mbps
  SL_WFX_RATE_INDEX_G_12MBPS                  = 8,             ///< Data rate 802.11g 12Mbps
  SL_WFX_RATE_INDEX_G_18MBPS                  = 9,             ///< Data rate 802.11g 18Mbps
  SL_WFX_RATE_INDEX_G_24MBPS                  = 10,            ///< Data rate 802.11g 24Mbps
  SL_WFX_RATE_INDEX_G_36MBPS                  = 11,            ///< Data rate 802.11g 36Mbps
  SL_WFX_RATE_INDEX_G_48MBPS                  = 12,            ///< Data rate 802.11g 48Mbps
  SL_WFX_RATE_INDEX_G_54MBPS                  = 13,            ///< Data rate 802.11g 54Mbps
  SL_WFX_RATE_INDEX_N_6P5MBPS                 = 14,            ///< Data rate 802.11n 6.5Mbps
  SL_WFX_RATE_INDEX_N_13MBPS                  = 15,            ///< Data rate 802.11n 13Mbps
  SL_WFX_RATE_INDEX_N_19P5MBPS                = 16,            ///< Data rate 802.11n 19.5Mbps
  SL_WFX_RATE_INDEX_N_26MBPS                  = 17,            ///< Data rate 802.11n 26Mbps
  SL_WFX_RATE_INDEX_N_39MBPS                  = 18,            ///< Data rate 802.11n 39Mbps
  SL_WFX_RATE_INDEX_N_52MBPS                  = 19,            ///< Data rate 802.11n 52Mbps
  SL_WFX_RATE_INDEX_N_58P5MBPS                = 20,            ///< Data rate 802.11n 58.5Mbps
  SL_WFX_RATE_INDEX_N_65MBPS                  = 21,            ///< Data rate 802.11n 65Mbps
  SL_WFX_RATE_NUM_ENTRIES                     = 22             ///< Number of defined data rates
} sl_wfx_rate_index_t;

/**
 * @addtogroup MESSAGE_CONSTRUCTION
 * @brief interface message formating
 *
 * The WLAN API handles 3 types of messages.
 * REQUEST, CONFIRMATION and INDICATION
 *
   \msc
   arcgradient = 8;
   a [label="Wlan"],b [label="Host"];
   a=>b [label="INDICATION"];
   b=>b [label="Process indication"];
   ...;
    ---;
   ...;
   a<=b [label="REQUEST"];
   a=>a [label="Process request"];
   a=>b [label="CONFIRMATION"];
   \endmsc
 *
 * WLAN can send an INDICATION message to the host at any time.@n
 * Host can send REQUEST to the WLAN and the WLAN will always answer to it (either immediately or after a while) with a CONFIRMATION.
 *
 * A message is composed of a header and a body (see sl_wfx_generic_msg_t).
 * In the rest of the API description, only the body is detailed for each message.
 *
 * @{
 */

/**
 * @brief General Message header structure
 *
 */
typedef struct __attribute__((__packed__)) sl_wfx_header_s {
  uint16_t length;       ///< Message length in bytes including this uint16_t. Maximum value is 8188 but maximum Request size is FW dependent and reported in the ::sl_wfx_startup_ind_body_t::size_inp_ch_buf
  uint8_t  id;           ///< TODO comment missing
  uint8_t  info;         ///< TODO comment missing
} sl_wfx_header_t;

/**
 * @brief Generic message structure for all requests, confirmations and indications
 *
 */
typedef struct __attribute__((__packed__)) sl_wfx_generic_message_s {
  sl_wfx_header_t header;  ///<4 bytes header
  uint8_t body[0];         ///<variable size payload of the message
} sl_wfx_generic_message_t;

/**
 * @brief Generic confirmation message with the body reduced to the status field.
 *
 * This structure is not related to a specific confirmation ID. @n
 * It is a global simplified structure that can be used to easily access the header and status fields.
 *
 * All confirmation bodies start with a status word and in a lot of them it is followed by other data (not present in this structure).
 */
typedef struct __attribute__((__packed__)) sl_wfx_generic_confirmation_s {
  sl_wfx_header_t  header;       ///<4 bytes header
  uint32_t    status;            ///<See enum sl_wfx_status_t and (wsm_status or wfm_status)
} sl_wfx_generic_confirmation_t;

/**
 * @}
 */

/**
 * @addtogroup GENERAL_API
 * @brief General API messages available in both split and full MAC.
 *
 * Mainly used to boot and configure the part.@n
 * But some message are also used to report errors or information.@n
 *@n
 * \arg general \b requests are sl_wfx_generic_requests_ids_t@n
 * \arg general \b indications are sl_wfx_general_indications_ids_t@n
 * @n
 * @{
 */

/**
 * @brief General request message IDs
 *
 * API general request message IDs available in both split and full MAC.
 * These are messages from the host towards the WLAN.
 */
typedef enum sl_wfx_generic_requests_ids_e {
  SL_WFX_CONFIGURATION_REQ_ID                         = 0x09,///< \b CONFIGURATION request Id use body sl_wfx_configuration_req_body_t and returns sl_wfx_configuration_cnf_body_t
  SL_WFX_CONTROL_GPIO_REQ_ID                          = 0x26,///< \b CONTROL_GPIO request Id use body sl_wfx_control_gpio_req_body_t and returns sl_wfx_control_gpio_cnf_body_t
  SL_WFX_SET_SL_MAC_KEY_REQ_ID                        = 0x27,///< \b SET_SL_MAC_KEY request Id use body sl_wfx_set_sl_mac_key_req_body_t and returns sl_wfx_set_sl_mac_key_cnf_body_t
  SL_WFX_SECURELINK_EXCHANGE_PUB_KEYS_REQ_ID                  = 0x28,///< \b SL_EXCHANGE_PUB_KEYS request Id use body sl_wfx_securelink_exchange_pub_keys_req_body_t and returns sl_wfx_securelink_exchange_pub_keys_cnf_body_t
  SL_WFX_SECURELINK_CONFIGURE_REQ_ID                          = 0x29,///< \b SL_CONFIGURE request Id use body sl_wfx_securelink_configure_req_body_t and returns sl_wfx_securelink_exchange_pub_keys_cnf_body_t
  SL_WFX_PREVENT_ROLLBACK_REQ_ID                      = 0x2a,///< \b PREVENT_ROLLBACK request Id use body sl_wfx_prevent_rollback_req_body_t and returns sl_wfx_prevent_rollback_cnf_body_t
  SL_WFX_SHUT_DOWN_REQ_ID                             = 0x32,///< \b SHUT_DOWN request Id use body sl_wfx_shut_down_req_t and never returns
} sl_wfx_generic_requests_ids_t;

/**
 * @brief General confirmation message IDs
 *
 * API general confirmation message IDs returned by requests described in sl_wfx_general_requests_ids.
 * These are messages from the WLAN towards the host.
 */
typedef enum sl_wfx_general_confirmations_ids_e {
  SL_WFX_CONFIGURATION_CNF_ID                         = 0x09,///< \b CONFIGURATION confirmation Id returns body sl_wfx_configuration_cnf_body_t
  SL_WFX_CONTROL_GPIO_CNF_ID                          = 0x26,///< \b CONTROL_GPIO confirmation Id returns body sl_wfx_control_gpio_cnf_body_t
  SL_WFX_SET_SL_MAC_KEY_CNF_ID                        = 0x27,///< \b SET_SL_MAC_KEY confirmation Id returns body sl_wfx_set_sl_mac_key_cnf_body_t
  SL_WFX_SECURELINK_EXCHANGE_PUB_KEYS_CNF_ID                  = 0x28,///< \b SL_EXCHANGE_PUB_KEYS confirmation Id returns body sl_wfx_securelink_exchange_pub_keys_cnf_body_t
  SL_WFX_SECURELINK_CONFIGURE_CNF_ID                          = 0x29,///< \b SL_CONFIGURE confirmation Id returns body sl_wfx_securelink_configure_cnf_body_t
  SL_WFX_PREVENT_ROLLBACK_CNF_ID                      = 0xe7,///< \b PREVENT_ROLLBACK confirmation Id use body sl_wfx_prevent_rollback_cnf_body_t
} sl_wfx_general_confirmations_ids_t;

/**
 * @brief General indications message IDs
 *
 * API general indication message IDs available in both split and full MAC.
 * These are messages from the WLAN towards the host.
 */
typedef enum sl_wfx_general_indications_ids_e {
  SL_WFX_EXCEPTION_IND_ID                             = 0xe0,///< \b EXCEPTION indication Id content is sl_wfx_exception_ind_body_t
  SL_WFX_STARTUP_IND_ID                               = 0xe1,///< \b STARTUP indication Id content is sl_wfx_startup_ind_body_t
  SL_WFX_GENERIC_IND_ID                               = 0xe3,///< \b GENERIC indication Id content is sl_wfx_generic_ind_body_t
  SL_WFX_ERROR_IND_ID                                 = 0xe4///< \b ERROR indication Id content is sl_wfx_error_ind_body_t
} sl_wfx_general_indications_ids_t;

/**
 * @brief General command message IDs
 *
 * All general API message IDs.
 */
typedef union sl_wfx_general_commands_ids_u {
  sl_wfx_generic_requests_ids_t request; ///< Request from the host to the wlan device
  sl_wfx_general_confirmations_ids_t confirmation; ///< Confirmation of a request from the wlan device to the host
  sl_wfx_general_indications_ids_t indication; ///< Indication from the wlan device to the host
} sl_wfx_general_commands_ids_t;

/**************************************************/

/**
 * @brief General confirmation possible values for returned 'status' field
 *
 * All general confirmation messages have a field 'status' just after the message header.@n
 * A value of zero indicates the request is completed successfully.
 *
 */
typedef enum sl_wfx_status_e {
  SL_WFX_STATUS_SUCCESS                         = 0x0,           ///<The firmware has successfully completed the request.
  SL_WFX_STATUS_FAILURE                         = 0x1,           ///<This is a generic failure code : other error codes do not apply.
  SL_WFX_INVALID_PARAMETER                      = 0x2,           ///<The request contains one or more invalid parameters.
  SL_WFX_STATUS_GPIO_WARNING                    = 0x3,           ///<Warning : the GPIO cmd is successful but the read value is not as expected (likely a drive conflict on the line)
  SL_WFX_ERROR_UNSUPPORTED_MSG_ID               = 0x4,       ///<Unkown request ID or wrong interface ID used
  /* Specific SecureLink statuses */
  SL_MAC_KEY_STATUS_SUCCESS                       = 0x5A,            ///<Key has been correctly written
  SL_MAC_KEY_STATUS_FAILED_KEY_ALREADY_BURNED     = 0x6B,            ///<Key already exists in OTP
  SL_MAC_KEY_STATUS_FAILED_RAM_MODE_NOT_ALLOWED   = 0x7C,            ///<RAM mode is not allowed
  SL_MAC_KEY_STATUS_FAILED_UNKNOWN_MODE           = 0x8D,            ///<Unknown mode (should be RAM or OTP)
  SL_PUB_KEY_EXCHANGE_STATUS_SUCCESS            = 0x9E,            ///<Host Public Key authenticated
  SL_PUB_KEY_EXCHANGE_STATUS_FAILED             = 0xAF,            ///<Host Public Key authentication failed
  /* Specific Prevent Rollback statuses */
  PREVENT_ROLLBACK_CNF_SUCCESS                = 0x1234,          ///<OTP rollback value has been successfully updated
  PREVENT_ROLLBACK_CNF_WRONG_MAGIC_WORD       = 0x1256           ///<Wrong magic word detected
} sl_wfx_status_t;

/**************************************************/

/**
 * @addtogroup General_Configuration
 * @brief General configuration commands
 *
 *
 * @{
 */
typedef enum sl_wfx_type_e {
  SL_WFX_FW_TYPE_ETF                             = 0x0,               /*Test Firmware*/
  SL_WFX_FW_TYPE_WFM                             = 0x1,               /*WLAN Full MAC (WFM)*/
  SL_WFX_FW_TYPE_WSM                             = 0x2                /*WLAN Split MAC (WSM)*/
} sl_wfx_fw_type_t;

/**
 * @brief Capabilities offered by the WLAN used in command sl_wfx_startup_ind_body_t
 */
typedef struct __attribute__((__packed__)) sl_wfx_capabilities_s {
  uint8_t    linkmode : 2;                           ///<Bit 0-1 : OTP SecureLink mode. 0=reserved, 1=untrusted (no secure link supported), 2=evaluation mode (no key burnt), 3=active
  uint8_t    reserved1 : 6;                          ///<Bit 2-7 : Reserved
  uint8_t    reserved2;                              ///<Bit 8-15 : Reserved
  uint8_t    reserved3;                              ///<Bit 16-23 : Reserved
  uint8_t    reserved4;                              ///<Bit 24-31 : Reserved
} sl_wfx_capabilities_t;

/**
 * @brief REGUL_SEL_MODE OTP field reported in command sl_wfx_startup_ind_body_t
 */
typedef struct __attribute__((__packed__)) sl_wfx_otp_regul_sel_mode_info_s {
  uint8_t    region_sel_mode : 4;                      ///<Bit 0-3 : indicates default FW behavior regarding DFS region setting
                                                     ///<          0=Unrestricted, 1=SuperGlobal (enforces FCC, CE and Japan regulations depending on the automatic region detection)
                                                     ///<          2=SingleRegion (enforces FCC, CE or Japan depending on OTP tables content), 3=Reserved
  uint8_t    Reserved : 4;                           ///<Bit 4-7 : Reserved
} sl_wfx_otp_regul_sel_mode_info_t;

/**
 * @brief OTP_PHY_INFO OTP field reported in command sl_wfx_startup_ind_body_t
 */
typedef struct __attribute__((__packed__)) sl_wfx_otp_phy_info_s {
  uint8_t    phy1_region : 3;                         ///<Bit 0-2 : DFS region corresponding to backoff vs. channel group table indexed 1
  uint8_t    phy0_region : 3;                         ///<Bit 3-5 : DFS region corresponding to backoff vs. channel group table indexed 0
  uint8_t    otp_phy_ver : 2;                         ///<Bit 6-7 : Revision of OTP info
} sl_wfx_otp_phy_info_t;

#define SL_WFX_OPN_SIZE                                    14
#define SL_WFX_UID_SIZE                                    8
#define SL_WFX_DISABLED_CHANNEL_LIST_SIZE                  2
#define SL_WFX_FIRMWARE_LABEL_SIZE                         128
/**
 * @brief Startup Indication message.
 * This is the first message sent to the host to confirm boot success.
 * It gives detailed information on the HW and FW versions and capabilities
 */
typedef struct __attribute__((__packed__)) sl_wfx_startup_ind_body_s {
  uint32_t   status;                                 ///<Initialization status. A value of zero indicates the boot is completed successfully  (see enum sl_wfx_status_t)
  uint16_t   hardware_id;                             ///<=RO misc_read_reg7 register value
  uint8_t    opn[SL_WFX_OPN_SIZE];                      ///<=OTP part_OPN
  uint8_t    uid[SL_WFX_UID_SIZE];                      ///<=OTP UID
  uint16_t   num_inp_ch_bufs;                           ///<Number of buffers available for request messages.
  uint16_t   size_inp_ch_buf;                           ///<Tx Buffer size in bytes=request message max size.
  uint8_t    num_links_aP;                             ///<number of STA that are supported in AP mode
  uint8_t    num_interfaces;                          ///<number of interfaces (WIFI link : STA or AP) that can be created by the user
  uint8_t    mac_addr[2][SL_WFX_MAC_ADDR_SIZE];          ///<MAC addresses derived from OTP
  uint8_t    api_version_minor;
  uint8_t    api_version_major;
  sl_wfx_capabilities_t capabilities;                      ///<List some FW options
  uint8_t    firmware_build;
  uint8_t    firmware_minor;
  uint8_t    firmware_major;
  uint8_t    firmware_type;                           ///<See enum sl_wfx_fw_type_t
  uint8_t    disabled_channel_list[SL_WFX_DISABLED_CHANNEL_LIST_SIZE];         ///<=OTP Disabled channel list info
  sl_wfx_otp_regul_sel_mode_info_t regul_sel_mode_info;       ///<OTP region selection mode info
  sl_wfx_otp_phy_info_t otp_phy_info;                        ///<info on OTP backoff tables used to enforce the different DFS regulations.
  uint32_t   supported_rate_mask;                      ///<A bit mask that indicates which rates are supported by the Physical layer. See enum api_rate_index.
  uint8_t    firmware_label[SL_WFX_FIRMWARE_LABEL_SIZE];         ///<Null terminated text string describing the loaded FW.
} sl_wfx_startup_ind_body_t;

typedef struct __attribute__((__packed__)) sl_wfx_startup_ind_s {
  sl_wfx_header_t header;
  sl_wfx_startup_ind_body_t body;
} sl_wfx_startup_ind_t;

/**
 * @brief Configure the device.
 * It sends a PDS compressed file that configures the device regarding board dependent parameters.
 * The PDS compressed file must fit in a command buffer and have less than 256 elements.
 *
 * @todo Need to create a specific doc to explain PDS*/
typedef struct __attribute__((__packed__)) sl_wfx_configuration_req_body_s {
  uint16_t   length;                                 ///<pds_data length in bytes
  uint8_t    pds_data[0];          ///<variable size PDS data byte array
} sl_wfx_configuration_req_body_t;

typedef struct __attribute__((__packed__)) sl_wfx_configuration_req_s {
  sl_wfx_header_t header;
  sl_wfx_configuration_req_body_t body;
} sl_wfx_configuration_req_t;

/**
 * @brief Confirmation message of CONFIGURATION command sl_wfx_configuration_req_body_t */
typedef struct __attribute__((__packed__)) sl_wfx_configuration_cnf_body_s {
  uint32_t   status;                                 ///<Configuration status. A value of zero indicates the boot is completed successfully (see enum sl_wfx_status_t)
} sl_wfx_configuration_cnf_body_t;

typedef struct __attribute__((__packed__)) sl_wfx_configuration_cnf_s {
  sl_wfx_header_t header;
  sl_wfx_configuration_cnf_body_t body;
} sl_wfx_configuration_cnf_t;

/**
 * @brief Configure GPIO mode. Used in sl_wfx_control_gpio_req_body_t
 * */
typedef enum sl_wfx_gpio_mode_e {
  SL_WFX_GPIO_MODE_D0                            = 0x0,               ///< Configure the GPIO to drive 0
  SL_WFX_GPIO_MODE_D1                            = 0x1,               ///< Configure the GPIO to drive 1
  SL_WFX_GPIO_MODE_OD0                           = 0x2,               ///< Configure the GPIO to open drain with pull_down to 0
  SL_WFX_GPIO_MODE_OD1                           = 0x3,               ///< Configure the GPIO to open drain with pull_up to 1
  SL_WFX_GPIO_MODE_TRISTATE                      = 0x4,               ///< Configure the GPIO to tristate
  SL_WFX_GPIO_MODE_TOGGLE                        = 0x5,               ///< Toggle the GPIO output value : switches between D0 and D1 or between OD0 and OD1
  SL_WFX_GPIO_MODE_READ                          = 0x6                ///< Read the level at the GPIO pin
} sl_wfx_gpio_mode_t;

/**
 * @brief Send a request to read or write a gpio identified by its label (that is defined in the PDS)
 *
 * After a write it also read back the value to check there is no drive conflict */
typedef struct __attribute__((__packed__)) sl_wfx_control_gpio_req_body_s {
  uint8_t gpio_label;      ///<Identify the gpio by its label (defined in the PDS)
  uint8_t gpio_mode;       ///<define how to set or read the gpio (see enum sl_wfx_gpio_mode_t)
} sl_wfx_control_gpio_req_body_t;

typedef struct __attribute__((__packed__)) sl_wfx_control_gpio_req_s {
  sl_wfx_header_t header;
  sl_wfx_control_gpio_req_body_t body;
} sl_wfx_control_gpio_req_t;

/**
 * @brief detailed error cause returned by CONTROL_GPIO confirmation message sl_wfx_control_gpio_cnf_body_t
 * */
typedef enum sl_wfx_gpio_error_e {
  SL_WFX_GPIO_ERROR_0                            = 0x0,               ///< Undefined GPIO_ID
  SL_WFX_GPIO_ERROR_1                            = 0x1,               ///< GPIO_ID not configured in gpio mode (gpio_enabled =0)
  SL_WFX_GPIO_ERROR_2                            = 0x2                ///< Toggle not possible while in tristate
} sl_wfx_gpio_error_t;

/**
 * @brief Confirmation from request to read and write a gpio */
typedef struct __attribute__((__packed__)) sl_wfx_control_gpio_cnf_body_s {
  uint32_t status;        ///<enum sl_wfx_status_t : a value of zero indicates the request is completed successfully.
  uint32_t value;         ///<the error detail (see enum sl_wfx_gpio_error_t) when ::sl_wfx_control_gpio_cnf_body_t::status reports an error else the gpio read value.
} sl_wfx_control_gpio_cnf_body_t;

typedef struct __attribute__((__packed__)) sl_wfx_control_gpio_cnf_s {
  sl_wfx_header_t header;
  sl_wfx_control_gpio_cnf_body_t body;
} sl_wfx_control_gpio_cnf_t;

/**
 * @brief SHUT_DOWN command.
 * A hardware reset and complete reboot is required to resume from that state.
 * There is no confirmation to this command.
 * It is effective when WUP register bit and WUP pin (when used) are both to 0.*/
typedef sl_wfx_header_t sl_wfx_shut_down_req_t;

/**
 * @brief specifies the type of data reported by the indication message sl_wfx_generic_ind_body_t
 *
 * */
typedef enum sl_wfx_generic_indication_type_e {
  SL_WFX_GENERIC_INDICATION_TYPE_RAW               = 0x0,               ///<Byte stream type, currently not used
  SL_WFX_GENERIC_INDICATION_TYPE_STRING            = 0x1,               ///<NULL terminating String
  SL_WFX_GENERIC_INDICATION_TYPE_RX_STATS          = 0x2                ///<Rx statistics structure
} sl_wfx_generic_indication_type_t;

/**
 * @brief RX stats from the GENERIC indication message sl_wfx_generic_ind_body_t
 */
typedef struct __attribute__((__packed__)) sl_wfx_rx_stats_s {
  uint32_t   nb_rx_frame;                              ///<Total number of frame received
  uint32_t   nb_crc_frame;                             ///<Number of frame received with bad CRC
  uint32_t   per_total;                               ///<PER on the total number of frame
  uint32_t   throughput;                             ///<Throughput calculated on correct frames received
  uint32_t   nb_rx_by_rate[SL_WFX_RATE_NUM_ENTRIES];       ///<Number of frame received by rate
  uint16_t   per[SL_WFX_RATE_NUM_ENTRIES];              ///<PER*10000 by frame rate
  int16_t    snr[SL_WFX_RATE_NUM_ENTRIES];              ///<SNR in Db*100 by frame rate
  int16_t    rssi[SL_WFX_RATE_NUM_ENTRIES];             ///<RSSI in Dbm*100 by frame rate
  int16_t    cfo[SL_WFX_RATE_NUM_ENTRIES];              ///<CFO in k_hz by frame rate
  uint32_t   date;                                   ///<This message transmission date in firmware timebase (microsecond)
  uint32_t   pwr_clk_freq;                             ///<Frequency of the low power clock in Hz
  uint8_t    is_ext_pwr_clk;                            ///<Indicate if the low power clock is external
} sl_wfx_rx_stats_t;

typedef union sl_wfx_indication_data_u {
  sl_wfx_rx_stats_t                                   rx_stats;
  uint8_t                                       raw_data[376];
} sl_wfx_indication_data_t;

/**
 * @brief the Generic indication message.
 *
 * It reports different type of information that can be printed by the driver.
 * */
typedef struct __attribute__((__packed__)) sl_wfx_generic_ind_body_s {
  uint32_t indication_type;                         ///<Identify the indication data (see enum type sl_wfx_generic_indication_type_t)
  sl_wfx_indication_data_t indication_data;               ///<Indication data.
} sl_wfx_generic_ind_body_t;

typedef struct __attribute__((__packed__)) sl_wfx_generic_ind_s {
  sl_wfx_header_t header;
  sl_wfx_generic_ind_body_t body;
} sl_wfx_generic_ind_t;

#define SL_WFX_EXCEPTION_DATA_SIZE            124
/**
 * @brief Exception indication message
 *
 * It reports unexpected errors. A reboot is needed after this message.
 * */
typedef struct __attribute__((__packed__)) sl_wfx_exception_ind_body_s {
  uint8_t    data[SL_WFX_EXCEPTION_DATA_SIZE];           ///<Raw data array
} sl_wfx_exception_ind_body_t;

typedef struct __attribute__((__packed__)) sl_wfx_exception_ind_s {
  sl_wfx_header_t header;
  sl_wfx_exception_ind_body_t body;
} sl_wfx_exception_ind_t;

/**
 * @brief specifies the type of error reported by the indication message sl_wfx_error_ind_body_t
 *
 * */
typedef enum sl_wfx_error_e {
  WSM_SL_WFX_ERROR_FIRMWARE_ROLLBACK             = 0x0,               ///<Firmware rollback error, no data returned
  WSM_SL_WFX_ERROR_FIRMWARE_DEBUG_ENABLED        = 0x1,               ///<Firmware debug feature enabled, no data returned
  WSM_SL_WFX_ERROR_OUTDATED_SESSION_KEY          = 0x2,               ///<SecureLink Session key is outdated, 4bytes returned (nonce counter)
  WSM_SL_WFX_ERROR_INVALID_SESSION_KEY           = 0x3,               ///<SecureLink Session key is invalid, 0 or 4bytes returned
  WSM_SL_WFX_ERROR_OOR_VOLTAGE                   = 0x4,           ///<Out-of-range power supply voltage detected, no data returned
  WSM_SL_WFX_ERROR_PDS_VERSION                   = 0x5            ///<wrong PDS version detected, no data returned
} sl_wfx_error_t;

/**
 * @brief Error indication message.
 *
 * It reports user configuration errors.
 * A reboot is needed after this message.
 * */
typedef struct __attribute__((__packed__)) sl_wfx_error_ind_body_s {
  uint32_t   type;                                   ///<error type, see enum wsm_sl_wfx_error_t
  uint8_t    data[0];                                ///<Generic data buffer - contents depends on the error type.
} sl_wfx_error_ind_body_t;

typedef struct __attribute__((__packed__)) sl_wfx_error_ind_s {
  sl_wfx_header_t header;
  sl_wfx_error_ind_body_t body;
} sl_wfx_error_ind_t;

/**
 * @}
 */
/* end of General_Configuration */

/******************************************************************************
 * Secure link section
 *****************************************************************************/
/**
 * @addtogroup Secure_Link
 * @brief APIs for Secure link configuration and usage.
 *
 * WFX family of product have the ability to encrypt the SDIO or SPI link.
 *
 * Link to more detailed documentation about the Secure Link feature : \ref SecureLink
 * @{
 */

/**
 * @brief *Secure Link' device state
 * */
typedef enum sl_wfx_secure_link_state_e {
  SECURE_LINK_NA_MODE                        = 0x0,   ///<Reserved
  SECURE_LINK_UNTRUSTED_MODE                 = 0x1,   ///<Untrusted mode - SecureLink not available
  SECURE_LINK_TRUSTED_MODE                   = 0x2,     ///<Trusted (Evaluation) mode
  SECURE_LINK_TRUSTED_ACTIVE_ENFORCED        = 0x3      ///<Trusted (Enforced) mode
} sl_wfx_secure_link_state_t;

/**
 * @brief destination of the *Secure Link MAC key*, used by request message sl_wfx_set_sl_mac_key_req_body_t
 * */
typedef enum sl_wfx_securelink_mac_key_dest_e {
  SL_MAC_KEY_DEST_OTP                        = 0x78,              ///<Key will be stored in OTP
  SL_MAC_KEY_DEST_RAM                        = 0x87               ///<Key will be stored in RAM
} sl_wfx_securelink_mac_key_dest_t;

#define SL_WFX_KEY_VALUE_SIZE      32
/**
 * @brief Set the Secure Link MAC key
 *
 * This API can be used for *Trusted Eval* devices in two contexts:
 * - to set a temporary *SecureLink MAC key* in RAM.
 * - to permanently burn the *SecureLink MAC key* in OTP memory. In that case, the OTP *SecureLink mode* will
 * switch to *Trusted Enforced* mode
 */
typedef struct __attribute__((__packed__)) sl_wfx_set_sl_mac_key_req_body_s {
  uint8_t    otp_or_ram;                               ///<Key destination - OTP or RAM (see enum sl_wfx_securelink_mac_key_dest_t)
  uint8_t    key_value[SL_WFX_KEY_VALUE_SIZE];           ///<Secure Link MAC Key value
} sl_wfx_set_sl_mac_key_req_body_t;

typedef struct __attribute__((__packed__)) sl_wfx_set_sl_mac_key_req_s {
  sl_wfx_header_t header;
  sl_wfx_set_sl_mac_key_req_body_t body;
} sl_wfx_set_sl_mac_key_req_t;

/**
 * @brief Confirmation for the Secure Link MAC key setting
 * */
typedef struct __attribute__((__packed__)) sl_wfx_set_sl_mac_key_cnf_body_s {
  uint32_t   status;                                 ///<Key upload status (see enum sl_wfx_status_t)
} sl_wfx_set_sl_mac_key_cnf_body_t;

typedef struct __attribute__((__packed__)) sl_wfx_set_sl_mac_key_cnf_s {
  sl_wfx_header_t header;
  sl_wfx_set_sl_mac_key_cnf_body_t body;
} sl_wfx_set_sl_mac_key_cnf_t;

#define SL_WFX_HOST_PUB_KEY_SIZE                           32
#define SL_WFX_HOST_PUB_KEY_MAC_SIZE                       64
/**
 * @brief Exchange Secure Link Public Keys
 *
 * This API is used by the Host to send its *curve25519* public key to Device, and get back Device public key in the confirmation message.
 * Once keys are exchanged and authenticated (using their respective MAC), each peer computes the Secure Link *session key* that will be used
 * to encrypt/decrypt future Host<->Device messages.
 */
typedef struct __attribute__((__packed__)) sl_wfx_securelink_exchange_pub_keys_req_body_s {
  uint8_t    host_pub_key[SL_WFX_HOST_PUB_KEY_SIZE];         ///<Host Public Key
  uint8_t    host_pub_key_mac[SL_WFX_HOST_PUB_KEY_MAC_SIZE];         ///<Host Public Key MAC
} sl_wfx_securelink_exchange_pub_keys_req_body_t;

typedef struct __attribute__((__packed__)) sl_wfx_securelink_exchange_pub_keys_req_s {
  sl_wfx_header_t header;
  sl_wfx_securelink_exchange_pub_keys_req_body_t body;
} sl_wfx_securelink_exchange_pub_keys_req_t;

#define SL_WFX_NCP_PUB_KEY_SIZE                            32
#define SL_WFX_NCP_PUB_KEY_MAC_SIZE                        64
/**
 * @brief Confirmation for exchange of Secure Link Public Keys
 * */
typedef struct __attribute__((__packed__)) sl_wfx_securelink_exchange_pub_keys_cnf_body_s {
  uint32_t   status;                            ///<Request status (see enum sl_wfx_status_t)
  uint8_t    ncp_pub_key[SL_WFX_NCP_PUB_KEY_SIZE];             ///<Device Public Key
  uint8_t    ncp_pub_key_mac[SL_WFX_NCP_PUB_KEY_MAC_SIZE];          ///<Device Public Key MAC
} sl_wfx_securelink_exchange_pub_keys_cnf_body_t;

typedef struct __attribute__((__packed__)) sl_wfx_securelink_exchange_pub_keys_cnf_s {
  sl_wfx_header_t header;
  sl_wfx_securelink_exchange_pub_keys_cnf_body_t body;
} sl_wfx_securelink_exchange_pub_keys_cnf_t;

/**
 * @brief used in request message sl_wfx_securelink_configure_req_body_t to trigger *Session Key* invalidation
 */
typedef enum sl_wfx_securelink_configure_skey_invld_e {
  SL_CONFIGURE_SKEY_INVLD_INVALIDATE         = 0x87,              ///<Force invalidating session key
  SL_CONFIGURE_SKEY_INVLD_NOP                = 0x00               ///<Do not invalidate session key
} sl_wfx_securelink_configure_skey_invld_t;

#define SL_WFX_ENCR_BMP_SIZE        32
/**
 * @brief Configure Secure Link Layer
 *
 * This API can be used to:
 * - Set/update the Secure Link *encryption bitmap*
 * - or, Invalidate the current *session key*
 *
 * In the later case, the encryption bitmap is left untouched.
 *
 */
typedef struct __attribute__((__packed__)) sl_wfx_securelink_configure_req_body_s {
  uint8_t    encr_bmp[SL_WFX_ENCR_BMP_SIZE];             ///<Encryption bitmap
  uint8_t    skey_invld;                              ///<Invalidate Session Key (see enum sl_configure_skey_invld_t)
} sl_wfx_securelink_configure_req_body_t;

typedef struct __attribute__((__packed__)) sl_wfx_securelink_configure_req_s {
  sl_wfx_header_t header;
  sl_wfx_securelink_configure_req_body_t body;
} sl_wfx_securelink_configure_req_t;

/**
 * @brief Confirmation of Secure Link Layer configuration sl_wfx_securelink_configure_req_body_t
 * */
typedef struct __attribute__((__packed__)) sl_wfx_securelink_configure_cnf_body_s {
  uint32_t status;                    ///<Request status (see enum wsm_status)
} sl_wfx_securelink_configure_cnf_body_t;

typedef struct __attribute__((__packed__)) sl_wfx_securelink_configure_cnf_s {
  sl_wfx_header_t header;
  sl_wfx_securelink_configure_cnf_body_t body;
} sl_wfx_securelink_configure_cnf_t;

/**
 * @}
 */
/* end of Secure_Link */

/******************************************************************************
 * Prevent Rollback section
 *****************************************************************************/
/**
 * @addtogroup Prevent_Firmware_Rollback
 * @brief APIs for preventing Rollback of unsafe firmware images.
 *
 * By enabling this feature Device is able to prevent unsafe/outdated firmwares to boot.
 *
 * Each firmware owns its internal *rollback revision number* which is compared to
 *   an equivalent revision number burned in Device OTP memory. Depending on the comparison result,
 *   two cases can occur:
 * - Firmware revision number is higher or equal to the OTP number -> the firmware is allowed
 *   to proceed
 * - Firmware revision number is lower than the OTP value -> the firmware is not allowed to proceed.
 *   An *Error indication* will be returned to the driver indicating the cause of the error (WSM_SL_WFX_ERROR_FIRMWARE_ROLLBACK).
 *
 * @note The firmware *rollback revision number* is different that the *firmware version*.
 * The former is incremented only when some important fixes (i.e. Security patches) are provided
 * by a given version of the firmware,that MUST be applied to Device and should not be reverted.
 *  Usually, subsequent firmware versions are supposed to embed the same rollback revision number.
 *
 * The rollback capability relies on the use of a dedicated API sl_wfx_prevent_rollback_req_body_t.
 *
 * All Device drivers supporting *Rollback Prevention* should send this request just after booting a new firmware.
 *   This way, any newer *rollback revision number* included in a firmware will be burned in the OTP.
 *
 * @{
 */

/**
 *@brief Prevent Rollback request
 *
 * *Prevent Rollback* asks WLAN firmware to burn a new *Firmware Rollback* value in a dedicated OTP section.
 *
 * The new value is encoded in the firmware itself. Once burned, this value will prevent from starting
 *  all firmwares whose internal rollback value is lower than the OTP value.
 *
 * *Magic Word* is used to prevent mistakenly sent requests to burn the OTP.
 *
 */
typedef struct __attribute__((__packed__)) sl_wfx_prevent_rollback_req_body_s {
  uint32_t   magic_word;                              /**< Magic Word - should be 0x5C8912F3*/
} sl_wfx_prevent_rollback_req_body_t;

typedef struct __attribute__((__packed__)) sl_wfx_prevent_rollback_req_s {
  sl_wfx_header_t header;
  sl_wfx_prevent_rollback_req_body_t body;
} sl_wfx_prevent_rollback_req_t;

/**
 * @brief Confirmation of the *Prevent Rollback* request
 *
 * The request might have failed for the following reasons:
 * - Wrong *magic word* value
 *
 */
typedef struct __attribute__((__packed__)) sl_wfx_prevent_rollback_cnf_body_s {
  uint32_t    status;                         ///<Confirmation status, see enum sl_wfx_status_t
} sl_wfx_prevent_rollback_cnf_body_t;

typedef struct __attribute__((__packed__)) sl_wfx_prevent_rollback_cnf_s {
  sl_wfx_header_t header;
  sl_wfx_prevent_rollback_cnf_body_t body;
} sl_wfx_prevent_rollback_cnf_t;

/**
 * @}
 */
/* end of Prevent_Roll_Back */

/**
 * @}
 */
/*end of GENERAL_API */

#endif /* _GENERAL_API_H_ */
