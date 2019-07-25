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
#ifndef _BMI_H_
#define _BMI_H_

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* Header files */
#include "a_config.h"
#include "athdefs.h"
#include "a_types.h"
//                                                                #include "hif.h"
#include "a_osapi.h"
#include "bmi_msg.h"

A_STATUS
BMIInit(QOSAL_VOID *pCxt);

void
  BMICleanup(QOSAL_VOID);

A_STATUS
BMIDone(QOSAL_VOID *pCxt);

A_STATUS
BMIGetTargetInfo(QOSAL_VOID *pCxt, struct bmi_target_info *targ_info);

A_STATUS
BMIReadMemory(QOSAL_VOID *pCxt,
              QOSAL_UINT32 address,
              QOSAL_UCHAR *buffer,
              QOSAL_UINT32 length);

A_STATUS
BMIWriteMemory(QOSAL_VOID *pCxt,
               QOSAL_UINT32 address,
               QOSAL_UCHAR *buffer,
               QOSAL_UINT32 length);

A_STATUS
BMIExecute(QOSAL_VOID *pCxt,
           QOSAL_UINT32 address,
           QOSAL_UINT32 *param);

A_STATUS
BMIReadSOCRegister(QOSAL_VOID *pCxt,
                   QOSAL_UINT32 address,
                   QOSAL_UINT32 *param);

A_STATUS
BMIWriteSOCRegister(QOSAL_VOID *pCxt,
                    QOSAL_UINT32 address,
                    QOSAL_UINT32 param);

A_STATUS
BMIRawWrite(QOSAL_VOID *pCxt,
            QOSAL_UCHAR *buffer,
            QOSAL_UINT32 length);

A_STATUS
BMIRawRead(QOSAL_VOID *pCxt,
           QOSAL_UCHAR *buffer,
           QOSAL_UINT32 length,
           QOSAL_BOOL want_timeout);

#ifdef __cplusplus
}
#endif

#endif /* _BMI_H_ */
