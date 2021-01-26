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

#include "ports/includes.h"
#include "utils/common.h"
#include "common/sae.h"

#include "sl_wfx.h"
#include "sl_wfx_sae.h"

#define SL_IANA_IKE_GROUP_NIST_P256 19

static struct sae_data sae_ctx;

/**************************************************************************//**
 * Prepare the SAE exchange.
 * This function must be called before sending a WPA3-SAE join request.
 *****************************************************************************/
sl_status_t sl_wfx_sae_prepare (const sl_wfx_mac_address_t *sta_addr,
                                const sl_wfx_mac_address_t *ap_addr,
                                const uint8_t              *passkey,
                                uint16_t                    passkey_length)
{
  int status = SL_STATUS_FAIL;
  int ret;

  // Reset the context
  memset(&sae_ctx, 0, sizeof(sae_ctx));

  // Prepare the commit step
  if (sae_set_group(&sae_ctx, SL_IANA_IKE_GROUP_NIST_P256) == 0) {
    ret = sae_prepare_commit((const uint8_t*)sta_addr,
                                (const uint8_t*)ap_addr,
                                passkey,
                                passkey_length,
                                NULL,
                                &sae_ctx);
    if (ret == 0) {
      status = SL_STATUS_OK;
    }
  }

  return status;
}

/**************************************************************************//**
 * Realize the SAE exchange step by step.
 *****************************************************************************/
void sl_wfx_sae_exchange (sl_wfx_ext_auth_ind_t *ext_auth_indication)
{
  static struct wpabuf* buf = NULL;
  int ret;
  
//  printf("2: ");
//  for (int i=0; i<ext_auth_indication->body.auth_data_length; i++)
//    printf("%02X", ext_auth_indication->body.auth_data[i]);
//  printf("\r\n");

  switch (ext_auth_indication->body.auth_data_type) {

    case 0:
      // SAE start
      buf = wpabuf_alloc(SAE_COMMIT_MAX_LEN);
//      printf("SAE write commit\r\n");
      sae_write_commit(&sae_ctx, buf, NULL, NULL);
      sl_wfx_ext_auth(WFM_EXT_AUTH_DATA_TYPE_SAE_COMMIT,
                      wpabuf_len(buf), wpabuf_head_u8(buf));
      wpabuf_free(buf);
      break;

    case 1:
      // SAE peer commit
//      printf("SAE parse commit\r\n");
      ret = sae_parse_commit(&sae_ctx,
                             ext_auth_indication->body.auth_data,
                             ext_auth_indication->body.auth_data_length,
                             NULL,
                             NULL,
                             NULL);
      if (!ret) {
//        printf("SAE process commit\r\n");
        ret = sae_process_commit(&sae_ctx);
      }
      if (!ret) {
        buf = wpabuf_alloc(SAE_COMMIT_MAX_LEN);
//        printf("SAE write confirm (%08lX)\r\n", (uint32_t)buf);
        sae_write_confirm(&sae_ctx, buf);
        sl_wfx_ext_auth(WFM_EXT_AUTH_DATA_TYPE_SAE_CONFIRM,
                        wpabuf_len(buf), wpabuf_head_u8(buf));
        wpabuf_free(buf);
//        printf("SAE write confirm (%08lX)\r\n", (uint32_t)status);
      }
      break;

    case 2:
      // SAE peer confirm
//      printf("SAE check confirm\r\n");
      ret = sae_check_confirm(&sae_ctx,
                              ext_auth_indication->body.auth_data,
                              ext_auth_indication->body.auth_data_length);
//      printf("SAE check confirm %d\r\n", ret);
      if (!ret) {
        sl_wfx_ext_auth(WFM_EXT_AUTH_DATA_TYPE_MSK, SAE_PMK_LEN, sae_ctx.pmk);
      }
      break;
  }
}

