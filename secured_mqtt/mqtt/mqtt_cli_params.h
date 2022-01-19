/**************************************************************************//**
 * # License
 * <b>Copyright 2022 Silicon Laboratories Inc. www.silabs.com</b>
 *******************************************************************************
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
#ifndef MQTT_CLI_PARAMS_H_
#define MQTT_CLI_PARAMS_H_

#include <stdint.h>
#include "os.h"

#include "sl_wfx_cmd_api.h"
#include "lwip/ip_addr.h"
#include "nvm3_default.h"


/*** USER DEFINES GOES HERE !!***/
#define SDIO_BUS  "sdio"
#define SPI_BUS   "spi"

#define BUF_LEN   128
#define SL_WFX_MQTT_CLI_MAX_PARAMS   30
#define SL_WFX_MQTT_CLI_MAX_CLIENTS  10

/// Create a NVM area across x Flash pages. Create a cache of y entries.
typedef enum {
  NVM3_KEY_AP_SSID                  = 1,
  NVM3_KEY_AP_SECURITY_MODE         = 2,
  NVM3_KEY_AP_PASSKEY               = 3,
  NVM3_KEY_MQTT_BROKER              = 4,
  NVM3_KEY_MQTT_PORT                = 5,
  NVM3_KEY_MQTT_CLIENT_ID           = 6,
  NVM3_KEY_MQTT_PUBLISH_TOPIC       = 7,
  NVM3_KEY_MQTT_SUBSCRIBE_TOPIC     = 8,
  NVM3_KEY_MQTT_CA_CERTIFICATE      = 9,
  NVM3_KEY_MQTT_DEVICE_CERTIFICATE  = 10,
  NVM3_KEY_MQTT_DEVICE_KEY          = 11,
  NVM3_KEY_MQTT_USERNAME            = 12,
  NVM3_KEY_MQTT_PASSWORD            = 13,
} nvm3_keys_t;
#define NVM3_NB_MANDATORY_KEYS_WIFI   (NVM3_KEY_AP_PASSKEY - NVM3_KEY_AP_SSID + 1)
#define NVM3_NB_MANDATORY_KEYS_MQTT   (NVM3_KEY_MQTT_SUBSCRIBE_TOPIC - NVM3_KEY_MQTT_BROKER + 1)
#define NVM3_NB_TOTAL_KEYS_MQTT       (NVM3_KEY_MQTT_PASSWORD - NVM3_KEY_MQTT_BROKER + 1)

// Parameter edit rights mask
#define SL_WFX_MQTT_CLI_PARAM_SET_RIGHT       (1<<0)
#define SL_WFX_MQTT_CLI_PARAM_GET_RIGHT       (1<<1)

/** Typedef **/
typedef int (*sl_wfx_mqtt_cli_param_custom_get_func_t)(char *param_name,
                                                  void *param_addr,
                                                  uint32_t param_size,
                                                  char *output_buf,
                                                  uint32_t output_buf_len);

typedef int (*sl_wfx_mqtt_cli_param_custom_set_func_t)(char *param_name,
                                                  void *param_addr,
                                                  uint32_t param_size,
                                                  char *new_value);

typedef enum sl_wfx_mqtt_cli_param_type_s {
  SL_WFX_MQTT_CLI_PARAM_TYPE_INTEGER = 0,
  SL_WFX_MQTT_CLI_PARAM_TYPE_UNSIGNED_INTEGER,
  SL_WFX_MQTT_CLI_PARAM_TYPE_HEXADECIMAL,
  SL_WFX_MQTT_CLI_PARAM_TYPE_ARRAY_INTEGER,
  SL_WFX_MQTT_CLI_PARAM_TYPE_ARRAY_UNSIGNED_INTEGER,
  SL_WFX_MQTT_CLI_PARAM_TYPE_ARRAY_HEXADECIMAL,
  SL_WFX_MQTT_CLI_PARAM_TYPE_ARRAY_STRING,
  SL_WFX_MQTT_CLI_PARAM_TYPE_STRING,
  SL_WFX_MQTT_CLI_PARAM_TYPE_CUSTOM
} sl_wfx_mqtt_cli_param_type_t;

typedef struct {
  char *name;
  void *address;
  char *description;
  sl_wfx_mqtt_cli_param_custom_get_func_t get_func;
  sl_wfx_mqtt_cli_param_custom_set_func_t set_func;
  sl_wfx_mqtt_cli_param_type_t type;
  uint16_t size;
  uint8_t rights;
} mqtt_param_t;

/*< Public variables */
extern mqtt_param_t mqtt_params[SL_WFX_MQTT_CLI_MAX_PARAMS];

/** SOME HELPER UTILITY TO HANDLE GLOBAL wifi_params **/
int mqtt_cli_param_search(char *name);
//int convert_str_to_mac_addr(char *mac_str, sl_wfx_mac_address_t *mac_addr);
void *mqtt_cli_get_param_addr(char *param_name);

/**HELPER FUNCTIONS***/

/*** INITIALIZING MQTT PARAMETERS ***/
int mqtt_cli_params_init(void);

#endif /* MQTT_CLI_PARAMS_H_ */
