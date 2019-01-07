FMAC API	{#wf200_fmac_api}  
============

To interact with WF200, the host has access to a set of API exposing a FMAC Wi-Fi interface. To find the related driver functions, you can refer to the table below or to the \ref FMAC_API group.

## FMAC API commands
Below is a table listing the FMAC API commands supported by WF200.
\anchor requests

| Commands                                   | Value | Associated driver function           | Description                          |
|--------------------------------------------|-------|--------------------------------------|--------------------------------------|
| WFM_HI_SET_MAC_ADDRESS_REQ_ID              | 0x42  | ::wf200_set_mac_address              | Set a MAC address for each interface |
| WFM_HI_CONNECT_REQ_ID                      | 0x43  | ::wf200_send_join_command            | Connect to a network                 |
| WFM_HI_DISCONNECT_REQ_ID                   | 0x44  | ::wf200_send_disconnect_command      | Disconnect from a network            |
| WFM_HI_START_AP_REQ_ID                     | 0x45  | ::wf200_start_ap_command             | Start AP mode                        |
| WFM_HI_UPDATE_AP_REQ_ID                    | 0x46  | ::wf200_update_ap_command            | Update AP mode parameters            |
| WFM_HI_STOP_AP_REQ_ID                      | 0x47  | ::wf200_stop_ap_command              | Stop AP mode                         |
| WFM_HI_SEND_FRAME_REQ_ID                   | 0x4a  | ::wf200_send_ethernet_frame          | Send a frame                         |
| WFM_HI_START_SCAN_REQ_ID                   | 0x4b  | ::wf200_send_scan_command            | Perform a scan                       |
| WFM_HI_STOP_SCAN_REQ_ID                    | 0x4c  | ::wf200_send_stop_scan_command       | Stop an ongoing scan                 |
| WFM_HI_GET_SIGNAL_STRENGTH_REQ_ID          | 0x4e  | ::wf200_get_signal_strength          | Get signal strength                  |
| WFM_HI_DISCONNECT_AP_CLIENT_REQ_ID         | 0x4f  | ::wf200_disconnect_ap_client_command | Disconnect AP client                 |
| WFM_HI_JOIN_IBSS_REQ_ID                    | 0x50  |                                      | Join or create an IBSS               |
| WFM_HI_LEAVE_IBSS_REQ_ID                   | 0x51  |                                      | Leave an IBSS                        |
| WFM_HI_SET_PM_MODE_REQ_ID                  | 0x52  | ::wf200_set_power_mode               | Set pm mode                          |
| WFM_HI_ADD_MULTICAST_ADDR_REQ_ID           | 0x53  | ::wf200_add_multicast_address        | Add multicast address to filter      |
| WFM_HI_REMOVE_MULTICAST_ADDR_REQ_ID        | 0x54  |                                      | Remove multicast address to filter   |
| WFM_HI_SET_MAX_AP_CLIENT_COUNT_REQ_ID      | 0x55  | ::wf200_set_max_ap_client            | Set client count limitation          |
| WFM_HI_SET_MAX_AP_CLIENT_INACTIVITY_REQ_ID | 0x56  |                                      | Set client inactivity limitation     |
| WFM_HI_SET_ROAM_PARAMETERS_REQ_ID          | 0x57  |                                      | Set roam parameters                  |
| WFM_HI_SET_TX_RATE_PARAMETERS_REQ_ID       | 0x58  |                                      | Set TX rate parameters               |

For each available command, there is an associated confirmation with the same ID value.

##FMAC API indications
\anchor indications
| Indications                       | Value | Associated struture                  | Description                           |
| --------------------------------- | ----- | ------------------------------------ | ------------------------------------- |
| WFM_HI_CONNECT_IND                | 0xc3  | ::WfmHiConnectIndBody_t              | Connection to a network complete      |
| WFM_HI_DISCONNECT_IND             | 0xc4  | ::WfmHiDisconnectIndBody_t           | Disconnection from a network complete |
| WFM_HI_START_AP_IND               | 0xc5  | ::WfmHiStartApIndBody_t              | Started AP mode                       |
| WFM_HI_STOP_AP_IND                | 0xc7  | ::WfmHiStopApInd_t                   | Stopped AP mode                       |
| WFM_HI_RECEIVED_IND               | 0xca  | ::WfmHiReceivedIndBody_t             | Framereceived                         |
| WFM_HI_SCAN_RESULT_IND            | 0xcb  | ::WfmHiScanResultIndBody_t           | Scan result received                  |
| WFM_HI_SCAN_COMPLETE_IND          | 0xcc  | ::WfmHiScanCompleteIndBody_t         | Scan complete                         |
| WFM_HI_AP_CLIENT_CONNECTED_IND    | 0xcd  | ::WfmHiApClientConnectedIndBody_t    | AP Client connected                   |
| WFM_HI_AP_CLIENT_REJECTED_IND     | 0xce  | ::WfmHiApClientRejectedIndBody_t     | AP Client rejected                    |
| WFM_HI_AP_CLIENT_DISCONNECTED_IND | 0xcf  | ::WfmHiApClientDisconnectedIndBody_t | AP Client disconnected                |
| WFM_HI_JOIN_IBSS_IND              | 0xd0  | ::WfmHiJoinIbssIndBody_t             | Connection to an IBSS complete        |
| WFM_HI_LEAVE_IBSS_IND             | 0xd1  | ::WfmHiLeaveIbssInd_t                | Left IBSS                             |

This indications have to be managed by the host depending on the application and use case.
