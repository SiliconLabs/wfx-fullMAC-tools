//                                                                ------------------------------------------------------------------------------
//                                                                 Copyright (c) Qualcomm Atheros, Inc.
//                                                                 All rights reserved.
//                                                                 Redistribution and use in source and binary forms, with or without modification, are permitted (subject to
//                                                                 the limitations in the disclaimer below) provided that the following conditions are met:
//
//                                                                 · Redistributions of source code must retain the above copyright notice, this list of conditions and the
//                                                                   following disclaimer.
//                                                                 · Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the
//                                                                   following disclaimer in the documentation and/or other materials provided with the distribution.
//                                                                 · Neither the name of nor the names of its contributors may be used to endorse or promote products derived
//                                                                   from this software without specific prior written permission.
//
//                                                                 NO EXPRESS OR IMPLIED LICENSES TO ANY PARTY'S PATENT RIGHTS ARE GRANTED BY THIS LICENSE. THIS SOFTWARE IS
//                                                                 PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING,
//                                                                 BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
//                                                                 DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
//                                                                 INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
//                                                                 SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
//                                                                 ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
//                                                                 ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//                                                                ------------------------------------------------------------------------------
//                                                                ==============================================================================
//                                                                 Author(s): ="Atheros"
//                                                                ==============================================================================
#include <a_config.h>
#include <a_types.h>
#include <a_osapi.h>
#include <driver_cxt.h>
#include <common_api.h>
#include <custom_wlan_api.h>
#include <wmi_api.h>
#include <netbuf.h>
#include <htc.h>
#include <spi_hcd_if.h>
#include "../hcd/hcd_api.h"

#include "atheros_wifi_api.h"
#include "atheros_wifi_internal.h"

/*****************************************************************************/
/********** IMPLEMENTATION **********/
/*****************************************************************************/

//                                                                volatile QOSAL_TASK_ID atheros_wifi_task_id = {QOSAL_NULL_TASK_ID};
QOSAL_UINT32 g_totAlloc = 0;
QOSAL_UINT32 g_poolid = 0xffffffff;

/*****************************************************************************/
/*  Driver_Main - This is the top level entry point for the Atheros wifi driver
 *   This should be called from either a dedicated thread running in a loop or,
 *   in the case of single threaded systems it should be called periodically
 *   from the main loop.  Also, if in a single threaded system a API call
 *   requires synchronous completion then it may be necessary to call this
 *   function from within the API call.
 *      QOSAL_VOID *pCxt - the driver context.
 *      QOSAL_UINT8 scope - Limits/allows what type of activity may be processed.
 *          Options are any combination of DRIVER_SCOPE_TX + DRIVER_SCOPE_RX.
 *      QOSAL_BOOL *pCanBlock - used to report to caller whether the driver is
 *          in a state where it is safe to block until further notice.
 *		QOSAL_UINT16 *pBlock_msec - used to report to caller how long the driver
 *			may block for. if zero then driver can block indefinitely.
 *****************************************************************************/
A_STATUS Driver_Main(QOSAL_VOID *pCxt, QOSAL_UINT8 scope, QOSAL_BOOL *pCanBlock,
                     QOSAL_UINT16 *pBlock_msec)
{
  QOSAL_VOID *pReq;
  A_DRIVER_CONTEXT *pDCxt = GET_DRIVER_COMMON(pCxt);

  HW_ProcessPendingInterrupts(pCxt);   //IGX_UD_CHANGES

  /* This is where packets are received from the
   * wifi device. Because system buffers must be
   * available to receive a packet we only call into
   * this function if we know that buffers are
   * available to satisfy the operation.
   */
  if ((scope & DRIVER_SCOPE_RX) && QOSAL_TRUE == Driver_RxReady(pCxt)) {
    pDCxt->driver_state = DRIVER_STATE_RX_PROCESSING;
    pReq = pDCxt->pRxReq;
    pDCxt->pRxReq = NULL;
    if (A_OK != Driver_RecvPacket(pCxt, pReq)) {
      /* if this happens it is a critical error */
      A_ASSERT(0);
    }
    /* FIXME: as an optimization the next lookahead may have
     * been acquired from the trailer of the previous rx packet.
     * in that case we should pre-load the lookahead so as to
     * avoid reading it from the registers. */
    /* reset the lookahead for the next read operation */
    pDCxt->lookAhead = 0;
    /* check to see if a deferred bus request has completed. if so
     * process it. */
    if (pDCxt->booleans & SDHD_BOOL_DMA_COMPLETE) {
      /* get the request */
      if (A_OK  == Driver_CompleteRequest(pCxt,
                                          pDCxt->spi_hcd.pCurrentRequest)) {
        /* clear the pending request and the boolean */
        pDCxt->spi_hcd.pCurrentRequest = NULL;
        pDCxt->booleans &= ~SDHD_BOOL_DMA_COMPLETE;
      }
    }
  }

  pDCxt->driver_state = DRIVER_STATE_PENDING_CONDITION_D;

  if ((scope & DRIVER_SCOPE_TX) && QOSAL_TRUE == Driver_TxReady(pCxt)) {
    pDCxt->driver_state = DRIVER_STATE_TX_PROCESSING;
    /* after processing any outstanding device interrupts
     * see if there is a packet that requires transmitting
     */
    if (pDCxt->txQueue.count) {
      /* accesslock here to sync with threads calling
       * submit TX
       */
      TXQUEUE_ACCESS_ACQUIRE(pCxt);
      {
        pReq = A_NETBUF_DEQUEUE(&(pDCxt->txQueue));
      }
      TXQUEUE_ACCESS_RELEASE(pCxt);

      if (pReq != NULL) {
        Driver_SendPacket(pCxt, pReq);
      }
    }
  }

  pDCxt->driver_state = DRIVER_STATE_PENDING_CONDITION_E;

  /* execute any asynchronous/special request when possible */
  if (0 == (pDCxt->booleans & SDHD_BOOL_DMA_IN_PROG)
      && pDCxt->asynchRequest) {
    pDCxt->driver_state = DRIVER_STATE_ASYNC_PROCESSING;
    pDCxt->asynchRequest(pCxt);
    pDCxt->driver_state = DRIVER_STATE_PENDING_CONDITION_F;
  }

  pDCxt->driver_state = DRIVER_STATE_PENDING_CONDITION_G;
  /* allow caller to block if all necessary conditions are satisfied */
  if (pCanBlock) {
    if ((pDCxt->spi_hcd.PendingIrqAcks == 0
         || pDCxt->rxBufferStatus == QOSAL_FALSE)
        && (pDCxt->spi_hcd.IrqDetected == A_FALSE)
        && (pDCxt->txQueue.count == 0 || Driver_TxReady(pCxt) == A_FALSE)
        && (0 == (pDCxt->booleans & SDHD_BOOL_DMA_COMPLETE))) {
      *pCanBlock = A_TRUE;

      if (pDCxt->creditDeadlock == QOSAL_TRUE) {
        if (pBlock_msec) {
          *pBlock_msec = DEADLOCK_BLOCK_MSEC;
        }

        pDCxt->creditDeadlockCounter++;
      }

      pDCxt->driver_state = DRIVER_STATE_IDLE;
    } else {
      *pCanBlock = QOSAL_FALSE;
    }
  }

  return A_OK;
}

/*****************************************************************************/
/*  Driver_ReportRxBuffStatus - Tracks availability of Rx Buffers for those
 *	 systems that have a limited pool. The driver quearies this status before
 *	 initiating a RX packet transaction.
 *      QOSAL_VOID *pCxt - the driver context.
 *      QOSAL_BOOL status - new status A_TRUE - RX buffers are available,
 *			A_FALSE - RX buffers are not available.
 *****************************************************************************/

#if defined(DRIVER_CONFIG_IMPLEMENT_RX_FREE_MULTIPLE_QUEUE)
QOSAL_UINT8 GetQueueIndexByEPID(QOSAL_UINT8 epid)
{
  QOSAL_UINT8 i;

  for (i = 0; i < 8; i++) {
    if (GET_QUEUE_MASK(i) & (1 << epid)) {
      break;
    }
  }
  if (i >= 1 && i < 8) {
    i = 7;
  }
  return i;
}

QOSAL_UINT8 GetQueueCtrlIndexByEPID(QOSAL_UINT8 epid)
{
  QOSAL_UINT8 i;

  for (i = 0; i < 8; i++) {
    if (GET_QUEUE_MASK(i) & (1 << epid)) {
      break;
    }
  }
  return i;
}

QOSAL_VOID
Driver_ReportRxBuffStatus(QOSAL_VOID *pCxt, QOSAL_BOOL status, QOSAL_UINT8 epid)
{
  QOSAL_BOOL oldStatus;
  QOSAL_UINT8 bufNdx;
  A_DRIVER_CONTEXT *pDCxt = GET_DRIVER_COMMON(pCxt);

  bufNdx = GetQueueIndexByEPID(epid);
  /* NOTE: Dont acquire the lock here instead the
   * caller should acquire this lock if necessary prior to calling this function */
  //                                                              RXBUFFER_ACCESS_ACQUIRE(pCxt);
  oldStatus = pDCxt->rxBufferStatus;

  if (status) {
    pDCxt->rxMultiBufferStatus |= 1 << bufNdx;
  } else {
    pDCxt->rxMultiBufferStatus &= ~(1 << bufNdx);
  }
  pDCxt->rxBufferStatus = (pDCxt->rxMultiBufferStatus != 0);

  //                                                              RXBUFFER_ACCESS_RELEASE(pCxt);
  /* the driver thread may have blocked on this
   * status so conditionally wake up the thread
   * via QueueWork */
  if ((oldStatus == QOSAL_FALSE) && (status == QOSAL_TRUE)) {
    if (pDCxt->hcd.PendingIrqAcks) {
      DRIVER_WAKE_DRIVER(pCxt);
    }
  }
}
#else
QOSAL_VOID Driver_ReportRxBuffStatus(QOSAL_VOID *pCxt, QOSAL_BOOL status)
{
  QOSAL_BOOL oldStatus;
  A_DRIVER_CONTEXT *pDCxt = GET_DRIVER_COMMON(pCxt);

  /* NOTE: Dont acquire the lock here instead the
   * caller should acquire this lock if necessary prior to calling this function */
  //                                                              RXBUFFER_ACCESS_ACQUIRE(pCxt);
  {
    oldStatus = pDCxt->rxBufferStatus;
    pDCxt->rxBufferStatus = status;
  }
  //                                                              RXBUFFER_ACCESS_RELEASE(pCxt);
  /* the driver thread may have blocked on this
   * status so conditionally wake up the thread
   * via QueueWork */
  if (oldStatus == QOSAL_FALSE && status == QOSAL_TRUE) {
    if (pDCxt->spi_hcd.PendingIrqAcks) {
      DRIVER_WAKE_DRIVER(pCxt);
    }
  }
}
#endif

/*****************************************************************************/
/*  Driver_CompleteRequest - Completes a deferred request. One that finished
 *      asynchronously. Such a request must have a non-null callback that will
 *      be called by this function to fullfill the operation.
 *      QOSAL_VOID *pCxt - the driver context.
 *      QOSAL_VOID *pReq - the bus request object.
 *****************************************************************************/
A_STATUS Driver_CompleteRequest(QOSAL_VOID *pCxt, QOSAL_VOID *pReq)
{
  A_STATUS status = A_ERROR;
  QOSAL_VOID (*cb)(QOSAL_VOID *, QOSAL_VOID *);
  A_DRIVER_CONTEXT *pDCxt = GET_DRIVER_COMMON(pCxt);
  /* A hardware WAR exists whereby interrupts from the device are disabled
   * during an ongoing bus transaction.  This function is called when
   * the bus transaction completes.  if irqMask is set then it means that
   * interrupts should be re-enabled */
  if (pDCxt->spi_hcd.irqMask) {
    Hcd_UnmaskInterrupts(pCxt, pDCxt->spi_hcd.irqMask);
    pDCxt->spi_hcd.irqMask = 0;
  }

  if (pReq != NULL
      && NULL != (cb = A_NETBUF_GET_ELEM(pReq, A_REQ_CALLBACK))) {
    cb(pCxt, pReq);
    status = A_OK;
  } else {
    A_ASSERT(0);
  }

  return status;
}

/*****************************************************************************/
/*  Driver_TxReady - Determines whether it is safe to start a TX operation. If
*     a TX operation can be started this function returns A_TRUE else A_FALSE.
*      QOSAL_VOID *pCxt - the driver context.
*****************************************************************************/
QOSAL_BOOL Driver_TxReady(QOSAL_VOID *pCxt)
{
  QOSAL_BOOL res = QOSAL_FALSE;
  QOSAL_VOID *pReq;
  A_ENDPOINT_T *pEp;
  A_DRIVER_CONTEXT *pDCxt = GET_DRIVER_COMMON(pCxt);

  pDCxt->creditDeadlock = QOSAL_FALSE;

  do {
    /* there can be no errors */
    if (pDCxt->booleans & SDHD_BOOL_FATAL_ERROR) {
      break;
    }
    /* there can be no in-complete requests */
    if ((pDCxt->booleans & SDHD_BOOL_DMA_IN_PROG)
        || (pDCxt->booleans & SDHD_BOOL_DMA_COMPLETE)
        ||      /* the write buffer watermark interrupt must not be configured */
        (pDCxt->booleans & SDHD_BOOL_DMA_WRITE_WAIT_FOR_BUFFER)) {
      break;
    }

    /* there must be enough credits for the target endpoint */
    if (NULL == (pReq = A_NETBUF_PEEK_QUEUE(&(pDCxt->txQueue)))) {
      break;
    }

    if (NULL
        == (pEp = Util_GetEndpoint(pCxt,
                                   A_NETBUF_GET_ELEM(pReq, A_REQ_EPID)))) {
      break;
    }
    /* ensure as sanity check that the request length is less than a credit length */
    if (pDCxt->creditSize
        < DEV_CALC_SEND_PADDED_LEN(pDCxt, A_NETBUF_LEN(pReq))) {
      //                                                           THIS IS AN ERROR AS REQUESTS SHOULD NEVER EXCEED THE CREDIT SIZE
      A_ASSERT(0);
#if DRIVER_CONFIG_DISABLE_ASSERT
      break;
#endif
    }
#if RESERVE_ONE_CREDIT_FOR_CONTROL
    if ((pEp->epIdx == 0) || (pEp->epIdx == 1)) {
      /* confirm there are enough credits for this transfer */
      if (0 == pEp->credits) {
        //                                                        if(pDCxt->rxBufferStatus == QOSAL_FALSE)
        {
          /* need to use alternate mode to acquire credits */
          Htc_GetCreditCounterUpdate(pCxt, A_NETBUF_GET_ELEM(pReq, A_REQ_EPID));

          if (0 == pEp->credits) {         /* test again in case no new credits were acquired */
            if (pDCxt->rxBufferStatus == QOSAL_FALSE) {
              /* with no rx buffers to receive a credit report and no interrupt
               * credits a condition exists where the driver may become deadlocked.
               * Hence the creditDeadlock bool is set to prevent the driver from
               * sleeping.  this will cause the driver to poll for credits until
               * the condition is passed.
               */
              pDCxt->creditDeadlock = QOSAL_TRUE;
#if 0
              if (pDCxt->creditDeadlockCounter >= MAX_CREDIT_DEADLOCK_ATTEMPTS) {
                /* This is a serious condition that can be brought about by
                 * a flood of RX packets which generate TX responses and do not
                 * return the RX buffer to the driver until the TX response
                 * completes. To mitigate this situation purge the tx queue
                 * of any packets on data endpoints.
                 */
                Driver_DropTxDataPackets(pCxt);
                pDCxt->creditDeadlockCounter = 0;
              }
#endif
            }

            break;
          }
        }
        //                                                        else{
        //                                                              break;/* wait for credit report from device */
        //                                                        }
      }
    } else {
      /* confirm there are enough credits for this transfer */
      if (pEp->credits <= 1) {
        //                                                        if(pDCxt->rxBufferStatus == QOSAL_FALSE)
        {
          /* need to use alternate mode to acquire credits */
          Htc_GetCreditCounterUpdate(pCxt, A_NETBUF_GET_ELEM(pReq, A_REQ_EPID));

          if (pEp->credits <= 1) {         /* test again in case no new credits were acquired */
            if (pDCxt->rxBufferStatus == QOSAL_FALSE) {
              /* with no rx buffers to receive a credit report and no interrupt
               * credits a condition exists where the driver may become deadlocked.
               * Hence the creditDeadlock bool is set to prevent the driver from
               * sleeping.  this will cause the driver to poll for credits until
               * the condition is passed.
               */
              pDCxt->creditDeadlock = QOSAL_TRUE;
#if 0
              if (pDCxt->creditDeadlockCounter >= MAX_CREDIT_DEADLOCK_ATTEMPTS) {
                /* This is a serious condition that can be brought about by
                 * a flood of RX packets which generate TX responses and do not
                 * return the RX buffer to the driver until the TX response
                 * completes. To mitigate this situation purge the tx queue
                 * of any packets on data endpoints.
                 */
                Driver_DropTxDataPackets(pCxt);
                pDCxt->creditDeadlockCounter = 0;
              }
#endif
            }

            break;
          }
        }
        //                                                        else{
        //                                                              break;/* wait for credit report from device */
        //                                                        }
      }
    }     /*(pEp->epIdx == 0) || (pEp->epIdx == 1)*/
#else
    if (0 == pEp->credits) {
      //                                                          if(pDCxt->rxBufferStatus == QOSAL_FALSE)
      {
        /* need to use alternate mode to acquire credits */
        Htc_GetCreditCounterUpdate(pCxt,
                                   A_NETBUF_GET_ELEM(pReq, A_REQ_EPID));

        if (0 == pEp->credits) {        /* test again in case no new credits were acquired */
          if (pDCxt->rxBufferStatus == QOSAL_FALSE) {
            /* with no rx buffers to receive a credit report and no interrupt
             * credits a condition exists where the driver may become deadlocked.
             * Hence the creditDeadlock bool is set to prevent the driver from
             * sleeping.  this will cause the driver to poll for credits until
             * the condition is passed.
             */
            pDCxt->creditDeadlock = QOSAL_TRUE;
#if 0
            if (pDCxt->creditDeadlockCounter >= MAX_CREDIT_DEADLOCK_ATTEMPTS) {
              /* This is a serious condition that can be brought about by
               * a flood of RX packets which generate TX responses and do not
               * return the RX buffer to the driver until the TX response
               * completes. To mitigate this situation purge the tx queue
               * of any packets on data endpoints.
               */
              Driver_DropTxDataPackets(pCxt);
              pDCxt->creditDeadlockCounter = 0;
            }
#endif
          }

          break;
        }
      }
      //                                                          else{
      //                                                                break;/* wait for credit report from device */
      //                                                          }
    }
#endif

    pDCxt->creditDeadlockCounter = 0;

    /* there must be enough write buffer space in the target fifo */
    if (pDCxt->spi_hcd.WriteBufferSpace
        < DEV_CALC_SEND_PADDED_LEN(pDCxt, A_NETBUF_LEN(pReq))
        + ATH_SPI_WRBUF_RSVD_BYTES) {
      /* this will read the internal register to get the latest value for write buffer space */
      Hcd_RefreshWriteBufferSpace(pCxt);

      if (pDCxt->spi_hcd.WriteBufferSpace
          < DEV_CALC_SEND_PADDED_LEN(pDCxt, A_NETBUF_LEN(pReq))
          + ATH_SPI_WRBUF_RSVD_BYTES) {
        /* there currently isn't enough space in the target fifo to accept this packet.
         * Hence setup the write buffer watermark interrupt for future notification and exit. */
        Hcd_ProgramWriteBufferWaterMark(pCxt,
                                        DEV_CALC_SEND_PADDED_LEN(pDCxt, A_NETBUF_LEN(pReq)));
        /* wait for interrupt to indicate space available */
        break;
      }
    }
    /* all conditions are met to transmit a packet */
    res = QOSAL_TRUE;
  } while (0);

  return res;
}

/*****************************************************************************/
/*  Driver_RxReady - Determines whether it is safe to start a RX operation. If
*     a RX operation can be started this function returns A_TRUE else A_FALSE.
*      QOSAL_VOID *pCxt - the driver context.
*****************************************************************************/
QOSAL_BOOL Driver_RxReady(QOSAL_VOID *pCxt)
{
  QOSAL_BOOL res = QOSAL_FALSE;
  HTC_FRAME_HDR *pHTCHdr;
  QOSAL_INT32 fullLength;
  QOSAL_UINT32 interrupts;
  A_DRIVER_CONTEXT *pDCxt = GET_DRIVER_COMMON(pCxt);

  do {
    /* there can be no errors */
    if (pDCxt->booleans & SDHD_BOOL_FATAL_ERROR) {
      break;
    }
    /* there can be no in-complete requests */
    if ((pDCxt->booleans & SDHD_BOOL_DMA_IN_PROG)
        || (pDCxt->booleans & SDHD_BOOL_DMA_COMPLETE)) {
      break;
    }
    /* the wifi device must have indicated that a packet is ready */
    if (0 == pDCxt->spi_hcd.PendingIrqAcks) {
      break;
    }
    /* there must be rx buffers to process the request */
    if (QOSAL_FALSE == pDCxt->rxBufferStatus && NULL == pDCxt->pRxReq) {
      break;
    }
    /* clear the pkt interrupt if its set. We do this here rather than in the
     * interrupt handler because we will continue to process packets until we
     * read a lookahead==0. only then will we unmask the interrupt */
    if (A_OK
        != Hcd_DoPioInternalAccess(pCxt, ATH_SPI_INTR_CAUSE_REG,
                                   &interrupts, QOSAL_TRUE)) {
      break;
    }

    if (interrupts & ATH_SPI_INTR_PKT_AVAIL) {
      interrupts = ATH_SPI_INTR_PKT_AVAIL;

      if (A_OK
          != Hcd_DoPioInternalAccess(pCxt, ATH_SPI_INTR_CAUSE_REG,
                                     &interrupts, QOSAL_FALSE)) {
        break;
      }
    }
    /* RX packet interrupt processing must be enabled */
    if (0 == (pDCxt->enabledSpiInts & ATH_SPI_INTR_PKT_AVAIL)) {
      break;
    }
    /* As a last step read the lookahead and the Receive buffer register in an
     * effort to determine that a complete packet is ready for transfer. Do this
     * operation last as it does require several BUS transactions to complete. */
    if (0 == pDCxt->lookAhead) {
      /* need to refresh the lookahead */
      if (A_OK != Hcd_GetLookAhead(pCxt)) {
        /* this is a major error */
        A_ASSERT(0);
      }
    }
    /* if lookahead is 0 then no need to proceed */
    if (0 == pDCxt->lookAhead) {
      if (0 != pDCxt->spi_hcd.PendingIrqAcks) {
        /* this should always be true here */
        pDCxt->spi_hcd.PendingIrqAcks = 0;
        Hcd_UnmaskInterrupts(pCxt, ATH_SPI_INTR_PKT_AVAIL);
      }

      break;
    }
    /* compare lookahead and recv buf ready value */
    pHTCHdr = (HTC_FRAME_HDR *) &(pDCxt->lookAhead);
    if (pDCxt->spi_hcd.ReadBufferSpace < sizeof(HTC_FRAME_HDR)
        || pDCxt->spi_hcd.ReadBufferSpace < A_LE2CPU16(pHTCHdr->PayloadLen)) {
      /* force a call to BUS_GetLookAhead on the next pass */
      pDCxt->lookAhead = 0;
      break;
    }
    /* there must be resources available to receive a packet */
    if (NULL == pDCxt->pRxReq) {
      fullLength = (QOSAL_INT32) DEV_CALC_RECV_PADDED_LEN(pDCxt,
                                                          A_LE2CPU16(pHTCHdr->PayloadLen) + sizeof(HTC_FRAME_HDR));

#if defined(DRIVER_CONFIG_IMPLEMENT_RX_FREE_MULTIPLE_QUEUE)
      {
        QOSAL_UINT32 flags, bufNdx;

        bufNdx = GetQueueIndexByEPID(pHTCHdr->EndpointID);
        flags = 1 << bufNdx;

        if (pDCxt->rxMultiBufferStatus & flags) {
          /* try to get an RX buffer */
          pDCxt->pRxReq = CUSTOM_DRIVER_GET_RX_REQ(pCxt, fullLength, pHTCHdr->EndpointID);
        }
      }
#else

      if (pDCxt->rxBufferStatus) {
        /* try to get an RX buffer */
        pDCxt->pRxReq = CUSTOM_DRIVER_GET_RX_REQ(pCxt, fullLength);
      }
#endif

      if (pDCxt->pRxReq == NULL) {
        break;
      }
      /* init the packet callback */
      A_NETBUF_SET_ELEM(pDCxt->pRxReq, A_REQ_CALLBACK, Driver_RxComplete);
    }
    /* all conditions are met to receive a packet */
    res = QOSAL_TRUE;
  } while (0);

  return res;
}

QOSAL_VOID Driver_DropTxDataPackets(QOSAL_VOID *pCxt)
{
  A_DRIVER_CONTEXT *pDCxt = GET_DRIVER_COMMON(pCxt);
  QOSAL_VOID *pReq;
  QOSAL_UINT16 i, count;

  if (pDCxt->txQueue.count) {
    count = pDCxt->txQueue.count;
    /* accesslock here to sync with threads calling
     * submit TX
     */
    for (i = 0; i < count; i++) {
      TXQUEUE_ACCESS_ACQUIRE(pCxt);
      {
        pReq = A_NETBUF_DEQUEUE(&(pDCxt->txQueue));
      }
      TXQUEUE_ACCESS_RELEASE(pCxt);

      if (pReq != NULL) {
        if (A_NETBUF_GET_ELEM(pReq, A_REQ_EPID) > ENDPOINT_1) {
          Driver_CompleteRequest(pCxt, pReq);
        } else {
          TXQUEUE_ACCESS_ACQUIRE(pCxt);
          /* re-queue the packet as it is not one we can purge */
          A_NETBUF_ENQUEUE(&(pDCxt->txQueue), pReq);
          TXQUEUE_ACCESS_RELEASE(pCxt);
        }
      }
    }
  }
}

#if defined(DRIVER_CONFIG_IMPLEMENT_RX_FREE_MULTIPLE_QUEUE)
extern A_NETBUF *ep0_buf;

void Driver_ReportReverseCredits(QOSAL_VOID *pReq)
{
  A_DRIVER_CONTEXT *pDCxt = GET_DRIVER_COMMON(p_Global_Cxt);
  A_NETBUF *pCurrTxReq;

#if 0
  A_ENDPOINT_T *pEp;
  void *osbuf;
  QOSAL_VOID *pReq;
  if (pDCxt->txQueue.count != 0) {
    pReq = A_NETBUF_PEEK_QUEUE(&(pDCxt->txQueue));
    pEp = Util_GetEndpoint(p_Global_Cxt, A_NETBUF_GET_ELEM(pReq, A_REQ_EPID));
    if (pEp->credits != 0) {
      return;
    }
  }
  osbuf = A_NETBUF_ALLOC(0);
  A_NETBUF_SET_ELEM(osbuf, A_REQ_EPID, 0);
  Driver_SubmitTxRequest(p_Global_Cxt, osbuf);
#else

  if (hasRCSReport() == A_FALSE) {
    return;
  }

  pCurrTxReq = A_NETBUF_PEEK_QUEUE(&(pDCxt->txQueue));
  if (pCurrTxReq != NULL) {
    if (A_NETBUF_GET_ELEM(pCurrTxReq, A_REQ_EPID) == ENDPOINT_0) {
      return;
    }
  }

  a_netbuf_reinit(ep0_buf, 0);
  A_NETBUF_SET_ELEM(ep0_buf, A_REQ_EPID, 0);

  qosal_intr_disable();
  Driver_SubmitEp0TxRequest(p_Global_Cxt, ep0_buf);
  qosal_intr_enable();

#endif
}
#endif

#if DRIVER_CONFIG_MULTI_TASKING
#define BSP_ALARM_FREQUENCY 1000
A_STATUS Driver_WaitForCondition(QOSAL_VOID *pCxt, volatile QOSAL_BOOL *pCond,
                                 QOSAL_BOOL value, QOSAL_UINT32 msec)
{
  QOSAL_UINT32    ret        = (msec) * BSP_ALARM_FREQUENCY / 1000;
  A_STATUS        status     = A_OK;
  QOSAL_UINT32    qosal_err  = QOSAL_OK;
  ret = (ret == 0) ? 1 : ret;

  while (1) {
    if ((*pCond != value)) {
      qosal_err = qosal_wait_for_event(&GET_DRIVER_CXT(pCxt)->userWakeEvent, 0x01, A_TRUE, 0, ret);
      switch (qosal_err) {
        case QOSAL_OK:
          qosal_clear_event(&GET_DRIVER_CXT(pCxt)->userWakeEvent, 0x01);
          break;
#if 0
        case QOSAL_ERR_ABORT:
          status = A_SOCK_ABORT;
          break;
#endif
        default:
          status = A_ERROR;
          break;
      }
    } else {
      break;
    }
    if (status != A_OK) {
      break;
    }
  }

  return status;
}

QOSAL_VOID Atheros_Driver_Task(QOSAL_UINT32 pContext)
{
  QOSAL_VOID *pCxt = (QOSAL_VOID *) pContext;
  QOSAL_BOOL canBlock = A_FALSE;
  QOSAL_UINT16 block_msec;
  QOSAL_UINT32 timeout;

  do {
    if (A_OK == Driver_Init(pCxt)) {
      while (1) {
        if (canBlock) {
          GET_DRIVER_COMMON(pCxt)->driverSleep = A_TRUE;
          timeout = (block_msec) * BSP_ALARM_FREQUENCY / 1000;

          if (block_msec && timeout == 0) {
            timeout = 1;
          }

          if (A_OK
              != qosal_wait_for_event(
                &(GET_DRIVER_CXT(pCxt)->driverWakeEvent),
                0x01, A_TRUE, 0, timeout)) {
            if (timeout == 0) {
              break;
            }
          }
        }

        GET_DRIVER_COMMON(pCxt)->driverSleep = A_FALSE;

        if (GET_DRIVER_COMMON(pCxt)->driverShutdown == A_TRUE) {
          break;
        }

        Driver_Main(pCxt, DRIVER_SCOPE_RX | DRIVER_SCOPE_TX, &canBlock,
                    &block_msec);
      }

      Driver_DeInit(pCxt);
      DRIVER_WAKE_USER(pCxt);
    }
#if QOSAL_TASK_DESTRUCTION
  } while (0);

  atheros_wifi_task_id = QOSAL_NULL_TASK_ID;
#else
    /* block on this event until a task wants to re-activate the driver thread */
    qosal_task_suspend(NULL);
  } while (1);
#endif
}

#define ATHEROS_TASK_PRIORITY   QOSAL_TASK_PRIORITY_HIGHEST //6
#define ATHEROS_TASK_STACKSIZE (3000)
/*
   A_STATUS
   Driver_CreateThread(QOSAL_VOID *pCxt)
   {
   A_STATUS status = A_ERROR;
   QOSAL_UINT32 task_handle;

   if(atheros_wifi_task_id == QOSAL_NULL_TASK_ID)
   {   //OSAL API
   status =  qosal_task_create( Atheros_Driver_Task,
   "Atheros_Wifi_Task",
   ATHEROS_TASK_STACKSIZE,
   pCxt,
   ATHEROS_TASK_PRIORITY,
   &task_handle,
   1 // auto_start
   );
   atheros_wifi_task_id = task_handle;
   }

   do
   {
   if(atheros_wifi_task_id == QOSAL_NULL_TASK_ID) //OSAL API
   {
   break;
   }
   //                                                             create event on which to wait for atheros task to init

   //                                                             start the atheros task

   //                                                             qosal_task_ready(qosal_task_get_td(atheros_wifi_task_id));

   //                                                             wait on completion or timeout

   status = A_OK;
   }
   while(A_FALSE);
   return status;
   }
 */
//                                                                Compliments CreateDriverThread
A_STATUS Driver_DestroyThread(QOSAL_VOID *pCxt)
{
  GET_DRIVER_COMMON(pCxt)->driverShutdown = A_TRUE;
  DRIVER_WAKE_DRIVER(pCxt);
  return A_OK;
}

#else /* DRIVER_CONFIG_MULTI_TASKING */

A_STATUS
Driver_WaitForCondition(QOSAL_VOID *pCxt, volatile QOSAL_BOOL *pCond, QOSAL_BOOL value, QOSAL_UINT32 msec)
{
  QOSAL_TICK_STRUCT start, current;
  A_STATUS status = A_OK;
  QOSAL_UINT32 temp;

  qosal_time_get_ticks(&start);

  while (1) {
    if ((*pCond != value)) {
      Driver_Main(pCxt, DRIVER_SCOPE_RX | DRIVER_SCOPE_TX, NULL);
      qosal_time_get_ticks(&current);
      if (qosal_time_diff_milliseconds(&current, &start, &temp) >= msec) {
        status = A_ERROR;
        break;
      }
    } else {
      break;
    }
  }

  return status;
}

#endif /* DRIVER_CONFIG_MULTI_TASKING */

#if DRIVER_CONFIG_MULTI_TASKING
QOSAL_VOID Driver_WakeDriver(QOSAL_VOID *pCxt)
{
  qosal_set_event(&(GET_DRIVER_CXT(pCxt)->driverWakeEvent), 0x01);
}

QOSAL_VOID Driver_WakeUser(QOSAL_VOID *pCxt)
{
  qosal_set_event(&(GET_DRIVER_CXT(pCxt)->userWakeEvent), 0x01);
}
#endif

QOSAL_UINT16 sillyloop = 0;

void assert_func(const char *func_name, QOSAL_UINT32 line)
{
  A_PRINTF("driver assert at %s:%d\n", func_name, line);

  while (1) {
    sillyloop++;
  }
}
