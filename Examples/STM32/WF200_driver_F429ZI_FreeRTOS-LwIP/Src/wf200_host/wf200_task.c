/*
 * EVALUATION AND USE OF THIS SOFTWARE IS SUBJECT TO THE TERMS AND
 * CONDITIONS OF THE CONTROLLING LICENSE AGREEMENT FOUND AT LICENSE.md
 * IN THIS SDK. IF YOU DO NOT AGREE TO THE LICENSE TERMS AND CONDITIONS,
 * PLEASE RETURN ALL SOURCE FILES TO SILICON LABORATORIES.
 * (c) Copyright 2018, Silicon Laboratories Inc.  All rights reserved.
 */

/*
 *  FreeRTOS WF200 task
 */

/* FreeRTOS includes. */
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"

#include "cmsis_os.h"
#include "wf200.h"
#include "wf200_host.h"
#include "lwip_freertos.h"
   
osThreadId busCommTaskHandle;
extern osSemaphoreId s_xDriverSemaphore;
extern QueueHandle_t rxFrameQueue;
extern wf200_context_t* wf200_context;

/* Default parameters */
wf200_context_t wifi;
char wlan_ssid[32]                 = WLAN_SSID;
char wlan_passkey[64]              = WLAN_PASSKEY;
WfmSecurityMode wlan_security      = WLAN_SECURITY;
char softap_ssid[32]               = SOFTAP_SSID;
char softap_passkey[64]            = SOFTAP_PASSKEY;
WfmSecurityMode softap_security    = SOFTAP_SECURITY;
uint8_t softap_channel             = SOFTAP_CHANNEL;
/* Default parameters */

/*
 * The task that implements the bus communication with WF200.
 */
static void prvBusCommTask(void const * pvParameters );

void vBusCommStart( void )
{
  osThreadDef(busCommTask, prvBusCommTask, osPriorityRealtime, 0, 128);
  busCommTaskHandle = osThreadCreate(osThread(busCommTask), NULL);
}
wf200_buffer_t *network_rx_buffer_gbl;

static sl_status_t receive_frames ()
{
  sl_status_t result;
  uint32_t frame_size = 0;
  
  do
  {
    result = wf200_receive_frame(&frame_size);
    ERROR_CHECK( result );
  }while ( frame_size != 0 );
  
error_handler:
  return result;
}

static void prvBusCommTask(void const * pvParameters )
{
  for( ;; )
  {
    /*Wait for an interrupt from WF200*/
    ulTaskNotifyTake( pdTRUE, portMAX_DELAY );
    /*Receive the frame(s) pending in WF200*/
    if( s_xDriverSemaphore != NULL )
    {
        /* See if we can obtain the semaphore.  If the semaphore is not
        available wait to see if it becomes free. */
        if( xSemaphoreTake( s_xDriverSemaphore, ( osWaitForever ) ) == pdTRUE )
        {
          receive_frames();
          xSemaphoreGive( s_xDriverSemaphore );
        }
        else
        {
          //unable to receive
        }
    }
    else
    {
       receive_frames();
    }
  }
}

