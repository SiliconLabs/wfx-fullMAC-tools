/***************************************************************************//**
 * @file
 * @brief Console management based on Micrium OS + Shell
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

/**************************************************************************//**
 * Get text input from user with a timeout.
 *
 * @param buf Buffer to hold the input string.
 * @param size Size of the buffer holding the input string.
 * @param timeout_sec Time in seconds to wait for an input (0: infinite timeout).
 * @param echo Enable or disable text echo to user.
 *****************************************************************************/
void console_get_input_tmo(char *buf, uint32_t size, uint8_t timeout_sec, CPU_BOOLEAN echo)
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
void console_get_input(char *buf, uint32_t size, CPU_BOOLEAN echo)
{
  console_get_input_tmo(buf, size, 0, echo);
}
