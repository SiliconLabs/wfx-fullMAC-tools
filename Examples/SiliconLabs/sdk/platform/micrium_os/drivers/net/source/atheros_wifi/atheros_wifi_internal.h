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
#ifndef __ATHEROS_WIFI_INTERNAL_H__
#define __ATHEROS_WIFI_INTERNAL_H__

#define ATH_PROG_FEEDBACK_BLANK_FLASH   (0x00000001)
#define ATH_PROG_FEEDBACK_LOADING       (0x00000002)
#define ATH_PROG_FEEDBACK_EXECUTING     (0x00000003)
#define ATH_PROG_FEEDBACK_DONE          (0x00000004)
#define ATH_PROG_FEEDBACK_LOAD_FAIL     (0x00000005)
#define ATH_PROG_FEEDBACK_COMM_ERROR    (0x00000006)

typedef struct {
  A_STATUS  (*Driver_TargetConfig)(QOSAL_VOID *);
  A_STATUS  (*Driver_BootComm)(QOSAL_VOID *);
  A_STATUS  (*Driver_BMIConfig)(QOSAL_VOID *);
  QOSAL_VOID      (*Api_TxComplete)(QOSAL_VOID*, QOSAL_VOID*);
  QOSAL_VOID      (*Api_RxComplete)(QOSAL_VOID*, QOSAL_VOID*);
  QOSAL_VOID    (*Boot_Profile)(QOSAL_UINT32 val);
  QOSAL_VOID      (*Custom_Delay)(QOSAL_UINT32 delay);
  QOSAL_VOID      (*Api_GpioDataEventRx)(QOSAL_UINT8 *datap, QOSAL_UINT32 len);
  QOSAL_VOID          (*Custom_reset_measure_timer)(QOSAL_VOID);
  QOSAL_VOID      (*Custom_Api_PfmDataEventRx)(QOSAL_UINT8 *datap, QOSAL_UINT32 len);
  QOSAL_VOID      (*Custom_Api_PfmDataDoneEventRx)(QOSAL_VOID *, QOSAL_UINT8 *datap, QOSAL_UINT32 len);
  A_STATUS    (*Driver_StoreRecallFirmwareDownload)(QOSAL_VOID *pCxt);
  QOSAL_BOOL    skipWmi;
  QOSAL_BOOL    exitAtBmi;
}ATH_CUSTOM_INIT_T;

extern ATH_CUSTOM_INIT_T ath_custom_init;

typedef struct {
  QOSAL_UINT32 (*ath_ioctl_handler_ext)(QOSAL_VOID*, ATH_IOCTL_PARAM_STRUCT_PTR);
}ATH_CUSTOM_MEDIACTL_T;

extern ATH_CUSTOM_MEDIACTL_T ath_custom_mediactl;

typedef struct {
  A_STATUS (*HTCConnectServiceExch)(QOSAL_VOID*, HTC_SERVICE_CONNECT_REQ*, HTC_SERVICE_CONNECT_RESP*,
                                    HTC_ENDPOINT_ID*, QOSAL_UINT32*);
  A_STATUS (*HTCSendSetupComplete)(QOSAL_VOID*);
  A_STATUS (*HTCGetReady)(QOSAL_VOID*);
}ATH_CUSTOM_HTC_T;

extern ATH_CUSTOM_HTC_T ath_custom_htc;

#endif /* __ATHEROS_WIFI_INTERNAL_H__ */
