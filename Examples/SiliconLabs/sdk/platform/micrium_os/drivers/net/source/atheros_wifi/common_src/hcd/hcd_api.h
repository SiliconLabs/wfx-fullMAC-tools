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
#ifndef _HCD_API_H_
#define _HCD_API_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <a_config.h>
#include <a_types.h>
#include <a_osapi.h>
#include <htc_api.h>
#include <driver_cxt.h>

A_STATUS
Hcd_Request(QOSAL_VOID *pCxt, QOSAL_VOID *pReq);
QOSAL_BOOL
Hcd_ReadCPUInterrupt(QOSAL_VOID *pCxt, QOSAL_UINT8 *cpuIntrCause);
QOSAL_VOID
Hcd_ClearCPUInterrupt(QOSAL_VOID *pCxt);
A_STATUS
Hcd_UnmaskInterrupts(QOSAL_VOID *pCxt, QOSAL_UINT16 Mask);//IGX_UD_CHANGES
A_STATUS
Hcd_UnmaskInterrupts(QOSAL_VOID *pCxt, QOSAL_UINT16 Mask);
A_STATUS
Hcd_GetLookAhead(QOSAL_VOID *pCxt);
QOSAL_BOOL
Hcd_BusInterrupt(QOSAL_VOID *pCxt);
A_STATUS
Hcd_ReinitTarget(QOSAL_VOID *pCxt);
QOSAL_VOID
Hcd_Deinitialize(QOSAL_VOID *pCxt);
A_STATUS
Hcd_Init(QOSAL_VOID *pCxt);
QOSAL_VOID
Hcd_EnableCPUInterrupt(QOSAL_VOID *pCxt);
A_STATUS
Hcd_DoPioExternalAccess(QOSAL_VOID *pCxt, QOSAL_VOID *pReq);
QOSAL_VOID
Hcd_MaskInterrupts(QOSAL_VOID *pCxt, QOSAL_UINT16 Mask);
A_STATUS
Hcd_ReinitTarget(QOSAL_VOID *pCxt);
A_STATUS
Hcd_UnmaskInterrupts(QOSAL_VOID *pCxt, QOSAL_UINT16 Mask);
A_STATUS
Bus_InOutDescriptorSet(QOSAL_VOID *pCxt, QOSAL_VOID *pReq);

#if 0
/*
 * The below 3-functions are need to implement and
 * HCD_SUSPEND_CB and HCD_RESUME_CB structures are not avialable with
 * current driver source
 */

QOSAL_VOID Hcd_AckInterrupt(QOSAL_VOID *pCxt, QOSAL_UINT16 Mask);
QOSAL_VOID Hcd_RegisterSuspendHandler(HCD_SUSPEND_CB *suspendCallback, QOSAL_VOID *pSuspContext);
QOSAL_VOID Hcd_RegisterResumeHandler(HCD_RESUME_CB *resumeCallback, QOSAL_VOID *pSuspContext);
QOSAL_VOID
Hcd_RegisterInterruptHandler(HCD_INTR_CALLBACK_T *callback, void *pContext);
#endif  //#if 0

#endif          //_HCD_API_H_
