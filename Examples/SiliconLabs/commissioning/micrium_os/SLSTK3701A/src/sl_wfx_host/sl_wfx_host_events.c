/***************************************************************************//**
 * @file
 * @brief WFX FMAC driver event processing task
 *******************************************************************************
 * # License
 * <b>Copyright 2021 Silicon Laboratories Inc. www.silabs.com</b>
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

#include "sl_wfx_host_events.h"
#include "sl_wfx_host.h"
#include "sl_wfx_task.h"
#include "sl_wfx_sae.h"
#include "sl_wfx.h"

/* Event Task Configurations*/
#define WFX_EVENTS_TASK_PRIO              21u
#define WFX_EVENTS_TASK_STK_SIZE        1024u
#define WFX_EVENTS_NB_MAX                 10u

extern OS_SEM scan_sem;
/* wifi event task stack*/
static CPU_STK wfx_events_task_stk[WFX_EVENTS_TASK_STK_SIZE];
/* wifi event task TCB*/
static OS_TCB wfx_events_task_tcb;


/* WiFi event queue*/
OS_Q wifi_events;

/***************************************************************************//**
 * WFX events processing task.
 ******************************************************************************/
static void wfx_events_task (void *p_arg) {

  RTOS_ERR err;
  OS_MSG_SIZE msg_size;
  sl_wfx_generic_message_t *msg;

  (void)p_arg;

  while (1) {
    msg = (sl_wfx_generic_message_t *)OSQPend(&wifi_events,
                                              0,
                                              OS_OPT_PEND_BLOCKING,
                                              &msg_size,
                                              NULL,
                                              &err);

    if (msg != NULL) {
      switch (msg->header.id) {
        case SL_WFX_CONNECT_IND_ID:
        {
          lwip_set_sta_link_up();

#ifdef SLEEP_ENABLED
          if (!(wifi_context.state & SL_WFX_AP_INTERFACE_UP)) {
            /* Enable the power save*/
            sl_wfx_set_power_mode(WFM_PM_MODE_PS, WFM_PM_POLL_FAST_PS ,1);
            sl_wfx_enable_device_power_save();
          }
#endif
          break;
        }
        case SL_WFX_DISCONNECT_IND_ID:
        {
          lwip_set_sta_link_down();
          break;
        }
        case SL_WFX_START_AP_IND_ID:
        {
          lwip_set_ap_link_up();

#ifdef SLEEP_ENABLED
          /* Power save always disabled when SoftAP mode enabled*/
          sl_wfx_set_power_mode(WFM_PM_MODE_ACTIVE, WFM_PM_POLL_FAST_PS ,0);
          sl_wfx_disable_device_power_save();
#endif
          break;
        }
        case SL_WFX_STOP_AP_IND_ID:
        {
          lwip_set_ap_link_down();

#ifdef SLEEP_ENABLED
          if (wifi_context.state & SL_WFX_STA_INTERFACE_CONNECTED) {
            /* Enable the power save*/
            sl_wfx_set_power_mode(WFM_PM_MODE_PS, WFM_PM_POLL_FAST_PS ,1);
            sl_wfx_enable_device_power_save();
          }
#endif
          break;
        }
        case SL_WFX_SCAN_COMPLETE_IND_ID:
        {
          OSSemPost(&scan_sem, OS_OPT_POST_ALL, &err);
          break;
        }
        case SL_WFX_EXT_AUTH_IND_ID:
        {
          sl_wfx_sae_exchange((sl_wfx_ext_auth_ind_t *)msg);
          break;
        }
      }

      sl_wfx_host_free_buffer(msg, SL_WFX_RX_FRAME_BUFFER);
    }
  }
}

/***************************************************************************//**
 * Creates WFX events processing task.
 ******************************************************************************/
void wfx_events_start () {
  RTOS_ERR err;

  OSQCreate(&wifi_events, "wifi events", WFX_EVENTS_NB_MAX, &err);
  /* Check error code.*/
  APP_RTOS_ASSERT_DBG((RTOS_ERR_CODE_GET(err) == RTOS_ERR_NONE), 1);

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
  /* Check error code.*/
  APP_RTOS_ASSERT_DBG((RTOS_ERR_CODE_GET(err) == RTOS_ERR_NONE), 1);
}
