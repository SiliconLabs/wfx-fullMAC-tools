/***************************************************************************//**
 * @file general_api.h
 * @brief This file contains the type definitions for WLAN general API
 * 	structures, enums, and other types.
 *
 * @copyright Copyright 2015 Silicon Laboratories, Inc. http://www.silabs.com
 ******************************************************************************/

#ifndef _GENERAL_API_H_
#define _GENERAL_API_H_

#include "basic_types.h"

//< API Internal Version encoding
#define HI_API_VERSION_MINOR							0x00
#define HI_API_VERSION_MAJOR							0x01

#define API_VARIABLE_SIZE_ARRAY_DUMMY_SIZE  1
#define API_MAC_ADDR_SIZE                   6

#define GENERAL_INTERFACE_ID                2

#define HI_MSG_ID_MASK	      				0x00FF
#define HI_MSG_TYPE_MASK					0x80
#define HI_MSG_SEQ_RANGE	    			0x0007 //range of field HostCount in U16msginfo_t

/* Message bases */
#define HI_REQ_BASE		 					0x00
#define HI_CNF_BASE		 					0x00
#define HI_IND_BASE		 					HI_MSG_TYPE_MASK

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
 * A message is composed of a header and a body (see ::HiGenericMsg_t).
 * In the rest of the API description, only the body is detailed for each message.
 *
 * @{
 */

/**
 * \typedef U16msginfo_t
 * @brief bitfield view of the Message header information element
 *
 */
typedef struct __attribute__((__packed__)) U16msginfo_s {
	uint8_t Id :7; ///< The message ID without the Type. First 7 bits of ::MsginfoBytes_t::MsgId
	uint8_t MsgType :1; ///< The message type 1 for indication, 0 for request or confirmation. MSB of ::MsginfoBytes_t::MsgId .
	uint8_t Reserved :1;
	uint8_t IntId :2; ///< The Wireless interface ID the message is intended to or coming from.
	uint8_t HostCount :3; ///< 3bits that can be used by the host has it wants in a request (typically a counter). They are the same in the associated confirmation.
	uint8_t SecLink :2; ///< must be set to '00' when filling the message : it means unciphered. It is then changed by the Secure Link cipher process.
} U16msginfo_t;

/**
 * @brief 2 bytes view of the Message header information element
 */
typedef struct MsginfoBytes_s {
	uint8_t MsgId; ///< Subset of ::U16msginfo_t containing the message Id indexed by ::HiGeneralCommandsIds_t, ::HiWsmCommandsIds_t or ::HI_WFM_COMMANDS_IDS.
	uint8_t MsgInfo;///< Subset of ::U16msginfo_t .
} MsginfoBytes_t;

/**
 * @brief Different views of the Message header (::HiMsgHdr_t) information element
 *
 */
typedef union  MsginfoUnion_u {
    uint16_t U16MsgInfo; ///< View of ::U16msginfo_t as a 16 bits word.
    MsginfoBytes_t t; ///< View of ::U16msginfo_t as 2 bytes.
    U16msginfo_t b; ///< Message information bitfield view.
} MsginfoUnion_t;

/**
 * @brief General Message header structure
 *
 */
typedef struct __attribute__((__packed__)) HiMsgHdr_s {
        uint16_t    MsgLen; ///< Message length in bytes including this uint16_t. Maximum value is 8188 but maximum Request size is FW dependent and reported in the ::HiStartupIndBody_t::SizeInpChBuf
        MsginfoUnion_t s; ///< ::U16msginfo_t
} HiMsgHdr_t ;

/**
 * @brief General message structure for all requests, confirmations and indications
 *
 */
typedef struct __attribute__((__packed__)) HiGenericMsg_s {
        HiMsgHdr_t Header; ///<4 bytes header
        uint8_t Body[API_VARIABLE_SIZE_ARRAY_DUMMY_SIZE]; ///<variable size payload of the message
} HiGenericMsg_t;


/**
 * @brief Generic confirmation message with the body reduced to the Status field.
 *
 * This structure is not related to a specific confirmation ID. @n
 * It is a global simplified structure that can be used to easily access the header and status fields.
 *
 * All confirmation bodies start with a Status word and in a lot of them it is followed by other data (not present in this structure).
 */
typedef struct __attribute__((__packed__)) HiGenericCnf_s {
        HiMsgHdr_t  Header; ///<4 bytes header
        uint32_t    Status; ///<See enum ::HiStatus and ::WsmStatus or ::WfmStatus
} HiGenericCnf_t;

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
 * \arg general \b requests are ::HiGeneralRequestsIds @n
 * \arg general \b indications are ::HiGeneralIndicationsIds@n
 * @n
 * @{
 */


/**
 * @brief General request message IDs
 *
 * API general request message IDs available in both split and full MAC.
 * These are messages from the host towards the WLAN.
 */
typedef enum HiGeneralRequestsIds_e {
 HI_CONFIGURATION_REQ_ID                         =0x09, ///< \b CONFIGURATION request Id use body ::HiConfigurationReqBody_t and returns ::HiConfigurationCnfBody_t
 HI_CONTROL_GPIO_REQ_ID                          =0x26, ///< \b CONTROL_GPIO request Id use body ::HiControlGpioReqBody_t and returns ::HiControlGpioCnfBody_t
 HI_SET_SL_MAC_KEY_REQ_ID                        =0x27, ///< \b SET_SL_MAC_KEY request Id use body ::HiSetSlMacKeyReqBody_t and returns ::HiSetSlMacKeyCnfBody_t
 HI_SL_EXCHANGE_PUB_KEYS_REQ_ID                  =0x28, ///< \b SL_EXCHANGE_PUB_KEYS request Id use body ::HiSlExchangePubKeysReqBody_t and returns ::HiSlExchangePubKeysCnfBody_t
 HI_SL_CONFIGURE_REQ_ID                          =0x29,  ///< \b SL_CONFIGURE request Id use body ::HiSlConfigureReqBody_t and returns ::HiSlExchangePubKeysCnfBody_t
 HI_PREVENT_ROLLBACK_REQ_ID                      =0x2a, ///< \b PREVENT_ROLLBACK request Id use body ::HiPreventRollbackReqBody_t and returns ::HiPreventRollbackCnfBody_t
 HI_SHUT_DOWN_REQ_ID                             =0x32, ///< \b SHUT_DOWN request Id use body ::HiShutDownReq_t and never returns
} HiGeneralRequestsIds;


/**
 * @brief General confirmation message IDs
 *
 * API general confirmation message IDs returned by requests described in ::HiGeneralRequestsIds.
 * These are messages from the WLAN towards the host.
 */
typedef enum HiGeneralConfirmationsIds_e {
 HI_CONFIGURATION_CNF_ID                         =0x09, ///< \b CONFIGURATION confirmation Id returns body ::HiConfigurationCnfBody_t
 HI_CONTROL_GPIO_CNF_ID                          =0x26, ///< \b CONTROL_GPIO confirmation Id returns body ::HiControlGpioCnfBody_t
 HI_SET_SL_MAC_KEY_CNF_ID                        =0x27, ///< \b SET_SL_MAC_KEY confirmation Id returns body ::HiSetSlMacKeyCnfBody_t
 HI_SL_EXCHANGE_PUB_KEYS_CNF_ID                  =0x28, ///< \b SL_EXCHANGE_PUB_KEYS confirmation Id returns body ::HiSlExchangePubKeysCnfBody_t
 HI_SL_CONFIGURE_CNF_ID                          =0x29, ///< \b SL_CONFIGURE confirmation Id returns body ::HiSlConfigureCnfBody_t
 HI_PREVENT_ROLLBACK_CNF_ID                      =0xe7, ///< \b PREVENT_ROLLBACK confirmation Id use body ::HiPreventRollbackCnfBody_t
} HiGeneralConfirmationsIds;


/**
 * @brief General indications message IDs
 *
 * API general indication message IDs available in both split and full MAC.
 * These are messages from the WLAN towards the host.
 */
typedef enum HiGeneralIndicationsIds_e {
 HI_EXCEPTION_IND_ID                             =0xe0, ///< \b EXCEPTION indication Id content is ::HiExceptionIndBody_t
 HI_STARTUP_IND_ID                               =0xe1, ///< \b STARTUP indication Id content is ::HiStartupIndBody_t
 HI_GENERIC_IND_ID                               =0xe3, ///< \b GENERIC indication Id content is ::HiGenericIndBody_t
 HI_ERROR_IND_ID                                 =0xe4 ///< \b ERROR indication Id content is ::HiErrorIndBody_t
} HiGeneralIndicationsIds;


/**
 * @brief General command message IDs
 *
 * All general API message IDs.
 */
typedef union HiGeneralCommandsIds_u {
	HiGeneralRequestsIds request; ///< Request from the host to the wlan device
	HiGeneralConfirmationsIds confirmation; ///< Confirmation of a request from the wlan device to the host
	HiGeneralIndicationsIds indication; ///< Indication from the wlan device to the host
} HiGeneralCommandsIds_t;



/**************************************************/

/**
 * @brief General confirmation possible values for returned 'Status' field
 *
 * All general confirmation messages have a field 'Status' just after the message header.@n
 * A value of zero indicates the request is completed successfully.
 *
 */
typedef enum HiStatus_e {
		HI_STATUS_SUCCESS                         = 0x0,         ///<The firmware has successfully completed the request.
		HI_STATUS_FAILURE                         = 0x1,         ///<This is a generic failure code : other error codes do not apply.
		HI_INVALID_PARAMETER                      = 0x2,         ///<The request contains one or more invalid parameters.
		HI_STATUS_GPIO_WARNING                    = 0x3,         ///<Warning : the GPIO cmd is successful but the read value is not as expected (likely a drive conflict on the line)
		HI_ERROR_UNSUPPORTED_MSG_ID               = 0x4, 		 ///<Unkown request ID or wrong interface ID used
		/* Specific SecureLink statuses */
        SL_MAC_KEY_STATUS_SUCCESS                     	= 0x5A,      ///<Key has been correctly written
        SL_MAC_KEY_STATUS_FAILED_KEY_ALREADY_BURNED   	= 0x6B,      ///<Key already exists in OTP
        SL_MAC_KEY_STATUS_FAILED_RAM_MODE_NOT_ALLOWED 	= 0x7C,      ///<RAM mode is not allowed
        SL_MAC_KEY_STATUS_FAILED_UNKNOWN_MODE         	= 0x8D,      ///<Unknown mode (should be RAM or OTP)
        SL_PUB_KEY_EXCHANGE_STATUS_SUCCESS        		= 0x9E,      ///<Host Public Key authenticated
        SL_PUB_KEY_EXCHANGE_STATUS_FAILED         		= 0xAF,      ///<Host Public Key authentication failed
		/* Specific Prevent Rollback statuses */
        PREVENT_ROLLBACK_CNF_SUCCESS           			= 0x1234,    ///<OTP rollback value has been successfully updated
        PREVENT_ROLLBACK_CNF_WRONG_MAGIC_WORD 	 		= 0x1256     ///<Wrong magic word detected
} HiStatus;

/**************************************************/



/**
 * @addtogroup General_Configuration
 * @brief General configuration commands
 *
 *
 * @{
 */
typedef enum HiFwType_e {
        HI_FW_TYPE_ETF                             = 0x0,         /*Test Firmware*/
        HI_FW_TYPE_WFM                             = 0x1,         /*WLAN Full MAC (WFM)*/
        HI_FW_TYPE_WSM                             = 0x2          /*WLAN Split MAC (WSM)*/
} HiFwType;




/**
 * @brief Capabilities offered by the WLAN used in command ::HiStartupIndBody_t
 */
typedef struct __attribute__((__packed__)) HiCapabilities_s {
        uint8_t    LinkMode : 2;                     ///<Bit 0-1 : reg OTPCTRL_FB_STATUS_fb_secure_link_mode @todo add details
        uint8_t    Reserved : 6;                     ///<Bit 2-7 : Reserved
        uint8_t    Reserved2;                        ///<Bit 8-15 : Reserved
        uint8_t    Reserved3;                        ///<Bit 16-23 : Reserved
        uint8_t    Reserved4;                        ///<Bit 24-31 : Reserved
} HiCapabilities_t;

/**
 * @brief REGUL_SEL_MODE OTP field reported in command ::HiStartupIndBody_t
 */
typedef struct __attribute__((__packed__)) HiOtpRegulSelModeInfo_s {
        uint8_t    RegionSelMode:4;                  ///<Bit 0-3 : indicates default FW behavior regarding DFS region setting
        uint8_t    Reserved:4;                       ///<Bit 4-7 : Reserved
} HiOtpRegulSelModeInfo_t;

/**
 * @brief OTP_PHY_INFO OTP field reported in command ::HiStartupIndBody_t
 */
typedef struct __attribute__((__packed__)) HiOtpPhyInfo_s {
        uint8_t    Phy1Region:3;                     ///<Bit 0-2 : DFS region corresponding to backoff vs. channel group table indexed 0
        uint8_t    Phy0Region:3;                     ///<Bit 3-5 : DFS region corresponding to backoff vs. channel group table indexed 1
        uint8_t    OtpPhyVer:2;                      ///<Bit 6-7 : Revision of OTP info
} HiOtpPhyInfo_t;

#define API_OPN_SIZE                                    14
#define API_UID_SIZE                                    8
#define API_DISABLED_CHANNEL_LIST_SIZE                  2
#define API_FIRMWARE_LABEL_SIZE                         128
/**
 * @brief Startup Indication message.
 * This is the first message sent to the host to confirm boot success.
 * It gives detailed information on the HW and FW versions and capabilities
 */
typedef struct __attribute__((__packed__)) HiStartupIndBody_s {
        uint32_t   Status;                           ///<Initialization status. A value of zero indicates the boot is completed successfully  (see enum ::HiStatus)
        uint16_t   HardwareId;                       ///<=RO misc_read_reg7 register value
        uint8_t    OPN[API_OPN_SIZE];                ///<=OTP part_OPN
        uint8_t    UID[API_UID_SIZE];                ///<=OTP UID
        uint16_t   NumInpChBufs;                     ///<Number of buffers available for request messages.
        uint16_t   SizeInpChBuf;                     ///<Tx Buffer size in bytes=request message max size.
        uint8_t    NumLinksAP;                       ///<number of STA that are supported in AP mode
        uint8_t    NumInterfaces;                    ///<number of interfaces (WIFI link : STA or AP) that can be created by the user
        uint8_t    MacAddr0[API_MAC_ADDR_SIZE];      ///<1st MAC address derived from OTP
        uint8_t    MacAddr1[API_MAC_ADDR_SIZE];      ///<2d MAC address derived from OTP
        uint8_t    ApiVersionMinor;
        uint8_t    ApiVersionMajor;
        HiCapabilities_t Capabilities;
        uint8_t    FirmwareBuild;
        uint8_t    FirmwareMinor;
        uint8_t    FirmwareMajor;
        uint8_t    FirmwareType;                     ///<See enum ::HiFwType
        uint8_t    DisabledChannelList[API_DISABLED_CHANNEL_LIST_SIZE];   ///<=OTP Disabled channel list info
        HiOtpRegulSelModeInfo_t RegulSelModeInfo;
        HiOtpPhyInfo_t OtpPhyInfo;
        uint32_t   SupportedRateMask;                ///<A bit mask that indicates which rates are supported by the Physical layer. See enum ::WsmTransmitRate.
        uint8_t    FirmwareLabel[API_FIRMWARE_LABEL_SIZE];   ///<Null terminated text string describing the loaded FW.
} HiStartupIndBody_t;

typedef struct __attribute__((__packed__)) HiStartupInd_s {
        HiMsgHdr_t Header;
        HiStartupIndBody_t Body;
} HiStartupInd_t;



/**
 * @brief Configure the device.
 * It sends a PDS compressed file that configures the device regarding board dependent parameters.
 * The PDS compressed file must fit in a command buffer and have less than 256 elements.
 *
 * @todo Need to create a specific doc to explain PDS*/
typedef struct __attribute__((__packed__)) HiConfigurationReqBody_s {
        uint16_t   Length;                           ///<PdsData length in bytes
        uint8_t    PdsData[API_VARIABLE_SIZE_ARRAY_DUMMY_SIZE];    ///<variable size PDS data byte array
} HiConfigurationReqBody_t;

typedef struct __attribute__((__packed__)) HiConfigurationReq_s {
        HiMsgHdr_t Header;
        HiConfigurationReqBody_t Body;
} HiConfigurationReq_t;

/**
 * @brief Confirmation message of CONFIGURATION command ::HiConfigurationReqBody_t */
typedef struct __attribute__((__packed__)) HiConfigurationCnfBody_s {
        uint32_t   Status;                           ///<Configuration status. A value of zero indicates the boot is completed successfully (see enum ::HiStatus)
} HiConfigurationCnfBody_t;

typedef struct __attribute__((__packed__)) HiConfigurationCnf_s {
        HiMsgHdr_t Header;
        HiConfigurationCnfBody_t Body;
} HiConfigurationCnf_t;


/**
 * @brief Configure GPIO mode. Used in ::HiControlGpioReqBody_t
 * */
typedef enum HiGpioMode_e {
        HI_GPIO_MODE_D0                            = 0x0,         ///< Configure the GPIO to drive 0
        HI_GPIO_MODE_D1                            = 0x1,         ///< Configure the GPIO to drive 1
        HI_GPIO_MODE_OD0                           = 0x2,         ///< Configure the GPIO to open drain with pull_down to 0
        HI_GPIO_MODE_OD1                           = 0x3,         ///< Configure the GPIO to open drain with pull_up to 1
        HI_GPIO_MODE_TRISTATE                      = 0x4,         ///< Configure the GPIO to tristate
        HI_GPIO_MODE_TOGGLE                        = 0x5,         ///< Toggle the GPIO output value : switches between D0 and D1 or between OD0 and OD1
        HI_GPIO_MODE_READ                          = 0x6          ///< Read the level at the GPIO pin
} HiGpioMode;

/**
 * @brief Send a request to read or write a gpio identified by its label (that is defined in the PDS)
 *
 * After a write it also read back the value to check there is no drive conflict */
typedef struct __attribute__((__packed__)) HiControlGpioReqBody_s {
	uint8_t GpioLabel;      ///<Identify the gpio by its label (defined in the PDS)
	uint8_t GpioMode;       ///<define how to set or read the gpio (see enum ::HiGpioMode)
} HiControlGpioReqBody_t;

typedef struct __attribute__((__packed__)) HiControlGpioReq_s {
        HiMsgHdr_t Header;             
        HiControlGpioReqBody_t Body;               
} HiControlGpioReq_t;

/**
 * @brief detailed error cause returned by CONTROL_GPIO confirmation message ::HiControlGpioCnfBody_t
 * */
typedef enum HiGpioError_e {
        HI_GPIO_ERROR_0                            = 0x0,         ///< Undefined GPIO_ID
        HI_GPIO_ERROR_1                            = 0x1,         ///< GPIO_ID not configured in gpio mode (gpio_enabled =0)
        HI_GPIO_ERROR_2                            = 0x2          ///< Toggle not possible while in tristate
} HiGpioError;

/**
 * @brief Confirmation from request to read and write a gpio */
typedef struct __attribute__((__packed__)) HiControlGpioCnfBody_s {
	uint32_t Status;        ///<enum ::HiStatus : a value of zero indicates the request is completed successfully.
	uint32_t Value;         ///<the error detail (see enum ::HiGpioError) when ::HiControlGpioCnfBody_t::Status reports an error else the gpio read value.
} HiControlGpioCnfBody_t;

typedef struct __attribute__((__packed__)) HiControlGpioCnf_s {
        HiMsgHdr_t Header;             
        HiControlGpioCnfBody_t Body;               
} HiControlGpioCnf_t;


/**
 * @brief SHUT_DOWN command.
 * A hardware reset and complete reboot is required to resume from that state.
 * There is no confirmation to this command.
 * It is effective when WUP register bit and WUP pin (when used) are both to 0.*/
typedef HiMsgHdr_t HiShutDownReq_t; 


/**
 * @brief specifies the type of data reported by the indication message ::HiGenericIndBody_t
 *
 * */
typedef enum HiGenericIndicationType_e {
        HI_GENERIC_INDICATION_TYPE_RAW               = 0x0,         ///<Byte stream type, currently not used
        HI_GENERIC_INDICATION_TYPE_STRING            = 0x1,         ///<NULL terminating String
        HI_GENERIC_INDICATION_TYPE_RX_STATS          = 0x2          ///<Rx statistics structure
} HiGenericIndicationType;

#define API_NB_RX_BY_RATE_SIZE                          22
#define API_PER_SIZE                                    22
#define API_SNR_SIZE                                    22
#define API_RSSI_SIZE                                   22
#define API_CFO_SIZE                                    22
/**
 * @brief RX stats from the GENERIC indication message ::HiGenericIndBody_t
 */
typedef struct __attribute__((__packed__)) HiRxStats_s {
        uint32_t   NbRxFrame;                        ///<Total number of frame received
        uint32_t   NbCrcFrame;                       ///<Number of frame received with bad CRC
        uint32_t   PerTotal;                         ///<PER on the total number of frame
        uint32_t   Throughput;                       ///<Throughput calculated on correct frames received
        uint32_t   NbRxByRate[API_NB_RX_BY_RATE_SIZE];   ///<Number of frame received by rate
        uint16_t   Per[API_PER_SIZE];                ///<PER*10000 by frame rate
        int16_t    Snr[API_SNR_SIZE];                ///<SNR in Db*100 by frame rate
        int16_t    Rssi[API_RSSI_SIZE];              ///<RSSI in Dbm*100 by frame rate
        int16_t    Cfo[API_CFO_SIZE];                ///<CFO in kHz by frame rate
        uint32_t   Date;                             ///<This message transmission date in firmware timebase (microsecond)
        uint32_t   PwrClkFreq;                       ///<Frequency of the low power clock in Hz
        uint8_t    IsExtPwrClk;                      ///<Indicate if the low power clock is external
} HiRxStats_t;


#define MAX_GENERIC_INDICATION_DATA_SIZE              376 // in bytes
typedef union HiIndicationData_u {
        HiRxStats_t                                   RxStats;
        uint8_t                                       RawData[MAX_GENERIC_INDICATION_DATA_SIZE];
} HiIndicationData_t;

/**
 * @brief the Generic indication message.
 *
 * It reports different type of information that can be printed by the driver.
 * */
typedef struct __attribute__((__packed__)) HiGenericIndBody_s {
        uint32_t IndicationType;                   ///<Identify the indication data (see enum type ::HiGenericIndicationType)
        HiIndicationData_t IndicationData;         ///<Indication data.
} HiGenericIndBody_t;

typedef struct __attribute__((__packed__)) HiGenericInd_s {
        HiMsgHdr_t Header;             
        HiGenericIndBody_t Body;               
} HiGenericInd_t;

#define HI_EXCEPTION_DATA_SIZE            124
/**
 * @brief Exception indication message
 *
 * It reports unexpected errors. A reboot is needed after this message.
 * */
typedef struct __attribute__((__packed__)) HiExceptionIndBody_s {
        uint8_t    Data[HI_EXCEPTION_DATA_SIZE];     ///<Raw data array
} HiExceptionIndBody_t;


typedef struct __attribute__((__packed__)) HiExceptionInd_s {
        HiMsgHdr_t Header;             
        HiExceptionIndBody_t Body;
} HiExceptionInd_t;


/**
 * @brief specifies the type of error reported by the indication message ::HiErrorIndBody_t
 *
 * */
typedef enum WsmHiError_e {
        WSM_HI_ERROR_FIRMWARE_ROLLBACK             = 0x0,         ///<Firmware rollback error, no data returned
        WSM_HI_ERROR_FIRMWARE_DEBUG_ENABLED        = 0x1,         ///<Firmware debug feature enabled, no data returned
        WSM_HI_ERROR_OUTDATED_SESSION_KEY          = 0x2,         ///<SecureLink Session key is outdated @todo what is returned in Data?
        WSM_HI_ERROR_INVALID_SESSION_KEY           = 0x3,         ///<SecureLink Session key is invalid, @todo what is returned in Data?
		WSM_HI_ERROR_OOR_VOLTAGE                   = 0x4,         ///<Out-of-range power supply voltage detected
		WSM_HI_ERROR_PDS_VERSION                   = 0x5          ///<wrong PDS version detected, no data returned
} WsmHiError;

#define API_DATA_SIZE_124                               124
/**
 * @brief Error indication message.
 *
 * It reports user configuration errors.
 * A reboot is needed after this message.
 * */
typedef struct __attribute__((__packed__)) HiErrorIndBody_s {
        uint32_t   Type;                             ///<error type, see enum ::WsmHiError
        uint8_t    Data[API_DATA_SIZE_124];          ///<Generic data buffer - contents depends on the error type.
} HiErrorIndBody_t;

typedef struct __attribute__((__packed__)) HiErrorInd_s {
        HiMsgHdr_t Header;             
        HiErrorIndBody_t Body;
} HiErrorInd_t;

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
typedef enum SecureLinkState_e {
        SECURE_LINK_NA_MODE                        = 0x0,	///<Reserved
		SECURE_LINK_UNTRUSTED_MODE                 = 0x1,	///<Untrusted mode - SecureLink not available
		SECURE_LINK_TRUSTED_MODE                   = 0x2,   ///<Trusted (Evaluation) mode
		SECURE_LINK_TRUSTED_ACTIVE_ENFORCED        = 0x3    ///<Trusted (Enforced) mode
} SecureLinkState;


/**
 * @brief destination of the *Secure Link MAC key*, used by request message ::HiSetSlMacKeyReqBody_t
 * */
typedef enum SlMacKeyDest_e {
        SL_MAC_KEY_DEST_OTP                        = 0x78,        ///<Key will be stored in OTP
        SL_MAC_KEY_DEST_RAM                        = 0x87         ///<Key will be stored in RAM
} SlMacKeyDest;

#define API_KEY_VALUE_SIZE      32
/**
 * @brief Set the Secure Link MAC key
 * 
 * This API can be used in two contexts:
 * - for *Trused Eval* chips : used to set a temporary *SecureLink MAC key* in RAM.
 * - for *Trused Enforced* chips : used to permanently burn the *SecureLink MAC key* in OTP memory
 */
typedef struct __attribute__((__packed__)) HiSetSlMacKeyReqBody_s {
        uint8_t    OtpOrRam;                         ///<Key destination - OTP or RAM (see enum ::SlMacKeyDest)
        uint8_t    KeyValue[API_KEY_VALUE_SIZE];     ///<Secure Link MAC Key value
} HiSetSlMacKeyReqBody_t;

typedef struct __attribute__((__packed__)) HiSetSlMacKeyReq_s {
        HiMsgHdr_t Header;             
        HiSetSlMacKeyReqBody_t Body;               
} HiSetSlMacKeyReq_t;

/**
 * @brief Confirmation for the Secure Link MAC key setting */
typedef struct __attribute__((__packed__)) HiSetSlMacKeyCnfBody_s {
        uint32_t   Status;                           ///<Key upload status (see enum ::HiStatus)
} HiSetSlMacKeyCnfBody_t;

typedef struct __attribute__((__packed__)) HiSetSlMacKeyCnf_s {
        HiMsgHdr_t Header;             
        HiSetSlMacKeyCnfBody_t Body;               
} HiSetSlMacKeyCnf_t;

#define API_HOST_PUB_KEY_SIZE                           32
#define API_HOST_PUB_KEY_MAC_SIZE                       64
/**
 * @brief Exchange Secure Link Public Keys
 * 
 * This API is used by the Host to send its *curve25519* public key to Device, and get back Device public key in the confirmation message.
 * Once keys are exchanged and authenticated (using their respective MAC), each peer computes the Secure Link *session key* that will be used
 * to encrypt/decrypt future Host<->Device messages.
 */
typedef struct __attribute__((__packed__)) HiSlExchangePubKeysReqBody_s {
        uint8_t    HostPubKey[API_HOST_PUB_KEY_SIZE];   ///<Host Public Key
        uint8_t    HostPubKeyMac[API_HOST_PUB_KEY_MAC_SIZE];   ///<Host Public Key MAC
} HiSlExchangePubKeysReqBody_t;

typedef struct __attribute__((__packed__)) HiSlExchangePubKeysReq_s {
        HiMsgHdr_t Header;             
        HiSlExchangePubKeysReqBody_t Body;               
} HiSlExchangePubKeysReq_t;

#define API_NCP_PUB_KEY_SIZE                            32
#define API_NCP_PUB_KEY_MAC_SIZE                        64
/**
 * @brief Confirmation for exchange of Secure Link Public Keys */
typedef struct __attribute__((__packed__)) HiSlExchangePubKeysCnfBody_s {
        uint32_t   Status;                      ///<Request status (see enum ::HiStatus)
        uint8_t    NcpPubKey[API_NCP_PUB_KEY_SIZE];   		///<Device Public Key
        uint8_t    NcpPubKeyMac[API_NCP_PUB_KEY_MAC_SIZE];   	///<Device Public Key MAC
} HiSlExchangePubKeysCnfBody_t;

typedef struct __attribute__((__packed__)) HiSlExchangePubKeysCnf_s {
        HiMsgHdr_t Header;             
        HiSlExchangePubKeysCnfBody_t Body;               
} HiSlExchangePubKeysCnf_t;


/**
 * @brief used in request message ::HiSlConfigureReqBody_t to trigger *Session Key* invalidation
 */
typedef enum SlConfigureSkeyInvld_e {
        SL_CONFIGURE_SKEY_INVLD_INVALIDATE         = 0x87,        ///<Force invalidating session key
        SL_CONFIGURE_SKEY_INVLD_NOP                = 0x00         ///<Do not invalidate session key
} SlConfigureSkeyInvld;

#define API_ENCR_BMP_SIZE        32
/**
 * @brief Configure Secure Link Layer
 * 
 * This API can be used to:
 * - Set/update the Secure Link *encryption bitmap*
 * - Optionally (and additionally), invalidate the current *session key*
 * 
 * Upon request reception, Device will update its own encryption bitmap and
 *  return the updated value in the confirmation.
 */
typedef struct __attribute__((__packed__)) HiSlConfigureReqBody_s {
        uint8_t    EncrBmp[API_ENCR_BMP_SIZE];       ///<Encryption bitmap
        uint8_t    SkeyInvld;                        ///<Invalidate Session Key (see enum ::SlConfigureSkeyInvld)
} HiSlConfigureReqBody_t;

typedef struct __attribute__((__packed__)) HiSlConfigureReq_s {
        HiMsgHdr_t Header;             
        HiSlConfigureReqBody_t Body;               
} HiSlConfigureReq_t;

#define API_NCP_ENCR_BMP_SIZE      32
/**
 * @brief Confirmation of Secure Link Layer configuration ::HiSlConfigureReqBody_t */
typedef struct __attribute__((__packed__)) HiSlConfigureCnfBody_s {
        uint32_t Status;							///<Request status (see enum ::WsmStatus)
} HiSlConfigureCnfBody_t;

typedef struct __attribute__((__packed__)) HiSlConfigureCnf_s {
        HiMsgHdr_t Header;             
        HiSlConfigureCnfBody_t Body;               
} HiSlConfigureCnf_t;


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
 *   An *Error indication* will be returned to the driver indicating the cause of the error (::WSM_HI_ERROR_FIRMWARE_ROLLBACK). 
 * 
 * @note The firmware *rollback revision number* is different that the *firmware version*.
 * The former is incremented only when some important fixes (i.e. Security patches) are provided 
 * by a given version of the firmware,that MUST be applied to Device and should not be reverted.
 *  Usually, subsequent firmware versions are supposed to embed the same rollback revision number.
 * 
 * The rollback capability relies on the use of a dedicated API ::HiPreventRollbackReqBody_t.
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
typedef struct __attribute__((__packed__)) HiPreventRollbackReqBody_s {
        uint32_t   MagicWord;                        /**< Magic Word - should be 0x5C8912F3*/
} HiPreventRollbackReqBody_t;

typedef struct __attribute__((__packed__)) HiPreventRollbackReq_s {
        HiMsgHdr_t Header;             
        HiPreventRollbackReqBody_t Body;               
} HiPreventRollbackReq_t;

/**
 * @brief Confirmation of the *Prevent Rollback* request
 *
 * The request might have failed for the following reasons:
 * - Wrong *magic word* value
 * 
*/
typedef struct __attribute__((__packed__)) HiPreventRollbackCnfBody_s {
        uint32_t    Status;             			///<Confirmation status, see enum ::HiStatus
} HiPreventRollbackCnfBody_t;

typedef struct __attribute__((__packed__)) HiPreventRollbackCnf_s {
        HiMsgHdr_t Header;             
        HiPreventRollbackCnfBody_t Body;               
} HiPreventRollbackCnf_t;

/**
 * @}
 */
 /* end of Prevent_Roll_Back */

/**
 * @}
 */
/*end of GENERAL_API */

#endif  /* _GENERAL_API_H_ */
