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
#include <netbuf.h>
#include <driver_cxt.h>
#include <wmi_api.h>
#include  <net/source/tcpip/net_if_priv.h>

A_VOID  *pGlobalCtx;

#define AR6000_DATA_OFFSET    64

A_VOID a_netbuf_init(A_VOID *p_ctx,
                     A_VOID *p_buf,
                     A_VOID *p_data_buf)
{
  A_NETBUF                *p_a_netbuf;
  NET_DEV_CFG_WIFI        *p_dev_cfg;
  NET_IF                  *p_if;
  /* -------- OBTAIN REFERENCE TO CFGs/REGs/BSP --------- */
  p_if       = (NET_IF               *)p_ctx;                   /* Ovtain ptr to the interface area.                    */
  p_dev_cfg  = (NET_DEV_CFG_WIFI     *)p_if->Dev_Cfg;           /* Obtain ptr to dev cfg area.                          */

  /* ---------- SET A_NETBUF PTR WITH DEV CFG ----------- */
  p_a_netbuf       = (A_NETBUF*) p_buf;
  p_a_netbuf->head = (A_UINT8 *) p_data_buf;
  p_a_netbuf->data = (A_UINT8 *)((A_UINT32)p_a_netbuf->head + p_dev_cfg->RxBufIxOffset - NET_DEV_ATHEROS_PACKET_HEADER);
  p_a_netbuf->tail =  p_a_netbuf->data;
  p_a_netbuf->end  = (A_UINT8 *)((A_UINT32)p_a_netbuf->data + p_dev_cfg->RxBufLargeSize);
}

A_VOID *a_netbuf_alloc(A_INT32 size)
{
  A_NETBUF        *p_a_netbuf;
  RTOS_ERR         err;
  NET_IF          *p_ctx;
  NET_DEV_DATA    *p_dev_data;
  NET_IF          *p_if;

  /* -------------------- GET NET IF -------------------- */
  p_ctx = pGlobalCtx;

  /* -------- OBTAIN REFERENCE TO CFGs/REGs/BSP --------- */
  p_if       = (NET_IF               *)p_ctx;                   /* Obtain ptr to the interface area.                    */
  p_dev_data = (NET_DEV_DATA         *)p_if->Dev_Data;          /* Obtain ptr to dev data area.                         */

  /* ----------- OBTAIN PTR TO NEW A_NETBUF ------------- */

  p_a_netbuf = Mem_DynPoolBlkGet(&(GET_DRIVER_CXT(p_ctx)->A_NetBufPool),
                                 &err);
  if (RTOS_ERR_CODE_GET(err) != RTOS_ERR_NONE) {
    return ((A_VOID *)0);
  }
  /* ----------- WAIT GLOBAL BUF IS AVAILABLE ----------- */
  KAL_SemPend(GET_DRIVER_CXT(p_ctx)->GlobalBufSemHandle,
              KAL_OPT_PEND_NONE,
              KAL_TIMEOUT_INFINITE,
              &err);
  /* ------------ A_NET_BUF INITIALIZATION -------------- */
  A_NETBUF_INIT(p_ctx, p_a_netbuf, p_dev_data->GlobalBufPtr);
  p_a_netbuf->pool_id = A_WMI_POOL;

  return ((A_VOID *)p_a_netbuf);
}

/*
 * Allocate an NETBUF w.o. any encapsulation requirement.
 */
A_VOID *a_netbuf_alloc_raw(A_INT32 size)
{
  return (A_VOID *)DEF_NULL;
}

A_VOID a_netbuf_free(A_VOID* buffptr)
{
  A_NETBUF   *p_a_netbuf;
  A_VOID     *p_ctx;
  RTOS_ERR    err;
  NET_IF     *p_if;

  RTOS_ERR_SET(err, RTOS_ERR_NONE);
  p_a_netbuf = (A_NETBUF *) buffptr;

  p_ctx = pGlobalCtx;
  p_if  = (NET_IF *)p_ctx;                                      /* Obtain ptr to the interface area.                    */

  switch (p_a_netbuf->pool_id) {
    case A_RX_NET_POOL:

      if (p_a_netbuf->RxBufDelivered == DEF_NO) {
        NetBuf_FreeBufDataAreaRx(p_if->Nbr, p_a_netbuf->head);
      }
      break;

    case A_TX_NET_POOL:

      NetIF_TxDeallocQPost((CPU_INT08U *)p_a_netbuf->dealloc, &err);
      NetIF_DevTxRdySignal(p_ctx);
      break;

    case A_WMI_POOL:

      KAL_SemPost(GET_DRIVER_CXT(p_ctx)->GlobalBufSemHandle,
                  KAL_OPT_PEND_NONE,
                  &err);
      break;

    default:
      break;
  }

  Mem_DynPoolBlkFree(&(GET_DRIVER_CXT(p_ctx)->A_NetBufPool),
                     p_a_netbuf,
                     &err);
}

A_UINT32 a_netbuf_to_len(A_VOID *bufPtr)
{
  A_NETBUF* a_netbuf_ptr = (A_NETBUF*)bufPtr;
  A_UINT32 len = (A_UINT32)a_netbuf_ptr->tail - (A_UINT32)a_netbuf_ptr->data;

  return len;
}

/* returns a buffer fragment of a packet.  If the packet is not
 * fragmented only index == 0 will return a buffer which will be
 * the whole packet. pLen will hold the length of the buffer in
 * bytes.
 */
A_VOID*
a_netbuf_get_fragment(A_VOID  *bufPtr,
                      A_UINT8  index,
                      A_INT32 *pLen)
{
  void* pBuf = NULL;
  A_NETBUF* a_netbuf_ptr = (A_NETBUF*)bufPtr;

  if (0 == index) {
    pBuf = a_netbuf_to_data(bufPtr);
    *pLen = (A_INT32)((A_UINT32)a_netbuf_ptr->tail - (A_UINT32)a_netbuf_ptr->data);
  } else {
  }

  return pBuf;
}

A_VOID a_netbuf_configure(A_VOID   *buffptr,
                          A_VOID   *buffer,
                          A_UINT16  headroom,
                          A_UINT16  length,
                          A_UINT16  size)
{
  A_NETBUF* a_netbuf_ptr = (A_NETBUF*)buffptr;

  A_MEMZERO(a_netbuf_ptr, sizeof(A_NETBUF));

  if (buffer != NULL) {
    a_netbuf_ptr->head = buffer;
    a_netbuf_ptr->data = &(((A_UINT8*)buffer)[headroom]);
    a_netbuf_ptr->tail = &(((A_UINT8*)buffer)[headroom + length]);
    a_netbuf_ptr->end = &(((A_UINT8*)buffer)[size]);
  }
}

A_VOID *a_netbuf_to_data(A_VOID *bufPtr)
{
  return (((A_NETBUF*)bufPtr)->data);
}

/*
 * Add len # of bytes to the beginning of the network buffer
 * pointed to by bufPtr
 */
A_STATUS a_netbuf_push(A_VOID  *bufPtr,
                       A_INT32  len)
{
  A_NETBUF *a_netbuf_ptr = (A_NETBUF*)bufPtr;

  if ((A_UINT32)a_netbuf_ptr->data - (A_UINT32)a_netbuf_ptr->head < len) {
    A_ASSERT(0);
  }

  a_netbuf_ptr->data = (pointer)(((A_UINT32)a_netbuf_ptr->data) - len);

  return A_OK;
}
/*
 * Add len # of bytes to the beginning of the network buffer
 * pointed to by bufPtr and also fill with data
 */
A_STATUS a_netbuf_push_data(A_VOID  *bufPtr,
                            A_UINT8 *srcPtr,
                            A_INT32  len)
{
  a_netbuf_push(bufPtr, len);
  A_MEMCPY(((A_NETBUF*)bufPtr)->data, srcPtr, (A_UINT32)len);

  return A_OK;
}
/*
 * Add len # of bytes to the end of the network buffer
 * pointed to by bufPtr
 */
A_STATUS a_netbuf_put(A_VOID  *bufPtr,
                      A_INT32  len)
{
  A_NETBUF* a_netbuf_ptr = (A_NETBUF*)bufPtr;

  if ((A_UINT32)a_netbuf_ptr->end - (A_UINT32)a_netbuf_ptr->tail < len) {
    A_ASSERT(0);
  }

  a_netbuf_ptr->tail = (pointer)(((A_UINT32)a_netbuf_ptr->tail) + len);

  return A_OK;
}

/*
 * Add len # of bytes to the end of the network buffer
 * pointed to by bufPtr and also fill with data
 */
A_STATUS a_netbuf_put_data(A_VOID  *bufPtr,
                           A_UINT8 *srcPtr,
                           A_INT32  len)
{
  A_NETBUF* a_netbuf_ptr = (A_NETBUF*)bufPtr;
  void *start = a_netbuf_ptr->tail;

  a_netbuf_put(bufPtr, len);
  A_MEMCPY(start, srcPtr, (A_UINT32)len);

  return A_OK;
}

/*
 * Returns the number of bytes available to a a_netbuf_push()
 */
A_INT32 a_netbuf_headroom(A_VOID *bufPtr)
{
  A_NETBUF* a_netbuf_ptr = (A_NETBUF*)bufPtr;

  return (A_INT32)((A_UINT32)a_netbuf_ptr->data - (A_UINT32)a_netbuf_ptr->head);
}

A_INT32 a_netbuf_tailroom(A_VOID *bufPtr)
{
  A_NETBUF* a_netbuf_ptr = (A_NETBUF*)bufPtr;

  return (A_INT32)((A_UINT32)a_netbuf_ptr->end - (A_UINT32)a_netbuf_ptr->tail);
}

/*
 * Removes specified number of bytes from the beginning of the buffer
 */
A_STATUS a_netbuf_pull(A_VOID  *bufPtr,
                       A_INT32  len)
{
  A_NETBUF* a_netbuf_ptr = (A_NETBUF*)bufPtr;

  if ((A_UINT32)a_netbuf_ptr->tail - (A_UINT32)a_netbuf_ptr->data < len) {
    A_ASSERT(0);
  }

  a_netbuf_ptr->data = (pointer)((A_UINT32)a_netbuf_ptr->data + len);

  return A_OK;
}

/*
 * Removes specified number of bytes from the end of the buffer
 */
A_STATUS a_netbuf_trim(A_VOID  *bufPtr,
                       A_INT32  len)
{
  A_NETBUF* a_netbuf_ptr = (A_NETBUF*)bufPtr;

  if ((A_UINT32)a_netbuf_ptr->tail - (A_UINT32)a_netbuf_ptr->data < len) {
    A_ASSERT(0);
  }

  a_netbuf_ptr->tail = (pointer)((A_UINT32)a_netbuf_ptr->tail - len);

  return A_OK;
}
#if 0
A_VOID* a_malloc(A_INT32 size,
                 A_UINT8 id)
{
  A_VOID      *addr;
  CPU_SIZE_T   reqd_octets;
  RTOS_ERR     lib_err;
  /* Temporary cause the MemHeapAlloc doesn<t have an  free equivalent */
  addr = Mem_SegAllocExt("QCA a_malloc pool",
                         DEF_NULL,
                         (CPU_SIZE_T  )size,
                         (CPU_SIZE_T  )4,
                         &reqd_octets,
                         &lib_err);

  /* in this implementation malloc ID is not used */
  UNUSED_ARGUMENT(id);
  return addr;
}

A_VOID a_free(A_VOID  *addr,
              A_UINT8  id)
{
  /* To be determined */
  /* in this implementation malloc ID is not used */
  UNUSED_ARGUMENT(id);
}
#endif
