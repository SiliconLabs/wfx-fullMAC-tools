/***************************************************************************//**
 * @file
 * @brief Wi-Fi application init.
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
#include <stdio.h>
#include "os.h"
#include "io.h"
#include "bsp_os.h"
#include "common.h"
#include "em_common.h"
#include "sl_simple_led_instances.h"
#ifdef SL_CATALOG_SIMPLE_BUTTON_PRESENT
#include "sl_simple_button_instances.h"
#include "sl_simple_button_config.h"
#endif
#include "app_webpage.h"
#include "app_wifi_events.h"
#include "app_wifi.h"
#include "sl_wfx_host.h"

#define START_APP_TASK_PRIO              30u
#define START_APP_TASK_STK_SIZE         600u
/// Start task stack.
static CPU_STK start_app_task_stk[START_APP_TASK_STK_SIZE];
/// Start task TCB.
static OS_TCB  start_app_task_tcb;
static void    start_app_task(void *p_arg);

#ifdef SL_CATALOG_SIMPLE_BUTTON_PRESENT
void sl_button_on_change(const sl_button_t *handle)
{
  if ((handle == &sl_button_btn0)
      && (sl_button_get_state(&sl_button_btn0) == SL_SIMPLE_BUTTON_POLARITY)) {
    sl_led_toggle(&sl_led_led0);
  } else if ((handle == &sl_button_btn1)
             && (sl_button_get_state(&sl_button_btn1) == SL_SIMPLE_BUTTON_POLARITY)) {
    sl_led_toggle(&sl_led_led1);
  }
}
#endif
static void start_app_task(void *p_arg)
{
  RTOS_ERR  err;
  PP_UNUSED_PARAM(p_arg); // Prevent compiler warning.

  OSSemPend(&wfx_init_sem, 0, OS_OPT_PEND_BLOCKING, 0, &err);
  // Display the example name
  printf("Multiprotocol Micrium OS Example\r\n");

  app_wifi_events_start();
  webpage_start();

  // Delete the init thread.
  OSTaskDel(0, &err);
}
/**************************************************************************//**
 * Wi-Fi Commissioning application init.
 *****************************************************************************/
void app_wifi_init(void)
{
  RTOS_ERR err;

  OSTaskCreate(&start_app_task_tcb,   // Create the Start Task.
               "Start APP Task",
               start_app_task,
               DEF_NULL,
               START_APP_TASK_PRIO,
               &start_app_task_stk[0],
               (START_APP_TASK_STK_SIZE / 10u),
               START_APP_TASK_STK_SIZE,
               0u,
               0u,
               DEF_NULL,
               (OS_OPT_TASK_STK_CLR),
               &err);

  APP_RTOS_ASSERT_DBG((RTOS_ERR_CODE_GET(err) == RTOS_ERR_NONE), 1);
}
