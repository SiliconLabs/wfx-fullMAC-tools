/***************************************************************************//**
 * @file
 * @brief CPU - Interrupt Controller Driver
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
 *               Core      : ARMv6M Cortex-M
 *                           ARMv7M Cortex-M
 *                           ARMv8M Cortex-M
 *               Toolchain : Any
 *******************************************************************************************************/

/********************************************************************************************************
 ********************************************************************************************************
 *                                                 MODULE
 ********************************************************************************************************
 *******************************************************************************************************/

#ifndef  _CPU_INT_ARMV6M_V7M_PRIV_H_
#define  _CPU_INT_ARMV6M_V7M_PRIV_H_

/********************************************************************************************************
 ********************************************************************************************************
 *                                             INCLUDE FILES
 ********************************************************************************************************
 *******************************************************************************************************/

#include  <common/include/rtos_path.h>

#include  <cpu/include/cpu_port_sel.h>

#include  <common/include/lib_def.h>

/********************************************************************************************************
 ********************************************************************************************************
 *                                              USAGE GUARD
 ********************************************************************************************************
 *******************************************************************************************************/

#if ((RTOS_INT_CONTROLLER_NAME == RTOS_INT_CONTROLLER_ARMV6_M) \
  || (RTOS_INT_CONTROLLER_NAME == RTOS_INT_CONTROLLER_ARMV7_M) \
  || (RTOS_INT_CONTROLLER_NAME == RTOS_INT_CONTROLLER_ARMV8_M))

/********************************************************************************************************
 *                                          CPU INTERRUPT TYPES
 *
 * @note     (1) The driver implementation MUST define, based on CPU types, the following types:
 *
 *                   CPU_INT_ID          Type used to identify an interrupt.
 *
 *                   CPU_INT_PRIO        Type used to hold the priority of an interrupt.
 *
 *                   CPU_INT_PRIO_RET    Type used to hold the effective priority. Can be negative.
 *
 *******************************************************************************************************/

typedef CPU_INT16U CPU_INT_ID;
typedef CPU_INT08U CPU_INT_PRIO;
typedef CPU_INT16S CPU_INT_PRIO_RET;

/********************************************************************************************************
 *                                      CPU INTERRUPT CONFIGURATION
 *
 * @note     (1) The driver implementation MUST define the following configuration parameters:
 *
 *                   CPU_INT_NBR_OF_INT          Indicates the number of interrupts the controller supports.
 *******************************************************************************************************/

//                                                                 Based on the selected MCU, determine number of
//                                                                 supported interrupts.
#if (RTOS_CPU_SEL == RTOS_CPU_SEL_SILABS_GECKO_AUTO)
  #include  <em_core.h>
    #define  CPU_INT_NBR_OF_INT                            (CORE_DEFAULT_VECTOR_TABLE_ENTRIES)

#elif  ((RTOS_CPU_SEL == RTOS_CPU_SEL_ARM_CORTEX_M3) \
  || (RTOS_CPU_SEL == RTOS_CPU_SEL_ARM_CORTEX_M4)    \
  || (RTOS_CPU_SEL == RTOS_CPU_SEL_ARM_CORTEX_M7)    \
  || (RTOS_CPU_SEL == RTOS_CPU_SEL_ARM_V6_M)         \
  || (RTOS_CPU_SEL == RTOS_CPU_SEL_ARM_V7_M))

  #ifndef  CPU_INT_NBR_OF_INT
        #define  CPU_INT_NBR_OF_INT                         256u
  #endif
#else
    #ifndef  CPU_INT_NBR_OF_INT
        #define  CPU_INT_NBR_OF_INT                         496u
    #endif
#endif

#ifdef   CPU_CFG_INT_DIS_MEAS_EN
//                                                                 Disable interrupts, ...
//                                                                 & start interrupts disabled time measurement.
#define  CPU_ATOMIC_ENTER()  do { CORE_ENTER_ATOMIC(); \
                                  CPU_IntDisMeasStart(); }  while (0)
//                                                                 Stop & measure   interrupts disabled time,
//                                                                 ...  & re-enable interrupts.
#define  CPU_ATOMIC_EXIT()   do { CPU_IntDisMeasStop(); \
                                  CORE_EXIT_ATOMIC(); }  while (0)

#else

#define  CPU_ATOMIC_ENTER()   CORE_ENTER_ATOMIC()  // Disable   interrupts.
#define  CPU_ATOMIC_EXIT()    CORE_EXIT_ATOMIC()   // Re-enable interrupts.

#endif

/********************************************************************************************************
 ********************************************************************************************************
 *                                              MODULE END
 ********************************************************************************************************
 *******************************************************************************************************/

#endif // RTOS_INT_CONTROLLER_NAME

#endif // _CPU_INT_ARMV6M_V7M_PRIV_H_
