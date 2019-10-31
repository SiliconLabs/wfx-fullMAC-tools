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
#include "sl_wfx.h"
#include "sl_wfx_test_agent.h"
#include "host_resources.h"

#define TEST_MAX_INPUT_SIZE     500

sl_wfx_context_t rf_test_context;

extern void sl_wfx_process(void);

static void exit_error (void)
{
  printf("Abort !!!\r\n");
  while(1) {;}
}

/***************************************************************************//**
 * @brief  Main function
 ******************************************************************************/
int main (void)
{
  static char rxed_char[TEST_MAX_INPUT_SIZE];
  int res;
  int nb_bytes;
  sl_status_t status;

  // Initialize the host resources
  res = host_init();
  if (res != 0) {
    printf("Failed to init host resources\r\n");
    exit_error();
  }

  // Initialize the WFX chip
  status = sl_wfx_init(&rf_test_context);
  switch (status) {
    case SL_SUCCESS:
      printf("WF200 init successful\r\n");
      break;
    case SL_WIFI_INVALID_KEY:
      printf("Failed to init WF200: Firmware keyset invalid\r\n");
      exit_error();
      break;
    case SL_WIFI_FIRMWARE_DOWNLOAD_TIMEOUT:
      printf("Failed to init WF200: Firmware download timeout\r\n");
      exit_error();
      break;
    case SL_TIMEOUT:
      printf("Failed to init WF200: Poll for value timeout\r\n");
      exit_error();
      break;
    case SL_ERROR:
      printf("Failed to init WF200: Error\r\n");
      exit_error();
      break;
    default :
      printf("Failed to init WF200: Unknown error\r\n");
      exit_error();
  }
  printf("RF test module ready\r\n");

  res = host_dma_start(rxed_char, sizeof(rxed_char));
  if (res != 0) {
    printf("Failed to start the UART DMA reception\r\n");
    exit_error();
  }

  /* Main loop */
  while (true) {

    /* Treat WiFi events */
    sl_wfx_process();

    host_dma_get_nb_bytes_received(&nb_bytes);
    if (0 < nb_bytes) {
      /* Wait for the command end */
      if (rxed_char[nb_bytes - 1] == '\n')
      {
        /* Stop UART reception */
        host_dma_stop();

        /* Process the incoming command */
        rf_test_agent(&rf_test_context, rxed_char);

        /* Clear buffer */
        memset(&rxed_char, 0, sizeof(rxed_char));

        /* Re-enable UARt reception */
        host_dma_start(rxed_char, sizeof(rxed_char));
      }
    }
  }
}
