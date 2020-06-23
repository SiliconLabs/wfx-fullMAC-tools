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
#include "sl_wfx_cli_generic.h"

#ifdef SL_WFX_USE_SECURE_LINK
#include "secure_link/sl_wfx_secure_link.h"

// X.x.x: Major version of the Secure Link CLI
#define SL_WFX_CLI_SECURE_LINK_VERSION_MAJOR      1
// x.X.x: Minor version of the Secure Link CLI
#define SL_WFX_CLI_SECURE_LINK_VERSION_MINOR      1
// x.x.X: Revision of the Secure Link CLI
#define SL_WFX_CLI_SECURE_LINK_VERSION_REVISION   0

// Provides the version of the Secure Link CLI
#define SL_WFX_CLI_SECURE_LINK_VERSION   SL_WFX_CLI_GEN_MODULE_VERSION(SL_WFX_CLI_SECURE_LINK_VERSION_MAJOR, \
                                                                       SL_WFX_CLI_SECURE_LINK_VERSION_MINOR, \
                                                                       SL_WFX_CLI_SECURE_LINK_VERSION_REVISION)

// Provides the version of the Secure Link CLI as string
#define SL_WFX_CLI_SECURE_LINK_VERSION_STRING     SL_WFX_CLI_GEN_MODULE_VERSION_STRING(SL_WFX_CLI_SECURE_LINK_VERSION_MAJOR, \
                                                                                       SL_WFX_CLI_SECURE_LINK_VERSION_MINOR, \
                                                                                       SL_WFX_CLI_SECURE_LINK_VERSION_REVISION)


typedef void (*secure_link_bitmap_change_fn_t)(uint8_t *bitmap, uint8_t request_id);

static int slk_renegotiate_key_cmd_cb (int argc,
                                       char **argv,
                                       char *output_buf,
                                       uint32_t output_buf_len)
{
  sl_status_t status;
  int res = -1;

  status = sl_wfx_secure_link_renegotiate_session_key();
  if (status == SL_STATUS_OK) {
    res = 0;
  } // else let the generic CLI display the error message

  return res;
}

static const sl_wfx_cli_generic_command_t slk_renegotiate_cmd =
{
  "slk-renegotiate",
  "slk-renegotiate          : Renegotiate Secure Link session key\r\n",
  slk_renegotiate_key_cmd_cb,
  0
};

static const sl_wfx_cli_generic_command_t slk_rekey_cmd =
{
  "slk-rekey",
  "slk-rekey                : Alias of slk-renegotiate\r\n",
  slk_renegotiate_key_cmd_cb,
  0
};

static int slk_bitmap_change (char *msg_id, secure_link_bitmap_change_fn_t func)
{
  uint8_t slk_bitmap[SL_WFX_SECURE_LINK_ENCRYPTION_BITMAP_SIZE];
  sl_status_t status;
  int res = -1;

  // Retrieve the current bitmap
  memcpy(slk_bitmap,
         sl_wfx_context->encryption_bitmap,
         SL_WFX_SECURE_LINK_ENCRYPTION_BITMAP_SIZE);

  // Update the local copy of the bitmap
  func(slk_bitmap, (uint8_t) strtol(msg_id, NULL, 0));

  // Update the driver bitmap
  status = sl_wfx_secure_link_configure(slk_bitmap, 0);
  if (status == SL_STATUS_OK) {
    res = 0;
  }

  return res;
}

static int slk_add_cmd_cb (int argc,
                           char **argv,
                           char *output_buf,
                           uint32_t output_buf_len)
{
  (void)argc;
  (void)output_buf;
  (void)output_buf_len;

  return slk_bitmap_change(argv[1], sl_wfx_secure_link_bitmap_add_request_id);  // Let the generic CLI display the error message if needed
}

static const sl_wfx_cli_generic_command_t slk_add_cmd =
{
  "slk-add",
  "slk-add                  : Enable the encryption of API General messages with the specified \"msg_id\"\r\n"
  "                         Usage: slk-add <msg_id>\r\n",
  slk_add_cmd_cb,
  1
};

static int slk_remove_cmd_cb (int argc,
                              char **argv,
                              char *output_buf,
                              uint32_t output_buf_len)
{
  (void)argc;
  (void)output_buf;
  (void)output_buf_len;

  return slk_bitmap_change(argv[1], sl_wfx_secure_link_bitmap_remove_request_id);   // Let the generic CLI display the error message if needed
}

static const sl_wfx_cli_generic_command_t slk_remove_cmd =
{
  "slk-remove",
  "slk-remove               : Disable the encryption of API General messages with the specified \"msg_id\"\r\n"
  "                         Usage: slk-remove <msg_id>\r\n",
  slk_remove_cmd_cb,
  1
};

static int slk_bitmap_cmd_cb (int argc,
                              char **argv,
                              char *output_buf,
                              uint32_t output_buf_len)
{
  int res = -1;

  if (argc == 2) {
    uint8_t msg_id = (uint8_t)strtol(argv[1], NULL, 0);
    printf("%sncrypted\r\n", sl_wfx_secure_link_encryption_required_get(msg_id) ? "E" : "Une");
    res = 0;
  } else if (argc == 1) {
    for (uint8_t i=0; i<SL_WFX_SECURE_LINK_ENCRYPTION_BITMAP_SIZE; i++) {
      printf("\t%d:\t%02X\r\n", i, sl_wfx_context->encryption_bitmap[i]);
    }
    printf("\r\n");
    res = 0;
  } else {
    // Format the output message
    strncpy(output_buf, (char *)invalid_command_msg, output_buf_len);
    if (output_buf_len > 0) {
      output_buf[output_buf_len - 1] = '\0';
    }
  }

  return res;
}

static const sl_wfx_cli_generic_command_t slk_bitmap_cmd =
{
  "slk-bitmap",
  "slk-bitmap               : Display the current state of the Secure Link Encryption Bitmap\r\n"
  "                         Usage: slk-bitmap [msg_id]\r\n",
  slk_bitmap_cmd_cb,
  -1
};
#endif //SL_WFX_USE_SECURE_LINK

int sl_wfx_cli_secure_link_init (void)
{
  int res = -1;

#ifdef SL_WFX_USE_SECURE_LINK
  // Add Secure Link commands to the CLI
  res = sl_wfx_cli_generic_register_cmd(&slk_renegotiate_cmd);
  res = sl_wfx_cli_generic_register_cmd(&slk_rekey_cmd);
  res = sl_wfx_cli_generic_register_cmd(&slk_add_cmd);
  res = sl_wfx_cli_generic_register_cmd(&slk_remove_cmd);
  res = sl_wfx_cli_generic_register_cmd(&slk_bitmap_cmd);

  if (res == 0) {
    // Register the WiFi CLI module
    res = sl_wfx_cli_generic_register_module("Secure Link CLI",
                                             SL_WFX_CLI_SECURE_LINK_VERSION_STRING);
  }
#endif

  return res;
}
