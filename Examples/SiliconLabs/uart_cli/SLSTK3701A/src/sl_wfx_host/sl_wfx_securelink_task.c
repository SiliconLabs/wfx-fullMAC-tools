#include "sl_wfx_secure_link.h"
#include "demo_config.h"
#include "sl_wfx_host_cfg.h"

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

// Securelink Task Configurations
#define SL_WFX_SECURELINK_TASK_PRIO        24u
#define SL_WFX_SECURELINK_TASK_STK_SIZE   512u

//Task Data Structures
static CPU_STK sl_wfx_securelink_task_stack[SL_WFX_SECURELINK_TASK_STK_SIZE];
OS_TCB sl_wfx_securelink_task_tcb;

OS_MUTEX   sl_wfx_securelink_rx_mutex;

/*
 * The task that implements the securelink renegotiation with WFX.
 */
static void sl_wfx_securelink_task (void *p_arg)
{
  RTOS_ERR err;
  sl_status_t result;
  OSMutexCreate(&sl_wfx_securelink_rx_mutex,"wfx secure link RX mutex", &err);
  for( ;; )
  {
  OSTaskSemPend (0,OS_OPT_PEND_BLOCKING,0,&err);
    result = sl_wfx_secure_link_renegotiate_session_key();
    if (result != SL_STATUS_OK)
    {
      printf ("session key negotiation error %lu\n",result);
    }
  }
}

/***************************************************************************//**
 * @brief Creates WF200 securelink key renegotiation task.
 ******************************************************************************/
void sl_wfx_securelink_start (void)
{
  RTOS_ERR err;

  OSTaskCreate(&sl_wfx_securelink_task_tcb,
               "WFX SecureLink Task",
               sl_wfx_securelink_task,
               DEF_NULL,
               SL_WFX_SECURELINK_TASK_PRIO,
               &sl_wfx_securelink_task_stack[0],
               (SL_WFX_SECURELINK_TASK_STK_SIZE / 10u),
               SL_WFX_SECURELINK_TASK_STK_SIZE,
               0u,
               0u,
               DEF_NULL,
               (OS_OPT_TASK_STK_CLR),
               &err);
  /*   Check error code.                                  */
  APP_RTOS_ASSERT_DBG((RTOS_ERR_CODE_GET(err) == RTOS_ERR_NONE), 1);
}
