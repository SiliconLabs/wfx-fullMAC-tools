/***************************************************************************//**
 * # License
 *
 * The licensor of this software is Silicon Laboratories Inc. Your use of this
 * software is governed by the terms of Silicon Labs Master Software License
 * Agreement (MSLA) available at
 * www.silabs.com/about-us/legal/master-software-license-agreement. This
 * software is Third Party Software licensed by Silicon Labs from a third party
 * and is governed by the sections of the MSLA applicable to Third Party
 * Software and the additional terms set forth below.
 *
 ******************************************************************************/
/**
 * \file doc_tcpip.h
 *
 * \brief TCP/IP communication module documentation file.
 */
/*
 *
 *  Copyright (C) 2006-2015, ARM Limited, All Rights Reserved
 *  SPDX-License-Identifier: Apache-2.0
 *
 *  Licensed under the Apache License, Version 2.0 (the "License"); you may
 *  not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 *  WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 *
 *  This file is part of mbed TLS (https://tls.mbed.org)
 */

/**
 * @addtogroup tcpip_communication_module TCP/IP communication module
 *
 * The TCP/IP communication module provides for a channel of
 * communication for the \link ssltls_communication_module SSL/TLS communication
 * module\endlink to use.
 * In the TCP/IP-model it provides for communication up to the Transport
 * (or Host-to-host) layer.
 * SSL/TLS resides on top of that, in the Application layer, and makes use of
 * its basic provisions:
 * - listening on a port (see \c mbedtls_net_bind()).
 * - accepting a connection (through \c mbedtls_net_accept()).
 * - read/write (through \c mbedtls_net_recv()/\c mbedtls_net_send()).
 * - close a connection (through \c mbedtls_net_close()).
 *
 * This way you have the means to, for example, implement and use an UDP or
 * IPSec communication solution as a basis.
 *
 * This module can be used at server- and clientside to provide a basic
 * means of communication over the internet.
 */
