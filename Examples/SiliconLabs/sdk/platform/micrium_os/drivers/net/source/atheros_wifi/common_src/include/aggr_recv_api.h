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

#ifndef __AGGR_RECV_API_H__
#define __AGGR_RECV_API_H__

#ifdef __cplusplus
extern "C" {
#endif

typedef QOSAL_VOID (* RX_CALLBACK)(QOSAL_VOID * dev, QOSAL_VOID *osbuf);

typedef QOSAL_VOID (* ALLOC_NETBUFS)(A_NETBUF_QUEUE_T *q, QOSAL_UINT16 num);

/*
 * aggr_init:
 * Initialises the data structures, allocates data queues and
 * os buffers. Netbuf allocator is the input param, used by the
 * aggr module for allocation of NETBUFs from driver context.
 * These NETBUFs are used for AMSDU processing.
 * Returns the context for the aggr module.
 */
QOSAL_VOID *
  aggr_init(QOSAL_VOID);

/*
 * aggr_deinit:
 * Frees any allocated resources allocated by aggr_init.
 */
QOSAL_VOID
aggr_deinit(QOSAL_VOID* cntxt);

/*
 * aggr_recv_addba_req_evt:
 * This event is to initiate/modify the receive side window.
 * Target will send WMI_ADDBA_REQ_EVENTID event to host - to setup
 * recv re-ordering queues. Target will negotiate ADDBA with peer,
 * and indicate via this event after succesfully completing the
 * negotiation. This happens in two situations:
 *  1. Initial setup of aggregation
 *  2. Renegotiation of current recv window.
 * Window size for re-ordering is limited by target buffer
 * space, which is reflected in win_sz.
 * (Re)Start the periodic timer to deliver long standing frames,
 * in hold_q to OS.
 */
QOSAL_VOID
aggr_recv_addba_req_evt(QOSAL_VOID * cntxt, QOSAL_UINT8 tid, QOSAL_UINT16 seq_no, QOSAL_UINT8 win_sz);

/*
 * aggr_recv_delba_req_evt:
 * Target indicates deletion of a BA window for a tid via the
 * WMI_DELBA_EVENTID. Host would deliver all the frames in the
 * hold_q, reset tid config and disable the periodic timer, if
 * aggr is not enabled on any tid.
 */
QOSAL_VOID
aggr_recv_delba_req_evt(QOSAL_VOID * cntxt, QOSAL_UINT8 tid);

/*
 * aggr_process_recv_frm:
 * Called only for data frames. When aggr is ON for a tid, the buffer
 * is always consumed, and osbuf would be NULL. For a non-aggr case,
 * osbuf is not modified.
 * AMSDU frames are consumed and are later freed. They are sliced and
 * diced to individual frames and dispatched to stack.
 * After consuming a osbuf(when aggr is ON), a previously registered
 * callback may be called to deliver frames in order.
 */
QOSAL_BOOL
aggr_process_recv_frm(QOSAL_VOID *cntxt, QOSAL_UINT8 tid, QOSAL_UINT16 seq_no, QOSAL_BOOL is_amsdu, QOSAL_VOID **osbuf);

/*
 * aggr_module_destroy:
 * Frees up all the queues and frames in them. Releases the cntxt to OS.
 */
QOSAL_VOID
aggr_module_destroy(QOSAL_VOID *cntxt);

/*
 * aggr_reset_state -- Called when it is deemed necessary to clear the aggregate
 *  hold Q state.  Examples include when a Connect event or disconnect event is
 *  received.
 */
QOSAL_VOID
aggr_reset_state(QOSAL_VOID *cntxt);

#ifdef __cplusplus
}
#endif

#endif /*__AGGR_RECV_API_H__ */
