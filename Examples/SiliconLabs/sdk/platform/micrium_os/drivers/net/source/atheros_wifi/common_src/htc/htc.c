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
//                                                                ==============================================================================
//                                                                 Author(s): ="Atheros"
//                                                                ==============================================================================

#include <a_config.h>
#include <a_types.h>
#include <a_osapi.h>
#include <common_api.h>
#include <htc.h>
#include "../hcd/hcd_api.h"
#include <atheros_wifi_api.h>
#include <atheros_wifi_internal.h>
#include <htc_services.h>
#include <spi_hcd_if.h>
#include "AR6002/hw2.0/hw/mbox_host_reg.h"
#include <a_osapi.h>

//                                                                #include "htc_internal.h"

extern QOSAL_UINT8  reverse_credits_init;
QOSAL_UINT8  trailor[sizeof(HTC_RECORD_HDR) + 6 * sizeof(HTC_CREDIT_REPORT)];

QOSAL_VOID
Htc_PrepareRecvPacket(QOSAL_VOID *pCxt, QOSAL_VOID *pReq)
{
  A_DRIVER_CONTEXT *pDCxt = GET_DRIVER_COMMON(pCxt);
  HTC_FRAME_HDR *pHTCHdr;
  QOSAL_UINT16 validLength, transLength;

  pHTCHdr = (HTC_FRAME_HDR *)&(pDCxt->lookAhead);
  /* Credits are only used in TX so init to 0 here. */
  A_NETBUF_SET_ELEM(pReq, A_REQ_CREDITS, 0);
  /* get the endpoint ID from the lookahead */
  A_NETBUF_SET_ELEM(pReq, A_REQ_EPID, pHTCHdr->EndpointID);
  /* calc the full recv length */
  validLength = A_LE2CPU16(pHTCHdr->PayloadLen) + sizeof(HTC_FRAME_HDR);
  transLength = DEV_CALC_RECV_PADDED_LEN(pDCxt, validLength);
  /* init the packet length = payloadLen + sizeof(HTC_FRAME_HDR) + <padding> */
  /* NOTE: For receive packets the request transfer length and actual buffer length are the same
   *  this may not be true for transmit as the driver does not have control of the transmit
   *  buffers.
   */
  A_NETBUF_PUT(pReq, transLength);
  A_NETBUF_SET_ELEM(pReq, A_REQ_TRANSFER_LEN, transLength);
  /* init the packet lookahead value */
  A_NETBUF_SET_ELEM(pReq, A_REQ_LOOKAHEAD, pDCxt->lookAhead);
}

/*****************************************************************************/
/*  HTC_RxComplete - Completes processing of received packets whose
 *   endpoint ID == ENDPOINT_0. This is the special HTC control endpoint.
 *      QOSAL_VOID *pCxt - the driver context.
 *      QOSAL_VOID *pReq - the packet.
 *****************************************************************************/
QOSAL_VOID
Htc_RxComplete(QOSAL_VOID *pCxt, QOSAL_VOID *pReq)
{
  UNUSED_ARGUMENT(pCxt);
  /* the only control messages we are expecting are NULL messages (credit resports) */
  if (A_NETBUF_LEN(pReq) > 0) {
    A_ASSERT(0);
  }

  A_NETBUF_FREE(pReq);
}

QOSAL_UINT32
Htc_ReadCreditCounter(QOSAL_VOID *pCxt, QOSAL_UINT32 index)
{
  A_NETBUF_DECLARE req;
  QOSAL_VOID *pReq = (QOSAL_VOID*)&req;
  QOSAL_UINT32 collector;

  A_NETBUF_CONFIGURE(pReq, &collector, 0, sizeof(QOSAL_UINT32), sizeof(QOSAL_UINT32));
  /* read the target credit counter */
  ATH_SET_PIO_EXTERNAL_READ_OPERATION(pReq, COUNT_ADDRESS + 4 * index, QOSAL_TRUE, sizeof(QOSAL_UINT32));

  if (A_OK != Hcd_DoPioExternalAccess(pCxt, pReq)) {
    A_ASSERT(0);
  }

  collector = A_LE2CPU32(collector);

  return collector;
}

QOSAL_VOID
Htc_GetCreditCounterUpdate(QOSAL_VOID *pCxt, QOSAL_UINT16 epId)
{
  A_ENDPOINT_T    *pEp;
  QOSAL_UINT32 collector;
  QOSAL_UINT16 reg_index, array_index;
  QOSAL_UINT8 credits;
        #if USE_16BIT_CREDIT_COUNTER
  QOSAL_UINT16 credits;
        #endif
  A_DRIVER_CONTEXT *pDCxt = GET_DRIVER_COMMON(pCxt);

  pEp = Util_GetEndpoint(pCxt, epId);

  if (pDCxt->htc_creditInit == 0) {
    /* never got interrupt from chip to indicate that this
     * credit scheme was supported */
    pEp->intCreditCounter = 0;
    return;
  }

  reg_index = pEp->epIdx >> 2;
  array_index = pEp->epIdx & 0x03;
  /* read the target credit counter */
  collector = Htc_ReadCreditCounter(pCxt, reg_index);

  credits = (collector >> (8 * array_index)) & 0xff;
#if USE_16BIT_CREDIT_COUNTER
  credits = (collector >> (8 * array_index)) & 0xffff;   // modify this credits variable to 16bit
  if ((credits & 0xFF00) >> 8) {
    credits = 255 + ((credits & 0xFF00) >> 8);
    //                                                            ath_io_printf("Credits 0x%x \n",255 + ((credits & 0xFF00) >> 8));
  } else {
    credits = (credits & 0xFF);
  }
#endif
  pEp->intCreditCounter = credits;
  credits = pEp->intCreditCounter - pEp->rptCreditCounter;

  /* The below condition occurs when we stop recieving in-band credits and are purely relying */
  /* on out-of-band credits. In such cases it is possible that out-of-band credit can wrap around and this prevents the same */
  if (credits < pEp->intCredits) {
    return;
  }

  credits = credits - pEp->intCredits; //deduct any previous int credits
  pEp->intCredits += credits; //track num credits acquired this way.
  /* add the new credits to the EP credit count */
  pEp->credits += credits;
}

static QOSAL_VOID
Htc_DistributeCredits(QOSAL_VOID *pCxt)
{
  QOSAL_UINT32 numCredits;
  A_DRIVER_CONTEXT *pDCxt = GET_DRIVER_COMMON(pCxt);

  numCredits = (QOSAL_UINT16)pDCxt->creditCount;
  pDCxt->ep[0].maxCredits = pDCxt->ep[0].credits = 1;
  pDCxt->ep[0].rptCreditCounter = 0;
  pDCxt->ep[0].intCreditCounter = 0;
  pDCxt->ep[0].intCredits = 0;
  numCredits--;
  pDCxt->ep[1].maxCredits = pDCxt->ep[1].credits = 1;
  pDCxt->ep[1].rptCreditCounter = 0;
  pDCxt->ep[1].intCreditCounter = 0;
  pDCxt->ep[1].intCredits = 0;
  numCredits--;
  pDCxt->ep[2].maxCredits = pDCxt->ep[2].credits = (QOSAL_UINT8)numCredits;
#if USE_16BIT_CREDIT_COUNTER
  pDCxt->ep[2].maxCredits = pDCxt->ep[2].credits = (QOSAL_UINT16)numCredits;
#endif
  pDCxt->ep[2].rptCreditCounter = 0;
  pDCxt->ep[2].intCreditCounter = 0;
  pDCxt->ep[2].intCredits = 0;
}

/* process credit reports and call distribution function */
static QOSAL_VOID
Htc_ProcessCreditRpt(QOSAL_VOID *pCxt, HTC_CREDIT_REPORT *pRpt, QOSAL_INT32 NumEntries, HTC_ENDPOINT_ID FromEndpoint)
{
  int             i;
  A_ENDPOINT_T    *pEndpoint;
  QOSAL_UINT8     credits;
#if USE_16BIT_CREDIT_COUNTER
  QOSAL_UINT16      credits;
#endif
  UNUSED_ARGUMENT(FromEndpoint);

  for (i = 0; i < NumEntries; i++) {
    if (pRpt[i].EndpointID >= ENDPOINT_MAX) {
      A_ASSERT(0);
#if DRIVER_CONFIG_DISABLE_ASSERT
      break;
#endif
    }

    pEndpoint = Util_GetEndpoint(pCxt, pRpt[i].EndpointID);
    /* this variable will and should wrap around. */
    pEndpoint->rptCreditCounter += pRpt[i].Credits;

    credits = pRpt[i].Credits;

    if (pEndpoint->intCredits) {
      if (pEndpoint->intCredits <= pRpt[i].Credits) {
        credits = pRpt[i].Credits - pEndpoint->intCredits;
        pEndpoint->intCredits = 0;
      } else {
        credits = 0;
        pEndpoint->intCredits -= pRpt[i].Credits;
      }
    }

    pEndpoint->credits += credits;
  }
}

/* Interrupt cause register bit definitions:
 * bit 0 - used for initial credit distribution
 * bit 1 - Start of frame in Argos
 * bit 2-7 - unused
 */
QOSAL_VOID
HTC_ProcessCpuInterrupt(QOSAL_VOID *pCxt)
{
  A_NETBUF_DECLARE req;
  QOSAL_VOID *pReq = (QOSAL_VOID*)&req;
  QOSAL_UINT32 collector;
  QOSAL_UINT8* ptr = (QOSAL_UINT8*)&collector;

  A_DRIVER_CONTEXT *pDCxt = GET_DRIVER_COMMON(pCxt);

  /* The interrupt cause register indicates the reason who raised
   * this interrupt. Use it to decode*/
  /* bit 0 is for credit distribution */
  if (pDCxt->spi_hcd.CpuInterruptCause & 0x1) {
    /* if this is the first such interrupt then acquire the
     * address of the HTC comm memory. */
    A_NETBUF_CONFIGURE(pReq, &collector, 0, sizeof(QOSAL_UINT32), sizeof(QOSAL_UINT32));

    if (pDCxt->htc_creditInit == 0) {
      pDCxt->htc_creditInit = 1;
      ATH_SET_PIO_EXTERNAL_READ_OPERATION(pReq, COUNT_ADDRESS + 4, QOSAL_TRUE, sizeof(QOSAL_UINT32));

      if (A_OK != Hcd_DoPioExternalAccess(pCxt, pReq)) {
        A_ASSERT(0);
      }

      /* the last byte of the COUNT_ADDRESS contain the creditCount */
      pDCxt->creditCount = ptr[3];
      /* distribute the credits among the endpoints */
      Htc_DistributeCredits(pCxt);
    }
  }

  /* bit 1 is for SOF in argos */
  if (pDCxt->spi_hcd.CpuInterruptCause & 0x2) {
    //                                                            printf("SOF intr\n");
  }

  /* All interrupts have been processed */
  pDCxt->spi_hcd.CpuInterruptCause = 0x0;
}

/*****************************************************************************/
/*  Htc_ProcessTrailer - Trailers can be appended to the end of a received
 *   packet. This function processes those trailers prior to their removal
 *   from the packet.
 *      QOSAL_VOID *pCxt - the driver context.
 *      QOSAL_UINT8 *pBuffer - the buffer holding the trailers.
 *      QOSAL_INT32 Length - the length of the pBuffer in bytes.
 *      QOSAL_UINT32   *pNextLookAheads - storage for any lookaheads found in
 *                  the trailer.
 *      QOSAL_INT32    *pNumLookAheads - storage for a count of valid lookaheads
 *                  stored in pNextLookAheads.
 *      HTC_ENDPOINT_ID FromEndpoint - the endpoint ID of the received packet.
 *****************************************************************************/
static A_STATUS
Htc_ProcessTrailer(QOSAL_VOID *pCxt,
                   QOSAL_UINT8    *pBuffer,
                   QOSAL_INT32    Length,
                   QOSAL_UINT32   *pNextLookAheads,
                   QOSAL_UINT32    *pNumLookAheads,
                   HTC_ENDPOINT_ID FromEndpoint)
{
  HTC_RECORD_HDR          *pRecord;
  QOSAL_UINT8                 *pRecordBuf;
  HTC_LOOKAHEAD_REPORT    *pLookAhead;
  A_STATUS                status;

  status = A_OK;

  while (Length > 0) {
    if (Length < sizeof(HTC_RECORD_HDR)) {
      status = A_EPROTO;
      break;
    }
    /* these are byte aligned structs */
    pRecord = (HTC_RECORD_HDR *)pBuffer;
    Length -= sizeof(HTC_RECORD_HDR);
    pBuffer += sizeof(HTC_RECORD_HDR);

    if (pRecord->Length > Length) {
      /* no room left in buffer for record */
      status = A_EPROTO;
      break;
    }
    /* start of record follows the header */
    pRecordBuf = pBuffer;

    switch (pRecord->RecordID) {
      case HTC_RECORD_CREDITS:       /* the device is indicating that new TX credits are available */
        Htc_ProcessCreditRpt(pCxt,
                             (HTC_CREDIT_REPORT *)pRecordBuf,
                             (QOSAL_INT32)(pRecord->Length / (sizeof(HTC_CREDIT_REPORT))),
                             FromEndpoint);
        break;
      case HTC_RECORD_LOOKAHEAD:       /* the device is providing the lookahead for the next packet */
        A_ASSERT(pRecord->Length >= sizeof(HTC_LOOKAHEAD_REPORT));
        pLookAhead = (HTC_LOOKAHEAD_REPORT *)pRecordBuf;
        if ((pLookAhead->PreValid == ((~pLookAhead->PostValid) & 0xFF))
            && (pNextLookAheads != NULL)) {
          /* look ahead bytes are valid, copy them over */
          ((QOSAL_UINT8 *)(&pNextLookAheads[0]))[0] = pLookAhead->LookAhead[0];
          ((QOSAL_UINT8 *)(&pNextLookAheads[0]))[1] = pLookAhead->LookAhead[1];
          ((QOSAL_UINT8 *)(&pNextLookAheads[0]))[2] = pLookAhead->LookAhead[2];
          ((QOSAL_UINT8 *)(&pNextLookAheads[0]))[3] = pLookAhead->LookAhead[3];

          /* just one normal lookahead */
          *pNumLookAheads = 1;
        }
        break;
      case HTC_RECORD_LOOKAHEAD_BUNDLE:
#if 0 /* this feature is not currently supported by this driver */
        AR_DEBUG_ASSERT(pRecord->Length >= sizeof(HTC_BUNDLED_LOOKAHEAD_REPORT));
        if (pRecord->Length >= sizeof(HTC_BUNDLED_LOOKAHEAD_REPORT)
            && (pNextLookAheads != NULL)) {
          HTC_BUNDLED_LOOKAHEAD_REPORT    *pBundledLookAheadRpt;
          QOSAL_INT32                             i;

          pBundledLookAheadRpt = (HTC_BUNDLED_LOOKAHEAD_REPORT *)pRecordBuf;

          if ((pRecord->Length / (sizeof(HTC_BUNDLED_LOOKAHEAD_REPORT)))
              > HTC_HOST_MAX_MSG_PER_BUNDLE) {
            /* this should never happen, the target restricts the number
             * of messages per bundle configured by the host */
            A_ASSERT(QOSAL_FALSE);
            status = A_EPROTO;
            break;
          }

          for (i = 0; i < (int)(pRecord->Length / (sizeof(HTC_BUNDLED_LOOKAHEAD_REPORT))); i++) {
            ((QOSAL_UINT8 *)(&pNextLookAheads[i]))[0] = pBundledLookAheadRpt->LookAhead[0];
            ((QOSAL_UINT8 *)(&pNextLookAheads[i]))[1] = pBundledLookAheadRpt->LookAhead[1];
            ((QOSAL_UINT8 *)(&pNextLookAheads[i]))[2] = pBundledLookAheadRpt->LookAhead[2];
            ((QOSAL_UINT8 *)(&pNextLookAheads[i]))[3] = pBundledLookAheadRpt->LookAhead[3];
            pBundledLookAheadRpt++;
          }

          *pNumLookAheads = i;
        }
        break;
#else
        A_ASSERT(0);
#if DRIVER_CONFIG_DISABLE_ASSERT
        break;
#endif
#endif

      default:
        break;
    }

    if (A_OK != status) {
      break;
    }

    /* advance buffer past this record for next time around */
    pBuffer += pRecord->Length;
    Length -= pRecord->Length;
  }

  if (A_OK != status) {
    A_ASSERT(0);
  }

  return status;
}

#if  defined(DRIVER_CONFIG_IMPLEMENT_RX_FREE_MULTIPLE_QUEUE)
int hasRCSReport()
{
  QOSAL_UINT8  i;

#if TWO_MSQ
  for (i = 0; i < 8; i++)
#else
  for (i = 1; i < 8; i++)
#endif
  {
    if (GET_CREDIT(i) != 0) {
      return QOSAL_TRUE;
    }
  }
  return QOSAL_FALSE;
}

static int CollectCredits(QOSAL_UINT8 *buff, int lenFreeTrail)
{
  HTC_RECORD_HDR  *hd;
  HTC_CREDIT_REPORT *rec;

  int len = 0;
  QOSAL_UINT8  i;

  hd = (HTC_RECORD_HDR *)buff;
  rec = (HTC_CREDIT_REPORT *)(buff + sizeof(HTC_RECORD_HDR));

//                                                                    qosal_intr_disable();

#if TWO_MSQ
  for (i = 0; i < 8; i++)
#else
  for (i = 1; i < 8; i++)
#endif
  {
    if (len + sizeof(HTC_CREDIT_REPORT) > lenFreeTrail) {
      break;
    }

    if (GET_CREDIT(i) != 0) {
      rec->EndpointID = i;
      rec->Credits = GET_CREDIT(i);
      CLEAR_CREDIT(i);

      len += sizeof(HTC_CREDIT_REPORT);
      rec++;
    }
  }
  hd->RecordID = HTC_RECORD_CREDITS;
  hd->Length = len;

  len += sizeof(HTC_RECORD_HDR);
//                                                                    send_reverse_credits_flag = 0;

//                                                                    qosal_intr_enable();

  return len;
}
#endif

A_NETBUF  *p;

A_STATUS
Htc_SendPacket(QOSAL_VOID *pCxt, QOSAL_VOID *pReq)
{
  A_STATUS status = A_ERROR;
  A_ENDPOINT_T *pEp;
  QOSAL_UINT8 htcFlags = 0;
  QOSAL_UINT8 SeqNo = 0;
  QOSAL_UINT8 *pHdrBuf;
  QOSAL_UINT16 len;
  A_DRIVER_CONTEXT *pDCxt = GET_DRIVER_COMMON(pCxt);

  do {
    /* adjust the Endpoint credit count */
    if (NULL == (pEp = Util_GetEndpoint(pCxt, A_NETBUF_GET_ELEM(pReq, A_REQ_EPID)))) {
      break;
    }
    if (pDCxt->htcStart == QOSAL_TRUE) {
      if (0 == pEp->credits) {
        break;
      }

      if (A_NETBUF_GET_ELEM(pReq, A_REQ_CREDITS) == 0) {
        SeqNo = 0;
      }

      A_ASSERT(A_NETBUF_GET_ELEM(pReq, A_REQ_CREDITS));
      pEp->credits -= A_NETBUF_GET_ELEM(pReq, A_REQ_CREDITS);
      SeqNo = pEp->seqNo;
      pEp->seqNo++;

      if (0 == pEp->credits) {
        htcFlags |= HTC_FLAGS_NEED_CREDIT_UPDATE;
      }
    }

    pHdrBuf = A_NETBUF_DATA(pReq);

#if defined(DRIVER_CONFIG_IMPLEMENT_RX_FREE_MULTIPLE_QUEUE)
    if (reverse_credits_init != 0) {
      int lenRecord, lenFreeTrail;
      QOSAL_UINT8  buff[sizeof(HTC_RECORD_HDR) + 6 * sizeof(HTC_CREDIT_REPORT)];
      QOSAL_UINT8  *data;

      if (A_NETBUF_GET_ELEM(pReq, A_REQ_EPID) == ENDPOINT_0) {
        lenFreeTrail = A_NETBUF_TAILROOM(pReq);
        if (lenFreeTrail < 12) {
          p = (A_NETBUF *)pReq;
        }
        A_ASSERT(lenFreeTrail >= 12);

        lenRecord = CollectCredits(buff, lenFreeTrail);

        pHdrBuf = A_NETBUF_DATA(pReq);
        if (lenRecord > sizeof(HTC_RECORD_HDR)) {
          A_NETBUF_PUT_DATA(pReq, buff, lenRecord);
          htcFlags |= HTC_FLAGS_SEND_TRAILER;
          A_SET_UINT8_FIELD(pHdrBuf, HTC_FRAME_HDR, ControlBytes[1], lenRecord);
        } else {
          A_SET_UINT8_FIELD(pHdrBuf, HTC_FRAME_HDR, ControlBytes[1], 0);
        }
      } else {
        lenRecord = CollectCredits(buff, lenFreeTrail);

        pHdrBuf = A_NETBUF_DATA(pReq);
        data = NULL;

        if (lenRecord > sizeof(HTC_RECORD_HDR)) {
          data = trailor;
        }

        if (data != NULL) {
          A_MEMCPY(data, buff, lenRecord);
          A_NETBUF_APPEND_FRAGMENT(pReq, data, lenRecord);
          htcFlags |= HTC_FLAGS_SEND_TRAILER;
          A_SET_UINT8_FIELD(pHdrBuf, HTC_FRAME_HDR, ControlBytes[1], lenRecord);
        } else {
          A_SET_UINT8_FIELD(pHdrBuf, HTC_FRAME_HDR, ControlBytes[1], 0);
        }
      }
    }
#else
    A_SET_UINT8_FIELD(pHdrBuf, HTC_FRAME_HDR, ControlBytes[1], (QOSAL_UINT8)(SeqNo));
#endif
    /* fill in elements of the HTC header. treat the header as an un-aligned struct for safety */
    //                                                            A_SET_UINT16_FIELD(pHdrBuf, HTC_FRAME_HDR, PayloadLen, (QOSAL_UINT16)A_NETBUF_LEN(pReq) - HTC_HDR_LENGTH);
    len        = (QOSAL_UINT16)A_NETBUF_LEN(pReq) - HTC_HDR_LENGTH;
    pHdrBuf[2] = (QOSAL_UINT8)(len & 0x00FF);
    pHdrBuf[3] = (QOSAL_UINT8)((len & 0xFF00) >> 8);
    A_SET_UINT8_FIELD(pHdrBuf, HTC_FRAME_HDR, Flags, (htcFlags));
    A_SET_UINT8_FIELD(pHdrBuf, HTC_FRAME_HDR, EndpointID, (QOSAL_UINT8)A_NETBUF_GET_ELEM(pReq, A_REQ_EPID));
    A_SET_UINT8_FIELD(pHdrBuf, HTC_FRAME_HDR, ControlBytes[0], (QOSAL_UINT8)(0));
/*	    A_SET_UINT8_FIELD(pHdrBuf, HTC_FRAME_HDR,ControlBytes[1], (QOSAL_UINT8)(SeqNo));
 */
    status = A_OK;
  } while (0);

  return status;
}

A_STATUS
Htc_ProcessTxComplete(QOSAL_VOID *pCxt, QOSAL_VOID *pReq)
{
  UNUSED_ARGUMENT(pCxt);
  /* remove the HTC header from the front of the packet. */
  return A_NETBUF_PULL(pReq, HTC_HDR_LENGTH);
}

/*****************************************************************************/
/*  HTC_ProcessRecvHeader - Processes HTC headers and removes them from the
 *   front of the packet.
 *      QOSAL_VOID *pCxt - the driver context.
 *      QOSAL_VOID *pReq - the packet.
 *      QOSAL_UINT32   *pNextLookAheads - storage for any lookaheads found in
 *                  the trailer.
 *      QOSAL_INT32    *pNumLookAheads - storage for a count of valid lookaheads
 *                  stored in pNextLookAheads.
 *****************************************************************************/
A_STATUS
Htc_ProcessRecvHeader(QOSAL_VOID *pCxt, QOSAL_VOID *pReq,
                      QOSAL_UINT32   *pNextLookAheads,
                      QOSAL_UINT32        *pNumLookAheads)
{
  QOSAL_UINT8   temp;
  QOSAL_UINT8   *pBuf;
  A_STATUS  status = A_OK;
  QOSAL_UINT16  payloadLen, bufLength;
  QOSAL_UINT32  lookAhead;

  if (pNumLookAheads != NULL) {
    *pNumLookAheads = 0;
  }

  do {
    bufLength = A_NETBUF_LEN(pReq);
    /* gets the first buffer which will/must contain the HTC header */
    if (NULL == (pBuf = A_NETBUF_DATA(pReq)) || bufLength < HTC_HDR_LENGTH) {
      /* this is a critical error */
      A_ASSERT(0);
#if DRIVER_CONFIG_DISABLE_ASSERT
      status = A_EPROTO;
      break;
#endif
    }
    /* note, we cannot assume the alignment of pBuffer, so we use the safe macros to
     * retrieve 16 bit fields */
    //                                                             payloadLen = (QOSAL_UINT16)A_GET_UINT16_FIELD(pBuf, HTC_FRAME_HDR, PayloadLen);
    payloadLen   = pBuf[3];
    payloadLen <<= 8;
    payloadLen  += pBuf[2];

    ((QOSAL_UINT8 *)&lookAhead)[0] = pBuf[0];
    ((QOSAL_UINT8 *)&lookAhead)[1] = pBuf[1];
    ((QOSAL_UINT8 *)&lookAhead)[2] = pBuf[2];
    ((QOSAL_UINT8 *)&lookAhead)[3] = pBuf[3];

    if (lookAhead != A_NETBUF_GET_ELEM(pReq, A_REQ_LOOKAHEAD)) {
      /* this is a critical error */
      A_ASSERT(0);
#if DRIVER_CONFIG_DISABLE_ASSERT
      status = A_EPROTO;
      break;
#endif
    }
    /* now that the payloadLen and the HTC header has been confirmed
     * remove any padding length that may have been added to the buffer
     */
    if (payloadLen + HTC_HDR_LENGTH < A_NETBUF_LEN(pReq)) {
      A_NETBUF_TRIM(pReq, A_NETBUF_LEN(pReq) - (payloadLen + HTC_HDR_LENGTH));
    }

    /* get flags */
    temp = A_GET_UINT8_FIELD(pBuf, HTC_FRAME_HDR, Flags);

    if (temp & HTC_FLAGS_RECV_TRAILER) {
      /* this packet has a trailer */

      /* extract the trailer length in control byte 0 */
      temp = A_GET_UINT8_FIELD(pBuf, HTC_FRAME_HDR, ControlBytes[0]);

      if ((temp < sizeof(HTC_RECORD_HDR)) || (temp > payloadLen)) {
        /* this is a critical error */
        A_ASSERT(0);
#if DRIVER_CONFIG_DISABLE_ASSERT
        status = A_EPROTO;
        break;
#endif
      }
      /* process trailer data that follows HDR + application payload */
      if (A_OK != (status = Htc_ProcessTrailer(pCxt,
                                               (pBuf + HTC_HDR_LENGTH + payloadLen - temp),
                                               temp,
                                               pNextLookAheads,
                                               pNumLookAheads,
                                               (HTC_ENDPOINT_ID)A_NETBUF_GET_ELEM(pReq, A_REQ_EPID)))) {
        break;
      }
      /* remove the trailer bytess via buffer trim */
      A_NETBUF_TRIM(pReq, temp);
    }
    /* if we get to this point, the packet is good */
    /* remove header and adjust length */
    A_NETBUF_PULL(pReq, HTC_HDR_LENGTH);
  } while (0);

  if (A_OK != status) {
    A_ASSERT(0);
  }

  return status;
}

A_STATUS
HTC_WaitTarget(QOSAL_VOID *pCxt)
{
  A_STATUS                 status;
  //                                                              HTC_READY_EX_MSG        *pRdyMsg;
  HTC_SERVICE_CONNECT_REQ  connect;
  HTC_SERVICE_CONNECT_RESP resp;
  A_DRIVER_CONTEXT *pDCxt = GET_DRIVER_COMMON(pCxt);

  do {
    if (ath_custom_htc.HTCGetReady != NULL) {
      if (A_OK != (status = ath_custom_htc.HTCGetReady(pCxt))) {
        break;
      }
    } else {
      pDCxt->creditCount = AR4100_DEFAULT_CREDIT_COUNT;
      pDCxt->creditSize = AR4100_BUFFER_SIZE;
    }
    /* setup our pseudo HTC control endpoint connection */
    A_MEMZERO(&connect, sizeof(connect));
    A_MEMZERO(&resp, sizeof(resp));

    connect.ServiceID = HTC_CTRL_RSVD_SVC;

    /* connect fake service */
    status = HTC_ConnectService(pCxt,
                                &connect,
                                &resp);

    if (!A_FAILED(status)) {
      break;
    }
  } while (QOSAL_FALSE);

  return status;
}

A_STATUS
HTC_ConnectService(QOSAL_VOID *pCxt,
                   HTC_SERVICE_CONNECT_REQ  *pConnectReq,
                   HTC_SERVICE_CONNECT_RESP *pConnectResp)
{
  A_STATUS    status = A_OK;
  HTC_ENDPOINT_ID assignedEndpoint = ENDPOINT_MAX;
  A_ENDPOINT_T  *pEndpoint;
  QOSAL_UINT32    maxMsgSize = 0;

  do {
    if (HTC_CTRL_RSVD_SVC == pConnectReq->ServiceID) {
      /* special case for pseudo control service */
      assignedEndpoint = ENDPOINT_0;
      maxMsgSize = HTC_MAX_CONTROL_MESSAGE_LENGTH;
    } else {
      if (ath_custom_htc.HTCConnectServiceExch != NULL) {
        status = ath_custom_htc.HTCConnectServiceExch(pCxt, pConnectReq, pConnectResp,
                                                      &assignedEndpoint, &maxMsgSize);

        if (status != A_OK) {
          break;
        }
      } else {
        pConnectResp->ConnectRespCode = HTC_SERVICE_SUCCESS;

        switch (pConnectReq->ServiceID) {
          case WMI_CONTROL_SVC:
            assignedEndpoint = ENDPOINT_1;
            maxMsgSize = 1542;
            break;
          case WMI_DATA_BE_SVC:
            assignedEndpoint = ENDPOINT_2;
            maxMsgSize = 4144;
            break;
          case WMI_DATA_BK_SVC:
            assignedEndpoint = ENDPOINT_3;
            maxMsgSize = 4144;
            break;
          case WMI_DATA_VI_SVC:
            assignedEndpoint = ENDPOINT_4;
            maxMsgSize = 4144;
            break;
          case WMI_DATA_VO_SVC:
            assignedEndpoint = ENDPOINT_5;
            maxMsgSize = 4144;
            break;
        }
      }
    }

    /* the rest of these are parameter checks so set the error status */
    status = A_EPROTO;

    if (0 == maxMsgSize) {
      A_ASSERT(0);
#if DRIVER_CONFIG_DISABLE_ASSERT
      break;
#endif
    }

    pEndpoint = Util_GetEndpoint(pCxt, assignedEndpoint);
    /* this marks the endpoint in use */
    pEndpoint->serviceID = pConnectReq->ServiceID;
    /* return assigned endpoint to caller */
    pConnectResp->Endpoint = assignedEndpoint;
    pConnectResp->MaxMsgLength = maxMsgSize;

    pEndpoint->maxMsgLength = (QOSAL_INT32)maxMsgSize;

    status = A_OK;
  } while (0);

  return status;
}

/* Start HTC, enable interrupts and let the target know host has finished setup */
A_STATUS
HTC_Start(QOSAL_VOID *pCxt)
{
  A_STATUS   status = A_OK;

  do {
    Htc_DistributeCredits(pCxt);

    if (ath_custom_htc.HTCSendSetupComplete != NULL) {
      /* the caller is done connecting to services, so we can indicate to the
       * target that the setup phase is complete */
      status = ath_custom_htc.HTCSendSetupComplete(pCxt);

      if (A_FAILED(status)) {
        break;
      }
    }

    /* unmask the packet interrupt on-chip */
    Hcd_UnmaskInterrupts(pCxt, ATH_SPI_INTR_PKT_AVAIL);
    GET_DRIVER_COMMON(pCxt)->htcStart = QOSAL_TRUE;
  } while (0);

  return status;
}

/* synchronously wait for a message from the target,
 */
#if 0
A_STATUS
Htc_WaitforMessage(HTC_HANDLE HTCHandle, HTC_PACKET **ppControlPacket, QOSAL_UINT8 EndpointID)
{
  A_STATUS        status;
  QOSAL_UINT32        lookAhead;
  HTC_PACKET      *pPacket = NULL;
  HTC_FRAME_HDR   *pHdr;
  HTC_TARGET      *target = GET_HTC_TARGET_FROM_HANDLE(HTCHandle);

  do {
    /* call the polling function to see if we have a message */
    status = DevPollMboxMsgRecv(&target->Device,
                                &lookAhead,
                                HTC_TARGET_RESPONSE_TIMEOUT);

    if (A_FAILED(status)) {
      break;
    }

    /* check the lookahead */
    pHdr = (HTC_FRAME_HDR *)&lookAhead;

    if (pHdr->EndpointID != EndpointID) {
      /* unexpected endpoint number, should be zero */
      status = A_EPROTO;
      break;
    }

    if (A_FAILED(status)) {
      /* bad message */
      status = A_EPROTO;
      break;
    }

    if (*ppControlPacket != NULL) {
      pPacket = *ppControlPacket;
    } else {
      pPacket = HTC_ALLOC_CONTROL_RX(target);

      if (pPacket == NULL) {
        status = A_NO_MEMORY;
        break;
      }
    }

    pPacket->PktInfo.AsRx.HTCRxFlags = 0;
    pPacket->PktInfo.AsRx.ExpectedHdr = lookAhead;
    pPacket->ActualLength = A_LE2CPU16(pHdr->PayloadLen) + HTC_HDR_LENGTH;

    if (pPacket->ActualLength > pPacket->BufferLength) {
      status = A_EPROTO;
      break;
    }

    /* we want synchronous operation */
    pPacket->Completion = NULL;

    /* get the message from the device, this will block */
    status = HTCIssueRecv(target, pPacket);

    if (A_FAILED(status)) {
      break;
    }

    /* process receive header */
    status = HTCProcessRecvHeader(target, pPacket, NULL, NULL);

    pPacket->Status = status;

    if (A_FAILED(status)) {
      AR_DEBUG_PRINTF(ATH_DEBUG_ERR,
                      ("HTCWaitforMessage, HTCProcessRecvHeader failed (status = %d) \n",
                       status));
      break;
    }

    /* give the caller this control message packet, they are responsible to free */
    *ppControlPacket = pPacket;
  } while (QOSAL_FALSE);

  if (A_FAILED(status)) {
    if (pPacket != NULL && *ppControlPacket == NULL) {
      /* cleanup buffer on error */
      HTC_FREE_CONTROL_RX(target, pPacket);
    }
  }

  return status;
}

#endif
