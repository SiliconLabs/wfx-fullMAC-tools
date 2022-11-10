/***************************************************************************//**
 * @file
 * @brief LwIP DHCP client interface
 *******************************************************************************
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
#ifndef DHCP_CLIENT_H
#define DHCP_CLIENT_H
#ifdef __cplusplus
extern "C" {
#endif
/***************************************************************************//**
 * Notify DHCP client task about the wifi status
 *
 * @param link_up link status
 ******************************************************************************/
void dhcpclient_set_link_state(int link_up);

/***************************************************************************//**
 * Start DHCP client task.
 ******************************************************************************/
void dhcpclient_start(void);
#ifdef __cplusplus
}
#endif
#endif
