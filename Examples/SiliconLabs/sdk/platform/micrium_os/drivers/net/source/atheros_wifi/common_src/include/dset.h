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
/**HEADER********************************************************************
 *
 * $FileName: dset.h$
 * $Version : 3.8.40.0$
 * $Date    : May-28-2014$
 *
 * Comments:
 *
 *   This file contains the defines, externs and data
 *   structure definitions required by application
 *   programs in order to use the Ethernet packet driver.
   //                                                              Copyright (c) Qualcomm Atheros, Inc.
   //                                                              All rights reserved.
   //                                                              Redistribution and use in source and binary forms, with or without modification, are permitted (subject to
   //                                                              the limitations in the disclaimer below) provided that the following conditions are met:
   //
   //                                                              · Redistributions of source code must retain the above copyright notice, this list of conditions and the
   //                                                                following disclaimer.
   //                                                              · Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the
   //                                                                following disclaimer in the documentation and/or other materials provided with the distribution.
   //                                                              · Neither the name of nor the names of its contributors may be used to endorse or promote products derived
   //                                                                from this software without specific prior written permission.
   //
   //                                                              NO EXPRESS OR IMPLIED LICENSES TO ANY PARTY'S PATENT RIGHTS ARE GRANTED BY THIS LICENSE. THIS SOFTWARE IS
   //                                                              PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING,
   //                                                              BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
   //                                                              DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
   //                                                              INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
   //                                                              SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
   //                                                              ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
   //                                                              ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 * END************************************************************************/
#ifndef __dset_h__
#define __dset_h__
#include <a_types.h>
/*--------------------------------------------------------------------------*/
/*
**                            CONSTANT DEFINITIONS
*/

#define     MAX_DSET_SIZE       16

/*--------------------------------------------------------------------------*/
/*
**                            TYPE DEFINITIONS
*/

typedef struct host_dset_struct {
  QOSAL_UINT32     dset_id;          /* dset id    */
  QOSAL_UINT32     length;           /* dset length            */
  QOSAL_UINT8     *data_ptr;         /* dset buffer address */
} HOST_DSET;

typedef struct host_dset_item {
  QOSAL_UINT32     dset_id;          /* dset id    */
  QOSAL_UINT32     length;           /* dset length            */
} HOST_DSET_ITEM;

/*--------------------------------------------------------------------------*/
/*
**                            PROTOTYPES AND GLOBAL EXTERNS
*/

HOST_DSET  *dset_find(QOSAL_UINT32 dset_id);
HOST_DSET  *dset_get(QOSAL_UINT32 dset_id, QOSAL_UINT32 length);
QOSAL_UINT32    dset_write(HOST_DSET *pDset, QOSAL_UINT8 *pData, QOSAL_UINT32 offset, QOSAL_UINT32 length);
QOSAL_UINT32    dset_read(HOST_DSET *pDset, QOSAL_UINT8 *pData, QOSAL_UINT32 offset, QOSAL_UINT32 length);
QOSAL_UINT32    dset_fill(QOSAL_UINT8 *pData, QOSAL_UINT32 max_dset_count);

HOST_DSET  *dset_get_first();
HOST_DSET  *dset_get_next();

#endif
