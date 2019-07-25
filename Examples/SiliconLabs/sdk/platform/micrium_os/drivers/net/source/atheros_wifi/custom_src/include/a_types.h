//                                                                ------------------------------------------------------------------------------
//                                                                 Copyright (c) Qualcomm Atheros, Inc.
//                                                                 All rights reserved.
//                                                                 Redistribution and use in source and binary forms, with or without modification, are permitted (subject to
//                                                                 the limitations in the disclaimer below) provided that the following conditions are met:
//
//                                                                 � Redistributions of source code must retain the above copyright notice, this list of conditions and the
//                                                                   following disclaimer.
//                                                                 � Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the
//                                                                   following disclaimer in the documentation and/or other materials provided with the distribution.
//                                                                 � Neither the name of nor the names of its contributors may be used to endorse or promote products derived
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

#ifndef _A_TYPES_H_
#define _A_TYPES_H_

#include  <cpu/include/cpu.h>

typedef CPU_INT08S      QOSAL_INT8;
typedef CPU_INT16S      QOSAL_INT16;
typedef CPU_INT32S      QOSAL_INT32;

typedef CPU_INT08U      QOSAL_UINT8;
typedef CPU_INT16U      QOSAL_UINT16;
typedef CPU_INT32U      QOSAL_UINT32;

typedef CPU_INT32U      QOSAL_ULONG;
typedef CPU_INT32S      QOSAL_LONG;

/* NOTE: QOSAL_BOOL is a type that is used in various WMI commands and events.
 * as such it is a type that is shared between the wifi chipset and the host.
 * It is required for that reason that QOSAL_BOOL be treated as a 32-bit/4-byte type.
 */
typedef CPU_INT32S       QOSAL_BOOL;
typedef CPU_CHAR         QOSAL_CHAR;
typedef CPU_INT08U       QOSAL_UCHAR;

#define QOSAL_VOID              void
#define QOSAL_CONST            const

#define QOSAL_TRUE            ((QOSAL_BOOL)1)
#define QOSAL_FALSE           ((QOSAL_BOOL)0)

typedef QOSAL_VOID *         pointer;

#endif /* _A_TYPES_H_ */
