#include "sl_wfx.h"
#include "sl_wfx_registers.h"
#include "lwip_micriumos.h"
#include "wfx_host_cfg.h"

#include "em_gpio.h"
#include "em_usart.h"
#include "em_cmu.h"
#include "em_ldma.h"
#include "em_bus.h"


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <kernel/include/os.h>
#include <common/include/rtos_utils.h>
#include <common/include/rtos_err.h>
#include <common/include/rtos_err.h>

#ifdef SL_WFX_USE_SECURE_LINK
// Securelink Task Configurations
#define WF200_SECURELINK_TASK_PRIO       24u
#define WF200_SECURELINK_TASK_STK_SIZE   512u

//Task Data Structures
static CPU_STK WF200SecurelinkTaskStk[WF200_SECURELINK_TASK_STK_SIZE];
OS_TCB WF200SecurelinkTaskTCB;

OS_MUTEX   wf200_securelink_rx_mutex;

/*
 * The task that implements the securelink renegotiation with WF200.
 */
static void WF200SecurelinkTask (void *p_arg)
{
  RTOS_ERR err;
  sl_status_t result;
  OSMutexCreate(&wf200_securelink_rx_mutex,"wf200 secure link RX mutex", &err);
  for( ;; )
  {
	OSTaskSemPend (0,OS_OPT_PEND_BLOCKING,0,&err);
    result = sl_wfx_secure_link_renegotiate_session_key();
    if (result != SL_SUCCESS)
    {
      printf ("session key negotiation error %d\n",result);
    }
  }
}

/***************************************************************************//**
 * @brief Creates WF200 securelink key renegotiation task.
 ******************************************************************************/
void WF200SecurelinkStart()
{
    RTOS_ERR err;

    OSTaskCreate(&WF200SecurelinkTaskTCB,
                 "WF200 SecureLink Task",
                  WF200SecurelinkTask,
                  DEF_NULL,
				  WF200_SECURELINK_TASK_PRIO,
                 &WF200SecurelinkTaskStk[0],
                 (WF200_SECURELINK_TASK_STK_SIZE / 10u),
                  WF200_SECURELINK_TASK_STK_SIZE,
                  0u,
                  0u,
                  DEF_NULL,
                 (OS_OPT_TASK_STK_CLR),
                 &err);
                                                                /*   Check error code.                                  */
    APP_RTOS_ASSERT_DBG((RTOS_ERR_CODE_GET(err) == RTOS_ERR_NONE), 1);
}

#endif //WFX_USE_SECURE_LINK




