;
;/***************************************************************************//**
; * @file
; * @brief CPU - CPU Port File
; *******************************************************************************
; * # License
; * <b>Copyright 2018 Silicon Laboratories Inc. www.silabs.com</b>
; *******************************************************************************
; *
; * The licensor of this software is Silicon Laboratories Inc.  Your use of this
; * software is governed by the terms of Silicon Labs Master Software License
; * Agreement (MSLA) available at
; * www.silabs.com/about-us/legal/master-software-license-agreement.  This
; * software is distributed to you in Source Code format and is governed by the
; * sections of the MSLA applicable to Source Code.
; *
; ******************************************************************************/

;*****************************************************************************************************//**
;* @note     (1) This driver targets the following:
;*               Core      : ARMv6M Cortex-M
;*               Toolchain : ARMCC Compiler
;********************************************************************************************************


;********************************************************************************************************
;                                      DEPRECATED PUBLIC FUNCTIONS
;********************************************************************************************************

        EXPORT  CPU_IntDis
        EXPORT  CPU_IntEn

        EXPORT  CPU_SR_Save
        EXPORT  CPU_SR_Restore

        EXPORT  CPU_WaitForInt
        EXPORT  CPU_WaitForExcept


;********************************************************************************************************
;                                      CODE GENERATION DIRECTIVES
;********************************************************************************************************

        AREA |.text|, CODE, READONLY, ALIGN=2
        THUMB
        REQUIRE8
        PRESERVE8

;********************************************************************************************************
;                                    DISABLE and ENABLE INTERRUPTS
;
; @brief    Disable/Enable interrupts.
;
; Prototypes  : void  CPU_IntDis(void);
;               void  CPU_IntEn (void);
;
; Note(s)    : These functions are deprecated.
;********************************************************************************************************

CPU_IntDis
        CPSID   I
        BX      LR


CPU_IntEn
        CPSIE   I
        BX      LR


;********************************************************************************************************
;                                      CRITICAL SECTION FUNCTIONS
;
; @brief    Disable/Enable interrupts by preserving the state of interrupts.  Generally speaking, the
;               state of the interrupt disable flag is stored in the local variable 'cpu_sr' & interrupts
;               are then disabled ('cpu_sr' is allocated in all functions that need to disable interrupts).
;               The previous interrupt state is restored by copying 'cpu_sr' into the CPU's status register.
;
; Prototypes  : CPU_SR  CPU_SR_Save   (void);
;               void    CPU_SR_Restore(CPU_SR  cpu_sr);
;
; @note     (1) These functions are used in general like this :
;
;                       void  Task (void  *p_arg)
;                       {
;                           CPU_SR_ALLOC();                     /* Allocate storage for CPU status register */
;                               :
;                               :
;                           CPU_CRITICAL_ENTER();               /* cpu_sr = CPU_SR_Save();                  */
;                               :
;                               :
;                           CPU_CRITICAL_EXIT();                /* CPU_SR_Restore(cpu_sr);                  */
;                               :
;                       }
;
;               (2) These functions are deprecated.
;********************************************************************************************************

CPU_SR_Save
        MRS     R0, PRIMASK                     ; Set prio int mask to mask all (except faults)
        CPSID   I
        BX      LR


CPU_SR_Restore                                  ; See Note #2.
        MSR     PRIMASK, R0
        BX      LR


;********************************************************************************************************
;                                         WAIT FOR INTERRUPT
;
; @brief    Enters sleep state, which will be exited when an interrupt is received.
;
; Prototypes  : void  CPU_WaitForInt (void)
;
; Note(s)     : This function is deprecated.
;********************************************************************************************************

CPU_WaitForInt
        WFI                                     ; Wait for interrupt
        BX      LR


;********************************************************************************************************
;                                         WAIT FOR EXCEPTION
;
; @brief    Enters sleep state, which will be exited when an exception is received.
;
; Prototypes  : void  CPU_WaitForExcept (void)
;
; Note(s)     : This function is deprecated.
;********************************************************************************************************

CPU_WaitForExcept
        WFE                                     ; Wait for exception
        BX      LR


;********************************************************************************************************
;                                     CPU ASSEMBLY PORT FILE END
;********************************************************************************************************

        END
