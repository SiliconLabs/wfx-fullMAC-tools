/***************************************************************************//**
 * @file
 * @brief CPU - Interrupt Controller Driver - Emulation
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
 * @note     (1) This driver targets the following:
 *               Core      : POSIX and Win32
 *******************************************************************************************************/

/********************************************************************************************************
 ********************************************************************************************************
 *                                                   MODULE
 ********************************************************************************************************
 *******************************************************************************************************/

#ifndef  _CPU_INT_NONE_PRIV_H_
#define  _CPU_INT_NONE_PRIV_H_

/********************************************************************************************************
 ********************************************************************************************************
 *                                               INCLUDE FILES
 ********************************************************************************************************
 *******************************************************************************************************/

#include  <common/include/rtos_path.h>

#include  <cpu/include/cpu_port_sel.h>

#include  <common/include/lib_def.h>

/********************************************************************************************************
 ********************************************************************************************************
 *                                               USAGE GUARD
 ********************************************************************************************************
 *******************************************************************************************************/

#if (RTOS_INT_CONTROLLER_NAME == RTOS_INT_CONTROLLER_NONE)

/********************************************************************************************************
 *                                           CPU INTERRUPT TYPES
 *
 * Note(s) : (1) The driver implementation MUST define, based on CPU types, the following types:
 *
 *                   CPU_INT_ID          Type used to identify an interrupt.
 *
 *                   CPU_INT_PRIO        Type used to hold the priority of an interrupt.
 *
 *                   CPU_INT_PRIO_RET    Type used to hold the effective priority. Can be nagative.
 *******************************************************************************************************/

typedef CPU_INT16U CPU_INT_ID;
typedef CPU_INT08U CPU_INT_PRIO;
typedef CPU_INT16S CPU_INT_PRIO_RET;

/********************************************************************************************************
 *                                       CPU INTERRUPT CONFIGURATION
 *
 * Note(s) : (1) The driver implementation MUST define the following configuration parameters:
 *
 *                   CPU_INT_NBR_OF_INT          Indicates the number of interrupts the controller supports.
 *******************************************************************************************************/

#define  CPU_INT_NBR_OF_INT                             128u

#endif // RTOS_INT_CONTROLLER_NAME

#endif // _CPU_INT_NONE_PRIV_H_
