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

#ifndef APP_CERTIFICATE_H
#define APP_CERTIFICATE_H

#include <stdint.h>
#include "sl_status.h"

/// Maximum length of certificate subject
#define APP_CERTIFICATE_MAX_SUBJECT_LENGTH 64

/// Maximum length of certificate algorithm
#define APP_CERTIFICATE_MAX_ALGORITHM_LENGTH 20

/// Enumerations for certificate type
typedef enum
{
  /// Non-existing type
  app_certificate_type_invalid    = 0x00,
  /// A trusted certificate, either a CA or an intermediate-CA
  app_certificate_type_trusted    = 0x01,
  /// Device certificate
  app_certificate_type_device     = 0x02,
  /// Device private key
  app_certificate_type_device_pk  = 0x03
} app_certificate_type_t;

typedef struct {
  /// Length of the certificate data.
  uint16_t data_length;
  /// Current offset for certificate data.
  uint16_t data_offset;
  /// Certificate data.
  uint8_t data[];
} app_certificate_t;

typedef struct {
  /// Certificate subject
  char subject[APP_CERTIFICATE_MAX_SUBJECT_LENGTH];
  /// Certificate algorithm
  char algorithm[APP_CERTIFICATE_MAX_ALGORITHM_LENGTH];
  /// Certificate type
  app_certificate_type_t type;
  /// Certificate key bitlength
  uint16_t bitlen;
} app_certicate_info_t;

/**************************************************************************//**
 * Initialize a certificate.
 *
 * @param size Size of the certificate.
 * @return Pointer to the created certificate. Ownership is transferred.
 *****************************************************************************/
app_certificate_t *app_certificate_init(uint16_t size);

/**************************************************************************//**
 * Get pointer to certificate data.
 *
 * @param certificate Pointer to the certificate.
 * @return Pointer to certificate data.
 *****************************************************************************/
uint8_t *app_certificate_data(app_certificate_t *certificate);

/**************************************************************************//**
 * Append uninitialized data to the certificate.
 *
 * @param certificate Pointer to the certificate.
 * @param data_length Length of appended data.
 * @return SL_STATUS_OK if successful, an error code otherwise
 *****************************************************************************/
sl_status_t app_certificate_append(app_certificate_t *certificate,
                                   uint16_t data_length);

/**************************************************************************//**
 * Append data to the certificate.
 *
 * @param certificate Pointer to the certificate.
 * @param data Pointer to appended data.
 * @param data_length Length of appended data.
 * @return SL_STATUS_OK if successful, an error code otherwise
 *****************************************************************************/
sl_status_t app_certificate_append_data(app_certificate_t *certificate,
                                        const char *data,
                                        uint16_t data_length);

/**************************************************************************//**
 * Append string to the certificate.
 *
 * @param certificate Pointer to the certificate.
 * @param data Pointer to appended string.
 * @return SL_STATUS_OK if successful, an error code otherwise
 *****************************************************************************/
sl_status_t app_certificate_append_string(app_certificate_t *certificate,
                                          const char *data);

/**************************************************************************//**
 * Free resources allocated to a certificate.
 *
 * @param certificate Pointer to the certificate. Ownership is transferred.
 *****************************************************************************/
void app_certificate_free(app_certificate_t *certificate);

/**************************************************************************//**
 * Parse certificate information.
 *
 * @param certificate Pointer to the certificate.
 * @param info Pointer to the structure where the information is written.
 * @return SL_STATUS_OK if successful, an error code otherwise
 *****************************************************************************/
//sl_status_t app_certificate_parse(const app_certificate_t *certificate,
//                                  app_certicate_info_t *info);

#endif  // APP_CERTIFICATE_H
