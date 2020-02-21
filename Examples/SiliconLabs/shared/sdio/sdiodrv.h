/***************************************************************************//**
 * @file
 * @brief SDIODRV API definition
 *******************************************************************************
 * # License
 * <b>Copyright 2019 Silicon Laboratories Inc. www.silabs.com</b>
 *******************************************************************************
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *    http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *****************************************************************************/
#ifndef SDIODRV_H
#define SDIODRV_H

#include "em_device.h"

#if defined(SDIO_PRESENT)
#include "sdio.h"
#else
#error "SDIO unsupported."
#endif

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>
#include "em_cmu.h"

/*******************************************************************************
 *******************************   DEFINES   ***********************************
 ******************************************************************************/

#define SDIODRV_ERROR_NONE                (0x00000000)
#define SDIODRV_ERROR_PARAM               (0x00000001)
#define SDIODRV_ERROR_CMDRESP             (0x00000002)
#define SDIODRV_ERROR_RESP_OUT_OF_RANGE   (0x00000004)
#define SDIODRV_ERROR_RESP_FUNC_NB        (0x00000008)
#define SDIODRV_ERROR_RESP_UNKNOWN        (0x00000010)
#define SDIODRV_ERROR_RESP_WRONG_STATE    (0x00000020)
#define SDIODRV_ERROR_RESP_ILLEGAL_CMD    (0x00000040)
#define SDIODRV_ERROR_RESP_CRC            (0x00000080)
#define SDIODRV_ERROR_RCA                 (0x00000100)

/*******************************************************************************
 ********************************   ENUMS   ************************************
 ******************************************************************************/

/** IO operation selection. */
typedef enum SDIODRV_IO_Operation_s {
  SDIODRV_IO_OP_READ = 0,
  SDIODRV_IO_OP_WRITE
} SDIODRV_IO_Operation_t;

/*******************************************************************************
 ******************************   CALLBACKS   **********************************
 ******************************************************************************/

/***************************************************************************//**
 * @brief
 *   SDIODRV callback function.
 *
 * @details
 *   The callback functions are called when a command/transfer completes or
 *   an error/event occurs.
 *
 * @param[in] arg
 *   An argument may be passed to the callback function.
 ******************************************************************************/
typedef void (*SDIODRV_Callback_t)(void *arg);

/*******************************************************************************
 *******************************   STRUCTS   ***********************************
 ******************************************************************************/

/**
 * SDIODRV initialization structure containing a number of SDIODRV
 * configuration options. This structure is passed to @ref SDIODRV_Init()
 * when initializing a SDIODRV instance.
 */
typedef struct SDIODRV_Init_s {
  SDIO_TypeDef       *instance;
  uint32_t            freq;
  uint16_t            sdioBlockMaxSize;
  uint8_t             portLocationClk;
  uint8_t             portLocationCmd;
  uint8_t             portLocationDat;
  uint8_t             portLocationCd;
  uint8_t             portLocationWp;
  CMU_Select_TypeDef  clockSource;
  SDIO_Transfer_Width_t transferWidth;
} SDIODRV_Init_t;

/**
 * An SDIO driver instance handle data structure.
 * The handle is allocated by the application using the SDIODRV.
 * The application is neither supposed to write or read
 * the contents of the handle.
 */
typedef struct SDIODRV_Handle_s {
  SDIODRV_Init_t  init;
  SDIODRV_Callback_t appCb;   // Variable storing the application defined callback
  uint8_t *dataAddr;          // Current User Data address
  SDIODRV_IO_Operation_t op;  // Current operation
} SDIODRV_Handle_t;

/** SDIODRV callback functions. */
typedef struct SDIODRV_Callbacks_s {
  SDIODRV_Callback_t errorCb;
  SDIODRV_Callback_t cmdCompleteCb;
  SDIODRV_Callback_t transferCompleteCb;
  SDIODRV_Callback_t cardInsertionCb;
  SDIODRV_Callback_t cardRemovalCb;
  SDIODRV_Callback_t cardInterruptCb;
  SDIODRV_Callback_t bootAckRcvCb;
  SDIODRV_Callback_t bootTerminateCb;
} SDIODRV_Callbacks_t;

/*******************************************************************************
 *****************************   PROTOTYPES   **********************************
 ******************************************************************************/

uint32_t SDIODRV_Init(SDIODRV_Handle_t *handle, SDIODRV_Init_t *init);
uint32_t SDIODRV_DeInit(SDIODRV_Handle_t *handle);
uint32_t SDIODRV_DeviceInitAndIdent(SDIODRV_Handle_t *handle, uint16_t *rca);
uint32_t SDIODRV_SelectCard(SDIODRV_Handle_t *handle, uint16_t rca);
uint32_t SDIODRV_IOReadWriteDirect(SDIODRV_Handle_t *handle,
                                   SDIODRV_IO_Operation_t op,
                                   uint8_t function,
                                   uint32_t address,
                                   uint8_t *data);
uint32_t SDIODRV_IOReadWriteExtendedBytes(SDIODRV_Handle_t *handle,
                                          SDIODRV_IO_Operation_t op,
                                          uint8_t function,
                                          uint32_t address,
                                          uint16_t bytesCnt,
                                          uint8_t *data);
uint32_t SDIODRV_IOReadWriteExtendedBlocks(SDIODRV_Handle_t *handle,
                                           SDIODRV_IO_Operation_t op,
                                           uint8_t function,
                                           uint32_t address,
                                           uint16_t blocksCnt,
                                           uint8_t *data);
uint32_t SDIODRV_EnableHighSpeed(SDIODRV_Handle_t *handle, bool state);
uint32_t SDIODRV_EnableInterrupts(SDIODRV_Handle_t *handle,
                                  uint32_t interrupts,
                                  bool state);
void SDIODRV_RegisterCallbacks(SDIODRV_Callbacks_t *callbacks);

#ifdef __cplusplus
}
#endif

#endif //SDIODRV_H
