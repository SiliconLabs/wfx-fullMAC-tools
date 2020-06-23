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

#include "traffic_agent.h"
#include "sl_wfx_cli_generic.h"
#include "sl_wfx_cli_common.h"

static int tg_cmd_cb (int argc,
                      char **argv,
                      char *output_buf,
                      uint32_t output_buf_len)
{
  int res;
  tg_cmd_t tg_cmd;
  tg_resp_t tg_resp;

  // Parse the command & arguments
  res = tg_cmd_str2struct(argc, argv, &tg_cmd);
  if (res == 0) {
    // Valid command, execute it
    tg_exec_cmd(&tg_cmd, &tg_resp);

    if (tg_cmd_is_inhib() && tg_resp.err != TG_ERR_NONE){
      tg_cmd_desinhib();
    }

    while(tg_cmd_is_inhib()) {
      tg_delay_ms(100);
    }

    // Convert the Traffic Agent return into a string
    tg_resp_struct2str(&tg_resp, output_buf, output_buf_len);
  } else {
    snprintf(output_buf,
             output_buf_len,
             "Command error\r\n");
  }

  return 0;
}

static const sl_wfx_cli_generic_command_t tg_cmd =
{
  "tg",
  "tg: Send a command to the Traffic Agent\r\n"
  "\tUsage: tg <cmd> [cmd_args]\r\n",
  tg_cmd_cb,
  -1
};

int sl_wfx_cli_traffic_agent_init (void)
{
  int res;

  // Add the Traffic Agent commands in the CLI
  res  = sl_wfx_cli_generic_register_cmd(&tg_cmd);

  return res;
}
