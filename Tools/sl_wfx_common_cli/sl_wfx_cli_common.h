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

#ifndef SL_WFX_CLI_COMMON_H_
#define SL_WFX_CLI_COMMON_H_

#include "sl_wfx.h"

#ifndef SL_WFX_CLI_WIFI_NB_MAX_CLIENTS
  #define SL_WFX_CLI_WIFI_NB_MAX_CLIENTS       10
#endif

#ifndef SL_WFX_CLI_PARAM_NB_MAX_PARAMS
  #define SL_WFX_CLI_PARAM_NB_MAX_PARAMS       20
#endif

// Parameter right mask
#define SL_WFX_CLI_PARAM_SET_RIGHT       (1<<0)
#define SL_WFX_CLI_PARAM_GET_RIGHT       (1<<1)

typedef int (*sl_wfx_cli_param_custom_get_func_t)(char *param_name,
                                                  void *param_addr,
                                                  char *output_buf,
                                                  uint32_t output_buf_len);

typedef int (*sl_wfx_cli_param_custom_set_func_t)(char *param_name,
                                                  void *param_addr,
                                                  char *new_value);

typedef enum sl_wfx_cli_param_type_s {
  SL_WFX_CLI_PARAM_TYPE_INTEGER = 0,
  SL_WFX_CLI_PARAM_TYPE_UNSIGNED_INTEGER,
  SL_WFX_CLI_PARAM_TYPE_HEXADECIMAL,
  SL_WFX_CLI_PARAM_TYPE_ARRAY_INTEGER,
  SL_WFX_CLI_PARAM_TYPE_ARRAY_UNSIGNED_INTEGER,
  SL_WFX_CLI_PARAM_TYPE_ARRAY_HEXADECIMAL,
  SL_WFX_CLI_PARAM_TYPE_STRING,
  SL_WFX_CLI_PARAM_TYPE_CUSTOM
} sl_wfx_cli_param_type_t;

#ifdef __cplusplus
extern "C"
{
#endif

int sl_wfx_cli_ip_init(void);

int sl_wfx_cli_param_init(void);
int sl_wfx_cli_param_register(char *name,
                              void *addr,
                              uint8_t size,
                              uint8_t rights,
                              sl_wfx_cli_param_type_t type,
                              char *description,
                              sl_wfx_cli_param_custom_get_func_t get_func,
                              sl_wfx_cli_param_custom_set_func_t set_func);
void *sl_wfx_cli_param_get_addr(char *param_name);

int sl_wfx_cli_rf_test_agent_init(sl_wfx_context_t *wfx_context,
                                  sl_wfx_rx_stats_t *wfx_rx_stats);

int sl_wfx_cli_secure_link_init(void);

int sl_wfx_cli_traffic_agent_init(void);

int sl_wfx_cli_wifi_init(sl_wfx_context_t *wfx_ctx);
int sl_wfx_cli_wifi_add_client(uint8_t *mac);
int sl_wfx_cli_wifi_remove_client(uint8_t *mac);

#ifdef __cplusplus
} /*extern "C" */
#endif

#endif //SL_WFX_CLI_COMMON_H_
