/*
 ********************************************************************************************************
 *                                               uC/TCP-IP
 *                                       The Embedded TCP/IP Suite
 *
 *                           (c) Copyright 2003-2014; Micrium, Inc.; Weston, FL
 *
 *               All rights reserved.  Protected by international copyright laws.
 *
 *               uC/TCP-IP is provided in source form to registered licensees ONLY.  It is
 *               illegal to distribute this source code to any third party unless you receive
 *               written permission by an authorized Micrium representative.  Knowledge of
 *               the source code may NOT be used to develop a similar product.
 *
 *               Please help us continue to provide the Embedded community with the finest
 *               software available.  Your honesty is greatly appreciated.
 *
 *               You can contact us at www.micrium.com.
 ********************************************************************************************************
 */

/*
 ********************************************************************************************************
 *
 *                                           NETWORK DEVICE DRIVER
 *
 *                                               QCA400X
 *
 * Filename      :
 * Version       : V1.00.00
 * Programmer(s) : AL
 ********************************************************************************************************
 * Note(s)       : (1) The net_dev_ar4100 module is essentially a port of the Qualcomm Atheros driver.
 *                       Please see the Qualcomm Atheros copyright below and on every other document
 *                       provided with this WiFi driver.
 ********************************************************************************************************
 */

/*
 ********************************************************************************************************
 *                                       QUALCOMM/ATHEROS COPYRIGHT
 ********************************************************************************************************
 */
//                                                                ------------------------------------------------------------------------------
//                                                                 Copyright (c) 2011 Qualcomm Atheros, Inc.
//                                                                 All Rights Reserved.
//                                                                 Qualcomm Atheros Confidential and Proprietary.
//                                                                 Permission to use, copy, modify, and/or distribute this software for any purpose with or without fee is
//                                                                 hereby granted, provided that the above copyright notice and this permission notice appear in all copies.
//
//                                                                 THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE
//                                                                 INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
//                                                                 ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF
//                                                                 USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
//                                                                 OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
//                                                                ------------------------------------------------------------------------------

//                                                                ==============================================================================
//                                                                 Author(s): ="Atheros"
//                                                                ==============================================================================

/*
 ********************************************************************************************************
 *                                               INCLUDE FILES
 ********************************************************************************************************
 */

#include <a_config.h>
#include <a_types.h>
#include <a_osapi.h>
#include <custom_api.h>
#include <common_api.h>
#include <netbuf.h>
#include "atheros_wifi_api.h"
#include "atheros_wifi_internal.h"

#include  <net/source/tcpip/net_if_priv.h>

extern A_VOID *pGlobalCtx;
extern A_VOID Custom_Api_RxComplete (A_VOID* p_cxt,
                                     A_VOID* p_req);

#define ATHEROS_TASK_NAME       "Atheros Wifi Driver"
#define DRIVER_AR4100_LOCKNAME  "Driver AR4100 Atheros"

ATH_CUSTOM_INIT_T ath_custom_init =
{
  NULL,
  NULL,
  NULL,
  NULL,
  Custom_Api_RxComplete,
  NULL,
  NULL,
  A_FALSE,
  A_FALSE,
};

ATH_CUSTOM_MEDIACTL_T ath_custom_mediactl =
{
  NULL
};

ATH_CUSTOM_HTC_T ath_custom_htc =
{
  NULL,
  NULL
};

/**
 ********************************************************************************************************
 *                                       Custom_Driver_WaitForCondition()
 *
 * @brief    Wait Wake user event Semaphore and verify that condition is met.
 *
 * @param    p_cxt   Pointer to a network interface. (equivalent to NET_IF *p_if)
 *
 * @param    p_cond  Pointer on the boolean condition to met.
 *
 * @param    value   Condition value to met.
 *
 * @param    msec    Time in millisecond that the condition must be met.
 *
 * @return   A_OK,       If the condition is met in time.
 *           A_ERROR,    If the condition is not met in time or it has failed to pend.
 *
 * @note     (1) This function use QualComm/Atheros variable type in order to match with the supplied
 *               driver. See atheros_wifi/custom_src/include/a_types.h to see Micrium standard type
 *               equivalent.
 ********************************************************************************************************
 */
A_STATUS  Custom_Driver_WaitForCondition(A_VOID   *p_cxt,
                                         volatile A_BOOL   *p_cond,
                                         A_BOOL    value,
                                         A_UINT32  msec)
{
  A_STATUS  status;
  RTOS_ERR  err;

  status = A_OK;

  while (*p_cond != value) {
    KAL_SemPend(GET_DRIVER_CXT(p_cxt)->userWakeEvent,
                KAL_OPT_PEND_NONE,
                msec,
                &err);
    if (RTOS_ERR_CODE_GET(err) != RTOS_ERR_NONE) {
      status = A_ERROR;
      break;
    }
  }
  return status;
}

/**
 ********************************************************************************************************
 *                                           Atheros_Driver_Task()
 *
 * @brief    Handle the driver task.
 *
 * @param    p_context   Pointer to a network interface. (equivalent to NET_IF *p_if)
 *
 *           Custom_Driver_CreateThread().
 *
 * @note     (1) This function use QualComm/Atheros variable type in order to match with the supplied
 *               driver. See atheros_wifi/custom_src/include/a_types.h to see Micrium standard type
 *               equivalent.
 ********************************************************************************************************
 */
static A_VOID Atheros_Driver_Task(void *p_context)
{
  A_VOID   *p_cxt;
  A_BOOL    can_block;
  A_UINT16  block_msec;
  RTOS_ERR  err;

  can_block    = A_FALSE;
  p_cxt        = (A_VOID *)p_context;
  pGlobalCtx   = p_cxt;

  do {
    /* ----------- WAIT FOR DRIVER START SIGNAL ----------- */
    KAL_SemPend(GET_DRIVER_CXT(p_cxt)->DriverStartSemHandle,
                KAL_OPT_PEND_NONE,
                KAL_TIMEOUT_INFINITE,
                &err);
    if (RTOS_ERR_CODE_GET(err) !=  RTOS_ERR_NONE) {
      A_ASSERT(0);
    }
    /* --------------- DRIVER TASK HANDLER ---------------- */
    while (DEF_TRUE) {
      /* Check if the Driver task can sleep.                  */
      if (can_block) {
        GET_DRIVER_COMMON(p_cxt)->driverSleep = A_TRUE;

        if (KAL_SEM_HANDLE_IS_NULL(GET_DRIVER_CXT(p_cxt)->driverWakeEvent)) {
          KAL_Dly(2);
        } else {
          KAL_SemPend(GET_DRIVER_CXT(p_cxt)->driverWakeEvent,
                      KAL_OPT_PEND_NONE,
                      block_msec,
                      &err);
          if ((RTOS_ERR_CODE_GET(err) != RTOS_ERR_NONE)
              && (RTOS_ERR_CODE_GET(err) != RTOS_ERR_TIMEOUT)) {
            break;
          }
        }
      }

      GET_DRIVER_COMMON(p_cxt)->driverSleep = A_FALSE;

      if (GET_DRIVER_CXT(p_cxt)->DriverShutdown == A_TRUE) {
        break;
      }
      /* Check if the driver should shutdown.                        */
      Driver_Main(p_cxt,
                  DRIVER_SCOPE_RX | DRIVER_SCOPE_TX,
                  &can_block,
                  &block_msec);
    }
    /* Signal that the driver is stopped.                          */
    CUSTOM_DRIVER_WAKE_USER(p_cxt);
  } while (DEF_TRUE);
}

/**
 ********************************************************************************************************
 *                                       Custom_Driver_CreateThread()
 *
 * @brief    Create the Driver task.
 *
 * @param    p_cxt   Pointer to a network interface. (equivalent to NET_IF *p_if)
 *
 * @return   A_OK,       if the driver task is successfully created.
 *
 *           A_ERROR,    if the driver task creation failed.
 *
 * @note     (1) This function use QualComm/Atheros variable type in order to match with the supplied
 *               driver. See atheros_wifi/custom_src/include/a_types.h to see Micrium standard type
 *               equivalent.
 ********************************************************************************************************
 */
A_STATUS Custom_Driver_CreateThread(A_VOID *p_cxt)
{
  RTOS_ERR              err;
  KAL_TASK_HANDLE      *p_task_handle;
  NET_IF               *p_if;
  NET_DEV_CFG_WIFI     *p_dev_cfg;
  NET_DEV_CFG_QCA_EXT  *p_qca_cfg;

  /* -------- OBTAIN REFERENCE TO CFGs/REGs/BSP --------- */
  p_if          = (NET_IF               *)p_cxt;                /* Obtain ptr to the interface area.                    */
  p_dev_cfg     = (NET_DEV_CFG_WIFI     *)p_if->Dev_Cfg;
  p_qca_cfg     = (NET_DEV_CFG_QCA_EXT  *)p_dev_cfg->CfgExtPtr;
  p_task_handle = &GET_DRIVER_CXT(p_cxt)->DriverTaskHandle;

  /* Get the driver task handle.                          */
  *p_task_handle = KAL_TaskAlloc(ATHEROS_TASK_NAME,
                                 p_qca_cfg->StkPtr,
                                 p_qca_cfg->StkSizeElements,
                                 DEF_NULL,
                                 &err);
  if (RTOS_ERR_CODE_GET(err) != RTOS_ERR_NONE) {
    return A_ERROR;
  }
  /* Create the driver task.                              */
  KAL_TaskCreate(*p_task_handle,
                 Atheros_Driver_Task,
                 p_cxt,
                 p_qca_cfg->Prio,
                 DEF_NULL,
                 &err);
  if (RTOS_ERR_CODE_GET(err) != RTOS_ERR_NONE) {
    return A_ERROR;
  }

  return A_OK;
}

/**
 ********************************************************************************************************
 *                                       Custom_Driver_DestroyThread()
 *
 * @brief    Shutdown the driver task and wait the for the completion. The driver task will wait for
 *           the driver start signal.
 *
 * @param    p_cxt   Pointer to a network interface. (equivalent to NET_IF *p_if)
 *
 * @return   A_OK.
 *
 * @note     (1) This function use QualComm/Atheros variable type in order to match with the supplied
 *               driver. See atheros_wifi/custom_src/include/a_types.h to see Micrium standard type
 *               equivalent.
 ********************************************************************************************************
 */
A_STATUS Custom_Driver_DestroyThread(A_VOID *p_cxt)
{
  GET_DRIVER_CXT(p_cxt)->DriverShutdown = A_TRUE;

  CUSTOM_DRIVER_WAKE_DRIVER(p_cxt);

  return A_OK;
}

/**
 ********************************************************************************************************
 *                                           Custom_GetRxRequest()
 *
 * @brief    Get a Rx Request buffer :
 *               - (1) Get a buffer from Rx Net pool.
 *               - (2) Get a buffer from A_NETBUF pool.
 *               - (3) Initialize the A_NETBUF buffer with the Rx Net buffer.
 *
 * @param    p_cxt   Pointer to a network interface. (equivalent to NET_IF *p_if)
 *
 * @param    length  Requested buffer length.
 *
 * @note     (1) This function use QualComm/Atheros variable type in order to match with the supplied
 *               driver. See atheros_wifi/custom_src/include/a_types.h to see Micrium standard type
 *               equivalent.
 ********************************************************************************************************
 */
A_VOID *Custom_GetRxRequest(A_VOID *p_cxt,
                            A_UINT16 length)
{
  A_VOID        *p_req;
  CPU_INT08U    *p_buf_new;
  RTOS_ERR       err;
  A_NETBUF      *p_a_netbuf;
  NET_IF        *p_if;
  MEM_DYN_POOL  *p_a_netbuf_pool;

  RTOS_ERR_SET(err, RTOS_ERR_NONE);

  /* -------- OBTAIN REFERENCE TO CFGs/REGs/BSP --------- */
  p_if            = (NET_IF *)p_cxt;                            /* Obtain ptr to the interface area.                    */
  p_req           = (A_VOID *) DEF_NULL;
  p_a_netbuf_pool = &(GET_DRIVER_CXT(p_cxt)->A_NetBufPool);

  RXBUFFER_ACCESS_ACQUIRE(p_cxt);
  {
    /* ----------- OBTAIN PTR TO NEW DATA AREA ------------ */
    /* Request an empty buffer.                             */
    p_buf_new = NetBuf_GetDataPtr(p_if,
                                  NET_TRANSACTION_RX,
                                  NET_IF_WIFI_CFG_RX_BUF_LARGE_SIZE,
                                  NET_IF_IX_RX,
                                  0,
                                  0,
                                  0,
                                  &err);

    /* ------------ OBTAIN PTR TO NEW A_NETBUF ------------ */
    if (RTOS_ERR_CODE_GET(err) ==  RTOS_ERR_NONE) {
      p_req = Mem_DynPoolBlkGet(p_a_netbuf_pool, &err);
    }
  }
  RXBUFFER_ACCESS_RELEASE(p_cxt);
  if (RTOS_ERR_CODE_GET(err) != RTOS_ERR_NONE) {                            /* If unable to get Rx Net buffer.                      */
    Driver_ReportRxBuffStatus(p_cxt, A_FALSE);
    return p_req;
  }
  /* If unable to get the A_NETBUF buf.                   */
  if (RTOS_ERR_CODE_GET(err) != RTOS_ERR_NONE) {
    Driver_ReportRxBuffStatus(p_cxt, A_FALSE);
    NetBuf_FreeBufDataAreaRx(p_if->Nbr, p_buf_new);
    return ((A_VOID *)0);
  }
  /* --------------- A_NETBUF STRUCT INIT --------------- */
  if (p_req != NULL) {
    A_NETBUF_INIT(p_cxt,
                  p_req,
                  p_buf_new);
    p_a_netbuf                  = (A_NETBUF *)p_req;
    p_a_netbuf->pool_id         = A_RX_NET_POOL;
    p_a_netbuf->RxBufDelivered  = DEF_NO;

    if (length > A_NETBUF_TAILROOM(p_req)) {
      A_ASSERT(length <= A_NETBUF_TAILROOM(p_req));
    }
  } else {
    A_ASSERT(0);                                                /* Should never happen thanks to HW_ReportRxBuffStatus. */
  }

  return p_req;
}

/**
 ********************************************************************************************************
 *                                       Custom_Driver_WakeDriver()
 *
 * @brief    Signal the driver to wake.
 *
 * @param    p_cxt   Pointer to a network interface. (equivalent to NET_IF *p_if)
 *
 * @note     (1) This function use QualComm/Atheros variable type in order to match with the supplied
 *               driver. See atheros_wifi/custom_src/include/a_types.h to see Micrium standard type
 *               equivalent.
 ********************************************************************************************************
 */
A_VOID Custom_Driver_WakeDriver(A_VOID *p_cxt)
{
  RTOS_ERR  err;

  KAL_SemPost(GET_DRIVER_CXT(p_cxt)->driverWakeEvent,
              KAL_OPT_PEND_NONE,
              &err);

  (void)&err;
}

/**
 ********************************************************************************************************
 *                                           Custom_Driver_WakeUser()
 *
 * @brief    Signal from the driver for updated value in the driver context.
 *
 * @param    p_cxt   Pointer to a network interface. (equivalent to NET_IF *p_if)
 *
 * @note     (1) This function use QualComm/Atheros variable type in order to match with the supplied
 *               driver. See atheros_wifi/custom_src/include/a_types.h to see Micrium standard type
 *               equivalent.
 ********************************************************************************************************
 */
A_VOID Custom_Driver_WakeUser(A_VOID *pCxt)
{
  RTOS_ERR err;

  KAL_SemPost(GET_DRIVER_CXT(pCxt)->userWakeEvent,
              KAL_OPT_PEND_NONE,
              &err);

  (void)&err;
}

/**
 ********************************************************************************************************
 *                                       Custom_Driver_ContextInit()
 *
 * @brief    Initialize the driver context elements.
 *
 * @param    p_cxt   Pointer to a network interface. (equivalent to NET_IF *p_if)
 *
 * @return   A_OK        In any case.
 *
 * @note     (1) This function use QualComm/Atheros variable type in order to match with the supplied
 *               driver. See atheros_wifi/custom_src/include/a_types.h to see Micrium standard type
 *               equivalent.
 ********************************************************************************************************
 */
A_STATUS Custom_Driver_ContextInit(A_VOID *p_cxt)
{
  A_DRIVER_CONTEXT     *p_dcxt;
  NET_IF               *p_if;
  NET_DEV_CFG_WIFI     *p_dev_cfg;
  NET_DEV_DATA         *p_dev_data;

  /* -------- OBTAIN REFERENCE TO CFGs/REGs/BSP --------- */
  p_if       = (NET_IF           *)p_cxt;                       /* Obtain ptr to the interface area.                    */
  p_dev_cfg  = (NET_DEV_CFG_WIFI *)p_if->Dev_Cfg;               /* Obtain ptr to dev cfg area.                          */
  p_dev_data = (NET_DEV_DATA     *)p_if->Dev_Data;              /* Obtain ptr to dev data area.                         */
  p_dcxt     = GET_DRIVER_COMMON(p_cxt);

  p_dcxt->tempStorage                    = p_dev_data->GlobalBufPtr;
  p_dcxt->tempStorageLength              = p_dev_cfg->RxBufLargeSize;
  p_dcxt->rxBufferStatus                 = A_TRUE;
  GET_DRIVER_CXT(p_cxt)->DriverShutdown  = A_FALSE;

  return A_OK;
}

/**
 ********************************************************************************************************
 *                                       Custom_Driver_ContextDeInit()
 *
 * @brief    Deinitialization of the driver context elements.
 *
 * @param    pCxt    $$$$ Add description for 'pCxt'
 *
 * @note     (1) This function use QualComm/Atheros variable type in order to match with the supplied
 *               driver. See atheros_wifi/custom_src/include/a_types.h to see Micrium standard type
 *               equivalent.
 ********************************************************************************************************
 */
A_VOID Custom_Driver_ContextDeInit(A_VOID *p_cxt)
{
  (void)p_cxt;
}

A_STATUS setup_host_dset(QOSAL_VOID* handle)
{
  return wmi_dset_host_cfg_cmd(handle);
}
