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

#include "sl_wfx_host.h"

osThreadId            sl_wfx_task_handle;
EventGroupHandle_t    sl_wfx_event_group;
SemaphoreHandle_t     sl_wfx_tx_queue_mutex;
sl_wfx_packet_queue_t sl_wfx_tx_queue_context;

static void        sl_wfx_task_entry(void const * pvParameters);
static sl_status_t sl_wfx_rx_process (uint16_t control_register);
static sl_status_t sl_wfx_tx_process (void);

/**************************************************************************//**
 * Init wfx process thread
 *****************************************************************************/
void sl_wfx_task_start (void) {
  sl_wfx_event_group = xEventGroupCreate();
  sl_wfx_tx_queue_mutex = xSemaphoreCreateMutex();
  sl_wfx_tx_queue_context.head_ptr = NULL;
  sl_wfx_tx_queue_context.tail_ptr = NULL;
  osThreadDef(sk_wfx_task, sl_wfx_task_entry, osPriorityHigh , 0, 512);//osPriorityHigh
  sl_wfx_task_handle = osThreadCreate(osThread(sk_wfx_task), NULL);
}

/**************************************************************************//**
 * Wfx process task entry
 *****************************************************************************/
static void sl_wfx_task_entry (void const * pvParameters) {
  uint16_t control_register = 0;
  EventBits_t wifi_event_bits;
  
  while(1) {
    /*Wait for an event*/
    wifi_event_bits = xEventGroupWaitBits(sl_wfx_event_group,
                                          SL_WFX_TX_PACKET_AVAILABLE | SL_WFX_RX_PACKET_AVAILABLE,
                                          pdTRUE,
                                          pdFALSE,
                                          portMAX_DELAY);

    if (wifi_event_bits & SL_WFX_TX_PACKET_AVAILABLE) {
      /* Process TX packets */
      sl_wfx_tx_process();
    }
    if (wifi_event_bits & SL_WFX_RX_PACKET_AVAILABLE) {
      /* Process RX packets */
      sl_wfx_rx_process(control_register);
    }
  }
}

/**************************************************************************//**
 * Wfx process receive frame
 *****************************************************************************/
static sl_status_t sl_wfx_rx_process (uint16_t control_register) {
  sl_status_t result;

  sl_wfx_host_disable_platform_interrupt();

  /* Receive a frame */
  result = sl_wfx_receive_frame(&control_register);
  
  if (result) {
    sl_wfx_host_enable_platform_interrupt();
  } else {
    if ((control_register & SL_WFX_CONT_NEXT_LEN_MASK) != 0) {
      
      /* if a packet is still available in the WF200, set an RX event */
      xEventGroupSetBits(sl_wfx_event_group, SL_WFX_RX_PACKET_AVAILABLE);
    } else {
      sl_wfx_host_enable_platform_interrupt();
    }
  }

  return result;
}

/**************************************************************************//**
 * Wfx process tx queue
 *****************************************************************************/
static sl_status_t sl_wfx_tx_process (void) {
  sl_status_t result;
  sl_wfx_packet_queue_item_t *item_to_free;

  if (sl_wfx_tx_queue_context.head_ptr == NULL) {
    return SL_STATUS_EMPTY;
  }

  /* Take TX queue mutex */
  xSemaphoreTake(sl_wfx_tx_queue_mutex, portMAX_DELAY);

  /* Send the packet */
  result = sl_wfx_send_ethernet_frame(&sl_wfx_tx_queue_context.head_ptr->buffer,
                                      sl_wfx_tx_queue_context.head_ptr->data_length,
                                      sl_wfx_tx_queue_context.head_ptr->interface,
                                      WFM_PRIORITY_BE0);

  if (result != SL_STATUS_OK) {
    /* If the packet is not successfully sent, set the associated event and return */
    xEventGroupSetBits(sl_wfx_event_group, SL_WFX_TX_PACKET_AVAILABLE);
    xSemaphoreGive(sl_wfx_tx_queue_mutex);
    return SL_STATUS_FULL;
  }
  
  /* The packet has been successfully sent, free it  */
  item_to_free = sl_wfx_tx_queue_context.head_ptr;
  
  /* Move the queue pointer to process the next packet */
  sl_wfx_tx_queue_context.head_ptr = sl_wfx_tx_queue_context.head_ptr->next;

  /* The packet has been sent, release the packet  */
  sl_wfx_free_command_buffer((sl_wfx_generic_message_t*) item_to_free,
                             SL_WFX_SEND_FRAME_REQ_ID,
                             SL_WFX_TX_FRAME_BUFFER);

  /* If a packet is available, set the associated event */
  if (sl_wfx_tx_queue_context.head_ptr != NULL) {
    xEventGroupSetBits(sl_wfx_event_group, SL_WFX_TX_PACKET_AVAILABLE);
  }
  
  /* Release TX queue mutex */
  xSemaphoreGive(sl_wfx_tx_queue_mutex);

  return result;
}

