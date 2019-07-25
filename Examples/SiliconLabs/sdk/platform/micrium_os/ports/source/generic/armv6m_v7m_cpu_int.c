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
 *
 * @note     (2) This file and all of its content is DEPRECATED and will be removed in a future version of
 *               this product.
 *******************************************************************************************************/

/********************************************************************************************************
 ********************************************************************************************************
 *                                             INCLUDE FILES
 ********************************************************************************************************
 *******************************************************************************************************/

#include  <cpu/include/cpu_int.h>

#include  <common/include/lib_def.h>
#include  <common/include/lib_utils.h>
#include  <common/include/toolchains.h>
#include  <common/include/rtos_path.h>

#ifdef  RTOS_MODULE_KERNEL_AVAIL
#include  <kernel/include/os.h>
#endif

#include  <em_device.h>

/********************************************************************************************************
 ********************************************************************************************************
 *                                              USAGE GUARD
 ********************************************************************************************************
 *******************************************************************************************************/

#if ((RTOS_INT_CONTROLLER_NAME == RTOS_INT_CONTROLLER_ARMV6_M) \
  || (RTOS_INT_CONTROLLER_NAME == RTOS_INT_CONTROLLER_ARMV7_M) \
  || (RTOS_INT_CONTROLLER_NAME == RTOS_INT_CONTROLLER_ARMV8_M))

/********************************************************************************************************
 *                              INTERRUPT CONTROLLER DRIVER IMPLEMENTATION
 *
 * @note     (1) This interrupt controller driver implements the necessary functions to use the NVIC in
 *               ARMv7-M and ARMv6-M devices.
 *
 * @note     (2) The CPU_INT_ID type contains the 'exception number' as defined in sections B1.5.(2-3)
 *               of the 'ARMv7-M and ARMv6-M Architecture Reference Manual'.
 *
 * @note     (3) The exception numbers 0-15 are core ARM exceptions. The exception numbers are defined below:
 *
 *                   0       Initial stack pointer.
 *                   1       Reset (initial program counter).
 *                   2       Non-Maskable Interrupt (NMI).
 *                   3       Hard Fault.
 *                   4       Memory Management Fault / Reserved, see note #4-C.
 *                   5       Bus Fault / Reserved, see note #4-C.
 *                   6       Usage Fault / Reserved, see note #4-C.
 *                   7-10    Reserved
 *                   11      Supervisor Call (SVCall).
 *                   12      Debug Monitor / Reserved, see note #4-C.
 *                   13      Reserved
 *                   14      PendSV.
 *                   15      SysTick.
 *                   16+     External interrupt.
 *
 * @note     (4) Several exceptions cannot contain an interrupt source:
 *
 *               (a) IDs 0 and 1, they contain the initial SP and PC.
 *               (b) IDs 7-10 and 13 are RESERVED.
 *               (c) IDs 4-6 and 12 are RESERVERD on ARMv6m architecture.
 *
 * @note     (5) Several exceptions cannot be disabled:
 *
 *               (a) Reset.
 *               (b) Non-Maskable Interrupt (NMI).
 *               (c) Hard Fault.
 *               (d) Supervisor Call (SVCall).
 *               (e) Debug Monitor.
 *               (f) PendSV.
 *
 * @note     (6) Three exceptions have a fixed priority:
 *
 *               (a) The Reset                        has a fixed priority of -3.
 *               (b) The Non-Maskable Interrupt (NMI) has a fixed priority of -2.
 *               (c) The Hard Fault                   has a fixed priority of -1.
 *******************************************************************************************************/

/********************************************************************************************************
 ********************************************************************************************************
 *                               INTERRUPT CONTROLLER SUPPORTED FUNCTIONS
 ********************************************************************************************************
 *******************************************************************************************************/

/****************************************************************************************************//**
 *                                            CPU_IntSrcDis()
 *
 * @brief    Disable an interrupt.
 *
 * @param    id  The ID of the interrupt.
 *
 * @note     (1) See 'INTERRUPT CONTROLLER DRIVER IMPLEMENTATION'.
 *
 * @note     (2) This function is DEPRECATED and will be removed in a future version of this product.
 *******************************************************************************************************/
void CPU_IntSrcDis(CPU_INT_ID id)
{
  CPU_SR_ALLOC();

  switch (id) {
    case CPU_INT_STK_PTR:                                       // ---------------- INVALID OR RESERVED ---------------
#if (RTOS_CPU_PORT_NAME == RTOS_CPU_SEL_ARM_V6_M)
    case CPU_INT_RSVD_04:
    case CPU_INT_RSVD_05:
    case CPU_INT_RSVD_06:
#endif
    case CPU_INT_RSVD_07:
    case CPU_INT_RSVD_08:
    case CPU_INT_RSVD_09:
    case CPU_INT_RSVD_10:
#if (RTOS_CPU_PORT_NAME == RTOS_CPU_SEL_ARM_V6_M)
    case CPU_INT_RSVD_12:
#endif
    case CPU_INT_RSVD_13:
    case CPU_INT_RESET:                                         // ------------------ CORE EXCEPTIONS -----------------
    case CPU_INT_NMI:
    case CPU_INT_HFAULT:
    case CPU_INT_SVCALL:
#if (RTOS_CPU_PORT_NAME == RTOS_CPU_SEL_ARM_V7_M)
    case CPU_INT_DBGMON:
#endif
    case CPU_INT_PENDSV:
      break;

#if (RTOS_CPU_PORT_NAME == RTOS_CPU_SEL_ARM_V7_M)
    case CPU_INT_MEM:                                           // Memory Management.
      CPU_CRITICAL_ENTER();
      SCB->SHCSR &= ~SCB_SHCSR_MEMFAULTENA_Msk;
      CPU_CRITICAL_EXIT();
      break;

    case CPU_INT_BUSFAULT:                                      // Bus Fault.
      CPU_CRITICAL_ENTER();
      SCB->SHCSR &= ~SCB_SHCSR_BUSFAULTENA_Msk;
      CPU_CRITICAL_EXIT();
      break;

    case CPU_INT_USAGEFAULT:                                    // Usage Fault.
      CPU_CRITICAL_ENTER();
      SCB->SHCSR &= ~SCB_SHCSR_USGFAULTENA_Msk;
      CPU_CRITICAL_EXIT();
      break;
#endif

    case CPU_INT_SYSTICK:                                       // SysTick.
      CPU_CRITICAL_ENTER();
      SysTick->CTRL &= ~SysTick_CTRL_ENABLE_Msk;
      CPU_CRITICAL_EXIT();
      break;
    //                                                             ---------------- EXTERNAL INTERRUPT ----------------
    default:
      if (id < CPU_INT_NBR_OF_INT) {
        CPU_CRITICAL_ENTER();
        NVIC_DisableIRQ((IRQn_Type) (id - CPU_INT_EXT0));
        CPU_CRITICAL_EXIT();
      }
      break;
  }
}

/****************************************************************************************************//**
 *                                            CPU_IntSrcEn()
 *
 * @brief    Enable an interrupt.
 *
 * @param    id  The ID of the interrupt.
 *
 * @note     (1) See 'INTERRUPT CONTROLLER DRIVER IMPLEMENTATION'.
 *
 * @note     (2) This function is DEPRECATED and will be removed in a future version of this product.
 *******************************************************************************************************/
void CPU_IntSrcEn(CPU_INT_ID id)
{
  CPU_SR_ALLOC();

  switch (id) {
    case CPU_INT_STK_PTR:                                       // ---------------- INVALID OR RESERVED ---------------
#if (RTOS_CPU_PORT_NAME == RTOS_CPU_SEL_ARM_V6_M)
    case CPU_INT_RSVD_04:
    case CPU_INT_RSVD_05:
    case CPU_INT_RSVD_06:
#endif
    case CPU_INT_RSVD_07:
    case CPU_INT_RSVD_08:
    case CPU_INT_RSVD_09:
    case CPU_INT_RSVD_10:
#if (RTOS_CPU_PORT_NAME == RTOS_CPU_SEL_ARM_V6_M)
    case CPU_INT_RSVD_12:
#endif
    case CPU_INT_RSVD_13:
      break;
    case CPU_INT_RESET:                                         // ------------------ CORE EXCEPTIONS -----------------
    case CPU_INT_NMI:
    case CPU_INT_HFAULT:
    case CPU_INT_SVCALL:
#if (RTOS_CPU_PORT_NAME == RTOS_CPU_SEL_ARM_V7_M)
    case CPU_INT_DBGMON:
#endif
    case CPU_INT_PENDSV:
      break;

#if (RTOS_CPU_PORT_NAME == RTOS_CPU_SEL_ARM_V7_M)
    case CPU_INT_MEM:                                           // Memory Management.
      CPU_CRITICAL_ENTER();
      SCB->SHCSR |= SCB_SHCSR_MEMFAULTENA_Msk;
      CPU_CRITICAL_EXIT();
      break;

    case CPU_INT_BUSFAULT:                                      // Bus Fault.
      CPU_CRITICAL_ENTER();
      SCB->SHCSR |= SCB_SHCSR_BUSFAULTENA_Msk;
      CPU_CRITICAL_EXIT();
      break;

    case CPU_INT_USAGEFAULT:                                    // Usage Fault.
      CPU_CRITICAL_ENTER();
      SCB->SHCSR |= SCB_SHCSR_USGFAULTENA_Msk;
      CPU_CRITICAL_EXIT();
      break;
#endif

    case CPU_INT_SYSTICK:                                       // SysTick.
      CPU_CRITICAL_ENTER();
      SysTick->CTRL |= SysTick_CTRL_ENABLE_Msk;
      CPU_CRITICAL_EXIT();
      break;
    //                                                             ---------------- EXTERNAL INTERRUPT ----------------
    default:
      if (id < CPU_INT_NBR_OF_INT) {
        CPU_CRITICAL_ENTER();
        NVIC_EnableIRQ((IRQn_Type) (id - CPU_INT_EXT0));
        CPU_CRITICAL_EXIT();
      }
      break;
  }
}

/****************************************************************************************************//**
 *                                          CPU_IntSrcPendClr()
 *
 * @brief    Clears the pending status of a specific interrupt.
 *
 * @param    id  The ID of the interrupt.
 *
 * @note     (1) This function is DEPRECATED and will be removed in a future version of this product.
 *******************************************************************************************************/
void CPU_IntSrcPendClr(CPU_INT_ID id)
{
  CPU_SR_ALLOC();

  switch (id) {
    case CPU_INT_STK_PTR:                                       // ---------------- INVALID OR RESERVED ---------------
#if (RTOS_CPU_PORT_NAME == RTOS_CPU_SEL_ARM_V6_M)
    case CPU_INT_RSVD_04:
    case CPU_INT_RSVD_05:
    case CPU_INT_RSVD_06:
#endif
    case CPU_INT_RSVD_07:
    case CPU_INT_RSVD_08:
    case CPU_INT_RSVD_09:
    case CPU_INT_RSVD_10:
#if (RTOS_CPU_PORT_NAME == RTOS_CPU_SEL_ARM_V6_M)
    case CPU_INT_RSVD_12:
#endif
    case CPU_INT_RSVD_13:
    case CPU_INT_RESET:                                         // ------------------ CORE EXCEPTIONS -----------------
    case CPU_INT_NMI:
    case CPU_INT_HFAULT:
#if (RTOS_CPU_PORT_NAME == RTOS_CPU_SEL_ARM_V7_M)
    case CPU_INT_MEM:
#endif
    case CPU_INT_SVCALL:
#if (RTOS_CPU_PORT_NAME == RTOS_CPU_SEL_ARM_V7_M)
    case CPU_INT_DBGMON:
    case CPU_INT_BUSFAULT:
    case CPU_INT_USAGEFAULT:
#endif
      break;

    case CPU_INT_PENDSV:                                        // PendSV.
      CPU_CRITICAL_ENTER();
      SCB->ICSR |= SCB_ICSR_PENDSVCLR_Msk;
      CPU_CRITICAL_EXIT();
      break;

    case CPU_INT_SYSTICK:                                       // SysTick.
      CPU_CRITICAL_ENTER();
      SCB->ICSR |= SCB_ICSR_PENDSTCLR_Msk;
      CPU_CRITICAL_EXIT();
      break;
    //                                                             ---------------- EXTERNAL INTERRUPTS ---------------
    default:
      if (id < CPU_INT_NBR_OF_INT) {
        CPU_CRITICAL_ENTER();
        NVIC_ClearPendingIRQ((IRQn_Type) (id - CPU_INT_EXT0));
        CPU_CRITICAL_EXIT();
      }
      break;
  }
}

/****************************************************************************************************//**
 *                                          CPU_IntSrcPendSet()
 *
 * @brief    Triggers a software generated interrupt for a specific interrupt.
 *
 * @param    id  The ID of the interrupt.
 *
 * @note     (1) This function is DEPRECATED and will be removed in a future version of this product.
 *******************************************************************************************************/
void CPU_IntSrcPendSet(CPU_INT_ID id)
{
  CPU_SR_ALLOC();

  switch (id) {
    case CPU_INT_STK_PTR:                                       // ---------------- INVALID OR RESERVED ---------------
#if (RTOS_CPU_PORT_NAME == RTOS_CPU_SEL_ARM_V6_M)
    case CPU_INT_RSVD_04:
    case CPU_INT_RSVD_05:
    case CPU_INT_RSVD_06:
#endif
    case CPU_INT_RSVD_07:
    case CPU_INT_RSVD_08:
    case CPU_INT_RSVD_09:
    case CPU_INT_RSVD_10:
#if (RTOS_CPU_PORT_NAME == RTOS_CPU_SEL_ARM_V6_M)
    case CPU_INT_RSVD_12:
#endif
    case CPU_INT_RSVD_13:
    case CPU_INT_RESET:                                         // ------------------ CORE EXCEPTIONS -----------------
    case CPU_INT_HFAULT:
#if (RTOS_CPU_PORT_NAME == RTOS_CPU_SEL_ARM_V7_M)
    case CPU_INT_MEM:
#endif
    case CPU_INT_SVCALL:
#if (RTOS_CPU_PORT_NAME == RTOS_CPU_SEL_ARM_V7_M)
    case CPU_INT_DBGMON:
    case CPU_INT_BUSFAULT:
    case CPU_INT_USAGEFAULT:
#endif
      break;

    case CPU_INT_NMI:                                           // Non-Maskable Interrupt.
      CPU_CRITICAL_ENTER();
#if (RTOS_INT_CONTROLLER_NAME == RTOS_INT_CONTROLLER_ARMV8_M)
      SCB->ICSR |= SCB_ICSR_PENDNMISET_Msk;
#else
      SCB->ICSR |= SCB_ICSR_NMIPENDSET_Msk;
#endif

      CPU_CRITICAL_EXIT();
      break;

    case CPU_INT_PENDSV:                                        // PendSV.
      CPU_CRITICAL_ENTER();
      SCB->ICSR |= SCB_ICSR_PENDSVSET_Msk;
      CPU_CRITICAL_EXIT();
      break;

    case CPU_INT_SYSTICK:                                       // SysTick.
      CPU_CRITICAL_ENTER();
      SCB->ICSR |= SCB_ICSR_PENDSTSET_Msk;
      CPU_CRITICAL_EXIT();
      break;
    //                                                             ---------------- EXTERNAL INTERRUPTS ---------------
    default:
      if (id < CPU_INT_NBR_OF_INT) {
        CPU_CRITICAL_ENTER();
        NVIC_SetPendingIRQ((IRQn_Type) (id - CPU_INT_EXT0));
        CPU_CRITICAL_EXIT();
      }
      break;
  }
}

/****************************************************************************************************//**
 *                                          CPU_IntSrcPrioGet()
 *
 * @brief    Gets the current priority for a specific interrupt.
 *
 * @param    id  The ID of the interrupt.
 *
 * @note     (1) See 'INTERRUPT CONTROLLER DRIVER IMPLEMENTATION'.
 *
 * @note     (2) This function is DEPRECATED and will be removed in a future version of this product.
 *******************************************************************************************************/
CPU_INT_PRIO_RET CPU_IntSrcPrioGet(CPU_INT_ID id)
{
  CPU_INT_PRIO_RET prio;
  CPU_SR_ALLOC();

  switch (id) {
    case CPU_INT_STK_PTR:                                       // ---------------- INVALID OR RESERVED ---------------
#if (RTOS_CPU_PORT_NAME == RTOS_CPU_SEL_ARM_V6_M)
    case CPU_INT_RSVD_04:
    case CPU_INT_RSVD_05:
    case CPU_INT_RSVD_06:
#endif
    case CPU_INT_RSVD_07:
    case CPU_INT_RSVD_08:
    case CPU_INT_RSVD_09:
    case CPU_INT_RSVD_10:
#if (RTOS_CPU_PORT_NAME == RTOS_CPU_SEL_ARM_V6_M)
    case CPU_INT_RSVD_12:
#endif
    case CPU_INT_RSVD_13:
      prio = DEF_INT_16S_MIN_VAL;
      break;

    default:
      CPU_CRITICAL_ENTER();
      prio = NVIC_GetPriority((IRQn_Type) (id - CPU_INT_EXT0));
      CPU_CRITICAL_EXIT();
      break;
  }

  return (prio);
}

/****************************************************************************************************//**
 *                                          CPU_IntSrcPrioSet()
 *
 * @brief    Sets the priority of a specific interrupt.
 *
 * @param    id      The ID of the interrupt.
 *
 * @param    prio    The priority.
 *
 * @note     (1) See 'INTERRUPT CONTROLLER DRIVER IMPLEMENTATION'.
 *
 * @note     (2) This function is DEPRECATED and will be removed in a future version of this product.
 *******************************************************************************************************/
void CPU_IntSrcPrioSet(CPU_INT_ID   id,
                       CPU_INT_PRIO prio)
{
  CPU_SR_ALLOC();

  if (id < CPU_INT_NBR_OF_INT) {
    CPU_CRITICAL_ENTER();
    NVIC_SetPriority((IRQn_Type) (id - CPU_INT_EXT0), prio);
    CPU_CRITICAL_EXIT();
  }
}

#endif // RTOS_INT_CONTROLLER_NAME
