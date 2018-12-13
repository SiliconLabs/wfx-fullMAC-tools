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

/*
 *  Host specific implementation 
 */

#include "cmsis_os.h"
#include "wf200.h"
#include "stm32f4xx_hal.h"
#include <stdlib.h>
#include <string.h>
#include "wf200_host.h"
#include "wf200_host_pin.h"

#if   defined( WF200_ALPHA_KEY )
#include "wfm_wf200_A0.h"
#elif defined( WF200_PROD_KEY )
#include "wfm_wf200_C0.h"
#else
#error Must define either WF200_ALPHA_KEY or WF200_PROD_KEY
#endif

QueueHandle_t eventQueue;
SemaphoreHandle_t eventMutex;
extern wf200_buffer_t* network_rx_buffer_gbl;

struct
{
    uint32_t wf200_firmware_download_progress;
} host_context;

/* Initialization phase*/
sl_status_t wf200_host_init( void )
{
  host_context.wf200_firmware_download_progress = 0;
  eventQueue = xQueueCreate( 1, sizeof( uint32_t ) );
  eventMutex = xSemaphoreCreateMutex();
  xSemaphoreGive( eventMutex );
  return SL_SUCCESS;
}

sl_status_t wf200_host_get_firmware_data( const uint8_t** data, uint32_t data_size )
{
  *data = &wf200_firmware[host_context.wf200_firmware_download_progress];
  host_context.wf200_firmware_download_progress += data_size;
  return SL_SUCCESS;
}

sl_status_t wf200_host_get_firmware_size( uint32_t* firmware_size )
{
#ifdef DEBUG
  printf("Firmware size    : %d kB\r\n", wf200_firmware_size);    
#endif
  *firmware_size = sizeof(wf200_firmware);
  return SL_SUCCESS;
}

sl_status_t wf200_host_deinit( void )
{
  //TODO
  return SL_SUCCESS;
}

/* GPIO interface */
sl_status_t wf200_host_reset_chip( void )
{
  // hold pin high to get chip out of reset
  HAL_GPIO_WritePin(WF200_RESET_PORT, WF200_RESET_GPIO, GPIO_PIN_RESET);
  HAL_Delay( 10 );
  HAL_GPIO_WritePin(WF200_RESET_PORT, WF200_RESET_GPIO, GPIO_PIN_SET);
  HAL_Delay( 30 );
  return SL_SUCCESS;
}

sl_status_t wf200_host_hold_in_reset( void )
{
  HAL_GPIO_WritePin(WF200_RESET_PORT, WF200_RESET_GPIO, GPIO_PIN_RESET);
  return SL_SUCCESS;
}

sl_status_t wf200_host_set_wake_up_pin( uint8_t state )
{
  if ( state > 0 )
  {
    HAL_GPIO_WritePin(WF200_WUP_PORT, WF200_WUP_GPIO, GPIO_PIN_SET);
  }
  else
  {
    HAL_GPIO_WritePin(WF200_WUP_PORT, WF200_WUP_GPIO, GPIO_PIN_RESET);
  }
  return SL_SUCCESS;
}

sl_status_t wf200_host_wait_for_wake_up( void )
{
  osDelay(2);
  return SL_SUCCESS;
}

sl_status_t wf200_host_wait_for_confirmation( uint32_t timeout, void** event_payload_out )
{
  uint32_t posted_event;
  for(uint32_t i = 0; i < timeout; i++)
  {
    if( xQueueReceive( eventQueue, &( posted_event ), 1 ) )
    { 
            
      if ( wf200_context->waited_event_id == posted_event )
      {
#ifdef DEBUG
        printf("event %#08X \n\r", posted_event); 
#endif 
        if ( event_payload_out != NULL )
        {
          *event_payload_out = wf200_context->event_payload_buffer;
        }
        return SL_SUCCESS;
      }
    }
  }
  return SL_TIMEOUT;
}

sl_status_t wf200_host_wait( uint32_t wait_time )
{
  osDelay(wait_time);
  return SL_SUCCESS;
}

sl_status_t wf200_host_post_event( uint32_t event_id, void* event_payload, uint32_t event_payload_length )
{
  //xQueueOverwrite (rxFrameQueue, event_payload);
  network_rx_buffer_gbl = (wf200_buffer_t*)event_payload;  
  if ( wf200_context->waited_event_id == event_id )
  {
    /* Post the event in the queue */
    memcpy( wf200_context->event_payload_buffer, event_payload, event_payload_length );
    xQueueOverwrite( eventQueue, ( void * ) &event_id);
  }
  return SL_SUCCESS;
}

/* Memory management */
sl_status_t wf200_host_allocate_buffer(wf200_buffer_t** buffer, wf200_buffer_type_t type, uint32_t buffer_size, uint32_t wait_duration)
{
  *buffer = pvPortMalloc( buffer_size );
  return SL_SUCCESS;
}

sl_status_t wf200_host_free_buffer( wf200_buffer_t* buffer, wf200_buffer_type_t type )
{
  vPortFree( buffer );
  return SL_SUCCESS;
}

/* Frame hook */
sl_status_t wf200_host_transmit_frame( wf200_buffer_t* frame )
{
    return wf200_data_write( frame, frame->msg_len );
}

sl_status_t wf200_host_setup_waited_event( uint32_t event_id )
{
  wf200_context->waited_event_id = event_id;
  wf200_context->posted_event_id = 0;
  return SL_SUCCESS;
}









