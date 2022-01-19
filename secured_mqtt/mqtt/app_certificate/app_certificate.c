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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "sl_malloc.h"
#include "app_certificate.h"

app_certificate_t *app_certificate_init(const uint16_t data_length)
{
  app_certificate_t *cert;
  size_t cert_size = sizeof(app_certificate_t) + data_length + 1;

  cert = sl_malloc(cert_size);
  if (!cert) {
    // Unable to allocate a certificate.
    return NULL;
  }

  // Always NULL-terminate the certificate data
  memset(cert, 0, cert_size);
  cert->data_length = 1;

  return cert;
}

uint8_t *app_certificate_data(app_certificate_t *certificate)
{
  return certificate->data;
}

sl_status_t app_certificate_append(app_certificate_t *certificate,
                                   uint16_t data_length)
{
  certificate->data_length += data_length;
  certificate->data_offset += data_length;

  return SL_STATUS_OK;
}

sl_status_t app_certificate_append_data(app_certificate_t *certificate,
                                        const char *data,
                                        uint16_t data_length)
{
  memcpy(&certificate->data[certificate->data_offset], data, data_length);
  app_certificate_append(certificate, data_length);

  return SL_STATUS_OK;
}

sl_status_t app_certificate_append_string(app_certificate_t *certificate,
                                          const char *data)
{
  memcpy(&certificate->data[certificate->data_offset], data, strlen(data));
  app_certificate_append(certificate, strlen(data));

  // Always terminate a string with a linefeed
  if ((certificate->data_offset) &&
      (certificate->data[certificate->data_offset - 1] != '\n')) {
    certificate->data[certificate->data_offset] = '\n';
    app_certificate_append(certificate, 1);
  }

  return SL_STATUS_OK;
}

void app_certificate_free(app_certificate_t *certificate)
{
  free(certificate);
}
