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

/**************************************************************************//**
 * Host specific implementation: EFM32GG11 + Bare Metal
 *****************************************************************************/

#include "em_gpio.h"
#include "sl_wfx.h"
#include "BRD802xx_pds.h"
#include "wfm_wf200_C0.h"
#include "wfx_host_cfg.h"
#include "host_resources.h"

extern sl_wfx_rx_stats_t rx_stats;
uint8_t wf200_interrupt_event = 0;
uint32_t sl_wfx_firmware_download_progress;
uint8_t waited_event_id;
uint8_t posted_event_id;

/* Initialization phase*/
sl_status_t sl_wfx_host_init(void)
{
  sl_wfx_firmware_download_progress = 0;
  return SL_SUCCESS;
}

sl_status_t sl_wfx_host_get_firmware_data(const uint8_t** data, uint32_t data_size)
{
  *data = &sl_wfx_firmware[sl_wfx_firmware_download_progress];
  sl_wfx_firmware_download_progress += data_size;
  return SL_SUCCESS;
}

sl_status_t sl_wfx_host_get_firmware_size(uint32_t* firmware_size)
{
  *firmware_size = sizeof(sl_wfx_firmware);
  return SL_SUCCESS;
}

sl_status_t sl_wfx_host_get_pds_data(const char **pds_data, uint16_t index)
{
  *pds_data = wf200_pds[index];
  return SL_SUCCESS;
}

sl_status_t sl_wfx_host_get_pds_size(uint16_t *pds_size)
{
  *pds_size = SL_WFX_ARRAY_COUNT(wf200_pds);
  return SL_SUCCESS;
}

sl_status_t sl_wfx_host_deinit(void)
{
  return SL_SUCCESS;
}

/* GPIO interface */
sl_status_t sl_wfx_host_reset_chip(void)
{
  // hold pin high to get chip out of reset
  GPIO_PinModeSet(WFX_HOST_CFG_RESET_PORT, WFX_HOST_CFG_RESET_PIN,
                  gpioModePushPull, 0);

  host_delay(30);

  GPIO_PinModeSet(WFX_HOST_CFG_RESET_PORT, WFX_HOST_CFG_RESET_PIN,
                  gpioModePushPull, 1);

  host_delay(10);

  return SL_SUCCESS;
}

sl_status_t sl_wfx_host_hold_in_reset(void)
{
  GPIO_PinModeSet(WFX_HOST_CFG_RESET_PORT, WFX_HOST_CFG_RESET_PIN,
                  gpioModePushPull, 0);

  return SL_SUCCESS;
}

sl_status_t sl_wfx_host_set_wake_up_pin(uint8_t state)
{
  if (state > 0) {
    GPIO_PinModeSet(WFX_HOST_CFG_WUP_PORT, WFX_HOST_CFG_WUP_PIN,
                    gpioModePushPull, 1);
  } else {
    GPIO_PinModeSet(WFX_HOST_CFG_WUP_PORT, WFX_HOST_CFG_WUP_PIN,
                    gpioModePushPull, 0);
  }
  return SL_SUCCESS;
}

sl_status_t sl_wfx_host_wait_for_wake_up(void)
{
  host_delay(2);
  return SL_SUCCESS;
}

sl_status_t sl_wfx_host_sleep_grant(sl_wfx_host_bus_tranfer_type_t type,
                                    sl_wfx_register_address_t address,
                                    uint32_t length)
{
  return SL_WIFI_SLEEP_NOT_GRANTED;
}

sl_status_t sl_wfx_host_setup_waited_event(uint8_t event_id)
{
  waited_event_id = event_id;
  return SL_SUCCESS;
}

sl_status_t sl_wfx_host_wait_for_confirmation(uint8_t confirmation_id,
                                              uint32_t timeout_ms,
                                              void **event_payload_out)
{
  uint16_t control_register = 0;

  for(uint32_t i = 0; i < timeout_ms; i++)
  {
    if(wf200_interrupt_event == 1)
    {
      wf200_interrupt_event = 0;
      do
      {
        sl_wfx_receive_frame(&control_register);
      }while ( (control_register & SL_WFX_CONT_NEXT_LEN_MASK) != 0 );
    }
    if (waited_event_id == posted_event_id)
    {
      posted_event_id = 0;
      if (event_payload_out != NULL)
      {
        *event_payload_out = sl_wfx_context->event_payload_buffer;
      }
      return SL_SUCCESS;
    }else{
      sl_wfx_host_wait(1);
    }
  }
  return SL_TIMEOUT;
}

sl_status_t sl_wfx_host_wait(uint32_t wait_time)
{
  host_delay(wait_time);
  return SL_SUCCESS;
}

sl_status_t sl_wfx_host_post_event(sl_wfx_generic_message_t *event_payload)
{
  switch(event_payload->header.id){
  /******** INDICATION ********/
  case SL_WFX_EXCEPTION_IND_ID:
    {
      printf("Firmware Exception\r\n");
      break;
    }
  case SL_WFX_ERROR_IND_ID:
    {
      printf("Firmware Error\r\n");
      break;
    }
  case SL_WFX_GENERIC_IND_ID:
    {
      sl_wfx_generic_ind_t* generic_indication = (sl_wfx_generic_ind_t*) event_payload;
      rx_stats = generic_indication->body.indication_data.rx_stats;
      break;
    }
  }

  if (waited_event_id == event_payload->header.id)
  {
    if(event_payload->header.length < 512){
      /* Post the event in the queue */
      memcpy(sl_wfx_context->event_payload_buffer, (void*) event_payload, event_payload->header.length);
      posted_event_id = event_payload->header.id;
    }
  }

  return SL_SUCCESS;
}

/* Memory management */
sl_status_t sl_wfx_host_allocate_buffer(void** buffer, sl_wfx_buffer_type_t type, uint32_t buffer_size)
{
  SL_WFX_UNUSED_PARAMETER(type);
  *buffer = malloc(buffer_size);
  return SL_SUCCESS;
}

sl_status_t sl_wfx_host_free_buffer(void* buffer, sl_wfx_buffer_type_t type)
{
  SL_WFX_UNUSED_PARAMETER(type);
  free(buffer);
  return SL_SUCCESS;
}

/* Frame hook */
sl_status_t sl_wfx_host_transmit_frame(void* frame, uint32_t frame_len)
{
  return sl_wfx_data_write(frame, frame_len);
}

/* Driver mutex handling */
sl_status_t sl_wfx_host_lock(void)
{
  /* Bare metal implementation, unused */
  return SL_SUCCESS;
}

sl_status_t sl_wfx_host_unlock(void)
{
  /* Bare metal implementation, unused */
  return SL_SUCCESS;
}

void sl_wfx_process(void)
{
  uint16_t control_register = 0;
  if(wf200_interrupt_event == 1)
  {
    wf200_interrupt_event = 0;
    do
    {
      sl_wfx_receive_frame(&control_register);
    }while ( (control_register & SL_WFX_CONT_NEXT_LEN_MASK) != 0 );
  }
}

