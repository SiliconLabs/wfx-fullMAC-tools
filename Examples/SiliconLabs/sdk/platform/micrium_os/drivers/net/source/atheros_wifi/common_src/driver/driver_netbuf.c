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
#include <netbuf.h>
#include <cust_netbuf.h>
#include <htc_api.h>

void Driver_ReportReverseCredits(QOSAL_VOID *pReq);

#if defined(DRIVER_CONFIG_IMPLEMENT_RX_FREE_MULTIPLE_QUEUE)
QOSAL_UINT8 GET_QUEUE_INDEX(A_NETBUF_QUEUE_T *q)
{
  QOSAL_UINT8  i;

  for (i = 0; i < 8; i++) {
    if (&GET_FREE_QUEUE(i) == q) {
      break;
    }
  }

  return i;
}
#endif

QOSAL_VOID a_netbuf_enqueue(A_NETBUF_QUEUE_T *q, QOSAL_VOID *pReq)
{
//                                                                    QOSAL_UINT8   i;

  /* _int_enable and _int_disable removed by Micrium.     */
  if (q->head == NULL) {
    q->head = pReq;
  } else {
    A_ASSIGN_QUEUE_LINK(q->tail, pReq);
    //                                                            ((A_NETBUF*)q->tail)->queueLink = (A_NETBUF*)pReq;
  }

  q->tail = pReq;
  A_CLEAR_QUEUE_LINK(pReq);
  //                                                              ((A_NETBUF*)pkt)->queueLink = NULL;
  q->count++;
}

QOSAL_VOID a_netbuf_prequeue(A_NETBUF_QUEUE_T *q, QOSAL_VOID *pReq)
{
  if (q->head == NULL) {
    q->tail = pReq;
  }
  A_ASSIGN_QUEUE_LINK(pReq, q->head);
  //                                                              ((A_NETBUF*)pkt)->queueLink = q->head;
  q->head = pReq;
  q->count++;
}

QOSAL_VOID *a_netbuf_dequeue(A_NETBUF_QUEUE_T *q)
{
  QOSAL_VOID* pReq;

  if (q->head == NULL) {
    return (QOSAL_VOID*)NULL;
  }
  /* _int_enable and _int_disable removed by Micrium.     */
  pReq = q->head;

  if (q->tail == q->head) {
    q->tail = q->head = NULL;
  } else {
    q->head = A_GET_QUEUE_LINK(pReq);
    //                                                            q->head = (QOSAL_VOID*)(curr->queueLink);
  }

  q->count--;
  A_CLEAR_QUEUE_LINK(pReq);
  //                                                              curr->queueLink = NULL;
  return (QOSAL_VOID*)pReq;
}

#if defined(DRIVER_CONFIG_IMPLEMENT_RX_FREE_MULTIPLE_QUEUE)

extern QOSAL_UINT8  reverse_credits_init;
QOSAL_UINT8 credits_test = 0;

QOSAL_VOID a_rxbuf_enqueue(A_NETBUF_QUEUE_T *q, QOSAL_VOID *pReq)
{
  QOSAL_UINT8   epid;
  QOSAL_UINT32  bufCtrlNdx;

  a_netbuf_enqueue(q, pReq);

  if (reverse_credits_init == 0) {
    return;
  }

  epid = A_NETBUF_GET_ELEM(pReq, A_REQ_EPID);
  if (epid == ENDPOINT_0) {
    return;
  }

//                                                                    i = GET_QUEUE_INDEX(q);
  bufCtrlNdx = GetQueueCtrlIndexByEPID(epid);
  if ( bufCtrlNdx < 8 && bufCtrlNdx != 7) {
    CREDIT_INC(bufCtrlNdx);
//                                                                        Driver_ReportReverseCredits(pReq);
  } else {
//                                                                       printf("wrong\n");
  }
}

QOSAL_VOID *a_rxbuf_dequeue(A_NETBUF_QUEUE_T *q)
{
  QOSAL_VOID* pReq;
//                                                                    QOSAL_UINT8     i;

  pReq = a_netbuf_dequeue(q);
  if (pReq == NULL) {
    return pReq;
  }

/*    i = GET_QUEUE_INDEX(q);

    if ( i < 8)
        CREDIT_DEC(i);
 */
  return pReq;
}
#endif

QOSAL_VOID *a_netbuf_peek_queue(A_NETBUF_QUEUE_T *q)
{
  return q->head;
}

QOSAL_INT32 a_netbuf_queue_size(A_NETBUF_QUEUE_T *q)
{
  return q->count;
}

QOSAL_INT32 a_netbuf_queue_empty(A_NETBUF_QUEUE_T *q)
{
  return((q->count == 0) ? 1 : 0);
}
QOSAL_VOID a_netbuf_queue_init(A_NETBUF_QUEUE_T *q)
{
  q->head = q->tail = NULL;
  q->count = 0;
}

QOSAL_VOID* a_malloc(QOSAL_INT32 size, QOSAL_UINT8 id)
{
  QOSAL_VOID* addr;
  int len;

  addr =  QOSAL_MALLOC((QOSAL_UINT32)size);
  if (addr != NULL) {
    //                                                            qosal_get_size(addr, &len);
    len = qosal_get_size(addr);
    g_totAlloc += len;
  }
  /* in this implementation malloc ID is not used */
  UNUSED_ARGUMENT(id);

  return addr;
}

QOSAL_VOID a_free(QOSAL_VOID* addr, QOSAL_UINT8 id)
{
  QOSAL_UINT32 len;

  //                                                              qosal_get_size(addr, &len);
  len = qosal_get_size(addr);
  g_totAlloc -= len;
  /* in this implementation malloc ID is not used */
  UNUSED_ARGUMENT(id);

  QOSAL_FREE(addr);
}

/* EOF */
