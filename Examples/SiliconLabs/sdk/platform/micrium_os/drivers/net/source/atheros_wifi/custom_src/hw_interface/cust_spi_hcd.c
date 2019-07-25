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
//                                                                 THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE
//                                                                 INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
//                                                                 ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF
//                                                                 USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
//                                                                 OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
//                                                                ------------------------------------------------------------------------------
//                                                                ==============================================================================
//                                                                 Author(s): ="Atheros"
//                                                                ==============================================================================
#include <a_config.h>
#include <a_types.h>
#include <a_osapi.h>
#include <custom_spi_api.h>
#include <common_api.h>
#include <atheros_wifi.h>
#include  <io/include/spi_slave.h>

#define POWER_UP_DELAY (1)

#define HW_SPI_CAPS (HW_SPI_FRAME_WIDTH_8 \
                     | HW_SPI_NO_DMA      \
                     | HW_SPI_INT_EDGE_DETECT)

#define NET_DEV_SPI_XFER_TIMEOUT_MS                    5000

QOSAL_VOID Custom_Hcd_EnableDisableSPIIRQ(QOSAL_VOID *pCxt,
                                          QOSAL_BOOL  enable)
{
  NET_IF            *p_if;
  NET_DEV_BSP_WIFI  *p_dev_bsp;
  RTOS_ERR           local_err;

  /* -------- OBTAIN REFERENCE TO CFGs/REGs/BSP --------- */
  p_if       = (NET_IF           *)pCxt;                        /* Obtain ptr to the interface area.                    */
  p_dev_bsp  = (NET_DEV_BSP_WIFI *)p_if->Dev_BSP;

  if (enable == A_TRUE) {
    p_dev_bsp->IntCtrl(p_if,
                       DEF_TRUE,
                       &local_err);
  } else {
    p_dev_bsp->IntCtrl(p_if,
                       DEF_FALSE,
                       &local_err);
  }
  (void)local_err;
}

A_VOID Custom_HW_UsecDelay(A_VOID   *pCxt,
                           A_UINT32  uSeconds)
{
  if (uSeconds < 1000) {
    KAL_Dly(1);
  } else {
    KAL_Dly(uSeconds / 1000);
  }

  (void)pCxt;
}

QOSAL_VOID Custom_HW_PowerUpDown(QOSAL_VOID   *pCxt,
                                 QOSAL_UINT32  powerUp)
{
  NET_IF            *p_if;
  NET_DEV_BSP_WIFI  *p_dev_bsp;
  RTOS_ERR           local_err;

  /* -------- OBTAIN REFERENCE TO CFGs/REGs/BSP --------- */
  p_if       = (NET_IF           *)pCxt;                        /* Obtain ptr to the interface area.                    */
  p_dev_bsp  = (NET_DEV_BSP_WIFI *)p_if->Dev_BSP;
  if (powerUp) {
    p_dev_bsp->Start(p_if,
                     &local_err);
  } else {
    p_dev_bsp->Stop(p_if,
                    &local_err);
  }

  (void)local_err;
}

/*****************************************************************************/
/* Custom_Bus_InOutBuffer - This is the platform specific solution to
 *  transfer a buffer on the SPI bus.  This solution is always synchronous
 *  regardless of sync param. The function will use the MQX fread and fwrite
 *  as appropriate.
 *      A_VOID * pCxt - the driver context.
 *      A_UINT8 *pBuffer - The buffer to transfer.
 *      A_UINT16 length - the length of the transfer in bytes.
 *      A_UINT8 doRead - 1 if operation is a read else 0.
 *      A_BOOL sync - TRUE is synchronous transfer is required else FALSE.
 *****************************************************************************/
A_STATUS Custom_Bus_InOutBuffer(QOSAL_VOID   *pCxt,
                                QOSAL_UINT8  *pBuffer,
                                QOSAL_UINT16 length,
                                QOSAL_UINT8  doRead,
                                QOSAL_BOOL   sync)
{
  A_STATUS       status;
  NET_IF        *p_if;
  NET_DEV_DATA  *p_dev_data;
  RTOS_ERR       local_err;

  status = A_OK;

  /* -------- OBTAIN REFERENCE TO CFGs/REGs/BSP --------- */
  p_if       = (NET_IF       *)pCxt;                            /* Obtain ptr to the interface area.                    */
  p_dev_data = (NET_DEV_DATA *)p_if->Dev_Data;

  UNUSED_ARGUMENT(sync);
  /* this function takes advantage of the SPI turbo mode which does not toggle the chip select
   * during the transfer.  Rather the chip select is asserted at the beginning of the transfer
   * and de-asserted at the end of the entire transfer via fflush(). */
  if (doRead) {
    SPI_SlaveRx(p_dev_data->SPI_Handle,
                pBuffer,
                length,
                NET_DEV_SPI_XFER_TIMEOUT_MS,
                &local_err);
    if (RTOS_ERR_CODE_GET(local_err) != RTOS_ERR_NONE) {
      status = A_HARDWARE;
    }
  } else {
    SPI_SlaveTx(p_dev_data->SPI_Handle,
                pBuffer,
                length,
                NET_DEV_SPI_XFER_TIMEOUT_MS,
                &local_err);
    if (RTOS_ERR_CODE_GET(local_err) != RTOS_ERR_NONE) {
      status = A_HARDWARE;
    }
  }

  return status;
}

/*****************************************************************************/
/* Custom_Bus_InOut_Token - This is the platform specific solution to
 *  transfer 4 or less bytes in both directions. The transfer must be
 *  synchronous. This solution uses the MQX spi ioctl to complete the request.
 *      A_VOID * pCxt - the driver context.
 *      A_UINT32 OutToken - the out going data.
 *      A_UINT8 DataSize - the length in bytes of the transfer.
 *      A_UINT32 *pInToken - A Buffer to hold the incoming bytes.
 *****************************************************************************/
A_STATUS Custom_Bus_InOutToken(QOSAL_VOID   *pCxt,
                               QOSAL_UINT32  OutToken,
                               QOSAL_UINT8   DataSize,
                               QOSAL_UINT32 *pInToken)
{
  NET_IF        *p_if;
  CPU_INT08U    *p_char_out;
  CPU_INT08U    *p_char_in;
  A_STATUS       status;
  NET_DEV_DATA  *p_dev_data;
  RTOS_ERR       local_err;

  status = A_OK;
  /* -------- OBTAIN REFERENCE TO CFGs/REGs/BSP --------- */
  p_if        = (NET_IF       *)pCxt;                           /* Obtain ptr to the interface area.                    */
  p_dev_data  = (NET_DEV_DATA *)p_if->Dev_Data;

  p_char_out = (CPU_INT08U *)&OutToken;
  p_char_in  = (CPU_INT08U *) pInToken;
  //                                                               data size if really a enum that is 1 less than number of bytes

  do {
    if (A_OK != Custom_Bus_StartTransfer(pCxt, A_TRUE)) {
      status = A_HARDWARE;
      break;
    }
    SPI_SlaveXfer(p_dev_data->SPI_Handle,
                  p_char_in,
                  p_char_out,
                  DataSize,
                  NET_DEV_SPI_XFER_TIMEOUT_MS,
                  &local_err);
    if (RTOS_ERR_CODE_GET(local_err) !=  RTOS_ERR_NONE) {
      status = A_HARDWARE;
    }

    Custom_Bus_CompleteTransfer(pCxt, A_TRUE);
  } while (0);

  return status;
}

/*****************************************************************************/
/* Custom_Bus_Start_Transfer - This function is called by common layer prior
 *  to starting a new bus transfer. This solution merely sets up the SPI
 *  mode as a precaution.
 *      A_VOID * pCxt - the driver context.
 *      A_BOOL sync - TRUE is synchronous transfer is required else FALSE.
 *****************************************************************************/
A_STATUS Custom_Bus_StartTransfer(QOSAL_VOID *pCxt,
                                  QOSAL_BOOL  sync)
{
  return A_OK;
}

/*****************************************************************************/
/* Custom_Bus_Complete_Transfer - This function is called by common layer prior
 *  to completing a bus transfer. This solution calls fflush to de-assert
 *  the chipselect.
 *      A_VOID * pCxt - the driver context.
 *      A_BOOL sync - TRUE is synchronous transfer is required else FALSE.
 *****************************************************************************/
A_STATUS Custom_Bus_CompleteTransfer(QOSAL_VOID *pCxt,
                                     QOSAL_BOOL sync)
{
  return A_OK;
}

/*extern A_VOID *p_Global_Cxt;*/

A_STATUS Custom_HW_Init(QOSAL_VOID *pCxt)
{
  A_DRIVER_CONTEXT      *pDCxt;
#if 0
  NET_IF                *p_if;
  NET_DEV_CFG_WIFI      *p_dev_cfg;
  NET_DEV_DATA          *p_dev_data;

  /* -------- OBTAIN REFERENCE TO CFGs/REGs/BSP --------- */
  p_if       = (NET_IF               *)pCxt;                    /* Obtain ptr to the interface area.                    */
  p_dev_cfg  = (NET_DEV_CFG_WIFI     *)p_if->Dev_Cfg;           /* Obtain ptr to dev cfg area.                          */
  p_dev_data = (NET_DEV_DATA         *)p_if->Dev_Data;
#endif
  pDCxt      = GET_DRIVER_COMMON(pCxt);

  /* -------------------- HW INIT ----------------------- */
  /* Hardware initialization is done in the driver API... */
  /* in NetIF_WiFi_IF_Add() via 'p_dev_api->Init()'.      */

  /* ---------------- SET UP HCD PARAMS ----------------- */
  /* IT is necessary for the custom code to populate ...  */
  /* ...the following parameters so that the common...    */
  /* ...code knows what kind of hardware it is working... */
  /* ...with.                                             */
//                                                                TODO-YP get actual freq
  pDCxt->spi_hcd.OperationalClock = 15000000;  // p_dev_cfg->SPI_ClkFreq;
  pDCxt->spi_hcd.PowerUpDelay = POWER_UP_DELAY;
  pDCxt->spi_hcd.SpiHWCapabilitiesFlags = HW_SPI_CAPS;
  pDCxt->spi_hcd.MiscFlags |= MISC_FLAG_RESET_SPI_IF_SHUTDOWN;

  return A_OK;
}

A_STATUS Custom_HW_DeInit(QOSAL_VOID *pCxt)
{
  return A_OK;
}
