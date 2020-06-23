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
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include "sl_wfx_cli_generic.h"
#include "sl_wfx_cli_common.h"

// X.x.x: Major version of the Param CLI
#define SL_WFX_CLI_PARAM_VERSION_MAJOR      2
// x.X.x: Minor version of the Param CLI
#define SL_WFX_CLI_PARAM_VERSION_MINOR      0
// x.x.X: Revision of the Param CLI
#define SL_WFX_CLI_PARAM_VERSION_REVISION   0

// Provides the version of the Param CLI
#define SL_WFX_CLI_PARAM_VERSION   SL_WFX_CLI_GEN_MODULE_VERSION(SL_WFX_CLI_PARAM_VERSION_MAJOR, \
                                                                 SL_WFX_CLI_PARAM_VERSION_MINOR, \
                                                                 SL_WFX_CLI_PARAM_VERSION_REVISION)

// Provides the version of the Param CLI as string
#define SL_WFX_CLI_PARAM_VERSION_STRING     SL_WFX_CLI_GEN_MODULE_VERSION_STRING(SL_WFX_CLI_PARAM_VERSION_MAJOR, \
                                                                                 SL_WFX_CLI_PARAM_VERSION_MINOR, \
                                                                                 SL_WFX_CLI_PARAM_VERSION_REVISION)

static const sl_wfx_cli_generic_command_t set_cmd;
static const sl_wfx_cli_generic_command_t get_cmd;

typedef struct param_s {
  char *name;
  void *address;
  char *description;
  sl_wfx_cli_param_custom_get_func_t get_func;
  sl_wfx_cli_param_custom_set_func_t set_func;
  sl_wfx_cli_param_type_t type;
  uint8_t size;
  uint8_t rights;
} param_t;

static const char unknown_param_msg[] = "Unknown parameter\r\n";
static const char unsupported_size_msg[] = "Unsupported size\r\n";
static const char value_too_large_msg[] = "Value too large\r\n";
static const char more_items_msg[] = "More items than expected\r\n";
static const char less_items_msg[] = "Less items than expected\r\n";
static const char wrong_array_format_msg[] = "Wrong array format (ex: [1,2,3])\r\n";

static param_t params[SL_WFX_CLI_PARAM_NB_MAX_PARAMS] = {0};
static uint8_t nb_param_registered = 0;

static int param_search (char *name)
{
  int param_index = -1;
  int i;

  for (i=0; i<nb_param_registered; i++) {
    if ((strncmp(name, params[i].name, strlen(params[i].name)) == 0)
        && (name[strlen(params[i].name)] == '\0')) {
      param_index = i;
      break;
    }
  }

  return param_index;
}

static bool is_array_parameter (sl_wfx_cli_param_type_t param_type)
{
  return ((param_type == SL_WFX_CLI_PARAM_TYPE_ARRAY_INTEGER)
          || (param_type == SL_WFX_CLI_PARAM_TYPE_ARRAY_UNSIGNED_INTEGER)
          || (param_type == SL_WFX_CLI_PARAM_TYPE_ARRAY_HEXADECIMAL));
}

static int convert_string_to_value (char **input,
                                    void *param_addr,
                                    sl_wfx_cli_param_type_t param_type,
                                    uint8_t value_size,
                                    char **error_msg)
{
  char *ptr_end;
  long long value;
  long long cast_int_value;
  unsigned long long cast_uint_value;
  int ret = -1;

  // Try converting a number
  value = strtoll(*input, &ptr_end, 0);

  // Update the point position
  *input = ptr_end;

  // Check the conversion result
  if (is_array_parameter(param_type)) {
    if ((*ptr_end != ',')
        && (*ptr_end != ']')) {
      *error_msg = (char *)wrong_array_format_msg;
    }
  } else {
    if (*ptr_end != '\0') {
      *error_msg = (char *)invalid_command_msg;
    }
  }

  if (*error_msg == NULL) {
    *error_msg = (char *)value_too_large_msg;

    switch (value_size) {
      case 1:
      {
        cast_int_value = (long long)((int8_t)value);
        cast_uint_value = (unsigned long long)value;

        if ((cast_int_value == value) /* Signed case */
            || (cast_uint_value <= UINT8_MAX) /* Unsigned case */) {
          *(int8_t *)param_addr = (int8_t)value;
          *error_msg = NULL;
          ret = 0;
        }
        break;
      }

      case 2:
      {
        cast_int_value = (long long)((int16_t)value);
        cast_uint_value = (unsigned long long)value;

        if ((cast_int_value == value) /* Signed case */
            || (cast_uint_value <= UINT16_MAX) /* Unsigned case */) {
          *(int16_t *)param_addr = (int16_t)value;
          *error_msg = NULL;
          ret = 0;
        }
        break;
      }

      case 4:
      {
        cast_int_value = (long long)((int32_t)value);
        cast_uint_value = (unsigned long long)value;

        if ((cast_int_value == value) /* Signed case */
            || (cast_uint_value <= UINT32_MAX) /* Unsigned case */) {
          *(int32_t *)param_addr = (int32_t)value;
          *error_msg = NULL;
          ret = 0;
        }
        break;
      }

      default:
        // Size error, should not happen
        *error_msg = (char *)unsupported_size_msg;
        break;
    }
  }

  return ret;
}

static int convert_signed_value_to_string (void *param_addr,
                                           uint8_t param_size,
                                           char *format,
                                           char *output_buf,
                                           uint32_t output_buf_len)
{
  int32_t value;
  int ret = 0;

  switch (param_size) {
    case 1:
      value = *(int8_t  *)param_addr;
      break;
    case 2:
      value = *(int16_t *)param_addr;
      break;
    case 4:
      value = *(int32_t *)param_addr;
      break;
    default:
      // Should not happen
      ret = -1;
      break;
  }

  if (ret == 0) {
    ret = snprintf(output_buf, output_buf_len, format, value);
  }

  return ret;
}

static int convert_unsigned_value_to_string (void *param_addr,
                                             uint8_t param_size,
                                             char *format,
                                             char *output_buf,
                                             uint32_t output_buf_len)
{
  uint32_t value;
  int ret = 0;

  switch (param_size) {
    case 1:
      value = *(uint8_t  *)param_addr;
      break;

    case 2:
      value = *(uint16_t *)param_addr;
      break;

    case 4:
      value = *(uint32_t *)param_addr;
      break;

    default:
      // Should not happen
      ret = -1;
      break;
  }

  if (ret == 0) {
    ret = snprintf(output_buf, output_buf_len, format, value);
  }

  return ret;
}

/*
 * Displays all parameters with the selected rights (potentially all).
 */
static void display_help (uint8_t rights)
{
  int i;

  // Directly display the output to avoid using a huge output buffer
  printf("\r\n%-5s%s\r\n", "", "Parameter list:");

  for (i=0; i<nb_param_registered; i++) {
    if (params[i].rights & rights) {
      printf ("\t%-30s%s\r\n", params[i].name, params[i].description);
    }
  }

  printf ("\r\n");
}

/*
 * Displays all parameters with get right.
 */
static void display_get_help (void)
{
  printf(get_cmd.help);
  display_help(SL_WFX_CLI_PARAM_GET_RIGHT);
}

/*
 * Displays all parameters with set right.
 */
static void display_set_help (void)
{
  printf(set_cmd.help);
  display_help(SL_WFX_CLI_PARAM_SET_RIGHT);
}

static int update_array (char *array,
                         param_t *param,
                         char *output_buf,
                         uint32_t output_buf_len)
{
  char *msg = NULL;
  char *ptr = NULL;
  uint8_t *param_val_ptr = (uint8_t *)param->address;
  int ret = -1;
  int len = strlen(array);
  bool parsing_error = false;
  uint8_t nb_item_updated = 0;

  // Check the input format
  if ((array[0] == '[')
      && (array[len-1] == ']')) {

    ptr = array + 1 /*Skip "["*/;

    do {
      // Try converting the input string into numbers
      //TODO manage arrays containing items of bigger size (8bit max)
      ret = convert_string_to_value(&ptr, param_val_ptr, param->type, 1, &msg);
      if (ret == 0) {
        if (nb_item_updated < param->size) {
          param_val_ptr++;
          nb_item_updated++;

          // Check if end of array reached
          if (*ptr != ']') {
            ptr++; // Skip the comma
          } else {
            // Parsing success, exit the loop
            break;
          }
        } else {
          // Error
          msg = (char *)more_items_msg;
          parsing_error = true;
          break;
        }
      } else {
        parsing_error = true;
        break;
      }
    } while (1);

    if (!parsing_error
        && (nb_item_updated < param->size)) {
      // Warning, not enough items in the input
      msg = (char *)less_items_msg;
    }

  } else {
    msg = (char *)wrong_array_format_msg;
  }

  if (msg != NULL) {
    // Format the output message
    strncpy(output_buf, msg, output_buf_len);
  }

  return ret;
}

static bool check_validity (param_t *param)
{
  bool is_valid = false;

  if ((param->name != NULL)
      && (param->name[0] != '\0')
      && (param->rights > 0)
      && (param->type >= SL_WFX_CLI_PARAM_TYPE_INTEGER) /*First*/
      && (param->type <= SL_WFX_CLI_PARAM_TYPE_CUSTOM)  /*Last*/) {

    if ((param->address == NULL)
        && (param->type != SL_WFX_CLI_PARAM_TYPE_CUSTOM)) {
      // Only allow parameter address null with custom types
      printf("Address required\r\n");
    } else if ((param->type == SL_WFX_CLI_PARAM_TYPE_INTEGER)
               || (param->type == SL_WFX_CLI_PARAM_TYPE_UNSIGNED_INTEGER)
               || (param->type == SL_WFX_CLI_PARAM_TYPE_HEXADECIMAL)) {
      // Check the parameter size
      if ((param->size == 1)
          || (param->size == 2)
          || (param->size == 4)) {
        is_valid = true;
      } else {
        printf("Unsupported parameter size (1/2/4 bytes)\r\n");
      }
    } else if (param->type == SL_WFX_CLI_PARAM_TYPE_CUSTOM) {
      // Check that the custom handlers are provided
      if (((param->rights & SL_WFX_CLI_PARAM_GET_RIGHT) && (param->get_func == NULL))
          || ((param->rights & SL_WFX_CLI_PARAM_SET_RIGHT) && (param->set_func == NULL))) {
        printf("Custom get/set functions missing\r\n");
      } else {
        is_valid = true;
      }
    } else {
      is_valid = true;
    }
  } else {
    printf("Wrong input (name, addr, rights or type)\r\n");
  }

  return is_valid;
}

static int get_cmd_cb (int argc,
                       char **argv,
                       char *output_buf,
                       uint32_t output_buf_len)
{
  char *format_table[] = { "%ld", "%lu", "%#lx", "%d,", "%u,", "%#02x," };
  char *msg = NULL;
  int param_index;
  int res = -1;

  // Retrieve the parameter with its name
  param_index = param_search(argv[1]);

  if (param_index >= 0) {
    // Check the parameter's rights
    if (params[param_index].rights & SL_WFX_CLI_PARAM_GET_RIGHT) {

      // Output the parameter value
      switch (params[param_index].type) {

        case SL_WFX_CLI_PARAM_TYPE_INTEGER:
        {
          res = convert_signed_value_to_string(params[param_index].address,
                                               params[param_index].size,
                                               format_table[params[param_index].type],
                                               output_buf,
                                               output_buf_len);

          // Add a new line in the output buffer
          if (res + 2 < output_buf_len) {
            strcat(output_buf, "\r\n");
          }
          res = 0;
          break;
        }

        case SL_WFX_CLI_PARAM_TYPE_UNSIGNED_INTEGER:
        case SL_WFX_CLI_PARAM_TYPE_HEXADECIMAL:
        {
          res = convert_unsigned_value_to_string(params[param_index].address,
                                                 params[param_index].size,
                                                 format_table[params[param_index].type],
                                                 output_buf,
                                                 output_buf_len);

          // Add a new line in the output buffer
          if (res + 2 < output_buf_len) {
            strcat(output_buf, "\r\n");
          }
          res = 0;
          break;
        }

        case SL_WFX_CLI_PARAM_TYPE_ARRAY_INTEGER:
        {
          int8_t *ptr = (int8_t *)params[param_index].address;
          int len = 0;

          // Write the array start symbol
          strcat(output_buf, "["); len++;

          // Write the array content
          for (int i=0; i < params[param_index].size; i++) {
            res = convert_signed_value_to_string(ptr++,
                                                 1,
                                                 format_table[params[param_index].type],
                                                 output_buf + len,
                                                 output_buf_len - len);
            if (res < 0) {
              break;
            }
            len += res;
          }

          if (len > 0) {
            // Delete the comma after the last item
            if (output_buf[len-1] == ',') {
              output_buf[len-1] = '\0';
            }

            // Write the array end symbol + new line
            if (len + 3 < output_buf_len) {
              strcat(output_buf, "]\r\n");
            }

            res = 0;
          }
          break;
        }

        case SL_WFX_CLI_PARAM_TYPE_ARRAY_UNSIGNED_INTEGER:
        case SL_WFX_CLI_PARAM_TYPE_ARRAY_HEXADECIMAL:
        {
          uint8_t *ptr = (uint8_t *)params[param_index].address;
          int len = 0;

          // Write the array start symbol
          strcat(output_buf, "["); len++;

          // Write the array content
          for (int i=0; i < params[param_index].size; i++) {
            res = convert_unsigned_value_to_string(ptr++,
                                                   1,
                                                   format_table[params[param_index].type],
                                                   output_buf + len,
                                                   output_buf_len - len);
            if (res < 0) {
              break;
            }
            len += res;
          }

          if (len > 0) {
            // Delete the comma after the last item
            if (output_buf[len-1] == ',') {
              output_buf[len-1] = '\0';
            }

            // Write the array end symbol + new line
            if (len + 3 < output_buf_len) {
              strcat(output_buf, "]\r\n");
            }

            res = 0;
          }
          break;
        }

        case SL_WFX_CLI_PARAM_TYPE_STRING:
        {
          snprintf(output_buf,
                   output_buf_len,
                   "%s\r\n",
                   (char *)params[param_index].address);
          res = 0;
          break;
        }

        case SL_WFX_CLI_PARAM_TYPE_CUSTOM:
        {
          res = params[param_index].get_func(params[param_index].name,
                                             params[param_index].address,
                                             output_buf,
                                             output_buf_len);
          break;
        }
      }
    }

    if (res < 0) {
      // Formatting or rights error, overwrite the output with the error message
      msg = (char *)command_error_msg;
    }
  } else if (strncmp(argv[1], "help", 4) == 0) {
    // Hijack the Generic CLI help to add another layer (parameter help)
    // Display all parameters with get right
    display_get_help();
    res = 0;
  } else {
    // Unknown parameter error
    msg = (char *)unknown_param_msg;
  }

  if (msg != NULL) {
    // Format the output message
    strncpy(output_buf, msg, output_buf_len);
    if (output_buf_len > 0) {
      output_buf[output_buf_len - 1] = '\0';
    }
  }

  return res;
}

static const sl_wfx_cli_generic_command_t get_cmd =
{
  "get",
  "get                      : Get a parameter value\r\n"
  "                         Usage: get <param_name>\r\n",
  get_cmd_cb,
  1
};

static int set_cmd_cb (int argc,
                       char **argv,
                       char *output_buf,
                       uint32_t output_buf_len)
{
  char *msg = NULL;
  char *ptr_value;
  int param_index;
  int res = -1;

  if (argc == 3) {
    // Retrieve the parameter with its name
    param_index = param_search(argv[1]);
    ptr_value = argv[2];

    if (param_index >= 0) {
      // Check the parameter's rights
      if (params[param_index].rights & SL_WFX_CLI_PARAM_SET_RIGHT) {

        // Update the parameter value
        switch (params[param_index].type) {

          case SL_WFX_CLI_PARAM_TYPE_INTEGER:
          case SL_WFX_CLI_PARAM_TYPE_UNSIGNED_INTEGER:
          case SL_WFX_CLI_PARAM_TYPE_HEXADECIMAL:
          {
            // Try converting the input string into a number
            convert_string_to_value(&ptr_value,
                                    params[param_index].address,
                                    params[param_index].type,
                                    params[param_index].size,
                                    &msg);

            // Force to zero, to avoid an overwrite of the output
            res = 0;
            break;
          }

          case SL_WFX_CLI_PARAM_TYPE_ARRAY_INTEGER:
          case SL_WFX_CLI_PARAM_TYPE_ARRAY_UNSIGNED_INTEGER:
          case SL_WFX_CLI_PARAM_TYPE_ARRAY_HEXADECIMAL:
          {
            update_array(ptr_value,
                         &params[param_index],
                         output_buf,
                         output_buf_len);

            // Force to zero, to avoid an overwrite of the output
            res = 0;
            break;
          }

          case SL_WFX_CLI_PARAM_TYPE_STRING:
          {
            strncpy((char *)params[param_index].address,
                    ptr_value,
                    params[param_index].size);
            // Make sure the string is NULL terminated
            ((char *)params[param_index].address)[params[param_index].size-1] = '\0';
            res = 0;
            break;
          }

          case SL_WFX_CLI_PARAM_TYPE_CUSTOM:
          {
            res = params[param_index].set_func(params[param_index].name,
                                               params[param_index].address,
                                               ptr_value);
            break;
          }
        }
      }

      if (res < 0) {
        // Updating or right error
        msg = (char *)command_error_msg;
      }
    } else {
      // Unknown parameter error
      msg = (char *)unknown_param_msg;
    }
  } else if ((argc == 2)
      && (strncmp(argv[1], "help", 4) == 0)) {
    // Hijack the Generic CLI help to add another layer (parameter help)
    // Display all parameters with set right
    display_set_help();
    res = 0;
  } else {
    // Command format error
    msg = (char *)invalid_command_msg;
  }

  if (msg != NULL) {
    // Format the output message
    strncpy(output_buf, msg, output_buf_len);
    if (output_buf_len > 0) {
      output_buf[output_buf_len - 1] = '\0';
    }
  }

  return res;
}

static const sl_wfx_cli_generic_command_t set_cmd =
{
  "set",
  "set                      : Set a parameter value\r\n"
  "                         Usage: set <param_name> <param_value>\r\n",
  set_cmd_cb,
  -1
};

/*
 * Add the parameter commands in the shell.
 */
int sl_wfx_cli_param_init (void)
{
  int res;

  // Add param commands to the CLI
  res  = sl_wfx_cli_generic_register_cmd(&get_cmd);
  res = sl_wfx_cli_generic_register_cmd(&set_cmd);

  if (res == 0) {
    // Register the Param CLI module
    res = sl_wfx_cli_generic_register_module("Param CLI",
                                             SL_WFX_CLI_PARAM_VERSION_STRING);
  }

  return res;
}

/*
 * Registers application parameters available in the CLI.
 */
int sl_wfx_cli_param_register (char *name,
                               void *address,
                               uint8_t size,
                               uint8_t rights,
                               sl_wfx_cli_param_type_t type,
                               char *description,
                               sl_wfx_cli_param_custom_get_func_t get_func,
                               sl_wfx_cli_param_custom_set_func_t set_func)
{
  int ret = -1;
  param_t param = {0};

  if (nb_param_registered < SL_WFX_CLI_PARAM_NB_MAX_PARAMS) {
    param.name = name;
    param.address = address;
    param.description = description;
    param.size = size;
    param.rights = rights;
    param.type = type;
    param.get_func = get_func;
    param.set_func = set_func;

    if (check_validity(&param)) {
      memcpy(&params[nb_param_registered], &param, sizeof(param_t));
      nb_param_registered++;
      ret = 0;
    }
  }

  return ret;
}

/*
 * Returns the application parameter address, allowing
 * CLI modules to get and/or change its value.
 */
void *sl_wfx_cli_param_get_addr (char *param_name)
{
  void *param_addr = NULL;
  int param_index;

  // Retrieve the parameter with its name
  param_index = param_search(param_name);
  if (param_index >= 0) {
    param_addr = params[param_index].address;
  }

  return param_addr;
}

