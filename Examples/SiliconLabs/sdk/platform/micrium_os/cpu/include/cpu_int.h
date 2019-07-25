/***************************************************************************//**
 * @file
 * @brief CPU - Generic Interrupt Support
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
 * @defgroup CPU_INT CPU Interrupt API
 * @ingroup CPU
 * @brief      CPU Interrupt API (Deprecated)
 *
 * @addtogroup CPU_INT
 * @{
 *******************************************************************************************************/

/********************************************************************************************************
 ********************************************************************************************************
 *                                                   MODULE
 ********************************************************************************************************
 *******************************************************************************************************/

#ifndef  _CPU_INT_H_
#define  _CPU_INT_H_

/********************************************************************************************************
 ********************************************************************************************************
 *                                               INCLUDE FILES
 ********************************************************************************************************
 *******************************************************************************************************/

#include  <common/include/rtos_path.h>

#if  (RTOS_INT_CONTROLLER_NAME == RTOS_INT_CONTROLLER_NONE)
#include  <ports/source/generic/emul_cpu_int_priv.h>
#elif ((RTOS_INT_CONTROLLER_NAME == RTOS_INT_CONTROLLER_ARMV6_M) \
  || (RTOS_INT_CONTROLLER_NAME == RTOS_INT_CONTROLLER_ARMV7_M)   \
  || (RTOS_INT_CONTROLLER_NAME == RTOS_INT_CONTROLLER_ARMV8_M))
#include  <ports/source/generic/armv6m_v7m_cpu_int_priv.h>
#else
#error  "Unsupported RTOS_INT_CONTROLLER_NAME specified."
#endif

#include  "cpu_port_sel.h"

/********************************************************************************************************
 ********************************************************************************************************
 *                                    DEPRECATED FUNCTION PROTOTYPES
 ********************************************************************************************************
 *******************************************************************************************************/

#ifdef __cplusplus
extern  "C" {
#endif

void CPU_IntSrcDis(CPU_INT_ID id);

void CPU_IntSrcEn(CPU_INT_ID id);

void CPU_IntSrcPendClr(CPU_INT_ID id);

void CPU_IntSrcPendSet(CPU_INT_ID id);

CPU_INT_PRIO_RET CPU_IntSrcPrioGet(CPU_INT_ID id);

void CPU_IntSrcPrioSet(CPU_INT_ID   id,
                       CPU_INT_PRIO prio);

#ifdef __cplusplus
}
#endif

/****************************************************************************************************//**
 ********************************************************************************************************
 * @}                                           MODULE END
 ********************************************************************************************************
 *******************************************************************************************************/

#endif // End of CPU interrupt module include.
