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

#ifndef __SL_WIFI_STATUS_H
#define __SL_WIFI_STATUS_H

#define SL_WIFI_ENUM_OFFSET        1000
#define SL_STATUS_ENUM(prefix, name, value)  prefix ## _ ## name = (prefix##_ENUM_OFFSET + value)

#define SL_WIFI_STATUS_LIST(prefix)                                                                                                        \
  SL_STATUS_ENUM(prefix, INVALID_KEY, 0),                          /**< Invalid firmware keyset */                                         \
  SL_STATUS_ENUM(prefix, FIRMWARE_DOWNLOAD_TIMEOUT, 1),            /**< The firmware download took too long */                             \
  SL_STATUS_ENUM(prefix, INVALID_PARAMETER, 2),                    /**< The request contains one or more invalid parameters */             \
  SL_STATUS_ENUM(prefix, UNSUPPORTED_MESSAGE_ID, 3),               /**< Unknown request ID or wrong interface ID used */                   \
  SL_STATUS_ENUM(prefix, WARNING, 4),                              /**< The request is successful but some parameters have been ignored */ \
  SL_STATUS_ENUM(prefix, NO_PACKET_TO_RECEIVE, 5),                 /**< No Packets waiting to be received */                               \
  SL_STATUS_ENUM(prefix, SLEEP_GRANTED, 8),                        /**< The sleep mode is granted */                                       \
  SL_STATUS_ENUM(prefix, SLEEP_NOT_GRANTED, 9),                    /**< The WFx does not go back to sleep */                               \
  SL_STATUS_ENUM(prefix, SECURE_LINK_MAC_KEY_ERROR, 16),           /**< The SecureLink MAC key was not found */                            \
  SL_STATUS_ENUM(prefix, SECURE_LINK_MAC_KEY_ALREADY_BURNED, 17),  /**< The SecureLink MAC key is already installed in OTP */              \
  SL_STATUS_ENUM(prefix, SECURE_LINK_RAM_MODE_NOT_ALLOWED, 18),    /**< The SecureLink MAC key cannot be installed in RAM */               \
  SL_STATUS_ENUM(prefix, SECURE_LINK_FAILED_UNKNOWN_MODE, 19),     /**< The SecureLink MAC key installation failed */                      \
  SL_STATUS_ENUM(prefix, SECURE_LINK_EXCHANGE_FAILED, 20),         /**< SecureLink key (re)negotiation failed */                           \
  SL_STATUS_ENUM(prefix, SECURE_LINK_DECRYPT_ERROR, 21),           /**< The decryption of the SecureLink packet failed */                  \
  SL_STATUS_ENUM(prefix, WRONG_STATE, 24),                         /**< The device is in an inappropriate state to perform the request */  \
  SL_STATUS_ENUM(prefix, CHANNEL_NOT_ALLOWED, 25),                 /**< The request failed due to regulatory limitations */                \
  SL_STATUS_ENUM(prefix, RETRY_EXCEEDED, 26),                      /**< The request failed because the retry limit was exceeded */         \
  SL_STATUS_ENUM(prefix, TX_LIFETIME_EXCEEDED, 27),                /**< The request failed because the MSDU life time was exceeded */

#endif // __SL_WIFI_STATUS_H
