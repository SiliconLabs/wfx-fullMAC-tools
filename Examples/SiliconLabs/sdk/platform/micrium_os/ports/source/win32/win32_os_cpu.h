/***************************************************************************//**
 * @file
 * @brief Kernel - Microsoft Win32 Emulation Port
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

/****************************************************************************************************//**
 * @note     (1) This port targets the following:
 *                   Core      : Win32
 *                   Mode      :
 *                   Toolchain : Visual Studio
 *
 * @note     (2) This port is an emulation support port.
 *******************************************************************************************************/

#ifndef  OS_CPU_H
#define  OS_CPU_H

#ifdef   OS_CPU_GLOBALS
#define  OS_CPU_EXT
#else
#define  OS_CPU_EXT  extern
#endif

#ifdef __cplusplus
extern  "C" {
#endif

/********************************************************************************************************
 *                                               INCLUDE FILES
 *******************************************************************************************************/

#include  <cpu/include/cpu.h>

#include  <common/include/rtos_path.h>
#include  <os_cfg.h>

#include  <cpu_cfg.h>

/********************************************************************************************************
 *                                               MACROS
 *******************************************************************************************************/

#define  OS_TASK_SW()               OSCtxSw()

/********************************************************************************************************
 *                                       TIMESTAMP CONFIGURATION
 *
 * Note(s) : (1) OS_TS_GET() is generally defined as CPU_TS_Get32() to allow CPU timestamp timer to be of
 *               any data type size.
 *
 *           (2) For architectures that provide 32-bit or higher precision free running counters
 *               (i.e. cycle count registers):
 *
 *               (a) OS_TS_GET() may be defined as CPU_TS_TmrRd() to improve performance when retrieving
 *                   the timestamp.
 *
 *               (b) CPU_TS_TmrRd() MUST be configured to be greater or equal to 32-bits to avoid
 *                   truncation of TS.
 *******************************************************************************************************/

#if      OS_CFG_TS_EN == 1u
#define  OS_TS_GET()               (CPU_TS)CPU_TS_TmrRd()       // See Note #2a.
#else
#define  OS_TS_GET()               (CPU_TS)0u
#endif

/********************************************************************************************************
 *                                   WIN32 TIMER METHOD CONFIGURATION
 *******************************************************************************************************/

#define  WIN32_SLEEP                       1u
#define  WIN32_MM_TMR                      2u                   // Use the high resolution Multimedia timer.

#ifndef  OS_CFG_TIMER_METHOD_WIN32
#define  OS_CFG_TIMER_METHOD_WIN32          WIN32_SLEEP
#endif

/********************************************************************************************************
 *                                           FUNCTION PROTOTYPES
 *******************************************************************************************************/

void OSCtxSw(void);
void OSIntCtxSw(void);

void OSStartHighRdy(void);

CPU_BOOLEAN OSIntCurTaskSuspend(void);
CPU_BOOLEAN OSIntCurTaskResume(void);

void OSDebuggerBreak(void);

#ifdef __cplusplus
}
#endif

#endif
