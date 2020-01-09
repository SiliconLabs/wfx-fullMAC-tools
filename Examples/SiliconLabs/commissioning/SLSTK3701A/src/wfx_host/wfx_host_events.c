/***************************************************************************//**
 * @file
 * @brief WFX FMAC driver event processing task
 *******************************************************************************
 * # License
 * <b>Copyright 2019 Silicon Laboratories Inc. www.silabs.com</b>
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
#include "lwip_micriumos.h"

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

#include "wfx_host_events.h"
#include "wfx_host.h"
#include "wfx_task.h"
#include "sl_wfx.h"

// Event Task Configurations
#define WFX_EVENTS_TASK_PRIO              21u
#define WFX_EVENTS_TASK_STK_SIZE         512u

/// wfx event task stack
static CPU_STK wfx_events_task_stk[WFX_EVENTS_TASK_STK_SIZE];
/// wfx event task TCB
static OS_TCB wfx_events_task_tcb;



/***************************************************************************//**
 * WFX events processing task.
 ******************************************************************************/
static void wfx_events_task(void *p_arg)
{
  (void)p_arg;
  RTOS_ERR err;
  OS_FLAGS  flags = 0;
  while (1)
  {
    if (sl_wfx_event_group.Type == OS_OBJ_TYPE_FLAG){
      flags = OSFlagPend(&sl_wfx_event_group,
                         SL_WFX_CONNECT | SL_WFX_STOP_AP | SL_WFX_DISCONNECT | SL_WFX_START_AP,
                         0,
                         OS_OPT_PEND_FLAG_SET_ANY | OS_OPT_PEND_BLOCKING | OS_OPT_PEND_FLAG_CONSUME,
                         0,
                         &err);
    }
    else {
      OSTimeDly(5000, OS_OPT_TIME_DLY,&err);
    }
    if (flags & SL_WFX_CONNECT) {
      lwip_set_sta_link_up();

#ifdef SLEEP_ENABLED
      if (!(wifi.state & SL_WFX_AP_INTERFACE_UP)) {
        // Enable the power save
        sl_wfx_set_power_mode(WFM_PM_MODE_PS, 1);
        sl_wfx_enable_device_power_save();
      }
#endif
    }
    if (flags & SL_WFX_DISCONNECT){
      lwip_set_sta_link_down();
    }
    if (flags & SL_WFX_START_AP){
      lwip_set_ap_link_up();

#ifdef SLEEP_ENABLED
      // Power save always disabled when SoftAP mode enabled
      sl_wfx_set_power_mode(WFM_PM_MODE_ACTIVE, 0);
      sl_wfx_disable_device_power_save();
#endif
    }
    if (flags & SL_WFX_STOP_AP){
      lwip_set_ap_link_down();

#ifdef SLEEP_ENABLED
      if (wifi.state & SL_WFX_STA_INTERFACE_CONNECTED) {
        // Enable the power save
        sl_wfx_set_power_mode(WFM_PM_MODE_PS, 1);
        sl_wfx_enable_device_power_save();
      }
#endif
    }
    if (flags & SL_WFX_SCAN_COMPLETE){
      //we don't process this here (see scan cgi handler)
    }
  }
}

/***************************************************************************//**
 * Creates WFX events processing task.
 ******************************************************************************/
void wfx_events_task_start()
{
  RTOS_ERR err;
  OSTaskCreate(&wfx_events_task_tcb,
               "WFX events task",
               wfx_events_task,
               DEF_NULL,
               WFX_EVENTS_TASK_PRIO,
               &wfx_events_task_stk[0],
               (WFX_EVENTS_TASK_STK_SIZE / 10u),
               WFX_EVENTS_TASK_STK_SIZE,
               0u,
               0u,
               DEF_NULL,
               (OS_OPT_TASK_STK_CLR),
               &err);
  //   Check error code.
  APP_RTOS_ASSERT_DBG((RTOS_ERR_CODE_GET(err) == RTOS_ERR_NONE), 1);
}
