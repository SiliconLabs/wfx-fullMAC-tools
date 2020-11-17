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
 * \file doc_rng.h
 *
 * \brief Random number generator (RNG) module documentation file.
 *
 *
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
 * @addtogroup rng_module Random number generator (RNG) module
 *
 * The Random number generator (RNG) module provides random number
 * generation, see \c mbedtls_ctr_drbg_random().
 *
 * The block-cipher counter-mode based deterministic random
 * bit generator (CTR_DBRG) as specified in NIST SP800-90. It needs an external
 * source of entropy. For these purposes \c mbedtls_entropy_func() can be used.
 * This is an implementation based on a simple entropy accumulator design.
 *
 * The other number generator that is included is less strong and uses the
 * HAVEGE (HArdware Volatile Entropy Gathering and Expansion) software heuristic
 * which considered unsafe for primary usage, but provides additional randomness
 * to the entropy pool if enabled.
 *
 * Meaning that there seems to be no practical algorithm that can guess
 * the next bit with a probability larger than 1/2 in an output sequence.
 *
 * This module can be used to generate random numbers.
 *
 * @section sl_rng Silicon Labs Hardware Entropy Source Plugins
 *
 * ADC, RADIO and True Random Number Generator (TRNG) hardware peripheral plugins
 * are available for Silicon Labs. Peripheral availability depends on the target device.
 * Entropy source plugins are implemented with a back-end interface for the entropy
 * accumulator of mbed TLS in order to generate cryptographically secure random numbers.
 *
 * @subsection sl_adc_entropy ADC Entropy Source Plugin
 * The plugin supports hardware generated entropy from the ADC peripheral. The IADC peripheral
 * is not supported. See @ref sl_entropy_adc for more details.
 *
 * @subsection sl_rail_entropy Radio (RAIL) Entropy Source Plugin
 * The RAIL entropy plugin collects data from the radio on EFR32 devices. This plugin depends on the RAIL library.
 * See @ref sl_entropy_rail for more details.
 *
 * @subsection sl_trng_entropy True Random Number Generator (TRNG) Plugin
 * The TRNG entropy plugin collects data from from a dedicated NIST-800-90B compliant source. The TRNG peripheral may
 * either be stand-alone or integrated in the SE or CRYPTOACC peripheral depending on the target device.
 * See @ref sl_entropy_trng for more details.
 *
 * @} end group rng_module */




