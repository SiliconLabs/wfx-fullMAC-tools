/***************************************************************************//**
 * @file
 * @brief WiFi demo configuration CLI
 *******************************************************************************
 * # License
 * <b>Copyright 2019 Silicon Laboratories Inc. www.silabs.com</b>
 *******************************************************************************
 *
 * The licensor of this software is Silicon Laboratories Inc. Your use of this
 * software is governed by the terms of Silicon Labs Master Software License
 * Agreement (MSLA) available at
 * www.silabs.com/about-us/legal/master-software-license-agreement. This
 * software is distributed to you in Source Code format and is governed by the
 * sections of the MSLA applicable to Source Code.
 *
 ******************************************************************************/

#include <common/include/lib_mem.h>
#include <common/include/rtos_err.h>
#include <common/include/rtos_utils.h>
#include <common/include/shell.h>
#include <cpu/include/cpu.h>
#include <kernel/include/os.h>
#include <stdio.h>

#include "em_chip.h"
#include "em_emu.h"
#include "retargetserial.h"

#include <bsp_os.h>

#include "demo_config.h"

#include "wifi_cli.h"

#include "lwip/netif.h"
#include "lwip/ip_addr.h"
#include "lwip/apps/lwiperf.h"

#include "sl_status.h"
#include "sl_wfx.h"
#include "wfx_task.h"
#include "wfx_host.h"

#define  WIFI_CLI_TASK_STK_SIZE        2048u
#define  WIFI_CLI_TASK_PRIO              25u
#define  WIFI_CLI_INPUT_BUF_SIZE        128u

/// Command line input buffer
static char wifi_cli_input_buf[WIFI_CLI_INPUT_BUF_SIZE];

/**************************************************************************//**
 * Get text input from user with a timeout.
 *
 * @param buf Buffer to hold the input string.
 * @param size Size of the buffer holding the input string.
 * @param timeout_sec Time in seconds to wait for an input (0: infinite timeout).
 * @param echo Enable or disable text echo to user.
 *****************************************************************************/
void wifi_cli_get_input_tmo(char *buf, uint32_t size, uint8_t timeout_sec, CPU_BOOLEAN echo)
{
  RTOS_ERR err;
  int c;
  size_t i;
  uint32_t ctr = timeout_sec * 1000;

  Mem_Set(buf, '\0', size); // Clear previous input

  for (i = 0; i < size - 1; i++) {
    while ((c = RETARGET_ReadChar()) < 0) {    // Wait for valid input
      OSTimeDly(100,
                OS_OPT_TIME_DLY,
                &err);
      ctr = ctr - 100;
      if (ctr == 0) {
        // Timeout reached, exit
        return;
      }
    }

    if (c == ASCII_CHAR_DELETE || c ==  0x08) { // User entered backspace
      if (i) {
        RETARGET_WriteChar('\b');
        RETARGET_WriteChar(' ');
        RETARGET_WriteChar('\b');
        buf[--i] = '\0';
      }

      i--;
      continue;
    } else if (c == '\r' || c == '\n') {
      if (i) {
        if (echo) {
          printf("\n");
        }
        break;
      }
    }
    if (echo) {
      RETARGET_WriteChar(c); // Echo to terminal
    }
    buf[i] = c;
  }

  buf[i] = '\0';
}

/**************************************************************************//**
 * Get text input from user.
 *
 * @param buf Buffer to hold the input string.
 * @param size Size of the buffer holding the input string.
 * @param echo Enable or disable text echo to user.
 *****************************************************************************/
void wifi_cli_get_input(char *buf, uint32_t size, CPU_BOOLEAN echo)
{
  wifi_cli_get_input_tmo(buf, size, 0, echo);
}

/**************************************************************************//**
 * WiFi configuration dialog via UART.
 *****************************************************************************/
void wifi_cli_cfg_dialog(void)
{
  printf("\nPress <Enter> within 5 seconds to configure the demo...\n");
  wifi_cli_get_input_tmo(wifi_cli_input_buf, sizeof(wifi_cli_input_buf), 5, 1);

  if (wifi_cli_input_buf[0] == '\r' || wifi_cli_input_buf[0] == '\n') {
    printf("Select a WiFi mode:\n1. Station\n2. SoftAP\nEnter 1 or 2:\n");

    wifi_cli_get_input(wifi_cli_input_buf, sizeof(wifi_cli_input_buf), 1);
    lwip_enable_dhcp_client();
    if (!Str_Cmp(wifi_cli_input_buf, "1")) {
      printf("Enter the SSID of the AP you want to connect:\n");
      wifi_cli_get_input(wifi_cli_input_buf, sizeof(wifi_cli_input_buf), 1);
      strncpy(&wlan_ssid[0], wifi_cli_input_buf, sizeof(wlan_ssid));
      printf("Enter the Passkey of the AP you want to connect (8-chars min):\n");
      wifi_cli_get_input(wifi_cli_input_buf, sizeof(wifi_cli_input_buf), 1);
      strncpy(&wlan_passkey[0], wifi_cli_input_buf, sizeof(wlan_passkey));
      printf("Select a security mode:\n1. Open\n2. WEP\n3. WPA1 or WPA2\n4. WPA2\nEnter 1,2,3 or 4:\n");
      wifi_cli_get_input(wifi_cli_input_buf, sizeof(wifi_cli_input_buf), 1);
      if (!Str_Cmp(wifi_cli_input_buf, "1")) {
    	  wlan_security = WFM_SECURITY_MODE_OPEN;
      } else if (!Str_Cmp(wifi_cli_input_buf, "2")) {
    	  wlan_security = WFM_SECURITY_MODE_WEP;
      }
      else if (!Str_Cmp(wifi_cli_input_buf, "3")) {
    	  wlan_security = WFM_SECURITY_MODE_WPA2_WPA1_PSK;
      }
      else if (!Str_Cmp(wifi_cli_input_buf, "4")) {
    	  wlan_security = WFM_SECURITY_MODE_WPA2_PSK;
      }
      sl_wfx_send_join_command((uint8_t*) wlan_ssid, strlen(wlan_ssid), NULL, 0, wlan_security, 1, 0, (uint8_t*) wlan_passkey, strlen(wlan_passkey), NULL, 0);
    } else if (!Str_Cmp(wifi_cli_input_buf, "2")) {
      printf("Enter the SSID of the SoftAP you want to create:\n");
      wifi_cli_get_input(wifi_cli_input_buf, sizeof(wifi_cli_input_buf), 1);
      strncpy(&softap_ssid[0], wifi_cli_input_buf, sizeof(softap_ssid));
      printf("Enter the Passkey of the SoftAP you want to create (8-chars min):\n");
      wifi_cli_get_input(wifi_cli_input_buf, sizeof(wifi_cli_input_buf), 1);
      strncpy(&softap_passkey[0], wifi_cli_input_buf, sizeof(softap_passkey));
      printf("Select a security mode:\n1. Open\n2. WEP\n3. WPA1 or WPA2\n4. WPA2\nEnter 1,2,3 or 4:\n");
      wifi_cli_get_input(wifi_cli_input_buf, sizeof(wifi_cli_input_buf), 1);
      if (!Str_Cmp(wifi_cli_input_buf, "1")) {
          softap_security = WFM_SECURITY_MODE_OPEN;
      } else if (!Str_Cmp(wifi_cli_input_buf, "2")) {
          softap_security = WFM_SECURITY_MODE_WEP;
      }
      else if (!Str_Cmp(wifi_cli_input_buf, "3")) {
          softap_security = WFM_SECURITY_MODE_WPA2_WPA1_PSK;
      }
      else if (!Str_Cmp(wifi_cli_input_buf, "4")) {
          softap_security = WFM_SECURITY_MODE_WPA2_PSK;
      }
      sl_wfx_start_ap_command(softap_channel, (uint8_t*) softap_ssid, strlen(softap_ssid), 0, 0, softap_security, 0, (uint8_t*) softap_passkey, strlen(softap_passkey), NULL, 0, NULL, 0);
    }
  } else {
    sl_wfx_start_ap_command(softap_channel, (uint8_t*) softap_ssid, strlen(softap_ssid), 0, 0, softap_security, 0, (uint8_t*) softap_passkey, strlen(softap_passkey), NULL, 0, NULL, 0);
  }
}
