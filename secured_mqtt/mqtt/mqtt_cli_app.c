/**************************************************************************//**
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
#include <string.h>
#include <stdio.h>
#include "os.h"

#include "mqtt_cli_app.h"
#include "mqtt_cli_cmd_registration.h"
#include "mqtt_cli_lwip.h"

#include "sl_wfx_host.h"  // to use wfx_init_sem
/*******************************************************************************
 *******************************   DEFINES   ***********************************
 ******************************************************************************/

#define WFX_MQTT_CLI_TASK_PRIO             31u
#define WFX_MQTT_CLI_TASK_STACK_SIZE       800u

// mqtt cli start app task's stack
static CPU_STK wfx_mqtt_cli_task_stack[WFX_MQTT_CLI_TASK_STACK_SIZE];

// mqtt cli start app task's TCB (task control block)
OS_TCB mqtt_cli_task_tcb;
OS_FLAG_GRP mqtt_cli_events;


/*******************************************************************************
 *******************************   Prototypes   ********************************
 ******************************************************************************/
static void mqtt_cli_app_subscribe_cb (void *arg, err_t err);

static void mqtt_cli_app_incoming_publish_cb (void *arg,
                                              const char *topic,
                                              uint32_t tot_len);

static void mqtt_cli_app_incoming_data_cb (void *arg,
                                           const uint8_t *data,
                                           uint16_t len,
                                           uint8_t flags);


/**************************************************************************//**
 * Manage a MQTT subscription result.
 *****************************************************************************/
static void mqtt_cli_app_subscribe_cb (void *arg, err_t err)
{
  (void)arg;
  RTOS_ERR rt_err;
  if (err == ERR_OK) {
      printf("Subscribe success\r\n");
      OSFlagPost(&mqtt_cli_events,
                 SL_WFX_EVENT_MQTT_PUBLISH,
                 OS_OPT_POST_FLAG_SET,
                 &rt_err);
  } else {
      printf("Subscribe error: %d\r\n", err);
  }
}

/**************************************************************************//**
 * Manage a MQTT publishing result.
 *****************************************************************************/
static void mqtt_cli_publish_cb (void *arg, err_t err)
{
  (void)arg;

  if (err != ERR_OK) {
      printf("Publish error: %d\r\n", err);
  } else {
      printf("Publish succeeded\r\n");
  }

}

/**************************************************************************//**
 * Manage a MQTT incoming data.
 *****************************************************************************/
static void mqtt_cli_app_incoming_publish_cb (void *arg,
                                              const char *topic,
                                              uint32_t tot_len)
{
  PP_UNUSED_PARAM(arg);
  PP_UNUSED_PARAM(topic);
  PP_UNUSED_PARAM(tot_len);
}

static void mqtt_cli_app_incoming_data_cb (void *arg,
                                           const uint8_t *data,
                                           uint16_t len,
                                           uint8_t flags)
{
  PP_UNUSED_PARAM(arg);
  PP_UNUSED_PARAM(flags);
  printf("Incoming data: %s, size %d\r\n", data, len);

}

static void mqtt_cli_task(void *p_arg) {
  PP_UNUSED_PARAM(p_arg);
  RTOS_ERR err;
  int res;
  OS_FLAGS  flags = 0;

  // Wait until finishing wfx_driver initialization
  OSSemPend(&wfx_init_sem,
            0,
            OS_OPT_PEND_BLOCKING,
            NULL,
            &err);

  OSFlagCreate(&mqtt_cli_events, "mqtt cli events", 0, &err);
  printf("MQTT CLI Example - Version 0.9\r\n");

  // CLI cmd, init mqtt params task
  mqtt_cli_cmd_registration_init();

  while(1) {

      //maybe need to wait for task registration init done
      flags = OSFlagPend(&mqtt_cli_events,
                         SL_WFX_EVENT_MQTT_CONNECT |
                         SL_WFX_EVENT_MQTT_PUBLISH |
                         SL_WFX_EVENT_MQTT_SAVE,
                         0,
                         OS_OPT_PEND_FLAG_SET_ANY |
                         OS_OPT_PEND_BLOCKING |
                         OS_OPT_PEND_FLAG_CONSUME,
                         0,
                         &err);

      if (flags & SL_WFX_EVENT_MQTT_CONNECT) {
          // check wifi connection before connecting, coding here
          res = mqtt_cli_lwip_initialization();
          if (res == 0 ) {
            res = mqtt_cli_lwip_connection();
            if (res == 0) {
              mqtt_cli_lwip_subscribe(mqtt_subscribe_topic,
                                      MQTT_QOS,
                                      mqtt_cli_app_subscribe_cb,
                                      mqtt_cli_app_incoming_publish_cb,
                                      mqtt_cli_app_incoming_data_cb);
            } else {
              printf("MQTT not connect....\r\n");
            }
          }
          else {
              printf("MQTT initialization error\r\n");
          }
      }

      if(flags & SL_WFX_EVENT_MQTT_PUBLISH) {
          mqtt_cli_lwip_publish(mqtt_publish_topic, mqtt_msg_publish,
                                MQTT_QOS,
                                MQTT_RETAIN ,
                                mqtt_cli_publish_cb);
      }
  }

}

/*****/
void mqtt_cli_app_init(void) {

  RTOS_ERR err;
  OSTaskCreate(&mqtt_cli_task_tcb,
               "mqtt cli app task",
               mqtt_cli_task,
               DEF_NULL,
               WFX_MQTT_CLI_TASK_PRIO,
               &wfx_mqtt_cli_task_stack[0],
               (WFX_MQTT_CLI_TASK_STACK_SIZE / 10u),
               WFX_MQTT_CLI_TASK_STACK_SIZE,
               0u,
               0u,
               DEF_NULL,
               (OS_OPT_TASK_STK_CLR),
               &err);

  // Check err code
  APP_RTOS_ASSERT_DBG((RTOS_ERR_CODE_GET(err) == RTOS_ERR_NONE), 1);

}

