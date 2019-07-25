/***************************************************************************//**
 * @file
 * @brief WiFi CLI based on Micrium OS Shell via UART.
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

#include "wifi_cli.h"
#include "lwip_micriumos.h"
#include "lwip/netif.h"
#include "lwip/ip_addr.h"
#include "lwip/apps/lwiperf.h"

// -----------------------------------------------------------------------------
// Local defines

#define  WIFI_CLI_TASK_STK_SIZE        2048u
#define  WIFI_CLI_TASK_PRIO              25u
#define  WIFI_CLI_INPUT_BUF_SIZE        128u


// -----------------------------------------------------------------------------
// Local global variables

// Shell Task Stack


static char wifiCliInputBuf[WIFI_CLI_INPUT_BUF_SIZE];






/***************************************************************************//**
 * @brief
 *   Get text input from user.
 *
 * @param buf
 *   Buffer to hold the input string.
 *
 * @param echo
 *   Bool indicating whether the inputed text should be echoed to the user
 ******************************************************************************/
static void wifiCliGetInput(char *buf, CPU_BOOLEAN echo)
{
  RTOS_ERR err;
  int c;
  size_t i;

  Mem_Set(buf, '\0', WIFI_CLI_INPUT_BUF_SIZE); // Clear previous input

  for (i = 0; i < WIFI_CLI_INPUT_BUF_SIZE - 1; i++) {
    while ((c = RETARGET_ReadChar()) < 0) {    // Wait for valid input
        OSTimeDly(100,
                  OS_OPT_TIME_DLY,
                 &err);
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
        if (echo){
          printf("\n");
        }
        break;
      } else {
        i--;
        continue;
      }
    }
    if (echo){
      RETARGET_WriteChar(c); // Echo to terminal
    }
    buf[i] = c;
  }

  buf[i] = '\0';
}

/***************************************************************************//**
 * @brief
 *   Get text input from user to configure the demo.
 ******************************************************************************/
void WIFI_CLI_CfgDialog(void)
{
    RTOS_ERR err;
    CPU_INT08U ctr;
    int c;

    printf("\nPress <Enter> within 5 seconds to configure the demo...\n");
    OSTimeDly( 100,                                                 /*   100 OS Ticks                                      */
               OS_OPT_TIME_DLY,                                     /*   from now.                                          */
               &err);
    ctr = 0;
    while ((c = RETARGET_ReadChar()) < 0) {    // Wait for valid input
        OSTimeDly(100,
                  OS_OPT_TIME_DLY,
                 &err);
        ctr++;
        if (ctr > 50) {
            break;
        }
    }

    if (c == '\r' || c == '\n') {
        printf ("Select a WiFi mode:\n1. Station\n2. SoftAP\nEnter 1 or 2:\n");

        wifiCliGetInput(wifiCliInputBuf, 1);

        if (!Str_Cmp(wifiCliInputBuf, "1")) {
        	soft_ap_mode = 0;
        	use_dhcp_client = 1;
            printf ("Enter the SSID of the AP you want to connect:\n");
            wifiCliGetInput(wifiCliInputBuf, 1);
            strncpy(&wlan_ssid[0], wifiCliInputBuf, sizeof(wlan_ssid));
            printf ("Enter the Passkey of the AP you want to connect (8-chars min):\n");
            wifiCliGetInput(wifiCliInputBuf, 1);
            strncpy(&wlan_passkey[0], wifiCliInputBuf, sizeof(wlan_passkey));
        } else if (!Str_Cmp(wifiCliInputBuf, "2")) {
        	soft_ap_mode = 1;
        	use_dhcp_client = 0;
            printf ("Enter the SSID of the SoftAP you want to create:\n");
            wifiCliGetInput(wifiCliInputBuf, 1);
            strncpy(&softap_ssid[0], wifiCliInputBuf, sizeof(softap_ssid));
            printf ("Enter the Passkey of the SoftAP you want to create (8-chars min):\n");
            wifiCliGetInput(wifiCliInputBuf, 1);
            strncpy(&softap_passkey[0], wifiCliInputBuf, sizeof(softap_passkey));
        }
    }
}
