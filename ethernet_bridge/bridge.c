/***************************************************************************//**
 * @file bridge.c
 * @brief Implementation of Wi-Fi to ethernet bridge functions
 *******************************************************************************
 * # License
 * <b>Copyright 2022 Silicon Laboratories Inc. www.silabs.com</b>
 *******************************************************************************
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
#include "app_ethernet_bridge.h"
#include "bridge.h"

/**************************************************************************//**
 * @brief:  Obtain the pointer to net_if by interface name
 *****************************************************************************/
static NET_IF* get_net_if_for_ethernet_interface()
{
	NET_IF      *p_if;
	RTOS_ERR    local_err;
	NET_IF_NBR  if_nbr;
	
	if_nbr = NetIF_NbrGetFromName("eth0");
	p_if = NetIF_Get(if_nbr, &local_err);
	return p_if;
}

/***************************************************************************//**
 * @brief:  This function is called in the ethernet driver to forward the frames
 * to FMAC driver for Transmitting packet(s)from ethernet interface to WF200.
 ******************************************************************************/
sl_status_t low_level_output_ethernet(uint8_t *data, uint32_t size)
{
  RTOS_ERR err;
  uint8_t *buffer;
  sl_status_t result;
  sl_wfx_packet_queue_item_t *queue_item = NULL;

  if (sl_wfx_context == NULL  || !(sl_wfx_context->state & SL_WFX_STARTED) ) {
      printf("WF200 not initialized\r\n");
      return SL_STATUS_WIFI_WRONG_STATE;
  }

  /* Take TX queue mutex */
  OSMutexPend(&sl_wfx_tx_queue_mutex, 0, OS_OPT_PEND_BLOCKING, 0, &err);
    
  /* Allocate a buffer for a queue item */
  result = sl_wfx_allocate_command_buffer(
                      (sl_wfx_generic_message_t**)(&queue_item),
                      SL_WFX_SEND_FRAME_REQ_ID,
                      SL_WFX_TX_FRAME_BUFFER,
                      size + sizeof(sl_wfx_packet_queue_item_t));

  if ((result != SL_STATUS_OK) || (queue_item == NULL)) {
      printf("sl_wfx_allocate_command_buffer() failed, err = %lu\r\n", result);
      OSMutexPost(&sl_wfx_tx_queue_mutex, OS_OPT_POST_NONE, &err);
      return SL_STATUS_ALLOCATION_FAILED;
  }

  buffer = queue_item->buffer.body.packet_data;
  memcpy( buffer, (uint8_t*)data, size);

  /* Provide the data length */
  queue_item->interface = SL_WFX_SOFTAP_INTERFACE;
  queue_item->data_length = size;
  
  /* Determine if there is anything on the tx packet queue */
  if (sl_wfx_tx_queue_context.head_ptr != NULL) {
    sl_wfx_tx_queue_context.tail_ptr->next = queue_item;
  } else {
    /* If tx packet queue is empty, setup head & tail pointers */
    sl_wfx_tx_queue_context.head_ptr = queue_item;
  }
    
  /* Update the tail pointer */
  sl_wfx_tx_queue_context.tail_ptr = queue_item;
  
  /* Notify that a TX frame is ready */
  OSFlagPost(&bus_events, SL_WFX_BUS_EVENT_FLAG_TX, OS_OPT_POST_FLAG_SET, &err);
  
  /* Release TX queue mutex */
  OSMutexPost(&sl_wfx_tx_queue_mutex, OS_OPT_POST_NONE, &err);

  return SL_STATUS_OK;
}

/***************************************************************************//**
 * Re-implement the NetIF_DevTxRdyWait() function in net_if.c
 ******************************************************************************/
static void wait_net_dev_tx_ready(NET_IF   *p_if,
                                  RTOS_ERR *p_err) 
{
    KAL_SEM_HANDLE *p_sem_obj = (KAL_SEM_HANDLE *)p_if->DevTxRdySignalObj;

    KAL_SemPend(*p_sem_obj, KAL_OPT_PEND_NONE, 15, p_err);
    if (RTOS_ERR_CODE_GET(*p_err) != RTOS_ERR_NONE) {
      RTOS_ERR_SET(*p_err, RTOS_ERR_CODE_GET(*p_err));
    }
}

/***************************************************************************//**
 * @brief: WFX callback to forward the received frames to the ethernet controller
 * @param
 *      rx_buffer: The ethernet frame received by the wfx
 * @return:
 *      None
 ******************************************************************************/
void sl_wfx_host_received_frame_callback(sl_wfx_received_ind_t* rx_buffer)
{
  uint16_t len = 0;
  uint8_t *buffer_ptr;
  NET_IF *p_if = DEF_NULL;
  NET_DEV_API  *p_dev_api = DEF_NULL;
  RTOS_ERR local_err = {.Code = RTOS_ERR_NONE};
  
  /* Received on SoftAP. Forward to ethernet */
  if ((rx_buffer->header.info & SL_WFX_MSG_INFO_INTERFACE_MASK)
  	      == (SL_WFX_SOFTAP_INTERFACE << SL_WFX_MSG_INFO_INTERFACE_OFFSET))
  {
	  /* Obtain the size of the frame and put it into the "len" variable. */
	  len = rx_buffer->body.frame_length;
	  
	  /* Buffer pointer to frame data */
	  buffer_ptr = (uint8_t *)&(rx_buffer->body.frame[rx_buffer->body.frame_padding]);
	  
	  p_if = get_net_if_for_ethernet_interface();
	  if (p_if == NULL) {
	      LOG_TRACE("Failed to obtain the pointer to net_if \r\n");
	      return;
	  }

	  p_dev_api = (NET_DEV_API *)p_if->Dev_API;
	  if (p_dev_api == NULL) {
	      LOG_TRACE("DEV_API is NULL\r\n");
	      return;
	  }

	  /* Forward the frame to ethernet's device driver to transmit */
	  LOG_TRACE("Host received frame & forward to ethernet, err = %d\r\n", local_err.Code);
	  p_dev_api->Tx(p_if, buffer_ptr, len, &local_err);
	  wait_net_dev_tx_ready(p_if, &local_err);
  }
}
