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
#include <a_config.h>
#include <a_types.h>
#include <a_osapi.h>
#include <common_api.h>
#include "../hcd/hcd_api.h"
#include <spi_hcd_if.h>
#include <htc_api.h>
#include <bmi_msg.h>
#include "AR6002/hw2.0/hw/mbox_host_reg.h"
//                                                                #include "bmi_internal.h"
#include "atheros_wifi_api.h"
#include "bmi.h"

#define BMI_ENDPOINT 0

#define BMI_COMMUNICATION_TIMEOUT       100000

/*
   Although we had envisioned BMI to run on top of HTC, this is not how the
   final implementation ended up. On the Target side, BMI is a part of the BSP
   and does not use the HTC protocol nor even DMA -- it is intentionally kept
   very simple.
 */

#if DRIVER_CONFIG_INCLUDE_BMI

static QOSAL_UINT8 bmiDone = QOSAL_TRUE;
static QOSAL_UINT32 *pBMICmdCredits;
static QOSAL_UCHAR *pBMICmdBuf;
#define MAX_BMI_CMDBUF_SZ (BMI_DATASZ_MAX                    \
                           + sizeof(QOSAL_UINT32) /* cmd */  \
                           + sizeof(QOSAL_UINT32) /* addr */ \
                           + sizeof(QOSAL_UINT32))/* length */
#define BMI_COMMAND_FITS(sz) ((sz) <= MAX_BMI_CMDBUF_SZ)

static A_STATUS
bmiBufferSend(QOSAL_VOID *pCxt,
              QOSAL_UCHAR *buffer,
              QOSAL_UINT32 length)
{
  A_NETBUF_DECLARE req;
  QOSAL_VOID *pReq = (QOSAL_VOID*)&req;
  A_STATUS status;
  QOSAL_UINT32 timeout;
  QOSAL_UINT32 address;
  //                                                              A_DRIVER_CONTEXT *pDCxt = GET_DRIVER_COMMON(pCxt);

  A_NETBUF_CONFIGURE(pReq, pBMICmdCredits, 0, sizeof(QOSAL_UINT32), sizeof(QOSAL_UINT32));

  *pBMICmdCredits = 0;
  timeout = BMI_COMMUNICATION_TIMEOUT;

  while (timeout-- && !(*pBMICmdCredits)) {
    /* Read the counter register to get the command credits */
    address = COUNT_DEC_ADDRESS + (HTC_MAILBOX_NUM_MAX + BMI_ENDPOINT) * 4;
    ATH_SET_PIO_EXTERNAL_READ_OPERATION(pReq, address, QOSAL_TRUE, sizeof(QOSAL_UINT32));
    /* hit the credit counter with a 4-byte access, the first byte read will hit the counter and cause
     * a decrement, while the remaining 3 bytes has no effect.  The rationale behind this is to
     * make all HIF accesses 4-byte aligned */
    if (A_OK != (status = Hcd_DoPioExternalAccess(pCxt, pReq))) {
      break;
    }

    /* the counter is only 8=bits, ignore anything in the upper 3 bytes */

    *pBMICmdCredits = A_LE2CPU32(*pBMICmdCredits);

    (*pBMICmdCredits) &= 0xFF;
  }

  if (*pBMICmdCredits) {
    A_NETBUF_CONFIGURE(pReq, buffer, 0, length, length);
    address =  HW_GetMboxAddress(pCxt, BMI_ENDPOINT, length);
    address &= ATH_TRANS_ADDR_MASK;
    A_NETBUF_SET_ELEM(pReq, A_REQ_ADDRESS, address);
    A_NETBUF_SET_ELEM(pReq, A_REQ_TRANSFER_LEN, length);
    /* init the packet read params/cmd incremental vs fixed address etc. */
    A_NETBUF_SET_ELEM(pReq, A_REQ_COMMAND, (ATH_TRANS_WRITE | ATH_TRANS_DMA));
    status = Hcd_Request(pCxt, pReq);
    if (status != A_OK) {
      return A_ERROR;
    }
  } else {
    return A_ERROR;
  }

  return status;
}

static A_STATUS
bmiBufferReceive(QOSAL_VOID *pCxt,
                 QOSAL_UCHAR *buffer,
                 QOSAL_UINT32 length,
                 QOSAL_BOOL want_timeout)
{
  A_STATUS status;
  QOSAL_UINT32 address;
  QOSAL_UINT32 availableRecvBytes;
  A_NETBUF_DECLARE req;
  QOSAL_VOID *pReq = (QOSAL_VOID*)&req;
  //                                                              A_DRIVER_CONTEXT *pDCxt = GET_DRIVER_COMMON(pCxt);

  if (length >= 4) {   /* NB: Currently, always true */
    /*
     * NB: word_available is declared static for esoteric reasons
     * having to do with protection on some OSes.
     */
    static QOSAL_UINT32 word_available;
    QOSAL_UINT32 timeout;

    word_available = 0;
    timeout = BMI_COMMUNICATION_TIMEOUT;
    while ((!want_timeout || timeout--) && !word_available) {
      status = Hcd_DoPioInternalAccess(pCxt, ATH_SPI_RDBUF_BYTE_AVA_REG, &availableRecvBytes, QOSAL_TRUE);
      if (status != A_OK) {
        break;
      }

      if (availableRecvBytes >= sizeof(QOSAL_UINT32)) {
        word_available = 1;
      }
    }

    if (!word_available) {
      return A_ERROR;
    }
  }

  A_NETBUF_CONFIGURE(pReq, buffer, 0, length, length);
  address =  HW_GetMboxAddress(pCxt, BMI_ENDPOINT, length);
  address &= ATH_TRANS_ADDR_MASK;
  A_NETBUF_SET_ELEM(pReq, A_REQ_ADDRESS, address);
  A_NETBUF_SET_ELEM(pReq, A_REQ_TRANSFER_LEN, length);
  /* init the packet read params/cmd incremental vs fixed address etc. */
  A_NETBUF_SET_ELEM(pReq, A_REQ_COMMAND, (ATH_TRANS_READ | ATH_TRANS_DMA));
  status = Hcd_Request(pCxt, pReq);
  if (status != A_OK) {
    return A_ERROR;
  }

  return A_OK;
}

/* APIs visible to the driver */
A_STATUS
BMIInit(QOSAL_VOID *pCxt)
{
  bmiDone = QOSAL_FALSE;

  /*
   * On some platforms, it's not possible to DMA to a static variable
   * in a device driver (e.g. Linux loadable driver module).
   * So we need to A_MALLOC space for "command credits" and for commands.
   *
   * Note: implicitly relies on A_MALLOC to provide a buffer that is
   * suitable for DMA (or PIO).  This buffer will be passed down the
   * bus stack.
   */
  if (!pBMICmdCredits) {
    pBMICmdCredits = (QOSAL_UINT32 *)A_MALLOC(4, MALLOC_ID_TEMPORARY);
    A_ASSERT(pBMICmdCredits);
  }

  if (!pBMICmdBuf) {
    pBMICmdBuf = (QOSAL_UCHAR *)A_MALLOC(MAX_BMI_CMDBUF_SZ, MALLOC_ID_TEMPORARY);
    A_ASSERT(pBMICmdBuf);
  }

  return A_OK;
}

void
BMICleanup(void)
{
  if (pBMICmdCredits) {
    A_FREE(pBMICmdCredits, MALLOC_ID_TEMPORARY);
    pBMICmdCredits = NULL;
  }

  if (pBMICmdBuf) {
    A_FREE(pBMICmdBuf, MALLOC_ID_TEMPORARY);
    pBMICmdBuf = NULL;
  }
}

A_STATUS
BMIDone(QOSAL_VOID *pCxt)
{
  A_STATUS status;
  QOSAL_UINT32 cid;

  if (bmiDone) {
    return A_OK;
  }

  bmiDone = QOSAL_TRUE;
  cid = A_CPU2LE32(BMI_DONE);

  status = bmiBufferSend(pCxt, (QOSAL_UCHAR *)&cid, sizeof(cid));
  if (status != A_OK) {
    return A_ERROR;
  }

  if (pBMICmdCredits) {
    A_FREE(pBMICmdCredits, MALLOC_ID_TEMPORARY);
    pBMICmdCredits = NULL;
  }

  if (pBMICmdBuf) {
    A_FREE(pBMICmdBuf, MALLOC_ID_TEMPORARY);
    pBMICmdBuf = NULL;
  }

  return A_OK;
}

A_STATUS
BMIGetTargetInfo(QOSAL_VOID *pCxt, struct bmi_target_info *targ_info)
{
  QOSAL_UINT32 cid;

  if (bmiDone) {
    return A_ERROR;
  }

  cid = A_CPU2LE32(BMI_GET_TARGET_INFO);

  if (A_OK != bmiBufferSend(pCxt, (QOSAL_UCHAR *)&cid, sizeof(cid))) {
    return A_ERROR;
  }

  if (A_OK != bmiBufferReceive(pCxt, (QOSAL_UCHAR *)&targ_info->target_ver,
                               sizeof(targ_info->target_ver), QOSAL_TRUE)) {
    return A_ERROR;
  }

  targ_info->target_ver = A_LE2CPU32(targ_info->target_ver);

  if (targ_info->target_ver == TARGET_VERSION_SENTINAL) {
    /* Determine how many bytes are in the Target's targ_info */
    if (A_OK != bmiBufferReceive(pCxt, (QOSAL_UCHAR *)&targ_info->target_info_byte_count,
                                 sizeof(targ_info->target_info_byte_count), QOSAL_TRUE)) {
      return A_ERROR;
    }

    targ_info->target_info_byte_count = A_LE2CPU32(targ_info->target_info_byte_count);

    /*
     * The Target's targ_info doesn't match the Host's targ_info.
     * We need to do some backwards compatibility work to make this OK.
     */
    A_ASSERT(targ_info->target_info_byte_count == sizeof(*targ_info));

    /* Read the remainder of the targ_info */
    if (A_OK != bmiBufferReceive(pCxt,
                                 ((QOSAL_UCHAR *)targ_info) + sizeof(targ_info->target_info_byte_count),
                                 sizeof(*targ_info) - sizeof(targ_info->target_info_byte_count), QOSAL_TRUE)) {
      return A_ERROR;
    }

    targ_info->target_ver = A_LE2CPU32(targ_info->target_ver);
    targ_info->target_type = A_LE2CPU32(targ_info->target_type);
  } else {
    return A_ERROR;
  }

  return A_OK;
}

#if 1
A_STATUS
BMIReadMemory(QOSAL_VOID *pCxt,
              QOSAL_UINT32 address,
              QOSAL_UCHAR *buffer,
              QOSAL_UINT32 length)
{
  QOSAL_UINT32 cid;
  A_STATUS status;
  QOSAL_UINT32 offset;
  QOSAL_UINT32 remaining, rxlen, temp;

  A_ASSERT(BMI_COMMAND_FITS(BMI_DATASZ_MAX + sizeof(cid) + sizeof(address) + sizeof(length)));
  memset(pBMICmdBuf, 0, BMI_DATASZ_MAX + sizeof(cid) + sizeof(address) + sizeof(length));

  if (bmiDone) {
    return A_ERROR;
  }

  cid = A_CPU2LE32(BMI_READ_MEMORY);

  remaining = length;

  while (remaining) {
    //                                                            rxlen = (remaining < BMI_DATASZ_MAX) ? remaining : BMI_DATASZ_MAX;
    rxlen = (remaining < 4) ? remaining : 4;
    offset = 0;
    A_MEMCPY(&(pBMICmdBuf[offset]), &cid, sizeof(cid));
    offset += sizeof(cid);
    temp = A_CPU2LE32(address);
    A_MEMCPY(&(pBMICmdBuf[offset]), &temp, sizeof(address));
    offset += sizeof(address);
    temp = A_CPU2LE32(rxlen);
    A_MEMCPY(&(pBMICmdBuf[offset]), &temp, sizeof(rxlen));
    offset += sizeof(length);

    status = bmiBufferSend(pCxt, pBMICmdBuf, offset);
    if (status != A_OK) {
      return A_ERROR;
    }
    status = bmiBufferReceive(pCxt, pBMICmdBuf, rxlen, QOSAL_TRUE);
    if (status != A_OK) {
      return A_ERROR;
    }
    A_MEMCPY(&buffer[length - remaining], pBMICmdBuf, rxlen);
    remaining -= rxlen; address += rxlen;
  }

  return A_OK;
}
#endif

A_STATUS
BMIWriteMemory(QOSAL_VOID *pCxt,
               QOSAL_UINT32 address,
               QOSAL_UCHAR *buffer,
               QOSAL_UINT32 length)
{
  QOSAL_UINT32 cid;
  QOSAL_UINT32 remaining, txlen, temp;
  const QOSAL_UINT32 header = sizeof(cid) + sizeof(address) + sizeof(length);
  //                                                              QOSAL_UCHAR alignedBuffer[BMI_DATASZ_MAX];
  QOSAL_UCHAR *src;
  QOSAL_UINT8 *ptr;

  A_ASSERT(BMI_COMMAND_FITS(BMI_DATASZ_MAX + header));
  memset(pBMICmdBuf, 0, BMI_DATASZ_MAX + header);

  if (bmiDone) {
    return A_ERROR;
  }

  cid = A_CPU2LE32(BMI_WRITE_MEMORY);

  remaining = length;
  while (remaining) {
    src = &buffer[length - remaining];
    if (remaining < (BMI_DATASZ_MAX - header)) {
      if (remaining & 3) {
        /* align it with 4 bytes */
        remaining = remaining + (4 - (remaining & 3));
        //                                                        memcpy(alignedBuffer, src, remaining);
        //                                                        src = alignedBuffer;
      }
      txlen = remaining;
    } else {
      txlen = (BMI_DATASZ_MAX - header);
    }

    ptr = pBMICmdBuf;
    A_MEMCPY(ptr, &cid, sizeof(cid));
    ptr += sizeof(cid);
    temp = A_CPU2LE32(address);
    A_MEMCPY(ptr, &temp, sizeof(address));
    ptr += sizeof(address);
    temp = A_CPU2LE32(txlen);
    A_MEMCPY(ptr, &temp, sizeof(txlen));
    ptr += sizeof(txlen);
    A_MEMCPY(ptr, src, txlen);
    ptr += txlen;

    if (A_OK != bmiBufferSend(pCxt, pBMICmdBuf, (QOSAL_UINT32)(ptr - pBMICmdBuf))) {
      return A_ERROR;
    }
    remaining -= txlen; address += txlen;
  }

  return A_OK;
}

A_STATUS
BMIExecute(QOSAL_VOID *pCxt,
           QOSAL_UINT32 address,
           QOSAL_UINT32 *param)
{
  QOSAL_UINT32 cid;
  QOSAL_UINT32 temp;
  QOSAL_UINT8 *ptr;

  A_ASSERT(BMI_COMMAND_FITS(sizeof(cid) + sizeof(address) + sizeof(param)));
  memset(pBMICmdBuf, 0, sizeof(cid) + sizeof(address) + sizeof(param));

  if (bmiDone) {
    return A_ERROR;
  }

  cid = A_CPU2LE32(BMI_EXECUTE);

  ptr = pBMICmdBuf;
  A_MEMCPY(ptr, &cid, sizeof(cid));
  ptr += sizeof(cid);
  temp = A_CPU2LE32(address);
  A_MEMCPY(ptr, &temp, sizeof(address));
  ptr += sizeof(address);
  A_MEMCPY(ptr, param, sizeof(*param));
  ptr += sizeof(*param);

  if (A_OK != bmiBufferSend(pCxt, pBMICmdBuf, (QOSAL_UINT32)(ptr - pBMICmdBuf))) {
    return A_ERROR;
  }

  if (A_OK != bmiBufferReceive(pCxt, pBMICmdBuf, sizeof(*param), QOSAL_FALSE)) {
    return A_ERROR;
  }

  A_MEMCPY(param, pBMICmdBuf, sizeof(*param));

  return A_OK;
}

A_STATUS
BMIReadSOCRegister(QOSAL_VOID *pCxt,
                   QOSAL_UINT32 address,
                   QOSAL_UINT32 *param)
{
  QOSAL_UINT32 cid;
  A_STATUS status;
  QOSAL_UINT32 offset, temp;

  A_ASSERT(BMI_COMMAND_FITS(sizeof(cid) + sizeof(address)));
  memset(pBMICmdBuf, 0, sizeof(cid) + sizeof(address));

  if (bmiDone) {
    return A_ERROR;
  }

  cid = A_CPU2LE32(BMI_READ_SOC_REGISTER);

  offset = 0;
  A_MEMCPY(&(pBMICmdBuf[offset]), &cid, sizeof(cid));
  offset += sizeof(cid);
  temp = A_CPU2LE32(address);
  A_MEMCPY(&(pBMICmdBuf[offset]), &temp, sizeof(address));
  offset += sizeof(address);

  status = bmiBufferSend(pCxt, pBMICmdBuf, offset);
  if (status != A_OK) {
    return A_ERROR;
  }

  status = bmiBufferReceive(pCxt, pBMICmdBuf, sizeof(*param), QOSAL_TRUE);
  if (status != A_OK) {
    return A_ERROR;
  }
  A_MEMCPY(param, pBMICmdBuf, sizeof(*param));

  return A_OK;
}

A_STATUS
BMIWriteSOCRegister(QOSAL_VOID *pCxt,
                    QOSAL_UINT32 address,
                    QOSAL_UINT32 param)
{
  QOSAL_UINT32 cid;
  A_STATUS status;
  QOSAL_UINT32 offset, temp;

  A_ASSERT(BMI_COMMAND_FITS(sizeof(cid) + sizeof(address) + sizeof(param)));
  memset(pBMICmdBuf, 0, sizeof(cid) + sizeof(address) + sizeof(param));

  if (bmiDone) {
    return A_ERROR;
  }

  cid = A_CPU2LE32(BMI_WRITE_SOC_REGISTER);

  offset = 0;
  A_MEMCPY(&(pBMICmdBuf[offset]), &cid, sizeof(cid));
  offset += sizeof(cid);
  temp = A_CPU2LE32(address);
  A_MEMCPY(&(pBMICmdBuf[offset]), &temp, sizeof(address));
  offset += sizeof(address);
  A_MEMCPY(&(pBMICmdBuf[offset]), &param, sizeof(param));
  offset += sizeof(param);

  status = bmiBufferSend(pCxt, pBMICmdBuf, offset);

  if (status != A_OK) {
    return A_ERROR;
  }

  return A_OK;
}

A_STATUS
BMIRawWrite(QOSAL_VOID *pCxt, QOSAL_UCHAR *buffer, QOSAL_UINT32 length)
{
  return bmiBufferSend(pCxt, buffer, length);
}

A_STATUS
BMIRawRead(QOSAL_VOID *pCxt, QOSAL_UCHAR *buffer, QOSAL_UINT32 length, QOSAL_BOOL want_timeout)
{
  return bmiBufferReceive(pCxt, buffer, length, want_timeout);
}

#else

/* STUB'd version when BMI is not used by the driver */

/* APIs visible to the driver */
A_STATUS
BMIInit(QOSAL_VOID *pCxt)
{
  return A_ERROR;
}

A_STATUS
BMIDone(QOSAL_VOID *pCxt)
{
  return A_ERROR;
}

A_STATUS
BMIGetTargetInfo(QOSAL_VOID *pCxt, struct bmi_target_info *targ_info)
{
  return A_ERROR;
}

A_STATUS
BMIWriteMemory(QOSAL_VOID *pCxt,
               QOSAL_UINT32 address,
               QOSAL_UCHAR *buffer,
               QOSAL_UINT32 length)
{
  return A_ERROR;
}

A_STATUS
BMIExecute(QOSAL_VOID *pCxt,
           QOSAL_UINT32 address,
           QOSAL_UINT32 *param)
{
  return A_ERROR;
}

A_STATUS
BMIReadSOCRegister(QOSAL_VOID *pCxt,
                   QOSAL_UINT32 address,
                   QOSAL_UINT32 *param)
{
  return A_ERROR;
}

A_STATUS
BMIWriteSOCRegister(QOSAL_VOID *pCxt,
                    QOSAL_UINT32 address,
                    QOSAL_UINT32 param)
{
  return A_ERROR;
}

A_STATUS
BMIRawWrite(QOSAL_VOID *pCxt, QOSAL_UCHAR *buffer, QOSAL_UINT32 length)
{
  return A_ERROR;
}

A_STATUS
BMIRawRead(QOSAL_VOID *pCxt, QOSAL_UCHAR *buffer, QOSAL_UINT32 length, QOSAL_BOOL want_timeout)
{
  return A_ERROR;
}

#endif /* DRIVER_CONFIG_INCLUDE_BMI */
