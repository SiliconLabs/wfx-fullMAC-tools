/**************************************************************************//**
 * Copyright 2021, Silicon Laboratories Inc.
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

#ifndef SL_WFX_HOST_H
#define SL_WFX_HOST_H

#include <stdint.h>
#include "sl_wfx.h"
#include "cmsis_os.h"
#include "lwip/netif.h"

#define SL_WFX_EVENT_MAX_SIZE  512
#define SL_WFX_EVENT_LIST_SIZE 1
#define SL_WFX_MAX_STATIONS    8
#define SL_WFX_MAX_SCAN_RESULTS 50

/* Wi-Fi events */
typedef enum {
  SL_WFX_TX_PACKET_AVAILABLE        = (1 << 0),
  SL_WFX_RX_PACKET_AVAILABLE        = (1 << 1),
  SL_WFX_TX_PACKET_CONF_RECEIVED    = (1 << 2),
  SL_WFX_CONNECTED                  = (1 << 3)
} sl_wfx_event_t;

/* WFX host callbacks */
void sl_wfx_connect_callback(sl_wfx_connect_ind_t *connect);
void sl_wfx_disconnect_callback(sl_wfx_disconnect_ind_t *disconnect);
void sl_wfx_start_ap_callback(sl_wfx_start_ap_ind_t *start_ap);
void sl_wfx_stop_ap_callback(sl_wfx_stop_ap_ind_t *stop_ap);
void sl_wfx_host_received_frame_callback(sl_wfx_received_ind_t *rx_buffer);
void sl_wfx_scan_result_callback(sl_wfx_scan_result_ind_t *scan_result);
void sl_wfx_scan_complete_callback(sl_wfx_scan_complete_ind_t *scan_complete);
void sl_wfx_generic_status_callback(sl_wfx_generic_ind_t *frame);
void sl_wfx_ap_client_connected_callback(sl_wfx_ap_client_connected_ind_t *ap_client_connected);
void sl_wfx_ap_client_rejected_callback(sl_wfx_ap_client_rejected_ind_t *ap_client_rejected);
void sl_wfx_ap_client_disconnected_callback(sl_wfx_ap_client_disconnected_ind_t *ap_client_disconnected);
void sl_wfx_ext_auth_callback(sl_wfx_ext_auth_ind_t *ext_auth_indication);

typedef struct __attribute__((__packed__)) scan_result_list_s {
  sl_wfx_ssid_def_t ssid_def;
  uint8_t  mac[SL_WFX_MAC_ADDR_SIZE];
  uint16_t channel;
  sl_wfx_security_mode_bitmask_t security_mode;
  uint16_t rcpi;
} scan_result_list_t;

/* Packet Queue */
typedef struct sl_wfx_packet_queue_item_t{
  struct sl_wfx_packet_queue_item_t *next;
  sl_wfx_interface_t interface;
  uint32_t data_length;
  sl_wfx_send_frame_req_t buffer;
}sl_wfx_packet_queue_item_t;

/* Packet Queue */
typedef struct{
  sl_wfx_packet_queue_item_t *head_ptr;
  sl_wfx_packet_queue_item_t *tail_ptr;
}sl_wfx_packet_queue_t;

extern sl_wfx_context_t      wifi_context;
extern QueueHandle_t         wifi_event_queue;
extern sl_wfx_packet_queue_t sl_wfx_tx_queue_context;
extern EventGroupHandle_t    sl_wfx_event_group;
extern SemaphoreHandle_t     sl_wfx_tx_queue_mutex;
extern osSemaphoreId         sl_wfx_wake_up_sem;

void sl_wfx_task_start(void);
#endif /* SL_WFX_HOST_H */