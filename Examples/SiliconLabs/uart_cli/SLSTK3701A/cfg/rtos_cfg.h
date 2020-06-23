/***************************************************************************//**
 * @file
 * @brief RTOS Configuration - Configuration Template File
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

#ifndef  _RTOS_CFG_H_
#define  _RTOS_CFG_H_

/********************************************************************************************************
 ********************************************************************************************************
 *                                             INCLUDE FILES
 ********************************************************************************************************
 *******************************************************************************************************/

#include  <common/include/rtos_opt_def.h>
#include  <common/include/lib_def.h>
#include  <cpu/include/cpu.h>

/********************************************************************************************************
 ********************************************************************************************************
 *                                               DEFINES
 ********************************************************************************************************
 *******************************************************************************************************/

/********************************************************************************************************
 *                                         ASSERTS CONFIGURATION
 *
 * Note(s) : (1) RTOS_CFG_ASSERT_DBG_ARG_CHK_EXT_MASK is used to configure whether asserts used in
 *               argument checking are enabled or not. These asserts are used to check the validity of the
 *               arguments passed to any stack function. This can include checking if a pointer is
 *               non-NULL, if a given value is within a certain range, if a configuration is valid, etc.
 *
 *               (a) This configuration define can be set as a bitmap of several module defines, to allow
 *                   more flexibility. For example, it could be define as RTOS_CFG_MODULE_ALL or
 *                   RTOS_CFG_MODULE_NONE to enable or disable it for every module. It could also be set
 *                   as a combination of particular modules such as: (RTOS_CFG_MODULE_CPU |
 *                   RTOS_CFG_MODULE_KERNEL), to keep the debug asserts only in the CPU and Kernel
 *                   modules, but not in any other one.
 *
 *               (b) It is recommended to disable this feature as much as possible in 'release' code, to
 *                   improve performance and reduce the amount of code space required.
 *
 *           (2) RTOS_CFG_RTOS_ASSERT_DBG_FAILED_END_CALL is used to configure what operations will happen
 *               in cases where a debug assert is failed. Debug asserts failing typically reflects an
 *               invalid argument passed by the programmer to a stack function or an operation that cannot
 *               be done in a particular context (such as an ISR). The program SHOULD NOT continue running
 *               after a debug assert failed, although it is possible to configure it to return (by
 *               defining it to: return ret_val (without parentheses or ';'), if the code is being
 *               executed in a test context. Typical operations could include breaking the CPU, looping
 *               indefinitely, calling a function or macro, etc. The default behavior is to loop
 *               indefinitely.
 *
 *           (3) RTOS_CFG_RTOS_ASSERT_CRITICAL_FAILED_END_CALL is used to configure what operations will
 *               happen in cases where a critical assert is failed. Critical asserts failing reflects an
 *               unrecoverable situations in the system such as corruption or other cases that are
 *               unexpected. The program MUST NOT continue to execute in those cases. The program MUST NOT
 *               return from this call, since the software is in an unknown and/or invalid state. Typical
 *               operations could include outputting logs or traces, dumping memory, halting and/or
 *               restarting the system. The default behavior is to call the CPU_SW_EXCEPTION macro.
 *******************************************************************************************************/

#define  RTOS_CFG_ASSERT_DBG_ARG_CHK_EXT_MASK                    (RTOS_CFG_MODULE_ALL)

#define  RTOS_CFG_RTOS_ASSERT_DBG_FAILED_END_CALL(ret_val)       while (1) {; }

#define  RTOS_CFG_RTOS_ASSERT_CRITICAL_FAILED_END_CALL(ret_val)  CPU_SW_EXCEPTION(ret_val)

/********************************************************************************************************
 *                                        DFLT CFG CONFIGURATION
 *
 * Note(s) : (1) RTOS_CFG_EXTERNALIZE_OPTIONAL_CFG_EN determines whether the configurations can be
 *               provided via optional 'Configure' calls to the stacks or if these configurations are
 *               assumed extern by the stacks and must be provided by the application. If set to
 *               DEF_DISABLED, default configurations will be present in stacks and they can optionally
 *               be overridden with 'Configure' calls. This option is the simpler and should be the
 *               preferred way to use when starting development of an application. The default values
 *               used by the stacks are available for the application to copy, modify only parts of it
 *               and set as the new configuration. See each stack's user's manual for more information.
 *               If set to DEF_ENABLED, no default configuration will be present in the stacks and every
 *               configuration that could be set via 'Configure' calls are now assumed extern by the
 *               stacks and the application must define them somewhere. This option is useful to reduce
 *               the amount of memory and code space used.
 *******************************************************************************************************/

#define  RTOS_CFG_EXTERNALIZE_OPTIONAL_CFG_EN               DEF_DISABLED

/********************************************************************************************************
 *                                         LOGGING CONFIGURATION
 *
 * Note(s) : (1) RTOS_CFG_LOG_EN is used to enable or disable the logging module as a whole.
 *
 *           (2) If enabled, RTOS_CFG_LOG_ALL MUST be defined. All logging channels will inherit this
 *               configuration if not overridden by a more specific configuration as shown below.
 *
 *               (a) Values that can be used to configure logging channels are the following:
 *
 *                   (1) Lowest level to log:
 *                       VRB         Log error-, debug- and verbose-level messages.
 *                       DBG         Log error- and debug-level messages.
 *                       ERR         Log error-level messages only.
 *                       OFF         Do not log any level.
 *                       DFLT        Use the inherited setting.
 *
 *                   (2) Synchronicity of output:
 *                       SYNC        Output logs synchronously, during execution. May disrupt timing.
 *                       ASYNC       Save logs in buffer, always keeping the most recent ones.
 *                       .           Output will only be done when Log_Output() is called (see
 *                       .           rtos/common/include/logging.h for more details).
 *                       DFLT        Use the inherited setting.
 *
 *                   (3) Include function name in log entry:
 *                       FUNC_EN     Function name where entry was made will be included in entry.
 *                       FUNC_DIS    Function name where entry was made will NOT be included in entry.
 *                       DFLT        Use the inherited setting.
 *
 *                   (4) Include time-stamp in log entry:
 *                       TS_EN       Time-stamp of when entry was made will be included in entry.
 *                       TS_DIS      Time-stamp of when entry was made will NOT be included in entry.
 *                       DFLT        Use the inherited setting.
 *
 *                   (5) Log output function. Function must be of type 'int foo(int character)', as
 *                       'putchar', for example.
 *
 *               (b) If RTOS_CFG_LOG_ALL is defined to: VRB, ASYNC, FUNC_DIS, TS_DIS, putchar  and that
 *                   no more configuration is defined, every logging channel will use this configuration,
 *                   meaning that they will all output verbose (VRB) logs, in an asynchronous manner
 *                   (ASYNC), without the function name (FUNC_DIS) or the timestamp (TS_DIS) using the
 *                   'putchar' function as output.
 *
 *               (c) If another configuration is set for example for the kernel using the
 *                   RTOS_CFG_LOG_KERNEL define (RTOS_CFG_LOG_ appended with the kernel's channel name,
 *                   'KERNEL') and that this define is set to ERR, ASYNC, DFLT, TS_EN, this would mean
 *                   that the kernel channel (and only that channel) will output only error-level logs
 *                   (ERR), asynchronously (ASYNC), use the inherited value for function name (DFLT) and
 *                   override the timestamp configuration from RTOS_CFG_LOG_ALL by enabling it (TS_EN).
 *******************************************************************************************************/

#define  RTOS_CFG_LOG_EN                                    DEF_DISABLED

#if (RTOS_CFG_LOG_EN == DEF_ENABLED)
#include  <stdio.h>
#define  RTOS_CFG_LOG_ALL                                   VRB, SYNC, FUNC_DIS, TS_DIS, putchar
#endif

/********************************************************************************************************
 ********************************************************************************************************
 *                                             MODULE END
 ********************************************************************************************************
 *******************************************************************************************************/

#endif // End of rtos_cfg.h module include.
