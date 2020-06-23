/**************************************************************************//**
 * Copyright 2019, Silicon Laboratories Inc.
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

#include <stdlib.h>
#include <string.h>
#include "sl_wfx_rf_test_agent.h"
#include "sl_wfx_cli_generic.h"

static sl_wfx_context_t *wfx_ctx = NULL;

static int rf_test_agent_cmd_cb (int argc,
                                 char **argv,
                                 char *output_buf,
                                 uint32_t output_buf_len)
{
  sl_status_t status;
  int res = -1;

  status = sl_wfx_rf_test_agent(wfx_ctx, argc, argv);
  if (status == SL_STATUS_OK) {
    res = 0;
  } // else let the generic CLI display the error message

  return res;
}

static const sl_wfx_cli_generic_command_t rf_test_agent_cmd =
{
  "wfx_test_agent",
  "wfx_test_agent           : Send a command to the RF Test Agent\r\n"
  "                         Usage: wfx_test_agent <cmd> [cmd_args]\r\n",
  rf_test_agent_cmd_cb,
  -1
};

int sl_wfx_cli_rf_test_agent_init (sl_wfx_context_t *wfx_context,
                                   sl_wfx_rx_stats_t *wfx_rx_stats)
{
  int res = -1;

  if ((wfx_context != NULL) && (wfx_rx_stats != NULL))  {
    // Initialize the resources needed by the commands
    wfx_ctx = wfx_context;
    sl_wfx_rf_test_agent_init(wfx_rx_stats);

    // Add RF Test Agent commands to the CLI
    res = sl_wfx_cli_generic_register_cmd(&rf_test_agent_cmd);

    if (res == 0) {
      // Register the RF Test Agent CLI module
      res = sl_wfx_cli_generic_register_module("RF Test Agent",
                                               SL_WFX_RF_TEST_AGENT_VERSION_STRING);
    }
  }

  return res;
}
