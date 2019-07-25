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
#ifndef _CUST_NETBUF_H_
#define _CUST_NETBUF_H_

#include <a_types.h>
#include <a_config.h>
#include "../../include/athdefs.h"
#include "../../common_src/include/netbuf.h"
#include <atheros_wifi_api.h>

typedef  enum net_dev_pool_type {
  A_RX_NET_POOL = 1u,
  A_TX_NET_POOL = 2u,
  A_WMI_POOL    = 3u,
}NET_DEV_POOL_TYPE;

/* A_NETBUF represents the data structure for a pReq object (packet)
 *      throughout the driver. It is part of the custom implementation
 *	because there may be more optimal ways to achieve the same results
 *	in other OS/systems. It is expected that any of the elements except
 *	for the A_TRANSPORT_OBJ can be replaced by some OS specific construct
 *      thereby making more efficient use of resources. */
typedef struct _ath_netbuf {
  NET_DEV_POOL_TYPE  pool_id;
  A_BOOL             RxBufDelivered;
  A_UINT8         *head;   // marks the start of head space
  A_UINT8         *data;   // marks the start of data space
  A_UINT8         *tail;   // marks the start of tail space
  A_UINT8         *end;    // marks the end of packet.
  A_UINT8           *dealloc;    // marks the end of packet
  A_VOID        *queueLink;   // used to enqueue packets
  A_TRANSPORT_OBJ    trans;         /* container object for netbuf elements */
} A_NETBUF, *A_NETBUF_PTR;

/* the A_REQ_... definitions are used in calls to A_NETBUF_GET_ELEM and
 *	A_NETBUF_SET_ELEM.  They identify what element is required. The
 *	developer can either adopt this implementation or create their
 *	own more suitable definitions as appropriate. */
#define A_REQ_ADDRESS       trans.address
#define A_REQ_EPID          trans.epid
#define A_REQ_STATUS        trans.status
#define A_REQ_CREDITS       trans.credits
#define A_REQ_CALLBACK      trans.cb
#define A_REQ_COMMAND       trans.cmd
#define A_REQ_LOOKAHEAD     trans.lookahead
#define A_REQ_TRANSFER_LEN  trans.transferLength

#define A_NETBUF_GET_ELEM(_obj, _e) ((A_NETBUF*)(_obj))->_e
#define A_NETBUF_SET_ELEM(_obj, _e, _v) ((A_NETBUF*)(_obj))->_e = (_v)

/* A_CLEAR_QUEUE_LINK - used by netbuf queue code to clear the link. */
#define A_CLEAR_QUEUE_LINK(_pReq) ((A_NETBUF*)(_pReq))->queueLink = NULL
/* A_ASSIGN_QUEUE_LINK - used by netbuf queue code to populate/connect the link. */
#define A_ASSIGN_QUEUE_LINK(_pReq, _pN) ((A_NETBUF*)(_pReq))->queueLink = (_pN)
/* A_GET_QUEUE_LINK - used by netbuf queue code to get the link. */
#define A_GET_QUEUE_LINK(_pReq) ((A_NETBUF*)(_pReq))->queueLink
/*
 * Network buffer support
 */

/* A_NETBUF_DECLARE - declare a NETBUF object on the stack */
#define A_NETBUF_DECLARE A_NETBUF
/* A_NETBUF_CONFIGURE - (re-)configure a NETBUF object */
#define A_NETBUF_CONFIGURE(bufPtr, buffer, headroom, length, size) \
  a_netbuf_configure((bufPtr), (buffer), (headroom), (length), (size))
/* A_NETBUF_ALLOC - allocate a NETBUF object from the "heap". */
#define A_NETBUF_ALLOC(size) \
  a_netbuf_alloc(size)
/* A_NETBUF_ALLOC_RAW - allocate a NETBUF object from the "heap". */
#define A_NETBUF_ALLOC_RAW(size) \
  a_netbuf_alloc_raw(size)
/* A_NETBUF_INIT - (re-)initialize a NETBUF object. */
#define A_NETBUF_INIT(bufPtr, freefn, priv) \
  a_netbuf_init((bufPtr), (freefn), (priv))
/* A_NETBUF_FREE - release a NETBUF object back to the "heap". */
#define A_NETBUF_FREE(bufPtr) \
  a_netbuf_free(bufPtr)
/* A_NETBUF_DATA - Get a pointer to the front of valid/populated NETBUF memory. */
#define A_NETBUF_DATA(bufPtr) \
  a_netbuf_to_data(bufPtr)
/* A_NETBUF_LEN - Get the length of valid/populated NETBUF memory (total for all fragments). */
#define A_NETBUF_LEN(bufPtr) \
  a_netbuf_to_len(bufPtr)
/* A_NETBUF_PUSH - Adds #len bytes to the beginning of valid/populated NETBUF memory */
#define A_NETBUF_PUSH(bufPtr, len) \
  a_netbuf_push(bufPtr, len)
/* A_NETBUF_PUT - Adds #len bytes to the end of valid/populated NETBUF memory */
#define A_NETBUF_PUT(bufPtr, len) \
  a_netbuf_put(bufPtr, len)
/* A_NETBUF_TRIM - Removes #len bytes from the end of the valid/populated NETBUF memory */
#define A_NETBUF_TRIM(bufPtr, len) \
  a_netbuf_trim(bufPtr, len)
/* A_NETBUF_PULL - Removes #len bytes from the beginning valid/populated NETBUF memory */
#define A_NETBUF_PULL(bufPtr, len) \
  a_netbuf_pull(bufPtr, len)
/* A_NETBUF_HEADROOM - return the amount of space remaining to prepend content/headers */
#define A_NETBUF_HEADROOM(bufPtr) \
  a_netbuf_headroom(bufPtr)
/* A_NETBUF_TAILROOM - return the amount of space remaining to append content/trailers */
#define A_NETBUF_TAILROOM(bufPtr) \
  a_netbuf_tailroom(bufPtr)
/* A_NETBUF_GET_FRAGMENT - Get the indexed fragment from the netbuf */
#define A_NETBUF_GET_FRAGMENT(bufPtr, index, pLen) \
  a_netbuf_get_fragment((bufPtr), (index), (pLen))

/* A_NETBUF_APPEND_FRAGMENT - Add a fragment to the end of the netbuf fragment list */
#define A_NETBUF_APPEND_FRAGMENT(bufPtr, frag, len) \
  a_netbuf_append_fragment((bufPtr), (frag), (len))

/* A_NETBUF_PUT_DATA - Add data to end of a buffer  */
#define A_NETBUF_PUT_DATA(bufPtr, srcPtr, len) \
  a_netbuf_put_data(bufPtr, srcPtr, len)

/*
 * OS specific network buffer access routines
 */

A_VOID   a_netbuf_configure         (A_VOID     *buffptr,
                                     A_VOID     *buffer,
                                     A_UINT16    headroom,
                                     A_UINT16    length,
                                     A_UINT16    size);

A_VOID   a_netbuf_init              (A_VOID*     buffptr,
                                     A_VOID*     freefn,
                                     A_VOID*     priv);

A_VOID  *a_netbuf_alloc             (A_INT32     size);

A_VOID  *a_netbuf_alloc_raw         (A_INT32     size);

A_VOID   a_netbuf_free              (A_VOID     *bufPtr);

A_VOID  *a_netbuf_to_data           (A_VOID     *bufPtr);

A_UINT32 a_netbuf_to_len            (A_VOID     *bufPtr);

A_STATUS a_netbuf_push              (A_VOID     *bufPtr,
                                     A_INT32     len);

A_STATUS a_netbuf_push_data         (A_VOID     *bufPtr,
                                     A_UINT8    *srcPtr,
                                     A_INT32     len);

A_STATUS a_netbuf_put               (A_VOID     *bufPtr,
                                     A_INT32     len);

A_STATUS a_netbuf_put_data          (A_VOID     *bufPtr,
                                     A_UINT8    *srcPtr,
                                     A_INT32     len);

A_STATUS a_netbuf_pull              (A_VOID     *bufPtr,
                                     A_INT32     len);

A_VOID  *a_netbuf_get_fragment      (A_VOID     *bufPtr,
                                     A_UINT8     index,
                                     A_INT32    *pLen);

A_VOID   a_netbuf_append_fragment   (A_VOID     *bufPtr,
                                     A_UINT8    *frag,
                                     A_INT32     len);

A_STATUS a_netbuf_trim              (A_VOID     *bufPtr,
                                     A_INT32     len);

A_INT32  a_netbuf_headroom          (A_VOID     *bufPtr);

A_INT32  a_netbuf_tailroom          (A_VOID     *bufPtr);

#endif /* _CUST_NETBUF_H_ */
