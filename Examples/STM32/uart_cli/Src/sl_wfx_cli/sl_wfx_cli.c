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

/* FreeRTOS includes. */
#include "cmsis_os.h"

/* Standard includes. */
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>

/* FreeRTOS+CLI includes. */
#include "FreeRTOS_CLI.h"

/* LwIP includes. */
#include "lwip/netif.h"
#include "lwip/ip_addr.h"
#include "lwip/apps/lwiperf.h"

#include "sl_wfx.h"
#include "sl_wfx_cli.h"
#include "sl_wfx_host.h"
#include "lwip_common.h"
#include "stm32f4xx_hal.h"

#define SL_WFX_STATE_TO_BINARY(state)  \
  ((state & SL_WFX_STARTED) ? '1' : '0'), \
  ((state & SL_WFX_STA_INTERFACE_CONNECTED) ? '1' : '0'), \
  ((state & SL_WFX_AP_INTERFACE_UP) ? '1' : '0'), \
  ((state & SL_WFX_SLEEPING) ? '1' : '0'), \
  ((state & SL_WFX_POWER_SAVE_ACTIVE) ? '1' : '0') 

uint32_t iperf_ms_duration;
uint32_t iperf_bytes_transferred;
uint32_t iperf_bandwidth_kbitpsec;

void lwip_iperf_results (void *arg, enum lwiperf_report_type report_type,
                         const ip_addr_t* local_addr, u16_t local_port, const ip_addr_t* remote_addr, u16_t remote_port,
                         u32_t bytes_transferred, u32_t ms_duration, u32_t bandwidth_kbitpsec)
{
  iperf_ms_duration = ms_duration;
  iperf_bytes_transferred = bytes_transferred;
  iperf_bandwidth_kbitpsec = bandwidth_kbitpsec;
  printf("\r\nIperf Report:\r\n" );
  printf("Interval %d.%ds\r\n",(int)(ms_duration/1000),(int)(ms_duration%1000));
  printf("Bytes transferred %d.%dM\r\n",(int)(bytes_transferred/1024/1024),(int)((((bytes_transferred/1024)*1000)/1024)%1000));
  printf("%d.%d Mbps\r\n\r\n",(int)(bandwidth_kbitpsec/1024),(int)(((bandwidth_kbitpsec*1000)/1024)%1000));
}

/* Default parameters */
extern sl_wfx_context_t wifi;
extern char wlan_ssid[32];
extern char wlan_passkey[64];
extern sl_wfx_security_mode_t wlan_security;
extern char softap_ssid[32];
extern char softap_passkey[64];
extern sl_wfx_security_mode_t softap_security;
extern uint8_t softap_channel;          
/* Default parameters */
/*
* Implements the run-time-stats command.
*/
static portBASE_TYPE prvInitCommand( char *pcWriteBuffer, size_t xWriteBufferLen, const char *pcCommandString );
static portBASE_TYPE prvScanCommand( char *pcWriteBuffer, size_t xWriteBufferLen, const char *pcCommandString );
static portBASE_TYPE prvNetworkUpCommand( char *pcWriteBuffer, size_t xWriteBufferLen, const char *pcCommandString );
static portBASE_TYPE prvNetworkDownCommand( char *pcWriteBuffer, size_t xWriteBufferLen, const char *pcCommandString );
static portBASE_TYPE prvSoftApUpCommand( char *pcWriteBuffer, size_t xWriteBufferLen, const char *pcCommandString );
static portBASE_TYPE prvSoftApDownCommand( char *pcWriteBuffer, size_t xWriteBufferLen, const char *pcCommandString );
static portBASE_TYPE prvRssiCommand( char *pcWriteBuffer, size_t xWriteBufferLen, const char *pcCommandString );
static portBASE_TYPE prvIPerfCommand( char *pcWriteBuffer, size_t xWriteBufferLen, const char *pcCommandString );
static portBASE_TYPE prvPowerManagementModeCommand( char *pcWriteBuffer, size_t xWriteBufferLen, const char *pcCommandString );
static portBASE_TYPE prvDevicePowerSaveCommand( char *pcWriteBuffer, size_t xWriteBufferLen, const char *pcCommandString );
static portBASE_TYPE prvShutdownCommand( char *pcWriteBuffer, size_t xWriteBufferLen, const char *pcCommandString );
static portBASE_TYPE prvResetCommand( char *pcWriteBuffer, size_t xWriteBufferLen, const char *pcCommandString );
static portBASE_TYPE prvGetCommand( char *pcWriteBuffer, size_t xWriteBufferLen, const char *pcCommandString );
static portBASE_TYPE prvSetCommand( char *pcWriteBuffer, size_t xWriteBufferLen, const char *pcCommandString );

static const CLI_Command_Definition_t prvInitCommandDefinition =
{
  "init", /* The command string to type. */
  "init              : initialize the Wi-Fi chip\r\n",
  prvInitCommand, /* The function to run. */
  0
};

static const CLI_Command_Definition_t prvScanCommandDefinition =
{
  "scan", /* The command string to type. */
  "scan              : perform a Wi-Fi scan\r\n",
  prvScanCommand, /* The function to run. */
  0
};

static const CLI_Command_Definition_t prvNUpCommandDefinition =
{
  "nup", /* The command string to type. */
  "nup               : connect to the Wi-Fi access point with the information stored in wlan.all\r\n",
  prvNetworkUpCommand, /* The function to run. */
  0
};

static const CLI_Command_Definition_t prvNDoCommandDefinition =
{
  "ndo", /* The command string to type. */
  "ndo               : disconnect from the Wi-Fi access point\r\n",
  prvNetworkDownCommand, /* The function to run. */
  0
};

static const CLI_Command_Definition_t prvSUpCommandDefinition =
{
  "sup", /* The command string to type. */
  "sup               : start the SoftAP interface using the information store in softap.all\r\n",
  prvSoftApUpCommand, /* The function to run. */
  0
};

static const CLI_Command_Definition_t prvSDoCommandDefinition =
{
  "sdo", /* The command string to type. */
  "sdo               : stop the SoftAP interface\r\n",
  prvSoftApDownCommand, /* The function to run. */
  0
};

static const CLI_Command_Definition_t prvRssiCommandDefinition =
{
  "rssi", /* The command string to type. */
  "rssi              : retrieve the RSSI value in station mode\r\n"
  "rssi <mac>        : retrieve the client RSSI value in softAP mode\r\n",
  prvRssiCommand, /* The function to run. */
  -1
};

static const CLI_Command_Definition_t prvIPerfCommandDefinition =
{
  "iperf", /* The command string to type. */
  "iperf -c <ip>     : start a TCP iPerf client for 10 seconds\r\n"
  "iperf -s          : start a TCP iPerf server\r\n",
  prvIPerfCommand, /* The function to run. */
  -1
};

static const CLI_Command_Definition_t prvPowerManagementModeCommandDefinition =
{
  "powermode", /* The command string to type. */
  "powermode <m> <i> : configure the power management mode, m: mode, i: interval \r\n",
  prvPowerManagementModeCommand, /* The function to run. */
  2
};

static const CLI_Command_Definition_t prvDevicePowerSaveCommandDefinition =
{
  "powersave", /* The command string to type. */
  "powersave <on/off>: enable/disable the device power save mechanism\r\n",
  prvDevicePowerSaveCommand, /* The function to run. */
  1
};

static const CLI_Command_Definition_t prvShutdownCommandDefinition =
{
  "shutdown", /* The command string to type. */
  "shutdown          : put the Wi-Fi chip in shutdown mode\r\n",
  prvShutdownCommand, /* The function to run. */
  0
};

static const CLI_Command_Definition_t prvResetCommandDefinition =
{
  "reset", /* The command string to type. */
  "reset             : reset the MCU\r\n",
  prvResetCommand, /* The function to run. */
  0
};

static const CLI_Command_Definition_t prvGetCommandDefinition =
{
  "get", /* The command string to type. */
  "get <param>       : get a parameter. For more info, type \"get help\"\r\n",
  prvGetCommand, /* The function to run. */
  1
};

static const CLI_Command_Definition_t prvSetCommandDefinition =
{
  "set", /* The command string to type. */
  "set <param> <val> : set a parameter value. For more info, type \"set help\"\r\n",
  prvSetCommand, /* The function to run. */
  -1
};

void vRegisterCLICommands( void )
{
  /* Register all the command line commands defined immediately above. */
  FreeRTOS_CLIRegisterCommand( &prvInitCommandDefinition );
  FreeRTOS_CLIRegisterCommand( &prvScanCommandDefinition );
  FreeRTOS_CLIRegisterCommand( &prvNUpCommandDefinition );
  FreeRTOS_CLIRegisterCommand( &prvNDoCommandDefinition );
  FreeRTOS_CLIRegisterCommand( &prvSUpCommandDefinition );
  FreeRTOS_CLIRegisterCommand( &prvSDoCommandDefinition );
  FreeRTOS_CLIRegisterCommand( &prvRssiCommandDefinition );
  FreeRTOS_CLIRegisterCommand( &prvIPerfCommandDefinition );
  FreeRTOS_CLIRegisterCommand( &prvPowerManagementModeCommandDefinition );	
  FreeRTOS_CLIRegisterCommand( &prvDevicePowerSaveCommandDefinition );
  FreeRTOS_CLIRegisterCommand( &prvShutdownCommandDefinition );	
  FreeRTOS_CLIRegisterCommand( &prvResetCommandDefinition );
  FreeRTOS_CLIRegisterCommand( &prvGetCommandDefinition );
  FreeRTOS_CLIRegisterCommand( &prvSetCommandDefinition );
}

static portBASE_TYPE prvInitCommand(char *pcWriteBuffer, size_t xWriteBufferLen, const char *pcCommandString)
{
  sl_status_t status;
  *pcWriteBuffer = 0;
  
  status = sl_wfx_init(&wifi);
  switch(status){
  case SL_STATUS_OK:
    wifi.state = SL_WFX_STARTED;
    sprintf(pcWriteBuffer, "WF200 init successful\r\nFW version %d.%d.%d\r\n", 
            wifi.firmware_major,
            wifi.firmware_minor,
            wifi.firmware_build);
    break;
  case SL_STATUS_WIFI_INVALID_KEY:
    sprintf(pcWriteBuffer, "Failed to init WF200: Firmware keyset invalid\r\n");
    break;
  case SL_STATUS_WIFI_FIRMWARE_DOWNLOAD_TIMEOUT:
    sprintf(pcWriteBuffer, "Failed to init WF200: Firmware download timeout\r\n");
    break;
  case SL_STATUS_TIMEOUT:
    sprintf(pcWriteBuffer, "Failed to init WF200: Poll for value timeout\r\n");
    break;
  case SL_STATUS_FAIL:
    sprintf(pcWriteBuffer, "Failed to init WF200: Error\r\n");
    break;
  default :
    sprintf(pcWriteBuffer, "Failed to init WF200: Unknown error\r\n");
  }

  return pdFALSE;
}

static portBASE_TYPE prvScanCommand(char *pcWriteBuffer, size_t xWriteBufferLen, const char *pcCommandString)
{
  sl_status_t status;
  *pcWriteBuffer = 0;
  
  printf("!  # Ch RSSI MAC (BSSID)        Network (SSID)\r\n");
  xEventGroupClearBits(sl_wfx_event_group, SL_WFX_SCAN_COMPLETE);
  status = sl_wfx_send_scan_command(WFM_SCAN_MODE_ACTIVE, NULL, 0, NULL, 0, NULL, 0, NULL);
  
  if(status == SL_STATUS_OK)
  {
    if(xEventGroupWaitBits(sl_wfx_event_group, SL_WFX_SCAN_COMPLETE, pdTRUE, pdTRUE, 5000/portTICK_PERIOD_MS) == 0)
    {
      sprintf(pcWriteBuffer, "Command timeout\r\n");
    }
  }else{
    sprintf(pcWriteBuffer, "Command error\r\n");
  }
  
  return pdFALSE;
}

static portBASE_TYPE prvNetworkUpCommand( char *pcWriteBuffer, size_t xWriteBufferLen, const char *pcCommandString )
{
  sl_status_t status;
  *pcWriteBuffer = 0;
  
  xEventGroupClearBits(sl_wfx_event_group, SL_WFX_CONNECT);
  status = sl_wfx_send_join_command((uint8_t*) wlan_ssid, strlen(wlan_ssid), NULL, 0, wlan_security, 1, 0, (uint8_t*) wlan_passkey, strlen(wlan_passkey), NULL, 0);
  
  if(status == SL_STATUS_OK)
  {
    if(xEventGroupWaitBits(sl_wfx_event_group, SL_WFX_CONNECT, pdTRUE, pdTRUE, 5000/portTICK_PERIOD_MS) == 0)
    {
      sprintf(pcWriteBuffer, "Command timeout\r\n");
    }
    lwip_set_sta_link_up();
  }else{
    sprintf(pcWriteBuffer, "Command error\r\n");
  }
  
  return pdFALSE;
}

static portBASE_TYPE prvNetworkDownCommand( char *pcWriteBuffer, size_t xWriteBufferLen, const char *pcCommandString )
{
  sl_status_t status;
  *pcWriteBuffer = 0;
  
  lwip_set_sta_link_down();
  
  xEventGroupClearBits(sl_wfx_event_group, SL_WFX_DISCONNECT);
  status = sl_wfx_send_disconnect_command();
  
  if(status == SL_STATUS_OK)
  {
    if(xEventGroupWaitBits(sl_wfx_event_group, SL_WFX_DISCONNECT, pdTRUE, pdTRUE, 5000/portTICK_PERIOD_MS) == 0)
    {
      sprintf(pcWriteBuffer, "Command timeout\r\n");
    }
  }else{
    sprintf(pcWriteBuffer, "Command error\r\n");
  }

  return pdFALSE;
}

static portBASE_TYPE prvSoftApUpCommand( char *pcWriteBuffer, size_t xWriteBufferLen, const char *pcCommandString )
{
  sl_status_t status;
  *pcWriteBuffer = 0;
  
  xEventGroupClearBits(sl_wfx_event_group, SL_WFX_START_AP);
  status = sl_wfx_start_ap_command(softap_channel, (uint8_t*) softap_ssid, strlen(softap_ssid), 0, 0, softap_security, 0, (uint8_t*) softap_passkey, strlen(softap_passkey), NULL, 0, NULL, 0);
  
  if(status == SL_STATUS_OK)
  {
    if(xEventGroupWaitBits(sl_wfx_event_group, SL_WFX_START_AP, pdTRUE, pdTRUE, 5000/portTICK_PERIOD_MS) == 0)
    {
      sprintf(pcWriteBuffer, "Command timeout\r\n");
    }
    lwip_set_ap_link_up();
  }else{
    sprintf(pcWriteBuffer, "Command error\r\n");
  }

  return pdFALSE;
}

static portBASE_TYPE prvSoftApDownCommand( char *pcWriteBuffer, size_t xWriteBufferLen, const char *pcCommandString )
{
  sl_status_t status;
  *pcWriteBuffer = 0;
  
  lwip_set_ap_link_down();
  
  xEventGroupClearBits(sl_wfx_event_group, SL_WFX_STOP_AP);
  status = sl_wfx_stop_ap_command();
  
  if(status == SL_STATUS_OK)
  {
    if(xEventGroupWaitBits(sl_wfx_event_group, SL_WFX_STOP_AP, pdTRUE, pdTRUE, 5000/portTICK_PERIOD_MS) == 0)
    {
      sprintf(pcWriteBuffer, "Command timeout\r\n");
    }
  }else{
    sprintf(pcWriteBuffer, "Command error\r\n");
  }

  return pdFALSE;
}

static portBASE_TYPE prvRssiCommand( char *pcWriteBuffer, size_t xWriteBufferLen, const char *pcCommandString )
{
  sl_status_t status;
  char *mac_address_string;
  char *mac_byte = NULL;
  sl_wfx_mac_address_t mac_address;
  portBASE_TYPE mac_length;
  *pcWriteBuffer = 0;
  const char separator[1] = ":";
  uint32_t rcpi;
  
  mac_address_string = (char*) FreeRTOS_CLIGetParameter(pcCommandString, 1, &mac_length);
  
  if(mac_length == 0)
  {
    status = sl_wfx_get_signal_strength(&rcpi);
    if(status == SL_STATUS_OK)
    {
      sprintf(pcWriteBuffer, "RSSI value : %d dBm\r\n", (int16_t)(rcpi - 220)/2);
    }else{
      sprintf(pcWriteBuffer, "Command error\r\n");
    }
  }else{
    mac_byte = strtok(mac_address_string, separator);
    mac_address.octet[0] = (uint8_t)strtoul(mac_byte, NULL, 16);
    for(uint8_t i = 1; i < 6; i++)
    {
      mac_byte = strtok(NULL, separator);
      mac_address.octet[i] = (uint8_t)strtoul(mac_byte, NULL, 16);
    }
    status = sl_wfx_get_ap_client_signal_strength(&mac_address , &rcpi);
    if(status == SL_STATUS_OK)
    {
      sprintf(pcWriteBuffer, "Client %02X:%02X:%02X:%02X:%02X:%02X RSSI value : %d dBm\r\n",
              mac_address.octet[0],
              mac_address.octet[1],
              mac_address.octet[2],
              mac_address.octet[3],
              mac_address.octet[4],
              mac_address.octet[5],
              (int16_t)(rcpi - 220)/2);
    }else{
      sprintf(pcWriteBuffer, "Command error\r\n");
    }
  }
  
  return pdFALSE;
}

static portBASE_TYPE prvIPerfCommand( char *pcWriteBuffer, size_t xWriteBufferLen, const char *pcCommandString )
{
  char *mode, *ip_address;
  portBASE_TYPE mode_length, ip_address_length;
  ip_addr_t client_address;
  *pcWriteBuffer = 0;
  
  mode = (char*) FreeRTOS_CLIGetParameter(pcCommandString, 1, &mode_length);
  ip_address = (char*) FreeRTOS_CLIGetParameter(pcCommandString, 2, &ip_address_length);
  
  mode[mode_length] = 0x00; 
  ip_address[ip_address_length] = 0x00;
  
  if (strcmp(mode, "-s") == 0) 
  {
    lwiperf_start_tcp_server_default(lwip_iperf_results,0);
    sprintf(pcWriteBuffer, "iPerf TCP server started\r\n");
  }else if(strcmp(mode, "-c") == 0) 
  {
    ipaddr_aton(ip_address, &client_address);
    lwiperf_start_tcp_client_default(&client_address, lwip_iperf_results, 0);
    sprintf(pcWriteBuffer, "iPerf TCP client started on server %s\r\n", ip_address);
  }else{
    sprintf(pcWriteBuffer, "Invalid parameter\r\n");
  }
  
  return pdFALSE;
}

static portBASE_TYPE prvPowerManagementModeCommand( char *pcWriteBuffer, size_t xWriteBufferLen, const char *pcCommandString )
{
  sl_status_t status;
  char *power_mode, *interval;
  portBASE_TYPE power_mode_length, interval_length;
  *pcWriteBuffer = 0;
  
  /* Retrieve command parameters */
  power_mode = (char*) FreeRTOS_CLIGetParameter(pcCommandString, 1, &power_mode_length);
  interval = (char*) FreeRTOS_CLIGetParameter(pcCommandString, 2, &interval_length);
  
  /* End the string parameters*/
  power_mode[power_mode_length] = 0x00;
  interval[interval_length] = 0x00;
  
  if(atoi(power_mode) == 0)
  {
    status = sl_wfx_set_power_mode(WFM_PM_MODE_ACTIVE, 0);
    if(status == SL_STATUS_OK)
    {
      sprintf(pcWriteBuffer, "Power mode disable \r\n");
    }else{
      sprintf(pcWriteBuffer, "Command error\r\n");
    }
  }else{
    status = sl_wfx_set_power_mode((sl_wfx_pm_mode_t) atoi(power_mode), (uint16_t) atoi(interval) );
    if(status == SL_STATUS_OK)
    {
      sprintf(pcWriteBuffer, "Power mode %d, interval %d\r\n", atoi(power_mode), atoi(interval));
    }else{
      sprintf(pcWriteBuffer, "Command error\r\n");
    }
  }

  return pdFALSE;
}

static portBASE_TYPE prvDevicePowerSaveCommand( char *pcWriteBuffer, size_t xWriteBufferLen, const char *pcCommandString )
{
  sl_status_t status;
  char *state;
  portBASE_TYPE state_length;
  *pcWriteBuffer = 0;

  /* Retrieve command parameters */
  state = (char*) FreeRTOS_CLIGetParameter(pcCommandString, 1, &state_length);
  
  /* End the string parameters*/
  state[state_length] = 0x00;
  
  if(strcmp(state, "on") == 0)
  {
    status = sl_wfx_enable_device_power_save();
    if(status == SL_STATUS_OK)
    {
      sprintf(pcWriteBuffer, "Enable device power save\r\n");
    }else{
      sprintf(pcWriteBuffer, "Command error\r\n");
    }
  }else if(strcmp(state, "off") == 0)
  {
    status = sl_wfx_disable_device_power_save();
    if(status == SL_STATUS_OK)
    {
      sprintf(pcWriteBuffer, "Disable device power save\r\n");
    }else{
      sprintf(pcWriteBuffer, "Command error\r\n");
    }
  }else{
    sprintf(pcWriteBuffer, "Invalid parameter\r\n");
  }
  
  return pdFALSE;
}

static portBASE_TYPE prvShutdownCommand( char *pcWriteBuffer, size_t xWriteBufferLen, const char *pcCommandString )
{
  sl_status_t status;
  *pcWriteBuffer = 0;
  
  status = sl_wfx_shutdown();
  if(status == SL_STATUS_OK)
  {
    sprintf(pcWriteBuffer, "Wi-Fi chip shutdown\r\n");
  }else{
    sprintf(pcWriteBuffer, "Command error\r\n");
  }
  return pdFALSE;
}

static portBASE_TYPE prvResetCommand( char *pcWriteBuffer, size_t xWriteBufferLen, const char *pcCommandString )
{
  *pcWriteBuffer = 0;

  printf("Reset the MCU\r\n");
  HAL_NVIC_SystemReset();
  
  return pdFALSE;
}

static portBASE_TYPE prvGetCommand( char *pcWriteBuffer, size_t xWriteBufferLen, const char *pcCommandString )
{
  char* parameter;
  portBASE_TYPE parameter_length;
  *pcWriteBuffer = 0;
  
  /* Retrieve command parameters */
  parameter = (char*) FreeRTOS_CLIGetParameter(pcCommandString, 1, &parameter_length);
  
  /* End the string parameters*/
  parameter[ parameter_length ] = 0x00;
  
  /* Compare the parameter to the available settings and return the current value */
  if (strcmp(parameter, "state") == 0){
    sprintf(pcWriteBuffer,
            "Initialized          :%c\r\n"
            "Connected as station :%c\r\n"
            "SoftAP up            :%c\r\n"
            "Sleeping             :%c\r\n"
            "Powersave enabled    :%c\r\n"
            , SL_WFX_STATE_TO_BINARY(wifi.state));
  }
  /* WLAN */
  else if (strcmp(parameter, "wlan.all") == 0) 
  {
    sprintf(pcWriteBuffer, 
            "wlan.ssid     = %s\r\n"
            "wlan.passkey  = %s\r\n"
            "wlan.security = %d\r\n"
            "wlan.mac      = %02X:%02X:%02X:%02X:%02X:%02X\r\n"
            "wlan.ip       = %d.%d.%d.%d\r\n",
            wlan_ssid,
            wlan_passkey,
            wlan_security,
            sl_wfx_context->mac_addr_0.octet[0],
            sl_wfx_context->mac_addr_0.octet[1],
            sl_wfx_context->mac_addr_0.octet[2],
            sl_wfx_context->mac_addr_0.octet[3],
            sl_wfx_context->mac_addr_0.octet[4],
            sl_wfx_context->mac_addr_0.octet[5],
            sta_netif.ip_addr.addr & 0xff,
            (sta_netif.ip_addr.addr >> 8) & 0xff,
            (sta_netif.ip_addr.addr >> 16) & 0xff,
            (sta_netif.ip_addr.addr >> 24) & 0xff);
  } 
  else if (strcmp(parameter, "wlan.ssid") == 0) 
  {
    sprintf(pcWriteBuffer, "%s\r\n", wlan_ssid);
  } 
  else if (strcmp(parameter, "wlan.passkey") == 0 )
  {
    sprintf(pcWriteBuffer, "%s\r\n", wlan_passkey);
  }
  else if (strcmp(parameter, "wlan.security") == 0 )
  {
    if(wlan_security == WFM_SECURITY_MODE_OPEN)                sprintf(pcWriteBuffer, "OPEN\r\n");
    else if(wlan_security == WFM_SECURITY_MODE_WEP)            sprintf(pcWriteBuffer, "WEP\r\n");
    else if(wlan_security == WFM_SECURITY_MODE_WPA2_WPA1_PSK)  sprintf(pcWriteBuffer, "WPA1/WPA2\r\n");
    else if(wlan_security == WFM_SECURITY_MODE_WPA2_PSK)       sprintf(pcWriteBuffer, "WPA2\r\n");
  }
  else if (strcmp(parameter, "wlan.mac") == 0 )
  {
    sprintf(pcWriteBuffer, 
            "%02X:%02X:%02X:%02X:%02X:%02X\r\n", 
            sl_wfx_context->mac_addr_0.octet[0],
            sl_wfx_context->mac_addr_0.octet[1],
            sl_wfx_context->mac_addr_0.octet[2],
            sl_wfx_context->mac_addr_0.octet[3],
            sl_wfx_context->mac_addr_0.octet[4],
            sl_wfx_context->mac_addr_0.octet[5]);
  }
  else if (strcmp(parameter, "wlan.ip") == 0 )
  {
    sprintf(pcWriteBuffer,
            "IP address : %d.%d.%d.%d\r\n",
            sta_netif.ip_addr.addr & 0xff,
            (sta_netif.ip_addr.addr >> 8) & 0xff,
            (sta_netif.ip_addr.addr >> 16) & 0xff,
            (sta_netif.ip_addr.addr >> 24) & 0xff);
  }
  /* SOFTAP */
  else if (strcmp(parameter, "softap.all") == 0) 
  {
    sprintf(pcWriteBuffer, 
            "softap.ssid     = %s\r\n"
            "softap.passkey  = %s\r\n"
            "softap.security = %d\r\n"
            "softap.mac      = %02X:%02X:%02X:%02X:%02X:%02X\r\n"
            "softap.ip       = %d.%d.%d.%d\r\n"
            "softap.channel  = %d\r\n",
            softap_ssid,
            softap_passkey,
            softap_security,
            sl_wfx_context->mac_addr_1.octet[0],
            sl_wfx_context->mac_addr_1.octet[1],
            sl_wfx_context->mac_addr_1.octet[2],
            sl_wfx_context->mac_addr_1.octet[3],
            sl_wfx_context->mac_addr_1.octet[4],
            sl_wfx_context->mac_addr_1.octet[5],
            ap_netif.ip_addr.addr & 0xff,
            (ap_netif.ip_addr.addr >> 8) & 0xff,
            (ap_netif.ip_addr.addr >> 16) & 0xff,
            (ap_netif.ip_addr.addr >> 24) & 0xff,
            softap_channel);
  } 
  else if (strcmp(parameter, "softap.ssid") == 0 )
  {
    sprintf(pcWriteBuffer, "%s\r\n", softap_ssid);
  }
  else if (strcmp(parameter, "softap.passkey") == 0 )
  {
    sprintf(pcWriteBuffer, "%s\r\n", softap_passkey);
  }
  else if (strcmp(parameter, "softap.security") == 0 )
  {
    if(softap_security == WFM_SECURITY_MODE_OPEN)                sprintf(pcWriteBuffer, "OPEN\r\n");
    else if(softap_security == WFM_SECURITY_MODE_WEP)            sprintf(pcWriteBuffer, "WEP\r\n");
    else if(softap_security == WFM_SECURITY_MODE_WPA2_WPA1_PSK)  sprintf(pcWriteBuffer, "WPA1/WPA2\r\n");
    else if(softap_security == WFM_SECURITY_MODE_WPA2_PSK)       sprintf(pcWriteBuffer, "WPA2\r\n");
  }
  else if (strcmp(parameter, "softap.channel") == 0 )
  {
    sprintf(pcWriteBuffer, "%d\r\n", softap_channel);
  }
  else if (strcmp(parameter, "softap.mac") == 0 )
  {
    sprintf(pcWriteBuffer, 
            "%02X:%02X:%02X:%02X:%02X:%02X\r\n", 
            sl_wfx_context->mac_addr_1.octet[0],
            sl_wfx_context->mac_addr_1.octet[1],
            sl_wfx_context->mac_addr_1.octet[2],
            sl_wfx_context->mac_addr_1.octet[3],
            sl_wfx_context->mac_addr_1.octet[4],
            sl_wfx_context->mac_addr_1.octet[5]);
  }
  else if (strcmp(parameter, "softap.ip") == 0 )
  {
    sprintf(pcWriteBuffer,
            "IP address : %d.%d.%d.%d\r\n",
            ap_netif.ip_addr.addr & 0xff,
            (ap_netif.ip_addr.addr >> 8) & 0xff,
            (ap_netif.ip_addr.addr >> 16) & 0xff,
            (ap_netif.ip_addr.addr >> 24) & 0xff);
  }
  else if (strcmp(parameter, "softap.clients") == 0 )
  {
  }
  else if (strcmp(parameter, "iperf") == 0 )
  {
    sprintf(pcWriteBuffer, 
            "\r\nIperf Client Report:\r\n"
            "Duration %dms\r\n"
            "Bytes transferred %d\r\n"
            "%d kbit/s\r\n",
            iperf_ms_duration,
            iperf_bytes_transferred,
            iperf_bandwidth_kbitpsec);
  }
  else if (strcmp(parameter, "ip.stats") == 0 )
  {
    stats_display();
  }
  else if (strcmp(parameter, "help") == 0 )
  {
    sprintf(pcWriteBuffer, 
            "state          : value of the context state\r\n"
            "wlan.all       : all the wlan.* parameter\r\n"
            "wlan.ssid      : wlan ssid used\r\n"
            "wlan.passkey   : wlan passkey used\r\n"
            "wlan.security  : wlan security level used\r\n"
            "wlan.mac       : wlan MAC address\r\n"
            "wlan.ip        : wlan IP address\r\n"
            "softap.all     : all the softap.* parameter\r\n"
            "softap.ssid    : softAP ssid used\r\n"
            "softap.passkey : softAP passkey used\r\n"
            "softap.security: softAP security level used\r\n"
            "softap.channel : channel used in softAP mode\r\n"
            "softap.mac     : softAP MAC address\r\n"
            "softap.clients : List clients connected to the softAP\r\n"
            "iperf          : return latest iPerf restuls\r\n"
            "ip.stats       : display the IP stack stats\r\n");
  }
  else /* default: */
  {
    sprintf(pcWriteBuffer, "Invalid parameter\r\n");
  }

  return pdFALSE;
}

static portBASE_TYPE prvSetCommand( char *pcWriteBuffer, size_t xWriteBufferLen, const char *pcCommandString )
{
  char *parameter, *value;
  portBASE_TYPE parameter_length, value_length;
  *pcWriteBuffer = 0;
  
  /* Retrieve command parameters */
  parameter = (char*) FreeRTOS_CLIGetParameter(pcCommandString, 1, &parameter_length);
  value = (char*) FreeRTOS_CLIGetParameter(pcCommandString, 2, &value_length);
  
  /* End the string parameters*/
  parameter[parameter_length] = 0x00;
  value[value_length] = 0x00;
  
  /* Compare the parameter to the available settings and return the current value */
  /* WLAN */
  if (strcmp(parameter, "wlan.ssid") == 0) 
  {
    strcpy(wlan_ssid, value);
    sprintf(pcWriteBuffer, "wlan.ssid = %s\r\n", wlan_ssid);
  } 
  else if (strcmp(parameter, "wlan.passkey") == 0 )
  {
    if(strlen(value) < 8)
    {
      sprintf(pcWriteBuffer, "Passkey parameter invalid, valid length range = [8;63]\r\n");
    }else{
      strcpy(wlan_passkey, value);
      sprintf(pcWriteBuffer, "wlan.passkey = %s\r\n", wlan_passkey);
    }
  }
  else if (strcmp(parameter, "wlan.security") == 0 )
  {
    sprintf(pcWriteBuffer, "wlan.security = %s\r\n", value);
    if(strcmp(value, "OPEN") == 0)           wlan_security = WFM_SECURITY_MODE_OPEN;
    else if(strcmp(value, "WEP") == 0)       wlan_security = WFM_SECURITY_MODE_WEP;
    else if(strcmp(value, "WPA1/WPA2") == 0) wlan_security = WFM_SECURITY_MODE_WPA2_WPA1_PSK;
    else if(strcmp(value, "WPA2") == 0)      wlan_security = WFM_SECURITY_MODE_WPA2_PSK;
    else sprintf(pcWriteBuffer, "Invalid parameter. Valid options: OPEN, WEP, WPA1/WPA2, WPA2\r\n");
  }
  else if (strcmp(parameter, "wlan.mac") == 0 )
  {
    sprintf(pcWriteBuffer, "Not supported\r\n");
  }
  /* SOFTAP */
  else if (strcmp(parameter, "softap.ssid") == 0 )
  {
    strcpy(softap_ssid, value);
    sprintf(pcWriteBuffer, "softap.ssid = %s\r\n", softap_ssid);
  }
  else if (strcmp(parameter, "softap.passkey") == 0 )
  {
    if(strlen(value) < 8)
    {
      sprintf(pcWriteBuffer, "Passkey parameter invalid, valid length range = [8;63]\r\n");
    }else{
      strcpy(softap_passkey, value);
      sprintf(pcWriteBuffer, "softap.passkey = %s\r\n", softap_passkey);
    }
  }
  else if (strcmp(parameter, "softap.security") == 0 )
  {
    sprintf(pcWriteBuffer, "softap.security = %s\r\n", value);
    if(strcmp(value, "OPEN") == 0)           softap_security = WFM_SECURITY_MODE_OPEN;
    else if(strcmp(value, "WEP") == 0)       softap_security = WFM_SECURITY_MODE_WEP;
    else if(strcmp(value, "WPA1/WPA2") == 0) softap_security = WFM_SECURITY_MODE_WPA2_WPA1_PSK;
    else if(strcmp(value, "WPA2") == 0)      softap_security = WFM_SECURITY_MODE_WPA2_PSK;
    else sprintf(pcWriteBuffer, "Invalid parameter. Valid options: OPEN, WEP, WPA1/WPA2, WPA2\r\n");
  }
  else if (strcmp(parameter, "softap.channel") == 0 )
  {
    uint8_t channel = (uint8_t) atoi(value);
    if((channel < 1) || (channel > 13))
    {
      sprintf(pcWriteBuffer, "Channel parameter invalid, valid range = [1;13]\r\n");
    }else{
      softap_channel = channel;
      sprintf(pcWriteBuffer, "softap.channel = %d\r\n", softap_channel);
    }
  }
  else if (strcmp(parameter, "softap.mac") == 0 )
  {
    sprintf(pcWriteBuffer, "Not supported\r\n");
  }
  else if (strcmp(parameter, "help") == 0 )
  {
    sprintf(pcWriteBuffer, 
            "wlan.ssid      : wlan ssid used\r\n"
            "wlan.passkey   : wlan passkey used, range [8;63]\r\n"
            "wlan.security  : wlan security level used (OPEN,WEP,WPA1/WPA2,WPA2)\r\n"
            "wlan.mac       : not supported\r\n"
            "softap.ssid    : softAP ssid used\r\n"
            "softap.passkey : softAP passkey used, range [8;63]\r\n"
            "softap.security: softAP security level used (OPEN,WEP,WPA1/WPA2,WPA2)\r\n"
            "softap.channel : channel used in softAP mode, range [1;14]\r\n"
            "softap.mac     : not supported\r\n");
  }
  else /* default: */
  {
    sprintf(pcWriteBuffer, "Invalid parameter\r\n");
  }

  return pdFALSE;
}
