#include <stdio.h>
#include "cmsis_os.h"
#include "sl_wfx.h"
#include "sl_wfx_secure_link.h"

osThreadId secureLinkTaskHandle;
osSemaphoreId s_xSLSemaphore;

/*
 * The task that implements the Secure Link renegotiation with WFX.
 */
static void prvSecureLinkTask(void const * pvParameters)
{
  sl_status_t result;

  /* Create a mutex used for making Secure Link renegotiations atomic */
  s_xSLSemaphore = xSemaphoreCreateMutex();

  for(;;)
  {
    /* Wait for a key renegotiation request */
    ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

    result = sl_wfx_secure_link_renegotiate_session_key();
    if (result != SL_STATUS_OK)
    {
      printf ("session key negotiation error %lu\n",result);
    }
  }
}

/***************************************************************************//**
 * @brief Creates WFX securelink key renegotiation task.
 ******************************************************************************/
void wfx_securelink_task_start(void)
{
  osThreadDef(secureLinkTask, prvSecureLinkTask, osPriorityLow, 0, 128);
  secureLinkTaskHandle = osThreadCreate(osThread(secureLinkTask), NULL);
}
