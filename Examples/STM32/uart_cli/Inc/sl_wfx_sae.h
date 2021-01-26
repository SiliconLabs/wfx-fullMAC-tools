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
#ifndef SL_WFX_SAE_H
#define SL_WFX_SAE_H

#ifdef __cplusplus
extern "C" {
#endif

/**************************************************************************//**
 * Prepare the SAE exchange.
 * This function must be called before sending a WPA3-SAE join request.
 *****************************************************************************/
sl_status_t sl_wfx_sae_prepare(const sl_wfx_mac_address_t *sta_addr,
                               const sl_wfx_mac_address_t *ap_addr,
                               const uint8_t              *passkey,
                               uint16_t                    passkey_length);

/**************************************************************************//**
 * Realize the SAE exchange step by step.
 *****************************************************************************/
void sl_wfx_sae_exchange (sl_wfx_ext_auth_ind_t* ext_auth_indication);

#ifdef __cplusplus
}
#endif

#endif
