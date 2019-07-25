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

#ifndef _WMI_HOST_H_
#define _WMI_HOST_H_

#ifdef __cplusplus
extern "C" {
#endif

struct wmi_stats {
  QOSAL_UINT32    cmd_len_err;
  QOSAL_UINT32    cmd_id_err;
};

#define BEACON_PERIOD_IE_INDEX 8
#define CAPS_IE_INDEX 10
#define IE_INDEX 12

/*
 * These constants are used with A_WLAN_BAND_SET.
 */
#define A_BAND_24GHZ           0
#define A_BAND_5GHZ            1
#define A_NUM_BANDS            2

struct wmi_t {
  QOSAL_BOOL                          wmi_numQoSStream;
  QOSAL_UINT16                        wmi_streamExistsForAC[WMM_NUM_AC];
  QOSAL_UINT8                         wmi_fatPipeExists;
  void                           *wmi_devt;
  struct wmi_stats                wmi_stats;
  A_MUTEX_T                       wmi_lock;
  HTC_ENDPOINT_ID                wmi_endpoint_id;
  void                            *persistentProfile;
  QOSAL_UINT16                        deviceid;
#if ENABLE_P2P_MODE
  //                                                              WMI_P2P_PERSISTENT_PROFILE_CMD	persistentNode;
  WMI_SET_PASSPHRASE_CMD      apPassPhrase;
#endif
};

#define LOCK_WMI(w)     A_MUTEX_ACQUIRE(&(w)->wmi_lock);
#define UNLOCK_WMI(w)   A_MUTEX_RELEASE(&(w)->wmi_lock);

#ifdef __cplusplus
}
#endif

#endif /* _WMI_HOST_H_ */
