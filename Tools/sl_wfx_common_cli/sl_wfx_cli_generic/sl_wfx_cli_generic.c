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

#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include "sl_wfx_cli_generic.h"

// X.x.x: Major version of the CLI
#define SL_WFX_CLI_GEN_VERSION_MAJOR      3
// x.X.x: Minor version of the CLI
#define SL_WFX_CLI_GEN_VERSION_MINOR      1
// x.x.X: Revision of the CLI
#define SL_WFX_CLI_GEN_VERSION_REVISION   6

// Provides the version of the CLI
#define SL_WFX_CLI_GEN_VERSION   SL_WFX_CLI_GEN_MODULE_VERSION(SL_WFX_CLI_GEN_VERSION_MAJOR, \
                                                               SL_WFX_CLI_GEN_VERSION_MINOR, \
                                                               SL_WFX_CLI_GEN_VERSION_REVISION)

// Provides the version of the CLI as string
#define SL_WFX_CLI_GEN_VERSION_STRING     SL_WFX_CLI_GEN_MODULE_VERSION_STRING(SL_WFX_CLI_GEN_VERSION_MAJOR, \
                                                                               SL_WFX_CLI_GEN_VERSION_MINOR, \
                                                                               SL_WFX_CLI_GEN_VERSION_REVISION)

extern sl_wfx_cli_generic_functions_t sl_wfx_cli_gen_funcs;

static bool sl_wfx_cli_gen_initialized = false;
static uint16_t sl_wfx_cli_gen_nb_module_registered = 0;

typedef struct sl_wfx_cli_gen_module_s {
  char *name;
  char *version;
} sl_wfx_cli_gen_module_t;

static sl_wfx_cli_gen_module_t cli_modules[SL_WFX_CLI_GEN_NB_MAX_MODULES];

// Shell configuration
const char welcome[] = "Type \"help\" to view a list of registered commands.\r\n";
const char prompt[10] = "@ ";
const char newline[] = "\r\n";

// Generic error messages
const char invalid_command_msg[] = "Invalid command\r\n";     // Error during the command parsing, the input command is wrongly formatted
const char command_error_msg[] = "Command error\r\n";         // Error during the command processing
const char command_timeout_msg[] = "Command timeout\r\n";     // The command processing timed out
const char missing_parameter_msg[] = "Missing parameter\r\n"; // A parameter required by a module has not been registered by the application
const char alloc_error_msg[] = "Allocation error\r\n";        // Dynamic allocation error

static int version_cmd_cb (int argc,
                           char **argv,
                           char *output_buf,
                           uint32_t output_buf_len)
{
  // Directly display the output to avoid using a huge output buffer
  printf("Module list:\r\n");

  for (uint16_t i=0; i<sl_wfx_cli_gen_nb_module_registered; i++) {
    printf("\t%s\t\t%s\r\n", cli_modules[i].name, cli_modules[i].version);
  }

  printf("\r\n");

  return 0;
}

static const sl_wfx_cli_generic_command_t version_cmd =
{
  "cli-version",
  "cli-version              : Provide the version of the registered modules\r\n",
  version_cmd_cb,
  0
};

int sl_wfx_cli_generic_init (void *config)
{
  int res;

  res = sl_wfx_cli_gen_funcs.init(config);
  if (res == 0) {
    // Init called with success
    sl_wfx_cli_gen_initialized = true;

    // Register the version command
    res = sl_wfx_cli_generic_register_cmd(&version_cmd);

    if (res == 0) {
      // Register the CLI module
      res = sl_wfx_cli_generic_register_module("Generic CLI",
                                               SL_WFX_CLI_GEN_VERSION_STRING);
    }
  }
  return res;
}

int sl_wfx_cli_generic_register_cmd (sl_wfx_cli_generic_command_t const *cmd)
{
  int res;

  if (sl_wfx_cli_gen_initialized) {
    res = sl_wfx_cli_gen_funcs.register_command(cmd);
  } else {
    res = SL_WFX_CLI_ERROR_NOT_INIT;
  }
  return res;
}

int sl_wfx_cli_generic_register_module (char *name, char *version)
{
  int res = SL_WFX_CLI_ERROR;
  bool module_registered = false;

  // Basic sanity check
  if ((name != NULL) && (name[0] != '\0')
      && (version != NULL) && (version[0] != '\0')) {

    if (sl_wfx_cli_gen_nb_module_registered < SL_WFX_CLI_GEN_NB_MAX_MODULES) {
      // Ensure the module is not already registered
      for (uint16_t i=0; i<sl_wfx_cli_gen_nb_module_registered; i++) {
        if ((strncmp(cli_modules[i].name, name, strlen(cli_modules[i].name)) == 0)
            && (name[strlen(cli_modules[i].name)] == '\0')) {
          // Module already registered
          module_registered = true;
          break;
        }
      }

      if (!module_registered) {
        // Register the module
        cli_modules[sl_wfx_cli_gen_nb_module_registered].name = name;
        cli_modules[sl_wfx_cli_gen_nb_module_registered].version = version;
        sl_wfx_cli_gen_nb_module_registered++;
        res = 0;
      }
    } else {
      res = SL_WFX_CLI_ERROR_NB_MAX_MODULES;
    }
  }

  return res;
}

int sl_wfx_cli_generic_output (char *buffer)
{
  printf("%s", buffer);
  return strlen(buffer);
}

int sl_wfx_cli_generic_config_wait (uint32_t event_flag)
{
  int res = SL_WFX_CLI_ERROR_NOT_INIT;

  if (sl_wfx_cli_gen_initialized) {
    res = sl_wfx_cli_gen_funcs.config_wait(event_flag);
  }

  return res;
}

int sl_wfx_cli_generic_wait (uint32_t timeout_ms)
{
  int res = SL_WFX_CLI_ERROR_NOT_INIT;

  if (sl_wfx_cli_gen_initialized) {
    res = sl_wfx_cli_gen_funcs.wait(timeout_ms);
  }

  return res;
}

int sl_wfx_cli_generic_resume (void)
{
  int res = SL_WFX_CLI_ERROR_NOT_INIT;

  if (sl_wfx_cli_gen_initialized) {
    res = sl_wfx_cli_gen_funcs.resume();
  }

  return res;
}

