/**************************************************************************//**
 * Copyright 2018, Silicon Laboratories Inc.
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
#include "string.h"
#include "cmsis_os.h"
#include "stm32f4xx_hal.h"
#include "sl_wfx.h"
#include "sl_wfx_host.h"
#include "sl_wfx_sae.h"
#include "demo_config.h"
#include "wifi_cli.h"

extern UART_HandleTypeDef huart3;

extern scan_result_list_t scan_list[];
extern uint8_t scan_count_web; 
extern bool scan_verbose;

/* Dimensions the buffer into which input characters are placed. */
#define WIFI_CLI_INPUT_BUF_SIZE	50

//wait 1 ms
#define WAIT_TIME_1MS (1UL / portTICK_RATE_MS)

/* Time to waitfor user input */
#define WAIT_TIME_FOR_INPUT (WAIT_TIME_1MS * 5000) 

/*
 * The task that implements the command console processing.
 */
static void prvUARTInputTask(void const * pvParameters );

osThreadId UARTInputTaskHandle;
extern SemaphoreHandle_t uart3Semaphore;
SemaphoreHandle_t stringRcvSemaphore;
SemaphoreHandle_t uartInputSemaphore;

/// Command line input buffer
static char wifi_cli_input_buf[WIFI_CLI_INPUT_BUF_SIZE];
static char stop_waiting = 0;
static const char * const pcNewLine = ( char * ) "\r\n";

static void vUARTInputStop (void)
{
  stop_waiting = 1;
}

static uint32_t wifi_cli_get_input(TickType_t waitTime)
{
  //start receive loop
  xSemaphoreGive (uartInputSemaphore);
  //wait for data 
  if (xSemaphoreTake(stringRcvSemaphore,waitTime) == pdFALSE)
  {
    vUARTInputStop();
    return 0;
  }
  return 1;
}

void wifi_cli_cfg_dialog(void)
{
  bool error = false;
  bool ap_found = false;
  
  printf("\nPress <Enter> within 5 seconds to configure the demo...\n");
  
  if (wifi_cli_get_input(WAIT_TIME_FOR_INPUT) == pdTRUE )
  {
    printf("Select a WiFi mode:\n1. Station\n2. SoftAP\nEnter 1 or 2:\n");
    
    wifi_cli_get_input(portMAX_DELAY);
    
    if (!strcmp(wifi_cli_input_buf, "1")) {
      do {
        error = false;
        printf("\nEnter the SSID of the AP you want to connect:\n");
        wifi_cli_get_input(portMAX_DELAY);
        strncpy(&wlan_ssid[0], wifi_cli_input_buf, sizeof(wlan_ssid));
        
        sl_status_t status;
        sl_wfx_ssid_def_t ssid;
        uint8_t retry = 0;
        
        memcpy(ssid.ssid, wlan_ssid, strlen(wlan_ssid));
        ssid.ssid_length = strlen(wlan_ssid);
        
        do {
          // Reset scan list
          scan_count_web = 0;
          memset(scan_list, 0, sizeof(scan_result_list_t) * SL_WFX_MAX_SCAN_RESULTS);
          scan_verbose = false;
          xSemaphoreTake(wifi_scan_sem, 0);
          
          // perform a scan on every Wi-Fi channel in active mode
          status = sl_wfx_send_scan_command(WFM_SCAN_MODE_ACTIVE, NULL,0,&ssid,1,NULL,0,NULL);
          if ((status == SL_STATUS_OK) || (status == SL_STATUS_WIFI_WARNING))
          {
            xSemaphoreTake(wifi_scan_sem, 5000/portTICK_PERIOD_MS);
            
            // Retrieve the AP information from the scan list, presuming that the
            // first matching SSID is the one (not necessarily true).
            for (uint16_t i = 0; i < scan_count_web; i++) {
              if (strcmp((char *) scan_list[i].ssid_def.ssid, wlan_ssid) == 0) {
                memcpy(&wlan_bssid, scan_list[i].mac, SL_WFX_BSSID_SIZE);
                ap_found = true;
                break;
              }
            }
          }
        } while (!ap_found && retry++ < 3);
        
        if (!ap_found) {
          printf("AP %s not found\r\n", wlan_ssid);
          error = true;
        }
      } while (error);
      
      scan_verbose = true;
      
      printf("\nEnter the Passkey of the AP you want to connect (8-chars min):\n");
      wifi_cli_get_input(portMAX_DELAY);
      strncpy(&wlan_passkey[0], wifi_cli_input_buf, sizeof(wlan_passkey));
      printf("\nSelect a security mode:\n1. Open\n2. WEP\n3. WPA1 or WPA2\n4. WPA2\n5. WPA3\nEnter 1,2,3,4 or 5:\n");
      wifi_cli_get_input(portMAX_DELAY);
      wifi_cli_input_buf[2] = '\0';
      switch (atoi(wifi_cli_input_buf)) {
      case 1:
        wlan_security = WFM_SECURITY_MODE_OPEN;
        break;
      case 2:
        wlan_security = WFM_SECURITY_MODE_WEP;
        break;
      case 3:
        wlan_security = WFM_SECURITY_MODE_WPA2_WPA1_PSK;
        break;
      case 4:
        wlan_security = WFM_SECURITY_MODE_WPA2_PSK;
        break;
      case 5:
        wlan_security = WFM_SECURITY_MODE_WPA3_SAE;
        if (sl_wfx_sae_prepare(&wifi_context.mac_addr_0,
                               (sl_wfx_mac_address_t *)wlan_bssid,
                               (uint8_t *)wlan_passkey,
                               strlen(wlan_passkey)) != SL_STATUS_OK) {
                                 printf("SAE prepare failure\r\n");
                                 return;
                               }          
        break;
      default:
        printf("Unknwon value, default value applied\n");
        break;
      }
      sl_wfx_send_join_command((uint8_t*) wlan_ssid, strlen(wlan_ssid), NULL, 0, wlan_security, 1, 0, (uint8_t*) wlan_passkey, strlen(wlan_passkey), NULL, 0);
    } else if (!strcmp(wifi_cli_input_buf, "2")) {
      printf("\nEnter the SSID of the SoftAP you want to create:\n");
      wifi_cli_get_input(portMAX_DELAY);
      strncpy(&softap_ssid[0], wifi_cli_input_buf, sizeof(softap_ssid));
      printf("\nEnter the Passkey of the SoftAP you want to create (8-chars min):\n");
      wifi_cli_get_input(portMAX_DELAY);
      strncpy(&softap_passkey[0], wifi_cli_input_buf, sizeof(softap_passkey));
      printf("\nSelect a security mode:\n1. Open\n2. WEP\n3. WPA1 or WPA2\n4. WPA2\n5. WPA3\nEnter 1,2,3,4 or 5:\n");
      wifi_cli_get_input(portMAX_DELAY);
      wifi_cli_input_buf[2] = '\0';
      switch (atoi(wifi_cli_input_buf)) {
        case 1:
          softap_security = WFM_SECURITY_MODE_OPEN;
          break;
        case 2:
          softap_security = WFM_SECURITY_MODE_WEP;
          break;
        case 3:
          softap_security = WFM_SECURITY_MODE_WPA2_WPA1_PSK;
          break;
        case 4:
          softap_security = WFM_SECURITY_MODE_WPA2_PSK;
          break;
        case 5:
          softap_security = WFM_SECURITY_MODE_WPA3_SAE;
          break;
        default:
          printf("Unknwon value, default value applied\n");
          break;
      }
      sl_wfx_start_ap_command(softap_channel, (uint8_t*) softap_ssid, strlen(softap_ssid), 0, 0, softap_security, 0, (uint8_t*) softap_passkey, strlen(softap_passkey), NULL, 0, NULL, 0);
    }
  }else{
    sl_wfx_start_ap_command(softap_channel, (uint8_t*) softap_ssid, strlen(softap_ssid), 0, 0, softap_security, 0, (uint8_t*) softap_passkey, strlen(softap_passkey), NULL, 0, NULL, 0);
  }
}

void wifi_cli_start(void)
{
  stringRcvSemaphore = xSemaphoreCreateBinary();
  uartInputSemaphore = xSemaphoreCreateBinary();
  osThreadDef(UARTInputTask, prvUARTInputTask, osPriorityLow, 0, 512);
  UARTInputTaskHandle = osThreadCreate(osThread(UARTInputTask), NULL);
}
/*-----------------------------------------------------------*/

static void prvUARTInputTask(void const * pvParameters )
{
  char cRxedChar, cInputIndex = 0;
  static char cLastInputString[WIFI_CLI_INPUT_BUF_SIZE];
  static char string_output[WIFI_CLI_INPUT_BUF_SIZE];
  static char waiting = 0;
  ( void ) pvParameters;
  
  for( ;; )
  {
    xSemaphoreTake (uartInputSemaphore, portMAX_DELAY);
    waiting = 1;
    while (waiting == 1)
    {
      strcpy(string_output, "");
      /* Only interested in reading one character at a time. */
      if( xSemaphoreTake( uart3Semaphore, portMAX_DELAY ) == pdTRUE )
      {
        HAL_UART_Receive_IT(&huart3, (uint8_t *) &cRxedChar, sizeof( cRxedChar ) );
      }
    
      /*Wait for a character to be received*/
      while ((ulTaskNotifyTake( pdTRUE, WAIT_TIME_1MS ) == pdFALSE) && (stop_waiting == 0));
      
      if (stop_waiting)
      {
          xSemaphoreGive(uart3Semaphore);
          stop_waiting = 0;
          waiting = 0;
      }
      /* Backspace is managed elsewhere */
      if( cRxedChar != '\b' )
      {
        /*Format cRxedChar in a string to use strcat*/
        char cToStr[2];
        cToStr[0] = cRxedChar;
        cToStr[1] = '\0';
        /* Echo the character back. */
        strcat(string_output, ( char * ) (cToStr));
      }
    
      if( cRxedChar == '\r' )
      {


        /* The input command string is complete.  Ensure the previous
        UART transmission has finished before sending any more data.
        This task will be held in the Blocked state while the Tx completes,
        if it has not already done so, so no CPU time will be wasted by
        polling. */
        strcat(string_output, ( char * ) pcNewLine);
      
        /* Pass the received string to the waiting task.  */
      
        xSemaphoreGive(stringRcvSemaphore);
        waiting = 0;
        /* Write the generated string to the UART. */

        /* All the strings generated by the input command have been sent.
        Clear the input	string ready to receive the next command.  Remember
        the command that was just processed first in case it is to be
        processed again. */
        strcpy( ( char * ) cLastInputString, ( char * ) wifi_cli_input_buf );
        cInputIndex = 0;
        memset( wifi_cli_input_buf, 0x00, WIFI_CLI_INPUT_BUF_SIZE );

      }else{
        if ( cRxedChar == '\n' ) {
          /* Ignore the character. */
        }else if ( cRxedChar == '\b' ) {
          /* Backspace was pressed.  Erase the last character in the
          string - if any. */
          if ( cInputIndex > 0 ) {       
            uint8_t* backspace = "\b \b";
            strcat(string_output, ( char * ) backspace);
            cInputIndex--;
            wifi_cli_input_buf[ cInputIndex ] = '\0';
          }
        }
        else
        {
          /* A character was entered.  Add it to the string
          entered so far.  When a \r is entered the complete
          string will be passed to the command interpreter. */
          if (( cRxedChar >= ' ' ) && ( cRxedChar <= '~' )) {
            if ( cInputIndex < WIFI_CLI_INPUT_BUF_SIZE ) {
              wifi_cli_input_buf[ cInputIndex ] = cRxedChar;
              cInputIndex++;
            }
          }
        }
      }
      /*Send back the UART response, wait for the binary semaphore to be available*/
      if (strlen( ( char * ) string_output ) != 0) {
        if ( xSemaphoreTake(uart3Semaphore, portMAX_DELAY ) == pdTRUE) {
          HAL_UART_Transmit_IT(&huart3, (uint8_t *) string_output, strlen((char *) string_output));
        }
      }
    }
  }
}
