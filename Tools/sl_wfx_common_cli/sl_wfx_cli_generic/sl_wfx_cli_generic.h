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
#ifndef SL_WFX_CLI_GENERIC_H
#define SL_WFX_CLI_GENERIC_H

#include <stdint.h>

#ifdef MICRIUMOS
#include "dmadrv.h"
#endif

// Some helper defines to get a version string
#define SL_WFX_CLI_GEN_VERSTR2(x) #x
#define SL_WFX_CLI_GEN_VERSTR(x)  SL_WFX_CLI_GEN_VERSTR2(x)

// Provides the version of a module
#define SL_WFX_CLI_GEN_MODULE_VERSION(major, minor, rev)   (  (major) << 16 \
                                                            | (minor) << 8 \
                                                            | (rev))

// Provides the version of a module as string
#define SL_WFX_CLI_GEN_MODULE_VERSION_STRING(major, minor, rev)     SL_WFX_CLI_GEN_VERSTR(major) "." \
                                                                    SL_WFX_CLI_GEN_VERSTR(minor) "." \
                                                                    SL_WFX_CLI_GEN_VERSTR(rev)

#ifndef SL_WFX_CLI_GEN_NB_MAX_MODULES
  #define SL_WFX_CLI_GEN_NB_MAX_MODULES   10u
#endif

#ifndef SL_WFX_CLI_GEN_NB_MAX_CMD
  #define SL_WFX_CLI_GEN_NB_MAX_CMD       35u
#endif

#ifndef SL_WFX_CLI_CMD_NAME_MAX_LEN
  #define SL_WFX_CLI_CMD_NAME_MAX_LEN     20u
#endif

#define SL_WFX_CLI_ERROR_NONE               0
#define SL_WFX_CLI_ERROR                  (-1)
#define SL_WFX_CLI_ERROR_NOT_INIT         (-2)
#define SL_WFX_CLI_ERROR_TIMEOUT          (-3)
#define SL_WFX_CLI_ERROR_NB_MAX_MODULES   (-4)
#define SL_WFX_CLI_ERROR_NB_MAX_COMMANDS  (-5)

typedef int (*sl_wfx_cli_generic_command_callback_t)(int argc,
                                                     char **argv,
                                                     char *output_buf,
                                                     uint32_t output_buf_len);

typedef struct sl_wfx_cli_generic_command_s {
  char *name;
  char *help;
  sl_wfx_cli_generic_command_callback_t callback;
  int nb_param;
} sl_wfx_cli_generic_command_t;

typedef struct sl_wfx_cli_generic_functions_s {
  int (*init)(void *config);
  int (*register_command)(sl_wfx_cli_generic_command_t const *cmd);
  int (*config_wait)(uint32_t event_flag);
  int (*wait)(uint32_t timeout_ms);
  int (*resume)(void);
} sl_wfx_cli_generic_functions_t;

#ifdef MICRIUMOS
typedef struct sl_wfx_cli_generic_micriumos_config_s {
  DMADRV_PeripheralSignal_t dma_peripheral_signal;
  uint8_t echo;
} sl_wfx_cli_generic_micriumos_config_t;
#endif

// Shell configuration
extern const char welcome[];
extern const char prompt[10];
extern const char newline[];

// Generic error messages
extern const char invalid_command_msg[];    // Error during the command parsing, the input command is wrongly formatted
extern const char command_error_msg[];      // Error during the command processing
extern const char command_timeout_msg[];    // The command processing timed out
extern const char missing_parameter_msg[];  // A parameter required by a module has not been registered by the application
extern const char alloc_error_msg[];        // Dynamic allocation error

#ifdef __cplusplus
extern "C"
{
#endif

int sl_wfx_cli_generic_init(void *config);
int sl_wfx_cli_generic_register_cmd(sl_wfx_cli_generic_command_t const *cmd);
int sl_wfx_cli_generic_register_module(char *name, char *version);
int sl_wfx_cli_generic_output(char *buffer);
int sl_wfx_cli_generic_config_wait(uint32_t event_flag);
int sl_wfx_cli_generic_wait(uint32_t timeout_ms);
int sl_wfx_cli_generic_resume(void);

#ifdef __cplusplus
} /*extern "C" */
#endif

#endif //SL_WFX_CLI_GENERIC_H
