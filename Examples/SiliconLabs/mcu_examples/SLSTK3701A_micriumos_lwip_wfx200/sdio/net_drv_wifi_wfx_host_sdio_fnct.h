/***************************************************************************//**
 * @file
 * @brief Network Device Driver - WFX Wi-Fi module
 *******************************************************************************
 * # License
 * <b>Copyright 2019 Silicon Laboratories Inc. www.silabs.com</b>
 *******************************************************************************
 *
 * The licensor of this software is Silicon Laboratories Inc.  Your use of this
 * software is governed by the terms of Silicon Labs Master Software License
 * Agreement (MSLA) available at
 * www.silabs.com/about-us/legal/master-software-license-agreement.
 * The software is governed by the sections of the MSLA applicable to Micrium
 * Software.
 *
 ******************************************************************************/

/********************************************************************************************************
 ********************************************************************************************************
 *                                               MODULE
 ********************************************************************************************************
 *******************************************************************************************************/

#ifndef  NET_DRV_WIFI_WFX_HOST_SDIO_FNCT_H
#define  NET_DRV_WIFI_WFX_HOST_SDIO_FNCT_H


/********************************************************************************************************
 ********************************************************************************************************
 *                                           FUNCTION PROTOTYPES
 ********************************************************************************************************
 *******************************************************************************************************/

void         NetDev_SDIO_FnctInit      (RTOS_ERR       *p_err);

CPU_INT08U   NetDev_SDIO_FnctRdByte    (CPU_INT32U      reg_addr,
                                        RTOS_ERR       *p_err);

void         NetDev_SDIO_FnctWrByte    (CPU_INT32U      reg_addr,
                                        CPU_INT08U      byte,
                                        RTOS_ERR       *p_err);

void         NetDev_SDIO_FnctRd        (CPU_INT32U      reg_addr,
                                        CPU_INT08U     *p_buf,
                                        CPU_INT16U      buf_len,
                                        CPU_BOOLEAN     fixed_addr,
                                        RTOS_ERR       *p_err);

void         NetDev_SDIO_FnctWr        (CPU_INT32U      reg_addr,
                                        CPU_INT08U     *p_buf,
                                        CPU_INT16U      buf_len,
                                        CPU_BOOLEAN     fixed_addr,
                                        RTOS_ERR       *p_err);

void         NetDev_SDIO_FnctRdBlk     (CPU_INT32U      reg_addr,
                                        CPU_INT08U     *p_buf,
                                        CPU_INT16U      blk_nbr,
                                        CPU_BOOLEAN     fixed_addr,
                                        RTOS_ERR       *p_err);

void         NetDev_SDIO_FnctWrBlk     (CPU_INT32U      reg_addr,
                                        CPU_INT08U     *p_buf,
                                        CPU_INT16U      blk_nbr,
                                        CPU_BOOLEAN     fixed_addr,
                                        RTOS_ERR       *p_err);

void         NetDev_SDIO_FnctIntEnDis  (CPU_BOOLEAN     enable,
                                        RTOS_ERR       *p_err);

void         NetDev_SDIO_FnctIntReg    (void           *callback);

#endif
