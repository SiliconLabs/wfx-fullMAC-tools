/***************************************************************************//**
 * @file
 * @brief CPU Configuration - Configuration Template File
 *******************************************************************************
 * # License
 * <b>Copyright 2018 Silicon Laboratories Inc. www.silabs.com</b>
 *******************************************************************************
 *
 * The licensor of this software is Silicon Laboratories Inc.  Your use of this
 * software is governed by the terms of Silicon Labs Master Software License
 * Agreement (MSLA) available at
 * www.silabs.com/about-us/legal/master-software-license-agreement.  This
 * software is distributed to you in Source Code format and is governed by the
 * sections of the MSLA applicable to Source Code.
 *
 ******************************************************************************/

/********************************************************************************************************
 ********************************************************************************************************
 *                                               MODULE
 ********************************************************************************************************
 *******************************************************************************************************/

#ifndef  _CPU_CFG_H_
#define  _CPU_CFG_H_

/********************************************************************************************************
 ********************************************************************************************************
 *                                        CPU NAME CONFIGURATION
 *
 * Note(s) : (1) Configure CPU_CFG_NAME_EN to enable/disable CPU host name feature for CPU host name
 *               storage and CPU host name API functions.
 *
 *           (2) Configure CPU_CFG_NAME_SIZE with the desired ASCII string size of the CPU host name,
 *               including the terminating NULL character.
 ********************************************************************************************************
 *******************************************************************************************************/

#define  CPU_CFG_NAME_EN                                    DEF_DISABLED

#define  CPU_CFG_NAME_SIZE                                  16

/********************************************************************************************************
 ********************************************************************************************************
 *                                     CPU TIMESTAMP CONFIGURATION
 *
 * Note(s) : (1) Configure CPU_CFG_TS_32_EN to enable/disable 32-bits CPU timestamp feature.
 *
 *           (2) Configure CPU_CFG_TS_64_EN to enable/disable 64-bits CPU timestamp feature.
 *
 *           (3) Configure CPU_CFG_TS_TMR_SIZE with the CPU timestamp timer's word size: CPU_WORD_SIZE_08
 *               for 8-bit word size, CPU_WORD_SIZE_16 for 16-bit word size, CPU_WORD_SIZE_32 for 32-bit
 *               word size or CPU_WORD_SIZE_64 for 64-bit word size.
 *
 *           (4) If the size of the CPU timestamp timer is not a binary multiple of 8-bit octets (e.g.
 *               20-bits or even 24-bits), then the next lower, binary-multiple octet word size SHOULD be
 *               configured (e.g. to 16-bits). However, the minimum supported word size for CPU timestamp
 *               timers is 8-bits. See also 'cpu.h  FUNCTION PROTOTYPES  CPU_TS_TmrRd()  Note #2a'.
 ********************************************************************************************************
 *******************************************************************************************************/

#define  CPU_CFG_TS_32_EN                                   DEF_ENABLED

#define  CPU_CFG_TS_64_EN                                   DEF_DISABLED

#define  CPU_CFG_TS_TMR_SIZE                                CPU_WORD_SIZE_32

/********************************************************************************************************
 ********************************************************************************************************
 *                        CPU INTERRUPTS DISABLED TIME MEASUREMENT CONFIGURATION
 *
 * Note(s) : (1) Configure CPU_CFG_INT_DIS_MEAS_EN to enable/disable measuring CPU's interrupts disabled
 *               time : enabled, if CPU_CFG_INT_DIS_MEAS_EN is #define'd in 'cpu_cfg.h'; disabled if
 *               CPU_CFG_INT_DIS_MEAS_EN is NOT #define'd.
 *
 *           (2) Configure CPU_CFG_INT_DIS_MEAS_OVRHD_NBR with the number of times to measure & average
 *               the interrupts disabled time measurements overhead. See also 'cpu.c CPU_IntDisMeasInit()
 *               Note #3a'.
 ********************************************************************************************************
 *******************************************************************************************************/

#define  CPU_CFG_INT_DIS_MEAS_OVRHD_NBR                     1u

#if 0
#define  CPU_CFG_INT_DIS_MEAS_EN
#endif

/********************************************************************************************************
 ********************************************************************************************************
 *                                           CACHE MANAGEMENT
 *
 * Note(s) : (1) Defining CPU_CFG_CACHE_MGMT_EN to DEF_ENABLED only enables the cache management
 *               function. Caches are assumed to be configured and enabled by the time CPU_init() is
 *               called.
 ********************************************************************************************************
 *******************************************************************************************************/

#define  CPU_CFG_CACHE_MGMT_EN                              DEF_DISABLED

/********************************************************************************************************
 ********************************************************************************************************
 *                                             MODULE END
 ********************************************************************************************************
 *******************************************************************************************************/

#endif // End of cpu_cfg.h module include.
