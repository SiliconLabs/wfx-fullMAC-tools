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
#ifndef _CUSTOM_SPI_API_H_
#define _CUSTOM_SPI_API_H_

QOSAL_VOID
Custom_HW_EnableDisableSPIIRQ(QOSAL_VOID *pCxt, QOSAL_BOOL enable);
A_STATUS
Custom_Bus_InOutBuffer(QOSAL_VOID *pCxt,
                       QOSAL_UINT8 *pBuffer,
                       QOSAL_UINT16 length,
                       QOSAL_UINT8 doRead,
                       QOSAL_BOOL sync);
A_STATUS
Custom_Bus_StartTransfer(QOSAL_VOID *pCxt, QOSAL_BOOL sync);
A_STATUS
Custom_Bus_CompleteTransfer(QOSAL_VOID *pCxt, QOSAL_BOOL sync);

QOSAL_VOID
Custom_HW_SetClock(QOSAL_VOID *pCxt, QOSAL_UINT32 *pClockRate);
A_STATUS
Custom_Bus_InOutToken(QOSAL_VOID *pCxt,
                      QOSAL_UINT32        OutToken,
                      QOSAL_UINT8         DataSize,
                      QOSAL_UINT32    *pInToken);

QOSAL_VOID Custom_Hcd_EnableDisableSPIIRQ(QOSAL_VOID *pCxt, QOSAL_BOOL enable);

#define CUSTOM_HCD_ENABLEDISABLESPIIRQ(pCxt, enable) Custom_Hcd_EnableDisableSPIIRQ((pCxt), (enable))
#define CUSTOM_BUS_INOUT_BUFFER(pCxt, pBuffer, length, doRead, sync) Custom_Bus_InOutBuffer((pCxt), (pBuffer), (length), (doRead), (sync))
#define CUSTOM_BUS_START_TRANSFER(pCxt, sync) Custom_Bus_StartTransfer((pCxt), (sync))
#define CUSTOM_BUS_COMPLETE_TRANSFER(pCxt, sync) Custom_Bus_CompleteTransfer((pCxt), (sync))

#define CUSTOM_BUS_INOUT_TOKEN(pCxt, oT, dS, pInT) Custom_Bus_InOutToken((pCxt), (oT), (dS), (pInT))

#endif  //_CUSTOM_SPI_API_H_
