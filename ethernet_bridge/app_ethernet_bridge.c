/***************************************************************************//**
 * @file  app_ethernet_bridge.c
 * @brief Main example of wi-fi ethernet bridge
 *******************************************************************************
 * # License
 * <b>Copyright 2022 Silicon Laboratories Inc. www.silabs.com</b>
 *******************************************************************************
 *
 * The licensor of this software is Silicon Laboratories Inc. Your use of this
 * software is governed by the terms of Silicon Labs Master Software License
 * Agreement (MSLA) available at
 * www.silabs.com/about-us/legal/master-software-license-agreement.
 * The software is governed by the sections of the MSLA applicable to Micrium
 * Software.
 *
 ******************************************************************************/
#include  "app_ethernet_bridge.h"
#include  "core_init/ex_net_core_init.h"
#include  "bridge.h"

/*******************************************************************************
 *                        MAIN START TASK CONFIGURATION                        *
 *******************************************************************************/
#define  MAIN_START_TASK_PRIO              30u
#define  MAIN_START_TASK_STK_SIZE         1024u

#ifdef SL_WFX_USE_SECURE_LINK
extern void wfx_securelink_task_start(void);
#endif

/// Main start task stack.
static  CPU_STK  main_start_task_stk[MAIN_START_TASK_STK_SIZE];
/// Main start task TCB.
static  OS_TCB   main_start_task_tcb;
static  void     main_start_task (void  *p_arg);

/*******************************************************************************
 *                        WIFI START SOFTAP TASK CONFIGURATION                 *
 *******************************************************************************/
#define WIFI_TASK_PRIO                    22u
#define WIFI_TASK_STK_SIZE                800u

char softap_ssid[32+1]                  = SOFTAP_SSID_DEFAULT;
char softap_passkey[64+1]               = SOFTAP_PASSKEY_DEFAULT;
sl_wfx_password_t softap_pmk            = {0};
sl_wfx_security_mode_t softap_security  = SOFTAP_SECURITY_DEFAULT;
uint8_t softap_channel                  = SOFTAP_CHANNEL_DEFAULT;

/// WiFi task stack
static CPU_STK wifi_task_stk[WIFI_TASK_STK_SIZE];
/// WiFi task TCB
static OS_TCB wifi_task_tcb;

/**************************************************************************//**
 * @func:  sl_wfx_host_process_event()
 * @brief: This function is called by FMAC Driver to process events
 *****************************************************************************/
sl_status_t sl_wfx_host_process_event(sl_wfx_generic_message_t *event_payload)
{
  switch (event_payload->header.id) {
      /******** INDICATION ********/
      case SL_WFX_RECEIVED_IND_ID:
      {
        sl_wfx_received_ind_t* ethernet_frame = (sl_wfx_received_ind_t*) event_payload;
        if ( ethernet_frame->body.frame_type == 0 ) {
          sl_wfx_host_received_frame_callback(ethernet_frame);
        }
        break;
      }
      case SL_WFX_START_AP_IND_ID:
      {
        sl_wfx_start_ap_ind_t  *start_ap = (sl_wfx_start_ap_ind_t*) event_payload;
        if (start_ap->body.status == 0) {
            printf("AP started\r\n");
            printf("Join the AP with SSID: %s\r\n", softap_ssid);
            sl_wfx_context->state |= SL_WFX_AP_INTERFACE_UP;
           
          } else {
            printf("AP start failed\r\n");         
          }
        break;
      }
      case SL_WFX_AP_CLIENT_CONNECTED_IND_ID:
      {
        sl_wfx_ap_client_connected_ind_t  *ap_client_connected = (sl_wfx_ap_client_connected_ind_t*) event_payload;
        printf("Client connected, "
                "MAC: %02X:%02X:%02X:%02X:%02X:%02X\r\n",
                 ap_client_connected->body.mac[0],
                 ap_client_connected->body.mac[1],
                 ap_client_connected->body.mac[2],
                 ap_client_connected->body.mac[3],
                 ap_client_connected->body.mac[4],
                 ap_client_connected->body.mac[5]);
        break;
      }
      case SL_WFX_AP_CLIENT_REJECTED_IND_ID:
      {
        sl_wfx_ap_client_rejected_ind_t  *ap_client_rejected = (sl_wfx_ap_client_rejected_ind_t*) event_payload;
        printf("Client rejected, reason: %d, "
                "MAC: %02X:%02X:%02X:%02X:%02X:%02X\r\n",
                 ap_client_rejected->body.reason,
                 ap_client_rejected->body.mac[0],
                 ap_client_rejected->body.mac[1],
                 ap_client_rejected->body.mac[2],
                 ap_client_rejected->body.mac[3],
                 ap_client_rejected->body.mac[4],
                 ap_client_rejected->body.mac[5]);
        break;
      }
      case SL_WFX_AP_CLIENT_DISCONNECTED_IND_ID:
      {
        sl_wfx_ap_client_disconnected_ind_t  *ap_client_disconnected = (sl_wfx_ap_client_disconnected_ind_t*) event_payload;
        printf("Client disconnected, reason: %d, "
               "MAC: %02X:%02X:%02X:%02X:%02X:%02X\r\n",
               ap_client_disconnected->body.reason,
               ap_client_disconnected->body.mac[0],
               ap_client_disconnected->body.mac[1],
               ap_client_disconnected->body.mac[2],
               ap_client_disconnected->body.mac[3],
               ap_client_disconnected->body.mac[4],
               ap_client_disconnected->body.mac[5]);
        break;
      }
      default:
        break;
    }

  return SL_STATUS_OK;
}


/***************************************************************************//**
 * Start Wi-Fi SoftAP function
 * @param p_arg Unused parameter.
 ******************************************************************************/
static void start_softap_task(void *p_arg)
{
  RTOS_ERR err;
  sl_status_t result;
  PP_UNUSED_PARAM(p_arg);

  // Start the SoftAP with the default configuration
  result = sl_wfx_start_ap_command(softap_channel,
                                    (uint8_t*)softap_ssid,
                                    strlen(softap_ssid),
                                    0,
                                    0,
                                    softap_security,
                                    0,
                                    (uint8_t*)softap_passkey,
                                    strlen(softap_passkey),
                                    NULL,
                                    0,
                                    NULL,
                                    0);
  if (result == SL_STATUS_OK) {
      /* Delay for 1000ms */
      OSTimeDly(1000, OS_OPT_TIME_DLY, &err);
      if (sl_wfx_set_unicast_filter(0) != SL_STATUS_OK) {
          printf("Set sl_wfx_set_unicast_filter fail\n");
      }
  } else {
      printf("Failed to start SoftAP, err = %lu \r\n", result);
  }

}

/**************************************************************************//**
 * Start SoftAP task
 *****************************************************************************/
void wifi_start_softap(void)
{
  RTOS_ERR err;
  OSTaskCreate(&wifi_task_tcb,
               "WiFi Task",
               start_softap_task,
               DEF_NULL,
               WIFI_TASK_PRIO,
               &wifi_task_stk[0],
               (WIFI_TASK_STK_SIZE / 10u),
               WIFI_TASK_STK_SIZE,
               0u,
               0u,
               DEF_NULL,
               (OS_OPT_TASK_STK_CLR),
               &err);
  /*   Check error code.                                  */
  APP_RTOS_ASSERT_DBG((RTOS_ERR_CODE_GET(err) == RTOS_ERR_NONE), 1);
  OSTaskDel(0, &err);
}


/**************************************************************************//**
 * @func: main_start_task()
 * @param p_arg Argument passed from task creation. Unused.
 * @brief:
 *  This is the main task to initialize net core, wf200 bus communication, secure
 *  link, start if & start softAP
 *****************************************************************************/
static void main_start_task(void *p_arg)
{
  RTOS_ERR  err;
  PP_UNUSED_PARAM(p_arg);        /* Prevent compiler warning.                 */

  OSSemPend(&wfx_init_sem, 0, OS_OPT_PEND_BLOCKING, 0, &err);
  printf("\033\143");
  printf("\033[3J");
  printf("Ethernet Bridge Micrium OS Example\r\n");

  Ex_Net_CoreInit();             /* Call Network module initialization example*/
  sl_wfx_task_start();           /* Start WF200 communication task            */

#ifdef SL_WFX_USE_SECURE_LINK
  wfx_securelink_task_start();   /* Start secure link key renegotiation task  */
#endif //SL_WFX_USE_SECURE_LINK

  Ex_Net_CoreStartIF();          /* Call network interface start example.     */
  wifi_start_softap();           /* Start SoftAP task                         */

  OSTaskDel(0, &err);
}


/**************************************************************************//**
 * Main function
 *****************************************************************************/
void app_ethernet_bridge_init(void)
{
  RTOS_ERR  err;

  // Create the Main Start Task.
  OSTaskCreate(&main_start_task_tcb,
               "Main Start Task",
               main_start_task,
               DEF_NULL,
               MAIN_START_TASK_PRIO,
               &main_start_task_stk[0],
               (MAIN_START_TASK_STK_SIZE / 10u),
               MAIN_START_TASK_STK_SIZE,
               0u,
               0u,
               DEF_NULL,
               (OS_OPT_TASK_STK_CLR),
               &err);

  /* Check err code */
  APP_RTOS_ASSERT_DBG((RTOS_ERR_CODE_GET(err) == RTOS_ERR_NONE), 1);
  OSTaskDel(0, &err);
}
