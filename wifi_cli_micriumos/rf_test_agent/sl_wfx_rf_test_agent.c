/**************************************************************************//**
 * Copyright 2022, Silicon Laboratories Inc.
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

#include "sl_wfx.h"
#include "sl_wfx_rf_test_agent.h"

#define RF_TEST_AGENT_CMD_ARG_0      "wifi"
#define RF_TEST_AGENT_CMD_ARG_1      "test"
#define TX_TEST_KEY_WORD            "write_test_data"
#define RX_TEST_KEY_WORD            "read_rx_stats"
#define FW_VERSION_KEY_WORD         "read_fw_version"
#define DRIVER_VERSION_KEY_WORD     "read_driver_version"
#define TEST_AGENT_VERSION_KEY_WORD "read_agent_version"

static const char *channel_names[] = {
  [0] = "1M  ",
  [1] = "2M  ",
  [2] = "5.5M",
  [3] = "11M ",
  /* Entries 4 and 5 does not exist */
  [6] = "6M  ",
  [7] = "9M  ",
  [8] = "12M ",
  [9] = "18M ",
  [10] = "24M ",
  [11] = "36M ",
  [12] = "48M ",
  [13] = "54M ",
  [14] = "MCS0",
  [15] = "MCS1",
  [16] = "MCS2",
  [17] = "MCS3",
  [18] = "MCS4",
  [19] = "MCS5",
  [20] = "MCS6",
  [21] = "MCS7",
};

static sl_wfx_rx_stats_t *rx_stats = NULL;

sl_status_t sl_wfx_rf_test_agent (sl_wfx_context_t *rf_test_context,
                                  int argc,
                                  char **argv)
{
  sl_status_t status = SL_STATUS_INVALID_PARAMETER;
  if ((argc >= 3)
      && (strncmp(RF_TEST_AGENT_CMD_ARG_0, argv[0], strlen(RF_TEST_AGENT_CMD_ARG_0)) == 0)
      && (argv[0][strlen(RF_TEST_AGENT_CMD_ARG_0)] == '\0')
      && (strncmp(RF_TEST_AGENT_CMD_ARG_1, argv[1], strlen(RF_TEST_AGENT_CMD_ARG_1)) == 0)
            && (argv[1][strlen(RF_TEST_AGENT_CMD_ARG_1)] == '\0')) {

    if ((strncmp(TX_TEST_KEY_WORD, argv[2], strlen(TX_TEST_KEY_WORD)) == 0)
        && (argv[2][strlen(TX_TEST_KEY_WORD)] == '\0')
        && (argc == 4)) {

      /* Start TX RF test */
      status = sl_wfx_send_configuration(argv[3], strlen(argv[3]));
      if (status != SL_STATUS_OK) {
        printf("Send PDS error\r\n");
      } else {
        printf("Send PDS OK\r\n");
      }

    } else if ((strncmp(RX_TEST_KEY_WORD, argv[2], strlen(RX_TEST_KEY_WORD)) == 0)
               && (argv[2][strlen(RX_TEST_KEY_WORD)] == '\0')) {

      /* Return RX stats */
      printf("Timestamp: %ldus\n", rx_stats->date);
      printf("Low power clock: frequency %luHz, external %s\n",
             rx_stats->pwr_clk_freq,
             rx_stats->is_ext_pwr_clk ? "yes" : "no");
      printf("Num. of frames: %ld, PER (x10e4): %ld, Throughput: %ldKbps/s\n",
             rx_stats->nb_rx_frame, rx_stats->per_total, rx_stats->throughput);
      printf(".     Num. of      PER     RSSI      SNR      CFO\n");
      printf("rate   frames  (x10e4)    (dBm)     (dB)    (kHz)\n");

      for (uint8_t i = 0; i < SL_WFX_ARRAY_COUNT(channel_names); i++) {
        if (channel_names[i]) {
          printf("%5s %8ld %8d %8d %8d %8d\n",
                 channel_names[i], rx_stats->nb_rx_by_rate[i],
                 rx_stats->per[i], rx_stats->rssi[i] / 100,
                 rx_stats->snr[i] / 100, rx_stats->cfo[i]);
        }
      }

      status = SL_STATUS_OK;

    } else if ((strncmp(FW_VERSION_KEY_WORD, argv[2], strlen(FW_VERSION_KEY_WORD)) == 0)
               && (argv[2][strlen(FW_VERSION_KEY_WORD)] == '\0')) {

      /* Return FW version */
      printf("%d.%d.%d\r\n",
             rf_test_context->firmware_major,
             rf_test_context->firmware_minor,
             rf_test_context->firmware_build);
      status = SL_STATUS_OK;

    } else if ((strncmp(DRIVER_VERSION_KEY_WORD, argv[2], strlen(DRIVER_VERSION_KEY_WORD)) == 0)
               && (argv[2][strlen(DRIVER_VERSION_KEY_WORD)] == '\0')) {

      /* Return driver version */
      printf("%s\r\n", FMAC_DRIVER_VERSION_STRING);
      status = SL_STATUS_OK;

    }else if ((strncmp(TEST_AGENT_VERSION_KEY_WORD, argv[2], strlen(TEST_AGENT_VERSION_KEY_WORD)) == 0)
              && (argv[2][strlen(TEST_AGENT_VERSION_KEY_WORD)] == '\0')) {

      /* Return RF test agent version */
      printf("%s\r\n", SL_WFX_RF_TEST_AGENT_VERSION_STRING);
      status = SL_STATUS_OK;

    } else {

      printf("Unknown command\r\n");
      status = SL_STATUS_NOT_SUPPORTED;

    }
  }

  return status;
}

sl_status_t sl_wfx_rf_test_agent_init (sl_wfx_rx_stats_t *wfx_rx_stats)
{
  sl_status_t status = SL_STATUS_FAIL;

  if (wfx_rx_stats != NULL) {
    rx_stats = wfx_rx_stats;
    status = SL_STATUS_OK;
  }

  return status;
}
