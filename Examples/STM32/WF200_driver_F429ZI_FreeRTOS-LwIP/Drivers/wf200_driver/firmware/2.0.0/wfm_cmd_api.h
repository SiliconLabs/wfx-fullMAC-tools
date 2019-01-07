/*
* Silicon Laboratories Confidential
* Copyright 2008-2018 Silicon Laboratories, Inc.
*
*/

#ifndef _WFM_CMD_API_H_
#define _WFM_CMD_API_H_

#include "general_api.h"

/**
 * @addtogroup FULL_MAC_API
 * @brief Wireless LAN Full MAC (WFM) API
 *
 * This module describes the functionality and the API messages of the Upper MAC (UMAC)
 * component of the device. UMAC provides the Wireless LAN Full MAC (WFM) API. WFM API
 * is used in conjunction with the general API messages described in @ref GENERAL_API.
 *
 * The device is capable of operating in different roles such as a Wi-Fi client and
 * as a Wi-Fi access point. In order to make it possible to use multiple roles at
 * the same time, the device has been split into logical units called interfaces.
 *
 * There are two interfaces available for WFM API. They are referenced with interface
 * IDs 0 and 1. For general information on how to format the API messages, see
 * @ref MESSAGE_CONSTRUCTION.
 *
 * For a list of WFM API messages, see @ref WfmMessageIds_t for further details.
 * @{
 */

/**
 * @brief WFM API request message IDs.
 */
typedef enum WfmRequestsIds_e {
WFM_HI_SET_MAC_ADDRESS_REQ_ID                   =0x42,  ///< \b SET_MAC_ADDRESS request ID use body ::HI_SET_MAC_ADDRESS_REQ_BODY and returns ::HI_SET_MAC_ADDRESS_CNF_BODY
WFM_HI_CONNECT_REQ_ID                           =0x43,  ///< \b CONNECT request ID use body ::HI_CONNECT_REQ_BODY and returns ::HI_CONNECT_CNF_BODY
WFM_HI_DISCONNECT_REQ_ID                        =0x44,  ///< \b DISCONNECT request ID use body ::HI_DISCONNECT_REQ_BODY and returns ::HI_DISCONNECT_CNF_BODY
WFM_HI_START_AP_REQ_ID                          =0x45,  ///< \b START_AP request ID use body ::HI_START_AP_REQ_BODY and returns ::HI_START_AP_CNF_BODY
WFM_HI_UPDATE_AP_REQ_ID                         =0x46,  ///< \b UPDATE_AP request ID use body ::HI_UPDATE_AP_REQ_BODY and returns ::HI_UPDATE_AP_CNF_BODY
WFM_HI_STOP_AP_REQ_ID                           =0x47,  ///< \b STOP_AP request ID use body ::HI_STOP_AP_REQ_BODY and returns ::HI_STOP_AP_CNF_BODY
WFM_HI_SEND_FRAME_REQ_ID                        =0x4a,  ///< \b SEND_FRAME request ID use body ::HI_SEND_FRAME_REQ_BODY and returns ::HI_SEND_FRAME_CNF_BODY
WFM_HI_START_SCAN_REQ_ID                        =0x4b,  ///< \b START_SCAN request ID use body ::HI_START_SCAN_REQ_BODY and returns ::HI_START_SCAN_CNF_BODY
WFM_HI_STOP_SCAN_REQ_ID                         =0x4c,  ///< \b STOP_SCAN request ID use body ::HI_STOP_SCAN_REQ_BODY and returns ::HI_STOP_SCAN_CNF_BODY
WFM_HI_GET_SIGNAL_STRENGTH_REQ_ID               =0x4e,  ///< \b GET_SIGNAL_STRENGTH request ID use body ::HI_GET_SIGNAL_STRENGTH_REQ_BODY and returns ::HI_GET_SIGNAL_STRENGTH_CNF_BODY
WFM_HI_DISCONNECT_AP_CLIENT_REQ_ID              =0x4f,  ///< \b DISCONNECT_AP_CLIENT request ID use body ::HI_DISCONNECT_AP_CLIENT_REQ_BODY and returns ::HI_DISCONNECT_AP_CLIENT_CNF_BODY
WFM_HI_JOIN_IBSS_REQ_ID                         =0x50,  ///< \b JOIN_IBSS request ID use body ::HI_JOIN_IBSS_REQ_BODY and returns ::HI_JOIN_IBSS_CNF_BODY
WFM_HI_LEAVE_IBSS_REQ_ID                        =0x51,  ///< \b LEAVE_IBSS request ID use body ::HI_LEAVE_IBSS_REQ_BODY and returns ::HI_LEAVE_IBSS_CNF_BODY
WFM_HI_SET_PM_MODE_REQ_ID                       =0x52,  ///< \b SET_PM_MODE request ID use body ::HI_SET_PM_MODE_REQ_BODY and returns ::HI_SET_PM_MODE_CNF_BODY
WFM_HI_ADD_MULTICAST_ADDR_REQ_ID                =0x53,  ///< \b ADD_MULTICAST_ADDR request ID use body ::HI_ADD_MULTICAST_ADDR_REQ_BODY and returns ::HI_ADD_MULTICAST_ADDR_CNF_BODY
WFM_HI_REMOVE_MULTICAST_ADDR_REQ_ID             =0x54,  ///< \b REMOVE_MULTICAST_ADDR request ID use body ::HI_REMOVE_MULTICAST_ADDR_REQ_BODY and returns ::HI_REMOVE_MULTICAST_ADDR_CNF_BODY
WFM_HI_SET_MAX_AP_CLIENT_COUNT_REQ_ID           =0x55,  ///< \b SET_MAX_AP_CLIENT_COUNT request ID use body ::HI_SET_MAX_AP_CLIENT_COUNT_REQ_BODY and returns ::HI_SET_MAX_AP_CLIENT_COUNT_CNF_BODY
WFM_HI_SET_MAX_AP_CLIENT_INACTIVITY_REQ_ID      =0x56,  ///< \b SET_MAX_AP_CLIENT_INACTIVITY request ID use body ::HI_SET_MAX_AP_CLIENT_INACTIVITY_REQ_BODY and returns ::HI_SET_MAX_AP_CLIENT_INACTIVITY_CNF_BODY
WFM_HI_SET_ROAM_PARAMETERS_REQ_ID               =0x57,  ///< \b SET_ROAM_PARAMETERS request ID use body ::HI_SET_ROAM_PARAMETERS_REQ_BODY and returns ::HI_SET_ROAM_PARAMETERS_CNF_BODY
WFM_HI_SET_TX_RATE_PARAMETERS_REQ_ID            =0x58,  ///< \b SET_TX_RATE_PARAMETERS request ID use body ::HI_SET_TX_RATE_PARAMETERS_REQ_BODY and returns ::HI_SET_TX_RATE_PARAMETERS_CNF_BODY
WFM_HI_SET_ARP_IP_ADDRESS_REQ_ID                =0x59,  ///< \b SET_ARP_IP_ADDRESS request ID use body ::HI_SET_ARP_IP_ADDRESS_REQ_BODY and returns ::HI_SET_ARP_IP_ADDRESS_CNF_BODY
WFM_HI_SET_NS_IP_ADDRESS_REQ_ID                 =0x5A,  ///< \b SET_NS_IP_ADDRESS request ID use body ::HI_SET_NS_IP_ADDRESS_REQ_BODY and returns ::HI_SET_NS_IP_ADDRESS_CNF_BODY
WFM_HI_SET_BROADCAST_FILTER_REQ_ID              =0x5B,  ///< \b SET_BROADCAST_FILTER request ID use body ::HI_SET_BROADCAST_FILTER_REQ_BODY and returns ::HI_SET_BROADCAST_FILTER_CNF_BODY
WFM_HI_SET_SCAN_PARAMETERS_REQ_ID				=0x5C	///< \b SET_SCAN_PARAMETERS request ID use body ::HI_SET_SCAN_PARAMETERS_REQ_BODY and returns ::HI_SET_SCAN_PARAMETERS_CNF_BODY
} WfmRequestsIds;

/**
 * @brief WFM API confirmation message IDs.
 */
typedef enum WfmIndicationsIds_e {
WFM_HI_CONNECT_IND_ID                           =0xc3,  ///< \b CONNECT indication id. Content is ::HI_CONNECT_IND_BODY
WFM_HI_DISCONNECT_IND_ID                        =0xc4,  ///< \b DISCONNECT indication id. Content is ::HI_DISCONNECT_IND_BODY
WFM_HI_START_AP_IND_ID                          =0xc5,  ///< \b START_AP indication id. Content is ::HI_START_AP_IND_BODY
WFM_HI_STOP_AP_IND_ID                           =0xc7,  ///< \b STOP_AP indication id. Content is ::HI_STOP_AP_IND_BODY
WFM_HI_RECEIVED_IND_ID                          =0xca,  ///< \b RECEIVED indication id. Content is ::HI_RECEIVED_IND_BODY
WFM_HI_SCAN_RESULT_IND_ID                       =0xcb,  ///< \b SCAN_RESULT indication id. Content is ::HI_SCAN_RESULT_IND_BODY
WFM_HI_SCAN_COMPLETE_IND_ID                     =0xcc,  ///< \b SCAN_COMPLETE indication id. Content is ::HI_SCAN_COMPLETE_IND_BODY
WFM_HI_AP_CLIENT_CONNECTED_IND_ID               =0xcd,  ///< \b AP_CLIENT_CONNECTED indication id. Content is ::HI_AP_CLIENT_CONNECTED_IND_BODY
WFM_HI_AP_CLIENT_REJECTED_IND_ID                =0xce,  ///< \b AP_CLIENT_REJECTED indication id. Content is ::HI_AP_CLIENT_REJECTED_IND_BODY
WFM_HI_AP_CLIENT_DISCONNECTED_IND_ID            =0xcf,  ///< \b AP_CLIENT_DISCONNECTED indication id. Content is ::HI_AP_CLIENT_DISCONNECTED_IND_BODY
WFM_HI_JOIN_IBSS_IND_ID                         =0xd0,  ///< \b JOIN_IBSS indication id. Content is ::HI_JOIN_IBSS_IND_BODY
WFM_HI_LEAVE_IBSS_IND_ID                        =0xd1   ///< \b LEAVE_IBSS indication id. Content is ::HI_LEAVE_IBSS_IND_BODY
} WfmIndicationsIds;

/**
 * @brief WFM API indication message IDs.
 */
typedef enum WfmConfirmationsIds_e {
WFM_HI_SET_MAC_ADDRESS_CNF_ID                   =0x42,  ///< \b SET_MAC_ADDRESS confirmation Id. Returns body ::HI_SET_MAC_ADDRESS_CNF_BODY
WFM_HI_CONNECT_CNF_ID                           =0x43,  ///< \b CONNECT confirmation Id. Returns body ::HI_CONNECT_CNF_BODY
WFM_HI_DISCONNECT_CNF_ID                        =0x44,  ///< \b DISCONNECT confirmation Id. Returns body ::HI_DISCONNECT_CNF_BODY
WFM_HI_START_AP_CNF_ID                          =0x45,  ///< \b START_AP confirmation Id. Returns body ::HI_START_AP_CNF_BODY
WFM_HI_UPDATE_AP_CNF_ID                         =0x46,  ///< \b UPDATE_AP confirmation Id. Returns body ::HI_UPDATE_AP_CNF_BODY
WFM_HI_STOP_AP_CNF_ID                           =0x47,  ///< \b STOP_AP confirmation Id. Returns body ::HI_STOP_AP_CNF_BODY
WFM_HI_SEND_FRAME_CNF_ID                        =0x4a,  ///< \b SEND_FRAME confirmation Id. Returns body ::HI_SEND_FRAME_CNF_BODY
WFM_HI_START_SCAN_CNF_ID                        =0x4b,  ///< \b START_SCAN confirmation Id. Returns body ::HI_START_SCAN_CNF_BODY
WFM_HI_STOP_SCAN_CNF_ID                         =0x4c,  ///< \b STOP_SCAN confirmation Id. Returns body ::HI_STOP_SCAN_CNF_BODY
WFM_HI_GET_SIGNAL_STRENGTH_CNF_ID               =0x4e,  ///< \b GET_SIGNAL_STRENGTH confirmation Id. Returns body ::HI_GET_SIGNAL_STRENGTH_CNF_BODY
WFM_HI_DISCONNECT_AP_CLIENT_CNF_ID              =0x4f,  ///< \b DISCONNECT_AP_CLIENT confirmation Id. Returns body ::HI_DISCONNECT_AP_CLIENT_CNF_BODY
WFM_HI_JOIN_IBSS_CNF_ID                         =0x50,  ///< \b JOIN_IBSS confirmation Id. Returns body ::HI_JOIN_IBSS_CNF_BODY
WFM_HI_LEAVE_IBSS_CNF_ID                        =0x51,  ///< \b LEAVE_IBSS confirmation Id. Returns body ::HI_LEAVE_IBSS_CNF_BODY
WFM_HI_SET_PM_MODE_CNF_ID                       =0x52,  ///< \b SET_PM_MODE confirmation Id. Returns body ::HI_SET_PM_MODE_CNF_BODY
WFM_HI_ADD_MULTICAST_ADDR_CNF_ID                =0x53,  ///< \b ADD_MULTICAST_ADDR confirmation Id. Returns body ::HI_ADD_MULTICAST_ADDR_CNF_BODY
WFM_HI_REMOVE_MULTICAST_ADDR_CNF_ID             =0x54,  ///< \b REMOVE_MULTICAST_ADDR confirmation Id. Returns body ::HI_REMOVE_MULTICAST_ADDR_CNF_BODY
WFM_HI_SET_MAX_AP_CLIENT_COUNT_CNF_ID           =0x55,  ///< \b SET_MAX_AP_CLIENT_COUNT confirmation Id. Returns body ::HI_SET_MAX_AP_CLIENT_COUNT_CNF_BODY
WFM_HI_SET_MAX_AP_CLIENT_INACTIVITY_CNF_ID      =0x56,  ///< \b SET_MAX_AP_CLIENT_INACTIVITY confirmation Id. Returns body ::HI_SET_MAX_AP_CLIENT_INACTIVITY_CNF_BODY
WFM_HI_SET_ROAM_PARAMETERS_CNF_ID               =0x57,  ///< \b SET_ROAM_PARAMETERS confirmation Id. Returns body ::HI_SET_ROAM_PARAMETERS_CNF_BODY
WFM_HI_SET_TX_RATE_PARAMETERS_CNF_ID            =0x58,  ///< \b SET_TX_RATE_PARAMETERS confirmation Id. Returns body ::HI_SET_TX_RATE_PARAMETERS_CNF_BODY
WFM_HI_SET_ARP_IP_ADDRESS_CNF_ID                =0x59,  ///< \b SET_ARP_IP_ADDRESS confirmation Id. Returns body ::HI_SET_ARP_IP_ADDRESS_CNF_BODY
WFM_HI_SET_NS_IP_ADDRESS_CNF_ID                 =0x5A,  ///< \b SET_NS_IP_ADDRESS confirmation Id. Returns body ::HI_SET_NS_IP_ADDRESS_CNF_BODY
WFM_HI_SET_BROADCAST_FILTER_CNF_ID              =0x5B,  ///< \b SET_BROADCAST_FILTER confirmation Id. Returns body ::HI_SET_BROADCAST_FILTER_CNF_BODY
WFM_HI_SET_SCAN_PARAMETERS_CNF_ID				=0x5C	///< \b SET_SCAN_PARAMETERS confirmation Id. Returns body ::HI_SET_SCAN_PARAMETERS_CNF_BODY
} WfmConfirmationsIds;

/**
 * @brief WFM API message IDs.
 */
typedef union WfmMessageIds_u {
    /** Request messages sent from the host to the device. */
    WfmRequestsIds Requests;
    /** Confirmation messages sent from the device to the host. */
    WfmConfirmationsIds Confirmations;
    /** Indication messages sent from the device to the host. */
    WfmIndicationsIds Indications;
} WfmMessageIds_t;

/**
 * @addtogroup WFM_CONSTANTS API constant values
 * @brief WFM API constant values.
 * @{
 */

/** Length of MAC address element. */
#define WFM_API_MAC_ADDR_SIZE                           API_MAC_ADDR_SIZE
/** Length of BSSID element. */
#define WFM_API_BSSID_SIZE                              6         
/** Length of password element. */
#define WFM_API_PASSWORD_SIZE                           64        
/** Length of MAC address element. */
#define WFM_API_MAC_SIZE                                6         
/** Maximum length of channel list element. */
#define WFM_API_CHANNEL_NUMBER_SIZE                     14        
/** Maximum length of SSID list element. */
#define WFM_API_SSID_DEF_SIZE                           2         
/** Length of Service Set Identifier (SSID) element. */
#define WFM_API_SSID_SIZE                               32
/** Length of ARP IP address list element. */
#define WFM_API_ARP_IP_ADDR_SIZE                        2
/** Length of NS IP address list element. */
#define WFM_API_NS_IP_ADDR_SIZE                         2
/** Length of IPv6 address element. */
#define WFM_API_IPV6_ADDR_SIZE                          16

/**
 * @brief Client Isolation toggling
 */
typedef enum WfmClientIsolation_e {
        WFM_CLIENT_ISOLATION_DISABLED              = 0x0,		///< Client isolation disabled
        WFM_CLIENT_ISOLATION_ENABLED               = 0x1		///< Client isolation enabled
} WfmClientIsolation;

/**
 * @brief Type of the frame to be sent
 *
 * Value unused, all frames are considered to be Data by default.@n
 * Support for the other types will be added later.
 */
typedef enum WfmFrameType_e {
        WFM_FRAME_TYPE_MGMT                        = 0x0,		///< Management Frame
        WFM_FRAME_TYPE_ACTION                      = 0x4,		///< Action Frame
        WFM_FRAME_TYPE_DATA                        = 0x8		///< Data Frame
} WfmFrameType;

/**
 * @brief Hidden SSID toggling
 */
typedef enum WfmHiddenSsid_e {
        WFM_HIDDEN_SSID_FALSE                      = 0x0,		///< SSID not hidden
        WFM_HIDDEN_SSID_TRUE                       = 0x1		///< SSID hidden
} WfmHiddenSsid;

/**
 * @brief Maximum data rate used by an AP
 */
typedef enum WfmMaxPhyRate_e {
        WFM_MAX_PHY_RATE_B_1MBPS                   = 0x0,       ///< Data rate 802.11b 1Mbps
        WFM_MAX_PHY_RATE_B_2MBPS                   = 0x1,       ///< Data rate 802.11b 2Mbps
        WFM_MAX_PHY_RATE_B_5P5MBPS                 = 0x2,       ///< Data rate 802.11b 5.5Mbps
        WFM_MAX_PHY_RATE_B_11MBPS                  = 0x3,       ///< Data rate 802.11b 11Mbps
        WFM_MAX_PHY_RATE_G_6MBPS                   = 0x6,       ///< Data rate 802.11g 6Mbps
        WFM_MAX_PHY_RATE_G_12MBPS                  = 0x7,       ///< Data rate 802.11g 12Mbps
        WFM_MAX_PHY_RATE_G_18MBPS                  = 0x8,       ///< Data rate 802.11g 18Mbps
        WFM_MAX_PHY_RATE_G_24MBPS                  = 0x9,       ///< Data rate 802.11g 24Mbps
        WFM_MAX_PHY_RATE_G_36MBPS                  = 0xa,       ///< Data rate 802.11g 36Mbps
        WFM_MAX_PHY_RATE_G_42MBPS                  = 0xb,       ///< Data rate 802.11g 42Mbps
        WFM_MAX_PHY_RATE_G_48MBPS                  = 0xc,       ///< Data rate 802.11g 48Mbps
        WFM_MAX_PHY_RATE_G_54MBPS                  = 0xd,       ///< Data rate 802.11g 54Mbps
        WFM_MAX_PHY_RATE_N_6P5MBPS                 = 0xe,       ///< Data rate 802.11n 6.5Mbps
        WFM_MAX_PHY_RATE_N_13MBPS                  = 0xf,       ///< Data rate 802.11n 13Mbps
        WFM_MAX_PHY_RATE_N_19P5MBPS                = 0x10,      ///< Data rate 802.11n 19.5Mbps
        WFM_MAX_PHY_RATE_N_26MBPS                  = 0x11,      ///< Data rate 802.11n 26Mbps
        WFM_MAX_PHY_RATE_N_39MBPS                  = 0x12,      ///< Data rate 802.11n 39Mbps
        WFM_MAX_PHY_RATE_N_52MBPS                  = 0x13,      ///< Data rate 802.11n 52Mbps
        WFM_MAX_PHY_RATE_N_58P5MBPS                = 0x14,      ///< Data rate 802.11n 58.5Mbps
        WFM_MAX_PHY_RATE_N_65MBPS                  = 0x15       ///< Data rate 802.11n 65Mbps
} WfmMaxPhyRate;

/**
 * @brief Device Protected Management Frame mode.
 */
typedef enum WfmMgmtFrameProtection_e {
        WFM_MGMT_FRAME_PROTECTION_DISABLED         = 0x0,		///< PMF disabled
        WFM_MGMT_FRAME_PROTECTION_OPTIONAL         = 0x1,		///< PMF optional
        WFM_MGMT_FRAME_PROTECTION_MANDATORY        = 0x2		///< PMF mandatory
} WfmMgmtFrameProtection;

/**
 * @brief Device power management mode.
 */
typedef enum WfmPmMode_e {
        WFM_PM_MODE_ACTIVE                         = 0x0,		///< Always on
        WFM_PM_MODE_PS                             = 0x1,		///< Use PowerSave and wake up on beacons
        WFM_PM_MODE_DTIM                           = 0x2		///< Use PowerSave and wake up on DTIM
} WfmPmMode;

/**
 * @brief Data priority level
 */
typedef enum WfmPriority_e {
        WFM_PRIORITY_BE                            = 0x0,       ///< Best Effort
        WFM_PRIORITY_BK                            = 0x1,       ///< Background
        WFM_PRIORITY_VI                            = 0x2,       ///< Video
        WFM_PRIORITY_VO                            = 0x3        ///< Voice
} WfmPriority;

/**
 * @brief Reasons for Ineo AP to reject or disconnect a client
 */
typedef enum WfmReason_e {
        WFM_REASON_UNSPECIFIED                     = 0x0,		///< Unspecified reason (unused)
        WFM_REASON_TIMEOUT                         = 0x1,		///< Client timed out
        WFM_REASON_LEAVING_BSS                     = 0x2,		///< Client left
        WFM_REASON_UNKNOWN_STA                     = 0x3,		///< Client not authenticated
        WFM_REASON_AP_FULL                         = 0x4		///< Too many clients already connected
} WfmReason;

/**
 * @brief Scan mode to be used
 */
typedef enum WfmScanMode_e {
        WFM_SCAN_MODE_PASSIVE                      = 0x0,		///< Passive scan: listen for beacons only
        WFM_SCAN_MODE_ACTIVE                       = 0x1		///< Active scan: send probe requests
} WfmScanMode;

/**
 * @brief Security mode of a network.
 */
typedef enum WfmSecurityMode_e {
        WFM_SECURITY_MODE_OPEN                     = 0x0,		///< No security
        WFM_SECURITY_MODE_WEP                      = 0x1,		///< Use WEP
        WFM_SECURITY_MODE_WPA2_WPA1_PSK            = 0x2,		///< Use WPA1 or WPA2
        WFM_SECURITY_MODE_WPA2_PSK                 = 0x4		///< Use only WPA2
} WfmSecurityMode;

/**
 * @brief Full MAC (UMAC) confirmation possible values for returned 'Status' field
 *
 * All Full MAC (UMAC) confirmation messages have a field 'Status' just after the message header.@n
 * A value of zero indicates the request has completed successfully.
 */
typedef enum WfmStatus_e {
        WFM_STATUS_SUCCESS                         = 0x0,      	///< The firmware has successfully completed a request.
        WFM_STATUS_INVALID_PARAMETER               = 0x1,       ///< A request contains one or more invalid parameters.
        WFM_STATUS_WRONG_STATE                     = 0x2,       ///< The request cannot be performed because the device is in an inappropriate state.
        WFM_STATUS_GENERAL_FAILURE                 = 0x3		///< The request failed due to an error.
} WfmStatus;

/**
 * @}
 */

/**
 * @addtogroup WFM_TYPES API types
 * @brief WFM API types.
 * @{
 */

/**
 * @brief Service Set Identifier (SSID) of a network.
 * @details Note that the Ssid element must always contain ::WFM_API_SSID_SIZE bytes.
 *          Only the bytes up to SsidLength are considered to be valid, the rest should be set to zero.
 */
typedef struct __attribute__((__packed__)) WfmHiSsidDef_s {
    /**
     * @brief Length of SSID data.
     * @details <B>0 - 32</B>: The amount of bytes.
     */
    uint32_t SsidLength;
    /** SSID data. */
    uint8_t Ssid[WFM_API_SSID_SIZE];
} WfmHiSsidDef_t;

/**
 * @brief Device TX rate set bitmask used in ::WfmHiSetTxRateParametersReqBody_t.
 */
typedef struct __attribute__((__packed__)) WfmHiRateSetBitmask_s {
        uint8_t    B1Mbps : 1;                       ///< If set, the device may use 802.11b 1Mbps data rate.
        uint8_t    B2Mbps : 1;                       ///< If set, the device may use 802.11b 2Mbps data rate.
        uint8_t    B5P5Mbps : 1;                     ///< If set, the device may use 802.11b 5.5Mbps data rate.
        uint8_t    B11Mbps : 1;                      ///< If set, the device may use 802.11b 11Mbps data rate.
        uint8_t    Unused : 4;                       ///< Reserved, set to zero
        uint8_t    G6Mbps : 1;                       ///< If set, the device may use 802.11g 6Mbps data rate.
        uint8_t    G9Mbps : 1;                       ///< If set, the device may use 802.11g 9Mbps data rate.
        uint8_t    G12Mbps : 1;                      ///< If set, the device may use 802.11g 12Mbps data rate.
        uint8_t    G18Mbps : 1;                      ///< If set, the device may use 802.11g 18Mbps data rate.
        uint8_t    G24Mbps : 1;                      ///< If set, the device may use 802.11g 24Mbps data rate.
        uint8_t    G36Mbps : 1;                      ///< If set, the device may use 802.11g 36Mbps data rate.
        uint8_t    G48Mbps : 1;                      ///< If set, the device may use 802.11g 48Mbps data rate.
        uint8_t    G54Mbps : 1;                      ///< If set, the device may use 802.11g 54Mbps data rate.
        uint8_t    Mcs0 : 1;                         ///< If set, the device may use 802.11n 6.5Mbps data rate.
        uint8_t    Mcs1 : 1;                         ///< If set, the device may use 802.11n 13Mbps data rate.
        uint8_t    Mcs2 : 1;                         ///< If set, the device may use 802.11n 19.5Mbps data rate.
        uint8_t    Mcs3 : 1;                         ///< If set, the device may use 802.11n 26Mbps data rate.
        uint8_t    Mcs4 : 1;                         ///< If set, the device may use 802.11n 39Mbps data rate.
        uint8_t    Mcs5 : 1;                         ///< If set, the device may use 802.11n 52Mbps data rate.
        uint8_t    Mcs6 : 1;                         ///< If set, the device may use 802.11n 58.5Mbps data rate.
        uint8_t    Mcs7 : 1;                         ///< If set, the device may use 802.11n 65Mbps data rate.
        uint8_t    Unused2;                          ///< Reserved, set to zero
} WfmHiRateSetBitmask_t;

/**
 * @brief NS IP address element.
 * @details Note that the IP element must always contain ::WFM_API_IPV6_ADDR_SIZE bytes.
 *          Only the bytes up to SsidLength are considered to be valid, the rest should be set to zero.
 */
typedef struct __attribute__((__packed__)) WfmHiNsIpAddr_s {
    /** NS IP address. */
    uint8_t Ipv6Addr[WFM_API_IPV6_ADDR_SIZE];
} WfmHiNsIpAddr_t;

/**
 * @}
 */

/**
 * @addtogroup WFM_CONCEPTS API concepts
 * @brief WFM API concepts.
 * @{
 */

/**
 * @page WFM_CONCEPT_SSID Service Set Identifier
 * @par
 * Service Set Identifier (SSID) is an unique identifier of a Wi-Fi network that can be
 * seen as a network name since it's typically an ASCII or UTF8 string.
 * @par
 * SSID contains 0 to 32 bytes of data. When SSID is 0 bytes long, it's known as
 * a broadcast SSID or a wildcard SSID.
 */

/**
 * @page WFM_CONCEPT_BSSID Basic Service Set Identifier
 * @par
 * Basic Service Set Identifier (BSSID) is an unique identifier of a Wi-Fi access point.
 * BSSID is a 6-byte field set in the same format as an IEEE 802 MAC address.
 * <BR>See @ref WFM_CONCEPT_MAC for further details.
 */

/**
 * @page WFM_CONCEPT_MAC Media Access Control address
 * @par Broadcast MAC address
 * When all bytes of the MAC address field are set to 0xFF, the MAC address is considered
 * to be a broadcast MAC address.
 * @code
 * { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF }
 * @endcode
 *
 * @par Multicast MAC address
 * When the lowest bit of the first byte is set, the MAC address is considered to
 * be a multicast MAC address.
 * @code
 * { 0x33, 0x33, 0x00, 0x00, 0x00, 0x01 }
 * @endcode
 *
 * @par Zero MAC address
 * When all bytes of the MAC address field are set to 0x00, the MAC address is considered
 * to be a zero MAC address.
 * @code
 * { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }
 * @endcode
 */

/**
 * @page WFM_CONCEPT_VENDOR_IE Vendor-specific IE
 * @par
 * 802.11 Vendor-Specific element starts with a 1-byte Element ID (0xDD), followed by 1-byte Element Length
 * specifying the amount of bytes after the length field. The next field is Organization Identifier (OUI)
 * field which is typically at least 3 bytes, followed by the vendor-specific content.
 * @par
 * An example of a vendor-specific IE that uses a Silicon Laboratories OUI (90-FD-9F),
 * a vendor-specific type of 0 and vendor-specific data "HELLO".
 * @code
 * { 0xDD, 0x09, 0x90, 0xFD, 0x9F, 0x00, 0x48 0x,45 0x4C, 0x4C, 0x4F }
 * @endcode
 */

/**
 * @page WFM_CONCEPT_PASSWORD Passwords and passphrases
 * @par
 * Note that the Password element must always contain ::WFM_API_PASSWORD_SIZE bytes.
 * Only the bytes up to PasswordLength are considered to be valid, the rest are ignored.
 * @par An example of a 64-bit WEP key "12345" in ASCII format
 * @code
 * { 0x31, 0x32, 0x33, 0x34, 0x35 }
 * @endcode
 *
 * @par An example of a 64-bit WEP key "\x31\x32\x33\x34\x35" in HEX format
 * @code
 * { 0x33, 0x31, 0x33, 0x32, 0x33, 0x33, 0x33, 0x34, 0x33, 0x35 }
 * @endcode
 */

/**
 * @page WFM_CONCEPT_RCPI Received Channel Power Indicator
 * @par
 * Received Channel Power Indicator (RCPI) is a monotonically increasing,
 * logarithmic function of the received power level.
 * | Value     | Description |
 * |-----------|-------------|
 * |        0  | Power level is less than -109.5 dBm         |
 * |   1 - 219 | Power level is between -109.5 dBm and 0 dBm |
 * |       220 | Power level is equal or greater than 0 dBm  |
 * | 221 - 255 | Reserved
 *
 * The following equation can be used to convert the RCPI value to corresponding dBm value.
 * <BR>[signal level in dBm] = (RCPI / 2) - 110
 * <BR>RCPI 60 => signal level is -80 dBm
 * <BR>RCPI 80 => signal level is -70 dBm
 */

/**
 * @page WFM_CONCEPT_PM Power management
 * @par
 * In Wi-Fi power management, a device has two power modes: active and power save. In active mode
 * the device is able to transmit and receive packets. When in power save, the device has signaled
 * other devices it's available only periodically and communicating with it requires specific
 * power management procedures.
 * @par
 * Wi-Fi power management is different from device power management. Even when the device
 * has enabled Wi-Fi power save, it doesn't necessarily mean the device is actually in a
 * low-power state. Wi-Fi power save must be enabled in order to use device power management
 * but not vice versa.
 * @par
 * Wi-Fi power save is only available in Wi-Fi client role. The functionality can be controlled
 * using ::WfmHiSetPmModeReq_t message. The device has three different power management modes.
 * @par WFM_PM_MODE_ACTIVE
 * In this mode the device does not use Wi-Fi power management. Since no special power management
 * procedures are required, this mode gives the best throughput and latency. Device power management
 * cannot be used in this mode. This is the default mode after connection has been established.
 * @par WFM_PM_MODE_BEACON
 * In this mode the device signals to the Wi-Fi access point it's in power save and thus only
 * available on periodic intervals. This causes the access point to buffer packets destined
 * to the device until the device retrieves them and therefore causes extra delays on received
 * packets. However, the mode allows the device to use device power management.
 * @par WFM_PM_MODE_DTIM
 * This mode is similar to WFM_PM_MODE_BEACON but it uses a different periodic interval called
 * DTIM period. Length of the DTIM period depends on the access point used but it is at least
 * as long as the period in WFM_PM_MODE_BEACON.
 */

/**
 * @page WFM_CONCEPT_HIDDEN Hidden network
 * @par
 * Usually a Wi-Fi access point will advertise its details in Beacon frames as well as in
 * Probe Response frames which are sent as a response to Probe Request frames. One of the
 * details advertised is the SSID of the network.
 * @par
 * When the network is hidden, the device will replace the actual SSID in Beacon frames with
 * a broadcast SSID and will stop responding to Probe Requests that do not specify the correct
 * SSID. In practise, other stations will still see there is a network near-by due to the
 * Beacon frames but they will not be able to determine the SSID.
 * @par
 * This feature shouldn't be seen as a security feature since it's fairly simple to determine
 * the SSID by passively observing stations that know the correct SSID.
 */

/**
 * @page WFM_CONCEPT_ISOLATION Client isolation
 * @par
 * When a Wi-Fi network has multiple stations connected, they communicate with each other
 * by first sending the data frame to the access point. Access Point then determines the
 * destination is one of its clients and resends the frame to the destination station.
 *
 * When client isolation is enabled, the access point will discard all data frames intended
 * to other stations. Therefore the stations will only be able to communicate with the
 * access point.
 */

/**
 * @page WFM_CONCEPT_PACKET Packet types
 * @par Data frame with Ethernet II header
 * When transmitting and receiving data frames in this format, the payload message such as
 * an IP packet or an ARP packet is encapsulated in an Ethernet II frame header without the
 * trailing CRC checksum.
 * @par
 * Ethernet II headers is 14 bytes (6-byte Destination MAC address, 6-byte Source MAC Address,
 * 2-byte EtherType). The EtherType is written in big-endian format.
 * @par
 * An ARP packet (EtherType 0x0806) sent to the broadcast address (FF:FF:FF:FF:FF:FF)
 * from the source MAC address (00:01:02:03:04:05) therefore starts with the header
 * @code
 * { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x08, 0x06 }
 * @endcode
 * and is immediately followed by the actual ARP packet.
 */

/**
 * @page WFM_CONCEPT_SCAN Scanning
 * @par Scan mode
 * The device supports two modes of scanning: active and passive. In passive mode, the device
 * will switch to each channel to be scanned in turn and listen for access points operating
 * on the channel. After a time limit, it switches to the next channel.
 * @par
 * The active mode is similar to the passive mode but in addition to listening, the device
 * will solicitate advertisements from the access points on the channel. Whether an access
 * point responds to a solicitation depends on the SSID(s) used in the scan. If the scan
 * is performed with a wildcard SSID, all access points on the channel will respond. If
 * performed with a specific SSID, only the access points having that SSID will respond.
 * @par
 * Note that even when scanning with a specific SSID, the scan results may contain other
 * access points if the device happened to hear the advertisement while on the channel.
 * @par Specific channel list
 * It's possible to specify a list of channels to be scanned. The channel list
 * is an array of bytes, one byte per channel. The channels are scanned in the order given.
 * @par
 * An example of channels list (1, 6, 11)
 * @code
 * { 0x01, 0x06, 0x11 }
 * @endcode
 * @par Specific SSID list
 * When performing an active scan, a list of SSIDs to be scanned can be specified. Only
 * the SSIDs on the list will be queried. The lists consists of multiple ::WfmHiSsidDef_t elements
 * concatenated together.
 */

/**
 * @page WFM_CONCEPT_OFFLOADING Offloading
 * @par
 * In order to facilitate low-power use-cases, there is a possibility to offload
 * replying to Address Resolution Protocol (ARP) requests and IPv6 Neighbor Solicitation (NS)
 * packets to the device. By default the offloading functionality is disabled.
 * @par ARP offloading
 * When the functionality is enabled, the device will respond to ARP requests that
 * specify one of the configured IPv4 addresses. The request is not forwarded to
 * the host. The functionality can be controlled using ::WfmHiSetArpIpAddressReq_t message.
 * @par NS offloading
 * When the functionality is enabled, the device will respond to IPv6 NS packets that
 * specify one of the configured IPv6 addresses. The request is not forwarded to
 * the host. The functionality can be controlled using ::WfmHiSetNsIpAddressReq_t message.
 */

/**
 * @page WFM_CONCEPT_FILTERING Filtering
 * @par
 * By default, the device will forward all packets it receives from the network to the host.
 * In a busy network, processing the broadcast and the multicast traffic may be a significant
 * source of power consumption on the host. In order to facilitate low-power use-cases, the
 * device offers multiple options for discarding some of the received packets in the firmware.
 * @par Multicast filtering
 * Filtering of multicast traffic is based on a whitelist of destination multicast addresses.
 * Any received packet whose destination address does not match the whitelist is automatically
 * discarded. In case the host is not interested in multicast traffic, it's also possible to
 * discard all multicast traffic. By default all multicast packets are accepted. The functionality
 * can be controlled using ::WfmHiAddMulticastAddrReq_t and ::WfmHiRemoveMulticastAddrReq_t
 * messages.
 * @par Broadcast filtering
 * Broadcast filtering is a boolean option. When enabled, the device will discard all received
 * packets sent to the broadcast address except ARP and DHCP messages. By default all broadcast
 * packets are accepted. The functionality can be controlled using ::WfmHiSetBroadcastFilterReq_t message.
 */

/**
 * @page WFM_CONCEPT_ROAM Roaming
 * @par
 * When operating in Wi-Fi client role, the device is capable of autonomously switching to
 * a different access point when the current access point is either lost or the signal strength
 * drops below the roaming threshold. The device will only consider access points that have
 * the same SSID and that otherwise have the same security capabilities as the previous access
 * point.
 * @par
 * The autonomous roaming functionality can be disabled by setting the corresponding option
 * in ::WfmHiConnectReq_t message. The various roaming parameters may be adjusted using
 * ::WfmHiSetRoamParametersReq_t message. Parameter changes will be applied at the next connection.
 */

/**
 * @page WFM_CONCEPT_TX_RATE Transmit rate
 * @par
 * A Wi-Fi device may use a number of different data rates for transmission. When operating as a client,
 * a device adapts the rates it uses to match those the access point authorizes. However it is possible
 * to specify rates that Ineo will not use even if they are allowed by the network.
 * @par
 * The precise allowed rates may be adjusted using ::WfmHiSetTxRateParametersReq_t message.
 * Parameter changes will be applied at the next connection. If no overlap between these parameters
 * and the access point's supported rates list is found then it will default to using only 1Mbits/sec rate.
 */

/**
 * @}
 */

/**
 * @defgroup WFM_STATE_IDLE Idle state
 * @brief WFM API messages applicable for an interface in idle mode.
 */

/**
 * @defgroup WFM_STATE_STA Wi-Fi client state
 * @brief WFM API messages applicable for an interface in STA mode.
 */

/**
 * @defgroup WFM_STATE_AP Wi-Fi access point state
 * @brief WFM API messages applicable for an interface in AP mode.
 */

/**
 * @defgroup WFM_STATE_IBSS IBSS station state
 * @brief WFM API messages applicable for an interface in IBSS mode.
 */

/**
 * @addtogroup WFM_MESSAGES API messages
 * @brief WFM API messages.
 * @{
 */

/**
 * @brief Request message body for WfmHiSetMacAddressReq_t.
 */
typedef struct __attribute__((__packed__)) WfmHiSetMacAddressReqBody_s {
    /** MAC address of the interface. */
    uint8_t MacAddr[WFM_API_MAC_ADDR_SIZE];
    /** Reserved, set to zero. */
    uint16_t Reserved;
} WfmHiSetMacAddressReqBody_t;

/**
 * @brief Request message to set MAC address of the interface.
 * @details The host can use this request to set the MAC address an interface.
 *          If not set, the device will use a built-in MAC address. Note that
 *          if multiple interfaces are used, the MAC address MUST be different
 *          on each interface.
 * @ingroup WFM_STATE_IDLE
 */
typedef struct __attribute__((__packed__)) WfmHiSetMacAddressReq_s {
    /** Common message header. */
    HiMsgHdr_t Header;
    /** Request message body. */
    WfmHiSetMacAddressReqBody_t Body;
} WfmHiSetMacAddressReq_t;

/**
 * @brief Confirmation message body for WfmHiSetMacAddressCnf_t.
 */
typedef struct __attribute__((__packed__)) WfmHiSetMacAddressCnfBody_s {
    /**
     * @brief Status of the set request.
     * @details <B>WFM_STATUS_SUCCESS</B>: the set request was completed successfully.
     *          <BR><B>any other value</B>: the set request failed.
     *          <BR>See ::WfmStatus for enumeration values.
     */
    uint32_t Status;
} WfmHiSetMacAddressCnfBody_t;

/**
 * @brief Confirmation message for WfmHiSetMacAddressReq_t.
 * @ingroup WFM_STATE_IDLE
 */
typedef struct __attribute__((__packed__)) WfmHiSetMacAddressCnf_s {
    /** Common message header. */
    HiMsgHdr_t Header;
    /** Confirmation message body. */
    WfmHiSetMacAddressCnfBody_t Body;
} WfmHiSetMacAddressCnf_t;

/**
 * @brief Request message body for WfmHiConnectReq_t.
 */
typedef struct __attribute__((__packed__)) WfmHiConnectReqBody_s {
    /**
     * @brief Service Set Identifier (SSID) of the network.
     */
    WfmHiSsidDef_t SsidDef;
    /**
     * @brief Basic Service Set Identifier (BSSID) of the access point.
     * @details <B>broadcast address</B>: The device will connect to any matching access point.
     *          <BR><B>unicast address</B>: The device will only connect to the given access point.
     *          <BR>See @ref WFM_CONCEPT_BSSID for further details.
     */
    uint8_t BSSID[WFM_API_BSSID_SIZE];
    /**
     * @brief Channel of the access point.
     * @details <B>0</B>: The device will connect to a matching access point on any channel.
     *          <BR><B>1 - 13</B>: The device will only connect to a matching access point on the given channel.
     */
    uint16_t Channel;
    /**
     * @brief Security mode of the network.
     * @details <B>WFM_SECURITY_MODE_OPEN</B>: The device will only connect to an unsecured access point.
     *          <BR><B>WFM_SECURITY_MODE_WEP</B>: The device will only connect to a WEP access point.
     *          <BR><B>WFM_SECURITY_MODE_WPA2_WPA1_PSK</B>: The device will only connect to a WPA-Personal or a WPA2-Personal access point.
     *          <BR><B>WFM_SECURITY_MODE_WPA2_PSK</B>: The device will only connect to a WPA2-Personal access point.
     *          <BR>See ::WfmSecurityMode for enumeration values.
     */
    uint8_t SecurityMode;
    /**
     * @brief Boolean option to prevent roaming between access points.
     * @details <B>0</B>: The device may roam to any matching access point within the same network.
     *          <BR><B>1</B>: The device will not roam to any other access point.
     */
    uint8_t PreventRoaming;
    /**
     * @brief Protected Management Frames (PMF) mode.
     * @details <B>WFM_MGMT_FRAME_PROTECTION_DISABLED</B>: The device will not use PMF even if supported by the access point.
     *          <BR><B>WFM_MGMT_FRAME_PROTECTION_OPTIONAL</B>: The device will use PMF if supported by the access point.
     *          <BR><B>WFM_MGMT_FRAME_PROTECTION_MANDATORY</B>: The device will only connect to an access point supporting PMF.
     *          <BR>See ::WfmMgmtFrameProtection for enumeration values.
     */
    uint16_t MgmtFrameProtection;
    /**
     * @brief Length of the network password.
     * @details <B>0 - 64</B>: The amount of bytes.
     */
    uint16_t PasswordLength;
    /**
     * @brief Password of the network.
     * @details <B>64-bit WEP key</B>: 5 bytes in ASCII format or 10 bytes in HEX format.
     *          <BR><B>128-bit WEP key</B>: 13 bytes in ASCII format or 26 bytes in HEX format.
     *          <BR><B>WPA passphrase</B>: 8 - 63 bytes in ASCII format.
     *          <BR><B>WPA-PSK (hashed passphrase)</B>: 64 bytes in HEX format.
     *          <BR>See @ref WFM_CONCEPT_PASSWORD for further details.
     */
    uint8_t Password[WFM_API_PASSWORD_SIZE];
    /**
     * @brief Length of vendor-specific Information Element (IE) data.
     * @details <B>0 - 255</B>: The amount of bytes.
     */
    uint16_t IeDataLength;
    /**
     * @brief Vendor-specific IE data added to the 802.11 Association Request frames.
     * @details The IE data must be in 802.11 Vendor-Specific Element format.
     *          It may contain multiple concatenated IEs, up to the maximum length.
     *          <BR>See @ref WFM_CONCEPT_VENDOR_IE for further details.
     */
    /*uint8_t IeData[];*/
} WfmHiConnectReqBody_t;

/**
 * @brief Request message for connecting to a Wi-Fi network.
 * @details The host can use this request to iniate a connection to a Wi-Fi network.
 *          <P>Successful connection request moves the interface to @ref WFM_STATE_STA.
 * @ingroup WFM_STATE_IDLE
 */
typedef struct __attribute__((__packed__)) WfmHiConnectReq_s {
    /** Common message header. */
    HiMsgHdr_t Header;
    /** Request message body. */
    WfmHiConnectReqBody_t Body;
} WfmHiConnectReq_t;

/**
 * @brief Confirmation message body for WfmHiConnectCnf_t.
 */
typedef struct __attribute__((__packed__)) WfmHiConnectCnfBody_s {
    /**
     * @brief Status of the connection request.
     * @details <B>WFM_STATUS_SUCCESS</B>: the connection request was accepted. It will be completed by ::WfmHiConnectInd_t.
     *          <BR><B>any other value</B>: the connection request failed.
     *          <BR>See ::WfmStatus for enumeration values.
     */
    uint32_t Status;
} WfmHiConnectCnfBody_t;

/**
 * @brief Confirmation message for WfmHiConnectReq_t.
 * @ingroup WFM_STATE_IDLE
 */
typedef struct __attribute__((__packed__)) WfmHiConnectCnf_s {
    /** Common message header. */
    HiMsgHdr_t Header;
    /** Confirmation message body. */
    WfmHiConnectCnfBody_t Body;
} WfmHiConnectCnf_t;

/**
 * @brief Indication message body for WfmHiConnectInd_t.
 */
typedef struct __attribute__((__packed__)) WfmHiConnectIndBody_s {    
    /**
     * @brief Status of the connection request.
     * @details <B>WFM_STATUS_SUCCESS</B>: the connection request was completed successfully.
     *          <BR><B>any other value</B>: the connection request failed.
     *          <BR>See ::WfmStatus for enumeration values.
     */
    uint32_t Status;
    /**
     * @brief MAC address of the connected access point.
     */
    uint8_t Mac[WFM_API_MAC_SIZE];
    /**
     * @brief Channel of the connected access point.
     * @details <B>1 - 13</B>: Channel number.
     */
    uint16_t Channel;
    /**
     * @brief Beacon Interval of the connected access point.
     */
    uint8_t BeaconInterval;
    /**
     * @brief DTIM period of the connected access point.
     * @details <B>1 - 255</B>: DTIM period.
     */
    uint8_t DtimPeriod;
    /**
     * @brief Maximum PHY data rate supported by the connection.
     * @details See ::WfmMaxPhyRate for enumeration values.
     */
    uint16_t MaxPhyRate;
} WfmHiConnectIndBody_t;

/**
 * @brief Indication message used to signal the completion of a connection operation.
 * @details The device will send this indication to signal the connection request initiated
 *          with ::WfmHiConnectReq_t has been completed. The indication is also sent when
 *          the device autonomously roams to another access point.
 * @ingroup WFM_STATE_IDLE
 */
typedef struct __attribute__((__packed__)) WfmHiConnectInd_s {
    /** Common message header. */
    HiMsgHdr_t Header;
    /** Indication message body. */
    WfmHiConnectIndBody_t Body;
} WfmHiConnectInd_t;

/**
 * @struct WfmHiDisconnectReq_t
 * @brief Request message for disconnecting from a Wi-Fi network.
 * @details The host can use this request to iniate a disconnection from a Wi-Fi network.
 *          <P>Successful disconnection request moves the interface to @ref WFM_STATE_IDLE.
 * @ingroup WFM_STATE_STA
 */
typedef HiMsgHdr_t WfmHiDisconnectReq_t; 

/**
 * @brief Confirmation message body for WfmHiDisconnectCnf_t.
 */
typedef struct __attribute__((__packed__)) WfmHiDisconnectCnfBody_s {
    /**
     * @brief Status of the disconnection request.
     * @details <B>WFM_STATUS_SUCCESS</B>: the disconnection request was accepted. It will be completed by ::WfmHiDisconnectInd_t.
     *          <BR><B>any other value</B>: the disconnection request failed.
     *          <BR>See ::WfmStatus for enumeration values.
     */
    uint32_t Status;
} WfmHiDisconnectCnfBody_t;

/**
 * @brief Confirmation message for WfmHiDisconnectReq_t.
 * @ingroup WFM_STATE_STA
 */
typedef struct __attribute__((__packed__)) WfmHiDisconnectCnf_s {
    /** Common message header. */
    HiMsgHdr_t Header;
    /** Confirmation message body. */
    WfmHiDisconnectCnfBody_t Body;
} WfmHiDisconnectCnf_t;

/**
 * @brief Indication message body for WfmHiDisconnectInd_t.
 */
typedef struct __attribute__((__packed__)) WfmHiDisconnectIndBody_s {
    /**
     * @brief MAC address of the access point.
     */
    uint8_t Mac[WFM_API_MAC_SIZE];
    /**
     * @brief Reason for disconnection.
     * @details <B>WFM_REASON_UNSPECIFIED</B>: The device was disconnected or it disconnected on its own.
     *          <BR>See ::WfmReason for enumeration values.
     */
    uint16_t Reason;
} WfmHiDisconnectIndBody_t;

/**
 * @brief Indication message used to signal the completion of a disconnection operation.
 * @details The device will send this indication to signal the disconnection request initiated
 *          with ::WfmHiDisconnectReq_t has been completed. The indication is also sent when
 *          the device has lost the connection to an access point and has been unable to regain it.
 * @ingroup WFM_STATE_STA
 */
typedef struct __attribute__((__packed__)) WfmHiDisconnectInd_s {
    /** Common message header. */
    HiMsgHdr_t Header;
    /** Indication message body. */
    WfmHiDisconnectIndBody_t Body;
} WfmHiDisconnectInd_t;

/**
 * @struct WfmHiGetSignalStrengthReq_t
 * @brief Request message for retrieving the signal strength of connection.
 * @details The host can use this request to retrieve the signal strength of the connection.
 * @ingroup WFM_STATE_STA
 */
typedef HiMsgHdr_t WfmHiGetSignalStrengthReq_t;

/**
 * @brief Confirmation message body for WfmHiGetSignalStrengthCnf_t.
 */
typedef struct __attribute__((__packed__)) WfmHiGetSignalStrengthCnfBody_s {
    /**
     * @brief Status of the get request.
     * @details <B>WFM_STATUS_SUCCESS</B>: the get request was completed.
     *          <BR><B>any other value</B>: the get request failed.
     *          <BR>See ::WfmStatus for enumeration values.
     */
    uint32_t Status;
    /**
     * @brief Received Channel Power Indicator (RCPI) of the connection.
     * @details See @ref WFM_CONCEPT_RCPI for further details.
     */
    uint32_t Rcpi;
} WfmHiGetSignalStrengthCnfBody_t;

/**
 * @brief Confirmation message for WfmHiGetSignalStrengthReq_t.
 * @ingroup WFM_STATE_STA
 */
typedef struct __attribute__((__packed__)) WfmHiGetSignalStrengthCnf_s {
    /** Common message header. */
    HiMsgHdr_t Header;
    /** Confirmation message body. */
    WfmHiGetSignalStrengthCnfBody_t Body;
} WfmHiGetSignalStrengthCnf_t;

/**
 * @brief Request message body for WfmHiSetPmModeReq_t.
 */
typedef struct __attribute__((__packed__)) WfmHiSetPmModeReqBody_s {
    /**
     * @brief Power management mode.
     * @details <B>WFM_PM_MODE_ACTIVE</B>: the device will not use Wi-Fi power management mechanisms.
     *          <BR><B>WFM_PM_MODE_BEACON</B>: the device will wake-up on beacons.
     *          <BR><B>WFM_PM_MODE_DTIM</B>: the device will wake-up on DTIMs.
     *          <BR>See ::WfmPmMode for enumeration values.
     */
    uint16_t PowerMode;
    /**
     * @brief Number of beacons/DTIMs to skip while sleeping.
     * @details <B>0</B>: wake-up on every beacon/DTIM.
     *          <BR>1 - 600</B>: the number of beacon/DTIMs to skip.
     *          <BR>See @ref WFM_CONCEPT_PM for further details.
     */
    uint16_t ListenInterval;
} WfmHiSetPmModeReqBody_t;

/**
 * @brief Request message for setting the power management mode of the device.
 * @details The host can use this request to enable or disable Wi-Fi power management mechanisms.
 * @ingroup WFM_STATE_STA
 */
typedef struct __attribute__((__packed__)) WfmHiSetPmModeReq_s {
    /** Common message header. */
    HiMsgHdr_t Header;
    /** Request message body. */
    WfmHiSetPmModeReqBody_t Body;
} WfmHiSetPmModeReq_t;

/**
 * @brief Confirmation message body for WfmHiSetPmModeCnf_t.
 */
typedef struct __attribute__((__packed__)) WfmHiSetPmModeCnfBody_s {
    /**
     * @brief Status of the power management request.
     * @details <B>WFM_STATUS_SUCCESS</B>: the power management request was completed.
     *          <BR><B>any other value</B>: the power management request failed.
     *          <BR>See ::WfmStatus for enumeration values.
     */
    uint32_t Status;
} WfmHiSetPmModeCnfBody_t;

/**
 * @brief Confirmation message for WfmHiSetPmModeReq_t.
 * @ingroup WFM_STATE_STA
 */
typedef struct __attribute__((__packed__)) WfmHiSetPmModeCnf_s {
    /** Common message header. */
    HiMsgHdr_t Header;
    /** Confirmation message body. */
    WfmHiSetPmModeCnfBody_t Body;
} WfmHiSetPmModeCnf_t;

/**
 * @brief Request message body for WfmHiStartApReq_t.
 */
typedef struct __attribute__((__packed__)) WfmHiStartApReqBody_s {
    /**
     * @brief Service Set Identifier (SSID) of the network.
     */
    WfmHiSsidDef_t SsidDef;
    /**
     * @brief Boolean option to hide the network.
     * @details <B>0</B>: The device will advertise the SSID of the network to any near-by stations.
     *          <BR><B>1</B>: The device will hide the SSID of the network and will only respond
     *                        to stations that specify the SSID.
     *          <BR>See @ref WFM_CONCEPT_HIDDEN for further details.
     */
    uint8_t HiddenSsid;
    /**
     * @brief Boolean option to isolate connected clients from each other.
     * @details <B>0</B>: The device will allow connected clients to communicate with each other.
     *          <BR><B>1</B>: The device will prevent multiple connected clients from communicating.
     *          <BR>Regardless of the value, the connected stations will always be able to communicate with the device.
     *          <BR>See @ref WFM_CONCEPT_ISOLATION for further details.
     */
    uint8_t ClientIsolation;
    /**
     * @brief Security mode of the access point.
     * @details <B>WFM_SECURITY_MODE_OPEN</B>: The device will only allow unsecured connections.
     *          <BR><B>WFM_SECURITY_MODE_WEP</B>: The device will only allow WEP connections.
     *          <BR><B>WFM_SECURITY_MODE_WPA2_WPA1_PSK</B>: The device will only allow WPA-Personal and WPA2-Personal connections.
     *          <BR><B>WFM_SECURITY_MODE_WPA2_PSK</B>: The device will only allow WPA2-Personal connections.
     *          <BR>See ::WfmSecurityMode for enumeration values.
     */
    uint8_t SecurityMode;
    /**
     * @brief Protected Management Frames (PMF) mode.
     * @details <B>WFM_MGMT_FRAME_PROTECTION_DISABLED</B>: The device will not use PMF even if supported by the connecting station.
     *          <BR><B>WFM_MGMT_FRAME_PROTECTION_OPTIONAL</B>: The device will use PMF if supported by the connecting station.
     *          <BR><B>WFM_MGMT_FRAME_PROTECTION_MANDATORY</B>: The device will only allow connecting stations that support PMF.
     *          <BR>See ::WfmMgmtFrameProtection for enumeration values.
     */
    uint8_t MgmtFrameProtection;
    /**
     * @brief Channel of the access point.
     * @details <B>1 - 13</B>: The device will create the access point on the given channel.
     */
    uint16_t Channel;
    /**
     * @brief Length of the network password.
     * @details <B>0 - 64</B>: The amount of bytes.
     */
    uint16_t PasswordLength;
    /**
     * @brief Password of the network.
     * @details <B>64-bit WEP key</B>: 5 bytes in ASCII format or 10 bytes in HEX format.
     *          <BR><B>128-bit WEP key</B>: 13 bytes in ASCII format or 26 bytes in HEX format.
     *          <BR><B>WPA passphrase</B>: 8 - 63 bytes in ASCII format.
     *          <BR><B>WPA-PSK (hashed passphrase)</B>: 64 bytes in HEX format.
     *          <BR>See @ref WFM_CONCEPT_PASSWORD for further details.
     */
    uint8_t Password[WFM_API_PASSWORD_SIZE];
    /**
     * @brief Length of vendor-specific Information Element (IE) data in 802.11 Beacon frames.
     * @details <B>0 - 255</B>: The amount of bytes.
     */
    uint16_t BeaconIeDataLength;
    /**
     * @brief Length of vendor-specific Information Element (IE) data in 802.11 Probe Response frames.
     * @details <B>0 - 255</B>: The amount of bytes.
     */
    uint16_t ProbeRespIeDataLength;
    /**
     * @brief Vendor-specific IE data added to the 802.11 Beacon frames.
     * @details The IE data must be in 802.11 Vendor-Specific Element format.
     *          It may contain multiple concatenated IEs, up to the maximum length.
     *          <BR>See @ref WFM_CONCEPT_VENDOR_IE for further details.
     */
    /*uint8_t BeaconIeData[];*/
    /**
     * @brief Vendor-specific IE data added to the 802.11 Probe Response frames.
     * @details The IE data must be in 802.11 Vendor-Specific Element format.
     *          It may contain multiple concatenated IEs, up to the maximum length.
     *          <BR>See @ref WFM_CONCEPT_VENDOR_IE for further details.
     */
    /*uint8_t ProbeRespIeData[];*/
} WfmHiStartApReqBody_t;

/**
 * @brief Request message for starting a Wi-Fi network.
 * @details The host can use this request to iniate a Wi-Fi network.
 *          <P>Successful start request moves the interface to @ref WFM_STATE_AP.
 * @ingroup WFM_STATE_IDLE
 */
typedef struct __attribute__((__packed__)) WfmHiStartApReq_s {
    /** Common message header. */
    HiMsgHdr_t Header;
    /** Request message body. */
    WfmHiStartApReqBody_t Body;
} WfmHiStartApReq_t;

/**
 * @brief Confirmation message body for WfmHiStartApCnf_t.
 */
typedef struct __attribute__((__packed__)) WfmHiStartApCnfBody_s {
    /**
     * @brief Status of the start request.
     * @details <B>WFM_STATUS_SUCCESS</B>: the start request was accepted. It will be completed by ::WfmHiStartApInd_t.
     *          <BR><B>any other value</B>: the start request failed.
     *          <BR>See ::WfmStatus for enumeration values.
     */
    uint32_t Status;
} WfmHiStartApCnfBody_t;

/**
 * @brief Confirmation message for WfmHiStartApReq_t.
 * @ingroup WFM_STATE_IDLE
 */
typedef struct __attribute__((__packed__)) WfmHiStartApCnf_s {
    /** Common message header. */
    HiMsgHdr_t Header;
    /** Confirmation message body. */
    WfmHiStartApCnfBody_t Body;
} WfmHiStartApCnf_t;

/**
 * @brief Indication message body for WfmHiStartApInd_t.
 */
typedef struct __attribute__((__packed__)) WfmHiStartApIndBody_s {
    /**
     * @brief Status of the start request.
     * @details <B>WFM_STATUS_SUCCESS</B>: the start request was completed successfully.
     *          <BR><B>any other value</B>: the start request failed.
     *          <BR>See ::WfmStatus for enumeration values.
     */
    uint32_t Status;
} WfmHiStartApIndBody_t;

/**
 * @brief Indication message used to signal the completion of a start request.
 * @details The device will send this indication to signal the start request initiated
 *          with ::WfmHiStartApReq_t has been completed.
 * @ingroup WFM_STATE_IDLE
 */
typedef struct __attribute__((__packed__)) WfmHiStartApInd_s {
    /** Common message header. */
    HiMsgHdr_t Header;
    /** Indication message body. */
    WfmHiStartApIndBody_t Body;
} WfmHiStartApInd_t;

/**
 * @brief Request message body for WfmHiUpdateApReq_t.
 */
typedef struct __attribute__((__packed__)) WfmHiUpdateApReqBody_s {
    /**
     * @brief Length of vendor-specific Information Element (IE) data in 802.11 Beacon frames.
     * @details <B>0 - 255</B>: The amount of bytes.
     */
    uint16_t BeaconIeDataLength;
    /**
     * @brief Length of vendor-specific Information Element (IE) data in 802.11 Probe Response frames.
     * @details <B>0 - 255</B>: The amount of bytes.
     */
    uint16_t ProbeRespIeDataLength;
    /**
     * @brief Vendor-specific IE data added to the 802.11 Beacon frames.
     * @details The IE data must be in 802.11 Vendor-Specific Element format.
     *          It may contain multiple concatenated IEs, up to the maximum length.
     *          <BR>See @ref WFM_CONCEPT_VENDOR_IE for further details.
     */
    /*uint8_t BeaconIeData[];*/
    /**
     * @brief Vendor-specific IE data added to the 802.11 Probe Response frames.
     * @details The IE data must be in 802.11 Vendor-Specific Element format.
     *          It may contain multiple concatenated IEs, up to the maximum length.
     *          <BR>See @ref WFM_CONCEPT_VENDOR_IE for further details.
     */
    /*uint8_t ProbeRespIeData[];*/
} WfmHiUpdateApReqBody_t;

/**
 * @brief Request message for updating parameters of the started Wi-Fi network.
 * @details The host can use this request to update parameters of the started Wi-Fi network.
 * @ingroup WFM_STATE_AP
 */
typedef struct __attribute__((__packed__)) WfmHiUpdateApReq_s {
    /** Common message header. */
    HiMsgHdr_t Header;
    /** Request message body. */
    WfmHiUpdateApReqBody_t Body;
} WfmHiUpdateApReq_t;

/**
 * @brief Confirmation message body for WfmHiUpdateApCnf_t.
 */
typedef struct __attribute__((__packed__)) WfmHiUpdateApCnfBody_s {
    /**
     * @brief Status of the update request.
     * @details <B>WFM_STATUS_SUCCESS</B>: the update request was completed.
     *          <BR><B>any other value</B>: the update request failed.
     *          <BR>See ::WfmStatus for enumeration values.
     */
    uint32_t Status;
} WfmHiUpdateApCnfBody_t;

/**
 * @brief Confirmation message for WfmHiUpdateApReq_t.
 * @ingroup WFM_STATE_AP
 */
typedef struct __attribute__((__packed__)) WfmHiUpdateApCnf_s {
    /** Common message header. */
    HiMsgHdr_t Header;
    /** Confirmation message body. */
    WfmHiUpdateApCnfBody_t Body;
} WfmHiUpdateApCnf_t;

/**
 * @struct WfmHiStopApReq_t
 * @brief Request message for stopping the started Wi-Fi network.
 * @details The host can use this request to stop the started Wi-Fi network.
 *          <P>Successful stop request moves the interface to @ref WFM_STATE_IDLE.
 * @ingroup WFM_STATE_AP
 */
typedef HiMsgHdr_t WfmHiStopApReq_t; 

/**
 * @brief Confirmation message body for WfmHiStopApCnf_t.
 */
typedef struct __attribute__((__packed__)) WfmHiStopApCnfBody_s {
    /**
     * @brief Status of the stop request.
     * @details <B>WFM_STATUS_SUCCESS</B>: the stop request was accepted. It will be completed by ::WfmHiStopApInd_t.
     *          <BR><B>any other value</B>: the stop request failed.
     *          <BR>See ::WfmStatus for enumeration values.
     */
    uint32_t Status;
} WfmHiStopApCnfBody_t;

/**
 * @brief Confirmation message for WfmHiStopApReq_t.
 * @ingroup WFM_STATE_AP
 */
typedef struct __attribute__((__packed__)) WfmHiStopApCnf_s {
    /** Common message header. */
    HiMsgHdr_t Header;
    /** Confirmation message body. */
    WfmHiStopApCnfBody_t Body;
} WfmHiStopApCnf_t;

/**
 * @struct WfmHiStopApInd_t
 * @brief Indication message used to signal the completion of a stop operation.
 * @details The device will send this indication to signal the stop request initiated
 *          with ::WfmHiStopApReq_t has been completed. The indication is also sent when
 *          the started network has encountered a fatal error.
 * @ingroup WFM_STATE_AP
 */
typedef HiMsgHdr_t WfmHiStopApInd_t; 

/**
 * @brief Indication message body for WfmHiApClientConnectedInd_t.
 */
typedef struct __attribute__((__packed__)) WfmHiApClientConnectedIndBody_s {
    /**
     * @brief MAC address of the station.
     */
    uint8_t Mac[WFM_API_MAC_SIZE];
} WfmHiApClientConnectedIndBody_t;

/**
 * @brief Indication message used to signal a connected station.
 * @details The device will send this indication to signal a station has connected
 *          to the started network.
 * @ingroup WFM_STATE_AP
 */
typedef struct __attribute__((__packed__)) WfmHiApClientConnectedInd_s {
    /** Common message header. */
    HiMsgHdr_t Header;
    /** Indication message body. */
    WfmHiApClientConnectedIndBody_t Body;
} WfmHiApClientConnectedInd_t;

/**
 * @brief Indication message body for WfmHiApClientRejectedInd_t.
 */
typedef struct __attribute__((__packed__)) WfmHiApClientRejectedIndBody_s {
    /**
     * @brief MAC address of the station.
     */
    uint8_t Mac[WFM_API_MAC_SIZE];
    /**
     * @brief Reason for rejection.
     * @details <B>WFM_REASON_TIMEOUT</B>: A timeout occurred during a station connection attempt.
     *          <BR><B>WFM_REASON_UNKNOWN_STA</B>: The device received data from a non-connected station.
     *          <BR><B>WFM_REASON_AP_FULL</B>: The device was not able to accommodate any more stations.
     *          <BR>See ::WfmReason for enumeration values.
     */
    uint16_t Reason;
} WfmHiApClientRejectedIndBody_t;

/**
 * @brief Indication message used to signal a rejected connection attempt from a station.
 * @details The device will send this indication to signal a station has attempted
 *          connection to the started network and was rejected by the device.
 * @ingroup WFM_STATE_AP
 */
typedef struct __attribute__((__packed__)) WfmHiApClientRejectedInd_s {
    /** Common message header. */
    HiMsgHdr_t Header;
    /** Indication message body. */
    WfmHiApClientRejectedIndBody_t Body;
} WfmHiApClientRejectedInd_t;

/**
 * @brief Request message body for WfmHiDisconnectApClientReq_t.
 */
typedef struct __attribute__((__packed__)) WfmHiDisconnectApClientReqBody_s {
    /**
     * @brief MAC address of the station.
     */
    uint8_t Mac[WFM_API_MAC_SIZE];
} WfmHiDisconnectApClientReqBody_t;

/**
 * @brief Request message for disconnecting a client from the started Wi-Fi network.
 * @details The host can use this request to disconnect a client from the started Wi-Fi network.
 * @ingroup WFM_STATE_AP
 */
typedef struct __attribute__((__packed__)) WfmHiDisconnectApClientReq_s {
    /** Common message header. */
    HiMsgHdr_t Header;
    /** Request message body. */
    WfmHiDisconnectApClientReqBody_t Body;
} WfmHiDisconnectApClientReq_t;

/**
 * @brief Confirmation message body for WfmHiDisconnectApClientCnf_t.
 */
typedef struct __attribute__((__packed__)) WfmHiDisconnectApClientCnfBody_s {
    /**
     * @brief Status of the disconnect request.
     * @details <B>WFM_STATUS_SUCCESS</B>: the disconnect request was completed.
     *          <BR><B>any other value</B>: the disconnect request failed.
     *          <BR>See ::WfmStatus for enumeration values.
     */
    uint32_t Status;
} WfmHiDisconnectApClientCnfBody_t;

/**
 * @brief Confirmation message for WfmHiDisconnectApClientReq_t.
 * @ingroup WFM_STATE_AP
 */
typedef struct __attribute__((__packed__)) WfmHiDisconnectApClientCnf_s {
    /** Common message header. */
    HiMsgHdr_t Header;
    /** Confirmation message body. */
    WfmHiDisconnectApClientCnfBody_t Body;
} WfmHiDisconnectApClientCnf_t;

/**
 * @brief Indication message body for WfmHiApClientDisconnectedInd_t.
 */
typedef struct __attribute__((__packed__)) WfmHiApClientDisconnectedIndBody_s {
    /**
     * @brief MAC address of the station.
     */
    uint8_t Mac[WFM_API_MAC_SIZE];
    /**
     * @brief Reason for disconnection.
     * @details <B>WFM_REASON_LEAVING_BSS</B>: The station was disconnected or it disconnected on its own.
     *          <BR>See ::WfmReason for enumeration values.
     */
    uint16_t Reason;
} WfmHiApClientDisconnectedIndBody_t;

/**
 * @brief Indication message used to signal a disconnected station.
 * @details The device will send this indication to signal a station has left
 *          the started network.
 * @ingroup WFM_STATE_AP
 */
typedef struct __attribute__((__packed__)) WfmHiApClientDisconnectedInd_s {
    /** Common message header. */
    HiMsgHdr_t Header;
    /** Indication message body. */
    WfmHiApClientDisconnectedIndBody_t Body;
} WfmHiApClientDisconnectedInd_t;

/**
 * @brief Request message body for WfmHiSendFrameReq_t.
 */
typedef struct __attribute__((__packed__)) WfmHiSendFrameReqBody_s {
    /**
     * @brief Type of the packet.
     * @details <B>0</B>: Data packet with Ethernet II frame header.
     */
    uint8_t FrameType;
    /**
     * @brief User Priority level.
     * @details <B>0 - 7</B>: 802.1D Priority field value.
     */
    uint8_t Priority;
    /**
     * @brief Packet ID number.
     * @details <B>0 - 65535</B>: Host-assigned unique number for the packet.
     *          <BR>The number is returned in the corresponding confirmation message.
     */
    uint16_t PacketId;
    /**
     * @brief Length of packet data.
     * @details <B>1 - 1604</B>: The amount of bytes.
     */
    uint32_t PacketDataLength;
    /**
     * @brief Data of the packet.
     * @details See @ref WFM_CONCEPT_PACKET for further details.
     */
    /*uint8_t PacketData[];*/
} WfmHiSendFrameReqBody_t;

/**
 * @brief Request message for sending a packet to the network.
 * @details The host can use this request to send a packet to the network.
 * @ingroup WFM_STATE_STA
 * @ingroup WFM_STATE_AP
 * @ingroup WFM_STATE_IBSS
 */
typedef struct __attribute__((__packed__)) WfmHiSendFrameReq_s {
    /** Common message header. */
    HiMsgHdr_t Header;
    /** Request message body. */
    WfmHiSendFrameReqBody_t Body;
} WfmHiSendFrameReq_t;

/**
 * @brief Confirmation message body for WfmHiSendFrameCnf_t.
 */
typedef struct __attribute__((__packed__)) WfmHiSendFrameCnfBody_s {
    /**
     * @brief Status of the send request.
     * @details <B>WFM_STATUS_SUCCESS</B>: the send request was completed.
     *          <BR><B>any other value</B>: the send request failed.
     *          <BR>See ::WfmStatus for enumeration values.
     */
    uint32_t Status;
    /**
     * @brief Packet ID number from the corresponding request.
     */
    uint16_t PacketId;
    /**
     * @brief Reserved.
     */
    uint16_t Reserved;
} WfmHiSendFrameCnfBody_t;

/**
 * @brief Confirmation message for WfmHiSendFrameReq_t.
 * @ingroup WFM_STATE_STA
 * @ingroup WFM_STATE_AP
 * @ingroup WFM_STATE_IBSS
 */
typedef struct __attribute__((__packed__)) WfmHiSendFrameCnf_s {
    /** Common message header. */
    HiMsgHdr_t Header;
    /** Confirmation message body. */
    WfmHiSendFrameCnfBody_t Body;
} WfmHiSendFrameCnf_t;

/**
 * @brief Indication message body for WfmHiReceivedInd_t.
 */
typedef struct __attribute__((__packed__)) WfmHiReceivedIndBody_s {
    /**
     * @brief Type of the packet.
     * @details <B>0</B>: Data packet with Ethernet II frame header.
     */
    uint8_t FrameType;
    /**
     * @brief The amount of padding bytes before the packet data.
     * @details <B>0 - 3</B>: The amount of bytes.
     */
    uint8_t FramePadding;
    /**
     * @brief Length of packet data excluding the padding bytes.
     * @details <B>1 - 2310</B>: The amount of bytes.
     */
    uint16_t FrameLength;
    /**
     * @brief Padding bytes.
     */
    /*uint8_t PaddingBytes[];*/
    /**
     * @brief Data of the packet.
     * @details See @ref WFM_CONCEPT_PACKET for further details.
     */
    /*uint8_t Frame[];*/
} WfmHiReceivedIndBody_t;

/**
 * @brief Indication message used to signal a received packet.
 * @details The device will send this indication to signal a packet
 *          has been received.
 * @ingroup WFM_STATE_STA
 * @ingroup WFM_STATE_AP
 * @ingroup WFM_STATE_IBSS
 */
typedef struct __attribute__((__packed__)) WfmHiReceivedInd_s {
    /** Common message header. */
    HiMsgHdr_t Header;
    /** Indication message body. */
    WfmHiReceivedIndBody_t Body;
} WfmHiReceivedInd_t;

/**
 * @brief Request message body for WfmHiStartScanReq_t.
 */
typedef struct __attribute__((__packed__)) WfmHiStartScanReqBody_s {
    /**
     * @brief Scan mode.
     * @details <B>WFM_SCAN_MODE_PASSIVE</B>: The device will perform a passive scan.
     *          <BR><B>WFM_SCAN_MODE_ACTIVE</B>: The device will perform an active scan.
     *          <BR>See ::WfmScanMode for enumeration values.
     *          <BR>See @ref WFM_CONCEPT_SCAN for further details.
     */
    uint16_t ScanMode;
    /**
     * @brief The amount of specific channels to scan.
     * @details <B>0</B>: The device will scan all channels
     *          <BR><B>1 - 13</B>: The amount of specific channels to scan.
     */
    uint16_t ChannelListCount;
    /**
     * @brief The amount of specific SSIDs to scan.
     * @details <B>0</B>: The device will perform a broadcast scan
     *          <BR><B>1 - 2</B>: The amount of specific SSIDs to scan.
     */
    uint16_t SsidListCount;
    /**
     * @brief Length of vendor-specific Information Element (IE) data in 802.11 Probe Request frames.
     * @details <B>0 - 255</B>: The amount of bytes.
     */
    uint16_t IeDataLength;
    /**
     * @brief List of specific channels to scan.
     * @details <B>1 - 13</B>: The channel number to scan per byte.
     *          <BR>Must contain the same number of channels as specified in ChannelListCount.
     *          <BR>See @ref WFM_CONCEPT_SCAN for further details.
     */
    /*uint8_t ChannelNumber[];*/
    /**
     * @brief List of specific SSIDs to scan.
     * @details Must contain the same number of ::WfmHiSsidDef_t elements as specified in SsidListCount.
     *          <BR>See @ref WFM_CONCEPT_SCAN for further details.
     */
    /*WfmHiSsidDef_t SsidDef[];*/
    /**
     * @brief Vendor-specific IE data added to the 802.11 Probe Request frames.
     * @details The IE data must be in 802.11 Vendor-Specific Element format.
     *          It may contain multiple concatenated IEs, up to the maximum length.
     *          <BR>See @ref WFM_CONCEPT_VENDOR_IE for further details.
     */
    /*uint8_t IeData[];*/
} WfmHiStartScanReqBody_t;

/**
 * @brief Request message for starting a scan to detect near-by access points.
 * @details The host can use this request to start a scan operation to detect near-by access points.
 * @ingroup WFM_STATE_IDLE
 * @ingroup WFM_STATE_STA
 */
typedef struct __attribute__((__packed__)) WfmHiStartScanReq_s {
    /** Common message header. */
    HiMsgHdr_t Header;
    /** Request message body. */
    WfmHiStartScanReqBody_t Body;
} WfmHiStartScanReq_t;

/**
 * @brief Confirmation message body for WfmHiStartScanCnf_t.
 */
typedef struct __attribute__((__packed__)) WfmHiStartScanCnfBody_s {
    /**
     * @brief Status of the scan request.
     * @details <B>WFM_STATUS_SUCCESS</B>: the scan request was accepted. It will be completed by ::WfmHiScanCompleteInd_t.
     *          <BR><B>any other value</B>: the scan request failed.
     *          <BR>See ::WfmStatus for enumeration values.
     */
    uint32_t Status;
} WfmHiStartScanCnfBody_t;

/**
 * @brief Confirmation message for WfmHiStartScanReq_t.
 * @ingroup WFM_STATE_IDLE
 * @ingroup WFM_STATE_STA
 */
typedef struct __attribute__((__packed__)) WfmHiStartScanCnf_s {
    /** Common message header. */
    HiMsgHdr_t Header;
    /** Confirmation message body. */
    WfmHiStartScanCnfBody_t Body;
} WfmHiStartScanCnf_t;

/**
 * @struct WfmHiStopScanReq_t
 * @brief Request message for stopping an ongoing scan.
 * @details The host can use this request to stop an ongoing scan operation.
 * @ingroup WFM_STATE_IDLE
 * @ingroup WFM_STATE_STA
 */
typedef HiMsgHdr_t WfmHiStopScanReq_t;

/**
 * @brief Confirmation message body for WfmHiStopScanCnf_t.
 */
typedef struct __attribute__((__packed__)) WfmHiStopScanCnfBody_s {
    /**
     * @brief Status of the stop request.
     * @details <B>WFM_STATUS_SUCCESS</B>: the stop request was accepted. It will be completed by ::WfmHiScanResultInd_t.
     *          <BR><B>any other value</B>: the stop request failed.
     *          <BR>See ::WfmStatus for enumeration values.
     */
    uint32_t Status;
} WfmHiStopScanCnfBody_t;

/**
 * @brief Confirmation message for WfmHiStopScanReq_t.
 * @ingroup WFM_STATE_IDLE
 * @ingroup WFM_STATE_STA
 */
typedef struct __attribute__((__packed__)) WfmHiStopScanCnf_s {
    /** Common message header. */
    HiMsgHdr_t Header;
    /** Confirmation message body. */
    WfmHiStopScanCnfBody_t Body;
} WfmHiStopScanCnf_t;

/**
 * @brief Indication message body for WfmHiScanResultInd_t.
 */
typedef struct __attribute__((__packed__)) WfmHiScanResultIndBody_s {
    /**
     * @brief Service Set Identifier (SSID) of the network.
     */
    WfmHiSsidDef_t SsidDef;
    /**
     * @brief MAC address of the access point.
     */
    uint8_t Mac[WFM_API_MAC_SIZE];
    /**
     * @brief Channel of the access point.
     * @details <B>1 - 13</B>: Channel number.
     */
    uint16_t Channel;
    /**
     * @brief Reserved.
     */
    uint32_t Reserved;
    /**
     * @brief Received Channel Power Indicator (RCPI) of the access point.
     * @details See @ref WFM_CONCEPT_RCPI for further details.
     */
    uint16_t Rcpi;
    /**
     * @brief Length of access point Information Element (IE) data in bytes.
     */
    uint16_t IeDataLength;
    /**
     * @brief Access point IE data from the 802.11 Beacon or Probe Response frame.
     */
    /*uint8_t IeData[];*/
} WfmHiScanResultIndBody_t;

/**
 * @brief Indication message used to signal an access point has been detected.
 * @details The device will send this indication to signal an access point has
 *          has been detected during the scan operation.
 * @ingroup WFM_STATE_IDLE
 * @ingroup WFM_STATE_STA
 */
typedef struct __attribute__((__packed__)) WfmHiScanResultInd_s {
    /** Common message header. */
    HiMsgHdr_t Header;
    /** Indication message body. */
    WfmHiScanResultIndBody_t Body;
} WfmHiScanResultInd_t;

/**
 * @brief Indication message body for WfmHiScanCompleteInd_t.
 */
typedef struct __attribute__((__packed__)) WfmHiScanCompleteIndBody_s {
    /**
     * @brief Status of the scan request.
     * @details <B>WFM_STATUS_SUCCESS</B>: the scan request was completed.
     *          <BR><B>any other value</B>: the scan request failed.
     *          <BR>See ::WfmStatus for enumeration values.
     */
    uint32_t Status;
} WfmHiScanCompleteIndBody_t;

/**
 * @brief Indication message used to signal a scan was completed.
 * @details The device will send this indication to signal a scan operation
 *          has been completed.
 * @ingroup WFM_STATE_IDLE
 * @ingroup WFM_STATE_STA
 */
typedef struct __attribute__((__packed__)) WfmHiScanCompleteInd_s {
    /** Common message header. */
    HiMsgHdr_t Header;
    /** Indication message body. */
    WfmHiScanCompleteIndBody_t Body;
} WfmHiScanCompleteInd_t;

/**
 * @brief Request message body for WfmHiJoinIbssReq_t.
 */
typedef struct __attribute__((__packed__)) WfmHiJoinIbssReqBody_s {
    /**
     * @brief Service Set Identifier (SSID) of the network.
     */
    WfmHiSsidDef_t SsidDef;
    /**
     * @brief Channel of the network.
     * @details <B>0</B>: The device will connect to a matching network on any channel.
     *          <BR><B>1 - 13</B>: The device will only connect to a matching network on the given channel.
     */
    uint32_t Channel;
    /**
     * @brief Security mode of the network.
     * @details <B>WFM_SECURITY_MODE_OPEN</B>: The device will only use unsecured connections.
     *          <BR><B>WFM_SECURITY_MODE_WEP</B>: The device will only use WEP connections.
     *          <BR>See ::WfmSecurityMode for enumeration values.
     */
    uint16_t SecurityMode;
    /**
     * @brief Length of the network password.
     * @details <B>0 - 26</B>: The amount of bytes.
     */
    uint16_t PasswordLength;
    /**
     * @brief Password of the network.
     * @details <B>64-bit WEP key</B>: 5 bytes in ASCII format or 10 bytes in HEX format.
     *          <BR><B>128-bit WEP key</B>: 13 bytes in ASCII format or 26 bytes in HEX format.
     *          <BR>See @ref WFM_CONCEPT_PASSWORD for further details.
     */
    uint8_t Password[WFM_API_PASSWORD_SIZE];
} WfmHiJoinIbssReqBody_t;

/**
 * @brief Request message for connecting to or starting an IBSS network.
 * @details The host can use this request to connect to an IBSS network. If no existing network
 *          is found, the device will start a new network.
 *          <P>Successful join request moves the interface to @ref WFM_STATE_IBSS.
 * @ingroup WFM_STATE_IDLE
 */
typedef struct __attribute__((__packed__)) WfmHiJoinIbssReq_s {
    /** Common message header. */
    HiMsgHdr_t Header;
    /** Request message body. */
    WfmHiJoinIbssReqBody_t Body;
} WfmHiJoinIbssReq_t;

/**
 * @brief Confirmation message body for WfmHiJoinIbssCnf_t.
 */
typedef struct __attribute__((__packed__)) WfmHiJoinIbssCnfBody_s {
    /**
     * @brief Status of the join request.
     * @details <B>WFM_STATUS_SUCCESS</B>: the join request was accepted. It will be completed by ::WfmHiJoinIbssInd_t.
     *          <BR><B>any other value</B>: the join request failed.
     *          <BR>See ::WfmStatus for enumeration values.
     */
    uint32_t Status;
} WfmHiJoinIbssCnfBody_t;

/**
 * @brief Confirmation message for WfmHiJoinIbssReq_t.
 * @ingroup WFM_STATE_IDLE
 */
typedef struct __attribute__((__packed__)) WfmHiJoinIbssCnf_s {
    /** Common message header. */
    HiMsgHdr_t Header;
    /** Confirmation message body. */
    WfmHiJoinIbssCnfBody_t Body;
} WfmHiJoinIbssCnf_t;

/**
 * @brief Indication message body for WfmHiJoinIbssInd_t.
 */
typedef struct __attribute__((__packed__)) WfmHiJoinIbssIndBody_s {
    /**
     * @brief Status of the join request.
     * @details <B>WFM_STATUS_SUCCESS</B>: the join request was completed successfully.
     *          <BR><B>any other value</B>: the join request failed.
     *          <BR>See ::WfmStatus for enumeration values.
     */
    uint32_t Status;
    /**
     * @brief Basic Service Set Identifier (BSSID) of the network.
     * @details <BR>See @ref WFM_CONCEPT_BSSID for further details.
     */
    uint8_t Bssid[WFM_API_BSSID_SIZE];
    /**
     * @brief Reserved.
     */
    uint16_t Reserved;
} WfmHiJoinIbssIndBody_t;

/**
 * @brief Indication message used to signal the completion of a join request.
 * @details The device will send this indication to signal the join request initiated
 *          with ::WfmHiJoinIbssReq_t has been completed.
 * @ingroup WFM_STATE_IDLE
 */
typedef struct __attribute__((__packed__)) WfmHiJoinIbssInd_s {
    /** Common message header. */
    HiMsgHdr_t Header;
    /** Indication message body. */
    WfmHiJoinIbssIndBody_t Body;
} WfmHiJoinIbssInd_t;

/**
 * @struct WfmHiLeaveIbssReq_t
 * @brief Request message for disconnecting from an IBSS network.
 * @details The host can use this request to disconnect from an IBSS network.
 *          <P>Successful disconnect request moves the interface to @ref WFM_STATE_IDLE.
 * @ingroup WFM_STATE_IBSS
 */
typedef HiMsgHdr_t WfmHiLeaveIbssReq_t; 

/**
 * @brief Confirmation message body for WfmHiLeaveIbssCnf_t.
 */
typedef struct __attribute__((__packed__)) WfmHiLeaveIbssCnfBody_s {
    /**
     * @brief Status of the disconnect request.
     * @details <B>WFM_STATUS_SUCCESS</B>: the disconnect request was accepted. It will be completed by ::WfmHiLeaveIbssInd_t.
     *          <BR><B>any other value</B>: the disconnect request failed.
     *          <BR>See ::WfmStatus for enumeration values.
     */
    uint32_t Status;
} WfmHiLeaveIbssCnfBody_t;

/**
 * @brief Confirmation message for WfmHiLeaveIbssReq_t.
 * @ingroup WFM_STATE_IBSS
 */
typedef struct __attribute__((__packed__)) WfmHiLeaveIbssCnf_s {
    /** Common message header. */
    HiMsgHdr_t Header;
    /** Confirmation message body. */
    WfmHiLeaveIbssCnfBody_t Body;
} WfmHiLeaveIbssCnf_t;

/**
 * @struct WfmHiLeaveIbssInd_t
 * @brief Indication message used to signal the completion of a disconnect operation.
 * @details The device will send this indication to signal the disconnect request initiated
 *          with ::WfmHiLeaveIbssReq_t has been completed. The indication is also sent when
 *          the network has encountered a fatal error.
 * @ingroup WFM_STATE_IBSS
 */
typedef HiMsgHdr_t WfmHiLeaveIbssInd_t; 

/**
 * @brief Request message body for WfmHiAddMulticastAddrReq_t.
 */
typedef struct __attribute__((__packed__)) WfmHiAddMulticastAddrReqBody_s {
    /**
     * @brief MAC address to add.
     * @details <B>broadcast address</B>: The device will empty the whitelist and allow all multicast addresses.
     *          <BR><B>zero address</B>: The device will empty the whitelist and deny all multicast addresses.
     *          <BR><B>unicast address</B>: The device will add the given address to the whitelist.
     *          <BR>See @ref WFM_CONCEPT_BSSID for further details.
     */
    uint8_t Mac[WFM_API_MAC_SIZE];
} WfmHiAddMulticastAddrReqBody_t;

/**
 * @brief Request message for adding a multicast address to the multicast filter whitelist.
 * @details The host can use this request to add a multicast address to the multicast filter
 *          whitelist. When the first address is added the whitelist, the device will discard
 *          all multicast frames whose destination address does not match any of the addresses
 *          on the list. The default state is to allow all multicast addresses.
 *          <BR>See @ref WFM_CONCEPT_FILTERING for further details.
 * @ingroup WFM_STATE_STA
 * @ingroup WFM_STATE_AP
 */
typedef struct __attribute__((__packed__)) WfmHiAddMulticastAddrReq_s {
    /** Common message header. */
    HiMsgHdr_t Header;
    /** Request message body. */
    WfmHiAddMulticastAddrReqBody_t Body;
} WfmHiAddMulticastAddrReq_t;

/**
 * @brief Confirmation message body for WfmHiAddMulticastAddrCnf_t.
 */
typedef struct __attribute__((__packed__)) WfmHiAddMulticastAddrCnfBody_s {
    /**
     * @brief Status of the add request.
     * @details <B>WFM_STATUS_SUCCESS</B>: the add request was completed successfully.
     *          <BR><B>any other value</B>: the add request failed.
     *          <BR>See ::WfmStatus for enumeration values.
     */
    uint32_t Status;
} WfmHiAddMulticastAddrCnfBody_t;

/**
 * @brief Confirmation message for WfmHiAddMulticastAddrReq_t.
 * @ingroup WFM_STATE_STA
 * @ingroup WFM_STATE_AP
 */
typedef struct __attribute__((__packed__)) WfmHiAddMulticastAddrCnf_s {
    /** Common message header. */
    HiMsgHdr_t Header;
    /** Confirmation message body. */
    WfmHiAddMulticastAddrCnfBody_t Body;
} WfmHiAddMulticastAddrCnf_t;

/**
 * @brief Request message body for WfmHiRemoveMulticastAddrReq_t.
 */
typedef struct __attribute__((__packed__)) WfmHiRemoveMulticastAddrReqBody_s {
    /**
     * @brief MAC address to remove.
     * @details <B>unicast address</B>: The device will remove the given address from the whitelist.
     *          <BR>See @ref WFM_CONCEPT_BSSID for further details.
     */
    uint8_t Mac[WFM_API_MAC_SIZE];
} WfmHiRemoveMulticastAddrReqBody_t;

/**
 * @brief Request message for removing a multicast address from the multicast filter whitelist.
 * @details The host can use this request to remove a multicast address from the multicast filter
 *          whitelist.
 * @ingroup WFM_STATE_STA
 * @ingroup WFM_STATE_AP
 */
typedef struct __attribute__((__packed__)) WfmHiRemoveMulticastAddrReq_s {
    /** Common message header. */
    HiMsgHdr_t Header;
    /** Request message body. */
    WfmHiRemoveMulticastAddrReqBody_t Body;
} WfmHiRemoveMulticastAddrReq_t;

/**
 * @brief Confirmation message body for WfmHiRemoveMulticastAddrCnf_t.
 */
typedef struct __attribute__((__packed__)) WfmHiRemoveMulticastAddrCnfBody_s {
    /**
     * @brief Status of the remove request.
     * @details <B>WFM_STATUS_SUCCESS</B>: the remove request was completed successfully.
     *          <BR><B>any other value</B>: the remove request failed.
     *          <BR>See ::WfmStatus for enumeration values.
     */
    uint32_t Status;
} WfmHiRemoveMulticastAddrCnfBody_t;

/**
 * @brief Confirmation message for WfmHiRemoveMulticastAddrReq_t.
 * @ingroup WFM_STATE_STA
 * @ingroup WFM_STATE_AP
 */
typedef struct __attribute__((__packed__)) WfmHiRemoveMulticastAddrCnf_s {
    /** Common message header. */
    HiMsgHdr_t Header;
    /** Confirmation message body. */
    WfmHiRemoveMulticastAddrCnfBody_t Body;
} WfmHiRemoveMulticastAddrCnf_t;

/**
 * @brief Request message body for WfmHiSetMaxApClientCountReq_t.
 */
typedef struct __attribute__((__packed__)) WfmHiSetMaxApClientCountReqBody_s {
    /**
     * @brief Maximum number of connected clients.
     * @details <B>0</B>: The device will set the limit value to the firmware default.
     *          <BR><B>1 - 8</B>: The maximum number of connected clients.
     */
    uint32_t Count;
} WfmHiSetMaxApClientCountReqBody_t;

/**
 * @brief Request message for setting the maximum number of connected clients.
 * @details The host can use this request to limit the number of stations that
 *          can connect the started Wi-Fi network.
 * @ingroup WFM_STATE_AP
 */
typedef struct __attribute__((__packed__)) WfmHiSetMaxApClientCountReq_s {
    /** Common message header. */
    HiMsgHdr_t Header;
    /** Request message body. */
    WfmHiSetMaxApClientCountReqBody_t Body;
} WfmHiSetMaxApClientCountReq_t;

/**
 * @brief Confirmation message body for WfmHiSetMaxApClientCountCnf_t.
 */
typedef struct __attribute__((__packed__)) WfmHiSetMaxApClientCountCnfBody_s {
    /**
     * @brief Status of the set request.
     * @details <B>WFM_STATUS_SUCCESS</B>: the set request was completed successfully.
     *          <BR><B>any other value</B>: the set request failed.
     *          <BR>See ::WfmStatus for enumeration values.
     */
    uint32_t Status;
} WfmHiSetMaxApClientCountCnfBody_t;

/**
 * @brief Confirmation message for WfmHiSetMaxApClientCountReq_t.
 * @ingroup WFM_STATE_AP
 */
typedef struct __attribute__((__packed__)) WfmHiSetMaxApClientCountCnf_s {
    /** Common message header. */
    HiMsgHdr_t Header;
    /** Confirmation message body. */
    WfmHiSetMaxApClientCountCnfBody_t Body;
} WfmHiSetMaxApClientCountCnf_t;

/**
 * @brief Request message body for WfmHiSetMaxApClientInactivityReq_t.
 */
typedef struct __attribute__((__packed__)) WfmHiSetMaxApClientInactivityReqBody_s {
    /**
     * @brief Maximum amount of client idle time.
     * @details <B>0</B>: The device will set the limit value to the firmware default.
     *          <BR><B>1 - 240</B>: The maximum number of seconds.
     */
    uint32_t InactivityTimeout;
} WfmHiSetMaxApClientInactivityReqBody_t;

/**
 * @brief Request message for setting the maximum number of connected clients.
 * @details The host can use this request to limit the number of stations that
 *          can connect the started Wi-Fi network.
 * @ingroup WFM_STATE_AP
 */
typedef struct __attribute__((__packed__)) WfmHiSetMaxApClientInactivityReq_s {
    /** Common message header. */
    HiMsgHdr_t Header;
    /** Request message body. */
    WfmHiSetMaxApClientInactivityReqBody_t Body;
} WfmHiSetMaxApClientInactivityReq_t;

/**
 * @brief Confirmation message body for WfmHiSetMaxApClientCountCnf_t.
 */
typedef struct __attribute__((__packed__)) WfmHiSetMaxApClientInactivityCnfBody_s {
    /**
     * @brief Status of the set request.
     * @details <B>WFM_STATUS_SUCCESS</B>: the set request was completed successfully.
     *          <BR><B>any other value</B>: the set request failed.
     *          <BR>See ::WfmStatus for enumeration values.
     */
    uint32_t Status;
} WfmHiSetMaxApClientInactivityCnfBody_t;

/**
 * @brief Confirmation message for WfmHiSetMaxApClientInactivityReq_t.
 * @ingroup WFM_STATE_AP
 */
typedef struct __attribute__((__packed__)) WfmHiSetMaxApClientInactivityCnf_s {
    /** Common message header. */
    HiMsgHdr_t Header;
    /** Confirmation message body. */
    WfmHiSetMaxApClientInactivityCnfBody_t Body;
} WfmHiSetMaxApClientInactivityCnf_t;

/**
 * @brief Request message body for WfmHiSetRoamParametersReq_t.
 */
typedef struct __attribute__((__packed__)) WfmHiSetRoamParametersReqBody_s {
    /**
     * @brief Received Channel Power Indicator (RCPI) limit for a roaming attempt.
     * @details <B>0</B>: The device will set the limit value to the firmware default.
     *          <BR>1 - 220</B>: RCPI limit for a roaming attempt.
     *          <BR>See @ref WFM_CONCEPT_RCPI for further details.
     */
    uint8_t RcpiThreshold;
    /**
     * @brief Upper RCPI limit that must be attained before triggering roaming again.
     * @details <B>0</B>: The device will set the limit value to the firmware default.
     *          <BR>1 - 220</B>: RCPI limit to re-enable automatic roaming based on RCPI.
     *          This value is used to prevent automatic roaming from triggering again and again
     *          while the signal strength stays close to the RcpiThreshold value.
     *          RcpiHysteresis should be set to a value grater than RcpiThreshold.
     *          <BR>See @ref WFM_CONCEPT_RCPI for further details.
     */
    uint8_t RcpiHysteresis;
    /**
     * @brief Beacon loss limit for a roaming attempt.
     * @details <B>0</B>: The device will set the limit value to the firmware default.
     *          <BR>1 - 255</B>: Beacon loss limit for a roaming attempt.
     */
    uint8_t BeaconLostCount;
    /**
     * @brief The amount of specific channels to scan.
     * @details <B>0</B>: The device will scan all channels.
     *          <BR><B>1 - 13</B>: The amount of specific channels to scan.
     */
    uint8_t ChannelListCount;
    /**
     * @brief List of specific channels to scan.
     * @details <B>1 - 13</B>: The channel number to scan per byte.
     *          <BR>Must contain the same number of channels as specified in ChannelListCount.
     *          <BR>See @ref WFM_CONCEPT_SCAN for further details.
     */
    /*uint8_t ChannelNumber[];*/
} WfmHiSetRoamParametersReqBody_t;

/**
 * @brief Request message for setting the roaming parameters.
 * @details The host can use this request to configure the roaming parameters
 *          of the device.
 *          <BR>See @ref WFM_CONCEPT_ROAM for further details.
 * @ingroup WFM_STATE_IDLE
 * @ingroup WFM_STATE_STA
 */
typedef struct __attribute__((__packed__)) WfmHiSetRoamParametersReq_s {
    /** Common message header. */
    HiMsgHdr_t Header;
    /** Request message body. */
    WfmHiSetRoamParametersReqBody_t Body;
} WfmHiSetRoamParametersReq_t;

/**
 * @brief Confirmation message body for WfmHiSetRoamParametersCnf_t.
 */
typedef struct __attribute__((__packed__)) WfmHiSetRoamParametersCnfBody_s {
    /**
     * @brief Status of the set request.
     * @details <B>WFM_STATUS_SUCCESS</B>: the set request was completed successfully.
     *          <BR><B>any other value</B>: the set request failed.
     *          <BR>See ::WfmStatus for enumeration values.
     */
    uint32_t Status;
} WfmHiSetRoamParametersCnfBody_t;

/**
 * @brief Confirmation message for WfmHiSetRoamParametersReq_t.
 * @ingroup WFM_STATE_IDLE
 * @ingroup WFM_STATE_STA
 */
typedef struct __attribute__((__packed__)) WfmHiSetRoamParametersCnf_s {
    /** Common message header. */
    HiMsgHdr_t Header;
    /** Confirmation message body. */
    WfmHiSetRoamParametersCnfBody_t Body;
} WfmHiSetRoamParametersCnf_t;

/**
 * @brief Request message body for WfmHiSetTxRateParametersReq_t.
 */
typedef struct __attribute__((__packed__)) WfmHiSetTxRateParametersReqBody_s {
    /**
     * @brief Reserved, set to zero.
     */
    uint32_t Reserved;
    /**
     * @brief TX rate set parameters.
     */
    WfmHiRateSetBitmask_t RateSetBitmask;
} WfmHiSetTxRateParametersReqBody_t;

/**
 * @brief Request message for setting the TX rate set parameters.
 * @details The host can use this request to configure the TX rate
 *          set parameters of the device. The rate set determines
 *          what data rates will be used by the device to transmit data
 *          frames.
 * @ingroup WFM_STATE_IDLE
 * @ingroup WFM_STATE_STA
 */
typedef struct __attribute__((__packed__)) WfmHiSetTxRateParametersReq_s {
    /** Common message header. */
    HiMsgHdr_t Header;
    /** Request message body. */
    WfmHiSetTxRateParametersReqBody_t Body;
} WfmHiSetTxRateParametersReq_t;

/**
 * @brief Confirmation message body for WfmHiSetTxRateParametersCnf_t.
 */
typedef struct __attribute__((__packed__)) WfmHiSetTxRateParametersCnfBody_s {
    /**
     * @brief Status of the set request.
     * @details <B>WFM_STATUS_SUCCESS</B>: the set request was completed successfully.
     *          <BR><B>any other value</B>: the set request failed.
     *          <BR>See ::WfmStatus for enumeration values.
     */
    uint32_t Status;
} WfmHiSetTxRateParametersCnfBody_t;

/**
 * @brief Confirmation message for WfmHiSetTxRateParametersReq_t.
 * @ingroup WFM_STATE_IDLE
 * @ingroup WFM_STATE_STA
 */
typedef struct __attribute__((__packed__)) WfmHiSetTxRateParametersCnf_s {
    /** Common message header. */
    HiMsgHdr_t Header;
    /** Confirmation message body. */
    WfmHiSetTxRateParametersCnfBody_t Body;
} WfmHiSetTxRateParametersCnf_t;

/**
 * @brief Request message body for WfmHiSetArpIpAddressReq_t.
 */
typedef struct __attribute__((__packed__)) WfmHiSetArpIpAddressReqBody_s {
    /**
     * @brief List of offloaded ARP IP addresses.
     * @details The device will automatically reply to an ARP request that matches
     *          one the addresses on the list. Note that addresses not in use must
     *          be set to zero. Offloading is disabled by setting all addresses to
     *          zero.
     *          <BR>See @ref WFM_CONCEPT_OFFLOADING for further details.
     */
    uint32_t ArpIpAddr[WFM_API_ARP_IP_ADDR_SIZE];
} WfmHiSetArpIpAddressReqBody_t;

/**
 * @brief Request message for setting the Address Resolution Protocol (ARP) offloading state.
 * @details The host can use this request to offload handling of ARP requests to the device.
 *          When offloading is enabled, the device will automatically respond to ARP requests
 *          with an ARP reply.
 *          <BR>See @ref WFM_CONCEPT_OFFLOADING for further details.
 * @ingroup WFM_STATE_STA
 */
typedef struct __attribute__((__packed__)) WfmHiSetArpIpAddressReq_s {
    /** Common message header. */
    HiMsgHdr_t Header;
    /** Request message body. */
    WfmHiSetArpIpAddressReqBody_t Body;
} WfmHiSetArpIpAddressReq_t;

/**
 * @brief Confirmation message body for WfmHiSetArpIpAddressCnf_t.
 */
typedef struct __attribute__((__packed__)) WfmHiSetArpIpAddressCnfBody_s {
    /**
     * @brief Status of the set request.
     * @details <B>WFM_STATUS_SUCCESS</B>: the set request was completed successfully.
     *          <BR><B>any other value</B>: the set request failed.
     *          <BR>See ::WfmStatus for enumeration values.
     */
    uint32_t Status;
} WfmHiSetArpIpAddressCnfBody_t;

/**
 * @brief Confirmation message for WfmHiSetArpIpAddressReq_t.
 * @ingroup WFM_STATE_STA
 */
typedef struct __attribute__((__packed__)) WfmHiSetArpIpAddressCnf_s {
    /** Common message header. */
    HiMsgHdr_t Header;
    /** Confirmation message body. */
    WfmHiSetArpIpAddressCnfBody_t Body;
} WfmHiSetArpIpAddressCnf_t;

/**
 * @brief Request message body for WfmHiSetNsIpAddressReq_t.
 */
typedef struct __attribute__((__packed__)) WfmHiSetNsIpAddressReqBody_s {
   /**
    * @brief List of offloaded NS IP addresses.
    * @details The device will automatically reply to a NS that matches one the
    *          addresses on the list. Note that addresses not in use must be set
    *          to zero. Offloading is disabled by setting all addresses to zero.
    *          <BR>See @ref WFM_CONCEPT_OFFLOADING for further details.
    */
    WfmHiNsIpAddr_t NsIpAddr[WFM_API_NS_IP_ADDR_SIZE];
} WfmHiSetNsIpAddressReqBody_t;

/**
 * @brief Request message for setting the Neighbor Discovery Protocol (NDP) offloading state.
 * @details The host can use this request to offload handling of IPv6 Neighbor Solicitations
 *          to the device. When offloading is enabled, the device will automatically respond
 *          to a solicitation with a Neighbor Advertisement.
 *          <BR>See @ref WFM_CONCEPT_OFFLOADING for further details.
 * @ingroup WFM_STATE_STA
 */
typedef struct __attribute__((__packed__)) WfmHiSetNsIpAddressReq_s {
    /** Common message header. */
    HiMsgHdr_t Header;
    /** Request message body. */
    WfmHiSetNsIpAddressReqBody_t Body;
} WfmHiSetNsIpAddressReq_t;

/**
 * @brief Confirmation message body for WfmHiSetNsIpAddressCnf_t.
 */
typedef struct __attribute__((__packed__)) WfmHiSetNsIpAddressCnfBody_s {
    /**
     * @brief Status of the set request.
     * @details <B>WFM_STATUS_SUCCESS</B>: the set request was completed successfully.
     *          <BR><B>any other value</B>: the set request failed.
     *          <BR>See ::WfmStatus for enumeration values.
     */
    uint32_t Status;
} WfmHiSetNsIpAddressCnfBody_t;

/**
 * @brief Confirmation message for WfmHiSetNsIpAddressReq_t.
 * @ingroup WFM_STATE_STA
 */
typedef struct __attribute__((__packed__)) WfmHiSetNsIpAddressCnf_s {
    /** Common message header. */
    HiMsgHdr_t Header;
    /** Confirmation message body. */
    WfmHiSetNsIpAddressCnfBody_t Body;
} WfmHiSetNsIpAddressCnf_t;

/**
 * @brief Request message body for WfmHiSetBroadcastFilterReq_t.
 */
typedef struct __attribute__((__packed__)) WfmHiSetBroadcastFilterReqBody_s {
    /**
     * @brief Boolean option for broadcast filtering.
     * @details <B>0</B>: The device will forward all received broadcast frames to the host.
     *          <BR><B>1</B>: The device will only forward ARP and DHCP frames to the host,
     *                        other broadcast frames are discarded.
     *          <BR>See @ref WFM_CONCEPT_FILTERING for further details.
     */
    uint32_t Filter;
} WfmHiSetBroadcastFilterReqBody_t;

/**
 * @brief Request message for setting broadcast filter state.
 * @details The host can use this request to configure the state of the broadcast filter.
 *          When enabled, the device will only forward certain broadcast frames to the
 *          host and automatically discard the rest. The default state is to allow all
 *          broadcast traffic.
 *          <BR>See @ref WFM_CONCEPT_FILTERING for further details.
 * @ingroup WFM_STATE_STA
 */
typedef struct __attribute__((__packed__)) WfmHiSetBroadcastFilterReq_s {
    /** Common message header. */
    HiMsgHdr_t Header;
    /** Request message body. */
    WfmHiSetBroadcastFilterReqBody_t Body;
} WfmHiSetBroadcastFilterReq_t;

/**
 * @brief Confirmation message body for WfmHiSetBroadcastFilterCnf_t.
 */
typedef struct __attribute__((__packed__)) WfmHiSetBroadcastFilterCnfBody_s {
    /**
     * @brief Status of the set request.
     * @details <B>WFM_STATUS_SUCCESS</B>: the set request was completed successfully.
     *          <BR><B>any other value</B>: the set request failed.
     *          <BR>See ::WfmStatus for enumeration values.
     */
    uint32_t Status;
} WfmHiSetBroadcastFilterCnfBody_t;

/**
 * @brief Confirmation message for WfmHiSetBroadcastFilterReq_t.
 * @ingroup WFM_STATE_STA
 */
typedef struct __attribute__((__packed__)) WfmHiSetBroadcastFilterCnf_s {
    /** Common message header. */
    HiMsgHdr_t Header;
    /** Confirmation message body. */
    WfmHiSetBroadcastFilterCnfBody_t Body;
} WfmHiSetBroadcastFilterCnf_t;

/**
 * @brief Request message body for WfmHiSetScanParametersReq_t.
 */
typedef struct __attribute__((__packed__)) WfmHiSetScanParametersReqBody_s {
    /**
     * @brief Set to 0 for FW default, 1 - 550 TUs
     */
    uint16_t ActiveChannelTime;
    /**
     * @brief Set to 0 for FW default, 1 - 550 TUs
     */
    uint16_t PassiveChannelTime;
    /**
     * @brief Set to 0 for FW default, 1 - 2
     */
    uint16_t NumOfProbeRequests;
    /**
     * @brief Reserved, set to zero.
     */
    uint16_t Reserved;
} WfmHiSetScanParametersReqBody_t;

/**
 * @brief Request message for setting scan parameters.
 * @details The host can use this request to configure scans.
 * @ingroup WFM_STATE_STA
 */
typedef struct __attribute__((__packed__)) WfmHiSetScanParametersReq_s {
    HiMsgHdr_t Header;
    WfmHiSetScanParametersReqBody_t Body;
} WfmHiSetScanParametersReq_t;

/**
 * @brief Confirmation message body for WfmHiSetScanParametersCnf_t.
 */
typedef struct __attribute__((__packed__)) WfmHiSetScanParametersCnfBody_s {
    /**
     * @brief Status of the set request.
     * @details <B>WFM_STATUS_SUCCESS</B>: the set request was completed successfully.
     *          <BR><B>any other value</B>: the set request failed.
     *          <BR>See ::WfmStatus for enumeration values.
     */
    uint32_t Status;
} WfmHiSetScanParametersCnfBody_t;

/**
 * @brief Confirmation message for WfmHiSetScanParametersReq_t.
 * @ingroup WFM_STATE_STA
 */
typedef struct __attribute__((__packed__)) WfmHiSetScanParametersCnf_s {
    HiMsgHdr_t Header;
    WfmHiSetScanParametersCnfBody_t Body;
} WfmHiSetScanParametersCnf_t;


/**************************************************/

/**
 * @}
 */

/**
 * @}
 */

#endif  /* _WFM_CMD_API_H_ */
