/***************************************************************************//**
 * @file
 * @brief Common Configuration - Configuration Template File
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

#ifndef  _COMMON_CFG_H_
#define  _COMMON_CFG_H_

/********************************************************************************************************
 ********************************************************************************************************
 *                                             INCLUDE FILES
 ********************************************************************************************************
 *******************************************************************************************************/

#include  <common/include/lib_def.h>

/********************************************************************************************************
 ********************************************************************************************************
 *                                     MEMORY LIBRARY CONFIGURATION
 ********************************************************************************************************
 *******************************************************************************************************/

/********************************************************************************************************
 *                              MEMORY LIBRARY OPTIMIZATION CONFIGURATION
 *
 * Note(s) : (1) Configure LIB_MEM_CFG_STD_C_LIB_EN to enable/disable use of standard C lib (will include
 *               standard <string.h>) for Mem_Set()/memset(), Mem_Copy()/memcpy(), Mem_Move()/memmove(),
 *               Mem_Clr()/memset() and Mem_Cmp()/memcmp() functions.
 *
 *           (2) Configure LIB_MEM_CFG_MEM_COPY_OPTIMIZE_ASM_EN to enable/disable assembly-optimized
 *               memory copy function.
 *
 *           (3) LIB_MEM_CFG_STD_C_LIB_EN and LIB_MEM_CFG_MEM_COPY_OPTIMIZE_ASM_EN cannot be both enabled
 *               at the same time because we cannot know how the memory functions are called within
 *               standard C library.
 *
 *           (4) When using IAR, the implementation provided by the compiler is overridden to use either
 *               Micrium OS' C or assembly implementation (based on LIB_MEM_CFG_MEM_COPY_OPTIMIZE_ASM_EN).
 *               This is a workaround for a known issue with IAR's memcpy not working properly and doing
 *               unaligned accesses to memory. The IAR extensions are required for this workaround to work
 *               properly. It is possible to force the use of IAR's memcpy by #define'ing
 *               LIB_MEM_COPY_FNCT_PREFIX as nothing or by setting LIB_MEM_CFG_STD_C_LIB_EN to
 *               DEF_ENABLED. In those cases, the unaligned access issue would still be present.
 *******************************************************************************************************/

#define  LIB_MEM_CFG_STD_C_LIB_EN                           DEF_DISABLED

#define  LIB_MEM_CFG_MEM_COPY_OPTIMIZE_ASM_EN               DEF_DISABLED

/********************************************************************************************************
 *                                   MEMORY ALLOCATION CONFIGURATION
 *
 * Note(s) : (1) Configure LIB_MEM_CFG_DBG_INFO_EN to enable/disable memory allocation usage tracking
 *               that associates a name with each segment or dynamic pool allocated.
 *
 *           (2) Configure LIB_MEM_CFG_HEAP_SIZE with the desired size of heap memory (in octets).
 *
 *           (3) Configure LIB_MEM_CFG_HEAP_BASE_ADDR to specify a base address for heap memory :
 *
 *               (a) Heap initialized to specified application memory, if LIB_MEM_CFG_HEAP_BASE_ADDR
 *                   #define'd in 'common_cfg.h'; CANNOT #define to address 0x0.
 *
 *               (b) Heap declared to Mem_Heap[] in 'lib_mem.c', if LIB_MEM_CFG_HEAP_BASE_ADDR NOT
 *                   #define'd in 'common_cfg.h'.
 *******************************************************************************************************/

#define  LIB_MEM_CFG_DBG_INFO_EN                            DEF_ENABLED

#define  LIB_MEM_CFG_HEAP_SIZE                              (36*1024)

#define  LIB_MEM_CFG_HEAP_PADDING_ALIGN                     LIB_MEM_PADDING_ALIGN_NONE

#if 0                                                           // Remove this to have heap alloc at specified addr.
#define  LIB_MEM_CFG_HEAP_BASE_ADDR       0x00000000            // Configure heap memory base address (see Note #2b).
#endif

/********************************************************************************************************
 ********************************************************************************************************
 *                                     STRING LIBRARY CONFIGURATION
 ********************************************************************************************************
 *******************************************************************************************************/

/********************************************************************************************************
 *                                 STRING FLOATING POINT CONFIGURATION
 *
 * Note(s) : (1) Configure LIB_STR_CFG_FP_EN to enable/disable floating point string function(s).
 *
 *           (2) Configure LIB_STR_CFG_FP_MAX_NBR_DIG_SIG to set the maximum number of significant digits
 *               to calculate and/or display for floating point string function(s). See also 'lib_str.h
 *               STRING FLOATING POINT DEFINES Note #1'.
 *******************************************************************************************************/

#define  LIB_STR_CFG_FP_EN                                  DEF_DISABLED

#define  LIB_STR_CFG_FP_MAX_NBR_DIG_SIG                     LIB_STR_FP_MAX_NBR_DIG_SIG_DFLT

/********************************************************************************************************
 ********************************************************************************************************
 *                                         CLOCK CONFIGURATION
 *
 * Note(s) : (1) Configure CLK_CFG_EXT_EN to enable/disable an externally-maintained clock :
 *
 *               (a) When ENABLED,  clock is maintained externally via hardware or another application
 *                   (see also 'clk.h  Note #3').
 *
 *               (b) When DISABLED, clock is maintained internally via software (see also 'clk.h  Note
 *                   #2').
 *
 *           (2) Configure CLK_CFG_SIGNAL_EN to enable/disable signaling of the internally-maintained
 *               software clock. CLK_CFG_SIGNAL_EN configuration is required only if CLK_CFG_EXT_EN is
 *               disabled. See also 'clk.h  Note #2b'.
 *
 *               (a) When ENABLED,  clock is signaled by application calls to Clk_SignalClk().
 *
 *               (b) When DISABLED, clock is signaled by OS-dependent timing features.
 *
 *           (3) Configure CLK_CFG_SIGNAL_FREQ_HZ to the number of times the application will signal the
 *               clock every second. CLK_CFG_SIGNAL_FREQ_HZ configuration is required only if
 *               CLK_CFG_SIGNAL_EN is enabled.
 ********************************************************************************************************
 *******************************************************************************************************/

#define  CLK_CFG_STR_CONV_EN                                DEF_ENABLED

#define  CLK_CFG_NTP_EN                                     DEF_ENABLED

#define  CLK_CFG_UNIX_EN                                    DEF_ENABLED

#define  CLK_CFG_EXT_EN                                     DEF_DISABLED

#define  CLK_CFG_SIGNAL_EN                                  DEF_DISABLED

#define  CLK_CFG_SIGNAL_FREQ_HZ                             1000u

/********************************************************************************************************
 ********************************************************************************************************
 *                                             MODULE END
 ********************************************************************************************************
 *******************************************************************************************************/

#endif // End of common_cfg.h module include.
