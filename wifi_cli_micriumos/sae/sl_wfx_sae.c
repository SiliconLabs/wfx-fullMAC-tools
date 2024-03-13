/***************************************************************************//**
 * @file
 * @brief WFX SAE exchange implementation
 *******************************************************************************
 * # License
 * <b>Copyright 2021 Silicon Laboratories Inc. www.silabs.com</b>
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
#include "ports/includes.h"
#include "utils/common.h"
#include "common/sae.h"
#include "common/ieee802_11_defs.h"
#include "sl_wfx.h"
#include "sl_wfx_sae.h"

#define SL_IANA_IKE_GROUP_NIST_P256         19
#define SL_D11_SUCCESS                      0
#define SL_D11_ANTI_CLOGGING_TOKEN_REQUIRED 76
#define SL_D11_SAE_HASH_TO_ELEMENT          126
/* Specify the status of each step during the SAE exchange process */
#define RET_STATUS(ret) ((ret == 0) ? "Success" : "Error")

static struct sae_data sae_ctx;
static struct sae_pt *sae_pt;
sae_pmksa_t sae_pmksa;

typedef union {
  sl_wfx_ext_auth_sae_start_t sae_start;
  sl_wfx_ext_auth_sae_message_t sae_message;
  sl_wfx_ext_auth_msk_t msk;
  sl_wfx_ext_auth_transition_disable_t tr_disable;
} sae_data_t;

/**
 * @brief Check if there is a Pairwise Master Key (PMK) already derived and cached
 * 
 * @return true There is a valid Pairwise Master Key (PMK)
 * @return false There is no valid Pairwise Master Key (PMK)
 */
bool validate_pmk()
{
  bool is_pmk_valid = false;
  uint8_t i; /* loop index */ 

  for (i = 0; i < SL_WFX_MSK_SIZE; i++) {
    if (sae_pmksa.msk.msk[i] != 0) {
      is_pmk_valid = true;
      break;
    }
  }

  return is_pmk_valid;
}

int sae_send_commit(bool h2e, struct wpabuf* token)
{
  struct wpabuf* buf = NULL;
  int ret;

  buf = wpabuf_alloc(SAE_COMMIT_MAX_LEN);
  if (h2e) {
    printf("SL_D11_SAE_HASH_TO_ELEMENT\r\n");
    wpabuf_put_le16(buf, SL_D11_SAE_HASH_TO_ELEMENT);
  } else {
    printf("SL_D11_SUCCESS\r\n");
   wpabuf_put_le16(buf, SL_D11_SUCCESS);
  }
  ret = sae_write_commit(&sae_ctx, buf, token, NULL);
  printf("sae_write_commit: %s\r\n", RET_STATUS(ret));
  if (!ret) {
    sl_wfx_ext_auth(WFM_EXT_AUTH_DATA_TYPE_SAE_COMMIT,
                    wpabuf_len(buf), wpabuf_head_u8(buf));
  }
  wpabuf_free(buf);

  return ret;
}

struct wpabuf *sae_parse_token(struct sae_data *sae, const u8 *data, size_t len)
{
  const u8 *pos = data, *end = data + len;
  u16 res;
  struct wpabuf* token_buf;
  u8 id, elen, extid;
  size_t token_len = 0;

  /* Check Finite Cyclic Group */
  if (end - pos < 2)
    return NULL;
  res = sae_group_allowed(sae, NULL, WPA_GET_LE16(pos));
  if (res)
    return NULL;
  pos += 2;

  if (sae->h2e) {
    /* When H2E is used, token is in a container */
    if (end - pos < 3)
      return NULL;

    id = *pos++;
    elen = *pos++;
    extid = *pos++;
    if (id != WLAN_EID_EXTENSION ||
        elen == 0 || elen > len - 4 ||
        extid != WLAN_EID_EXT_ANTI_CLOGGING_TOKEN)
        return NULL;
    token_len = elen - 1;
  } else {
    token_len = end - pos;
  }

  token_buf = wpabuf_alloc_copy(pos, token_len);
  printf("token length: %u\r\n", token_len);

  return token_buf;
}

/**************************************************************************//**
 * Prepare the SAE exchange.
 * This function must be called before sending a WPA3-SAE join request.
 *****************************************************************************/
sl_status_t sl_wfx_sae_prepare (const sl_wfx_mac_address_t *sta_addr,
                                const sl_wfx_mac_address_t *ap_addr,
                                const uint8_t              *ssid,
                                uint16_t                   ssid_length,
                                const uint8_t              *passkey,
                                uint16_t                   passkey_length,
                                bool                       h2e) {
  int status = SL_STATUS_FAIL;
  int ret;

  /* Reset the context */
  memset(&sae_ctx, 0, sizeof(sae_ctx));

  /* Prepare the commit step */
  if (sae_set_group(&sae_ctx, SL_IANA_IKE_GROUP_NIST_P256) == 0) {
    if (!h2e) {
      ret = sae_prepare_commit((const uint8_t*)sta_addr,
                               (const uint8_t*)ap_addr,
                               passkey,
                               passkey_length,
                               &sae_ctx);
        printf("sae_prepare_commit: %s\r\n", RET_STATUS(ret));
    } else {
      /* Derive the secret element PT */
      sae_pt = sae_derive_pt(NULL, /* use default groups */
                             ssid,
                             ssid_length,
                             passkey,
                             passkey_length,
                             NULL); /* no password identifier */
      printf("sae_derive_pt: %p\r\n", sae_pt);

      ret = sae_prepare_commit_pt(&sae_ctx,
                                  sae_pt,
                                  (const uint8_t*)sta_addr,
                                  (const uint8_t*)ap_addr,
                                  NULL,  /* no rejected groups */
                                  NULL); /* no public key */
      printf("sae_prepare_commit_pt: %s\r\n", RET_STATUS(ret));
    }
    if (ret == 0) {
      status = SL_STATUS_OK;
    }
  }

  return status;
}

/**************************************************************************//**
 * Realize the SAE exchange step by step.
 *****************************************************************************/
void sl_wfx_sae_exchange (sl_wfx_ext_auth_ind_t *ext_auth_indication, bool *caching_pmk) {
  static struct wpabuf* buf = NULL;
  struct wpabuf* token_buf = NULL;
  sae_data_t *sae_data = (sae_data_t *)ext_auth_indication->body.auth_data;
  int ret;

  switch (ext_auth_indication->body.auth_data_type) {
    case WFM_EXT_AUTH_DATA_TYPE_SAE_START: {
      /* SAE start */
      printf("SAE-START: %02x:%02x:%02x:%02x:%02x:%02x (0x%02x)\r\n",
              sae_data->sae_start.bssid[0], sae_data->sae_start.bssid[1],
              sae_data->sae_start.bssid[2], sae_data->sae_start.bssid[3],
              sae_data->sae_start.bssid[4], sae_data->sae_start.bssid[5],
              *((uint8_t*)&sae_data->sae_start.security_mode));

      if ((memcmp(sae_pmksa.bssid, sae_data->sae_start.bssid, SL_WFX_BSSID_SIZE)) 
          || (!validate_pmk())) {
        printf("Sending SAE-COMMIT\r\n");
        ret = sae_send_commit(sae_ctx.h2e, NULL);
      } else {
        printf("Sending MSK\r\n");
        sl_wfx_ext_auth(WFM_EXT_AUTH_DATA_TYPE_MSK, sizeof(sae_pmksa.msk), (const uint8_t *)&sae_pmksa.msk);
      }
      break;
    }

    case WFM_EXT_AUTH_DATA_TYPE_SAE_COMMIT: {
      /* SAE peer commit */
      printf("SAE-COMMIT: 0x%04x\r\n", sae_data->sae_message.status_code);

      if (sae_data->sae_message.status_code == SL_D11_ANTI_CLOGGING_TOKEN_REQUIRED) {
        /* Peer sent an anti-clogging token, resend own commit together with the token */
        token_buf = sae_parse_token(&sae_ctx,
                                    sae_data->sae_message.sae_data,
                                    ext_auth_indication->body.auth_data_length - 2);
        ret = sae_send_commit(sae_ctx.h2e, token_buf);
        wpabuf_free(token_buf);
      } else {
        ret = sae_parse_commit(&sae_ctx,
                               sae_data->sae_message.sae_data,
                               ext_auth_indication->body.auth_data_length - 2,
                               NULL,
                               NULL,
							   NULL, 	/* allowed_groups */
							   sae_ctx.h2e,
							   NULL); /* ie_offset */
        printf("sae_parse_commit: %s\r\n", RET_STATUS(ret));
        if (!ret) {
          ret = sae_process_commit(&sae_ctx);
          printf("sae_process_commit: %s\r\n", RET_STATUS(ret));

          if (!ret) {
            buf = wpabuf_alloc(SAE_COMMIT_MAX_LEN);
            wpabuf_put_le16(buf, SL_D11_SUCCESS);
            ret = sae_write_confirm(&sae_ctx, buf);
            printf("sae_write_confirm: %s\r\n", RET_STATUS(ret));
            sl_wfx_ext_auth(WFM_EXT_AUTH_DATA_TYPE_SAE_CONFIRM,
                            wpabuf_len(buf), wpabuf_head_u8(buf));
            wpabuf_free(buf);
          }
        }
      }
      break;
    }

    case WFM_EXT_AUTH_DATA_TYPE_SAE_CONFIRM: {
      /* SAE peer confirm */
      printf("SAE-CONFIRM: 0x%04x\r\n", sae_data->sae_message.status_code);

      ret = sae_check_confirm(&sae_ctx,
                              sae_data->sae_message.sae_data,
                              ext_auth_indication->body.auth_data_length - 2,
							  NULL); /* ie_offset */
      printf("sae_check_confirm: %s\r\n", RET_STATUS(ret));
      if (!ret) {
        memcpy(sae_pmksa.msk.msk, sae_ctx.pmk, SAE_PMK_LEN);
        memcpy(sae_pmksa.msk.mskid, sae_ctx.pmkid, SAE_PMKID_LEN);
        sl_wfx_ext_auth(WFM_EXT_AUTH_DATA_TYPE_MSK, sizeof(sae_pmksa.msk), (const uint8_t *)&sae_pmksa.msk);
        *caching_pmk = true;
      }
      break;
    }

    case WFM_EXT_AUTH_DATA_TYPE_TRANSITION_DISABLE: {
      /* Transition Disable */
      printf("TR-DISABLE: 0x%02x\r\n", *((uint8_t*)&sae_data->tr_disable.mask));
      break;
    }
  }
}

void sl_wfx_sae_invalidate_pmksa()
{
  uint8_t zeros_bssid[6] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

  if (memcmp(sae_pmksa.bssid, zeros_bssid, SL_WFX_BSSID_SIZE))
  {
    printf("Invalidating PMKSA for %02x:%02x:%02x:%02x:%02x:%02x\r\n",
            sae_pmksa.bssid[0], sae_pmksa.bssid[1], sae_pmksa.bssid[2],
            sae_pmksa.bssid[3], sae_pmksa.bssid[4], sae_pmksa.bssid[5]);
    memset(sae_pmksa.bssid, 0, SL_WFX_BSSID_SIZE);
  }
}

void sl_wfx_sae_cache_pmksa(const sl_wfx_mac_address_t *bssid)
{
  if (memcmp(sae_pmksa.bssid, bssid, SL_WFX_BSSID_SIZE))
  {
    printf("Caching PMKSA for %02x:%02x:%02x:%02x:%02x:%02x\r\n",
         bssid->octet[0], bssid->octet[1], bssid->octet[2],
         bssid->octet[3], bssid->octet[4], bssid->octet[5]);
    memcpy(sae_pmksa.bssid, bssid, SL_WFX_BSSID_SIZE);
  }
}