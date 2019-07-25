/**************************************************************************//**
 * Copyright 2019, Silicon Laboratories Inc.
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
#ifdef  SL_WFX_USE_SPI
#include  <rtos_description.h>



#include "sl_wfx.h"
#include "sl_wfx_host_api.h"
#include "sl_wfx_bus.h"
#include "wfx_pin_config.h"

#include "em_gpio.h"
#include "em_usart.h"
#include "em_cmu.h"
#include "em_ldma.h"
#include "em_bus.h"

#include "dmadrv.h"



#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <kernel/include/os.h>
#include <common/include/rtos_utils.h>
#include <common/include/rtos_err.h>
#include "sleep.h"

#define USART USART0
#define USART_CLK cmuClock_USART0
#define RX_DMA_SIGNAL dmadrvPeripheralSignal_USART0_RXDATAV
#define TX_DMA_SIGNAL dmadrvPeripheralSignal_USART0_TXBL
#define USART_PORT gpioPortE
#define USART_CS_PIN 13
#define USART_TX_PIN 10
#define USART_RX_PIN 11
#define USART_CLK_PIN 12

static OS_SEM spiSem;
static unsigned int        txDMACh;
static unsigned int        rxDMACh;
static uint32_t            dummyRx;
static uint32_t            dummyTxValue;
static bool spiEnabled = false;


sl_status_t sl_wfx_host_init_bus( void )
{
  RTOS_ERR err;


  // Initialize and enable the USART
  USART_InitSync_TypeDef usartInit = USART_INITSYNC_DEFAULT;

#ifdef SLEEP_ENABLED
  SLEEP_SleepBlockBegin(sleepEM2);
#endif
  spiEnabled = true;
  dummyTxValue = 0;
  usartInit.baudrate = 36000000u;
  usartInit.msbf = true;
  CMU_ClockEnable(cmuClock_HFPER, true);
  CMU_ClockEnable(cmuClock_GPIO, true);
  CMU_ClockEnable(USART_CLK, true);
  USART_InitSync(USART, &usartInit);
  USART->CTRL |= (1<<_USART_CTRL_SMSDELAY_SHIFT);
  USART->ROUTELOC0 = (USART->ROUTELOC0
                                   & ~(_USART_ROUTELOC0_TXLOC_MASK
                                       | _USART_ROUTELOC0_RXLOC_MASK
                                       | _USART_ROUTELOC0_CLKLOC_MASK))
                                  | (0  << _USART_ROUTELOC0_TXLOC_SHIFT)
                                  | (0  << _USART_ROUTELOC0_RXLOC_SHIFT)
                                  | (0 << _USART_ROUTELOC0_CLKLOC_SHIFT);

  USART->ROUTEPEN = USART_ROUTEPEN_TXPEN
                                 | USART_ROUTEPEN_RXPEN
                                 | USART_ROUTEPEN_CLKPEN;
  GPIO_DriveStrengthSet(USART_PORT,gpioDriveStrengthStrongAlternateStrong);
  GPIO_PinModeSet(USART_PORT, USART_TX_PIN,gpioModePushPull, 0);
  GPIO_PinModeSet(USART_PORT, USART_RX_PIN,gpioModeInput, 0);
  GPIO_PinModeSet(USART_PORT, USART_CLK_PIN,gpioModePushPull, 0);
  OSSemCreate(&spiSem,"spi semaphore",0,&err);
  DMADRV_Init();
  DMADRV_AllocateChannel(&txDMACh, NULL);
  DMADRV_AllocateChannel(&rxDMACh, NULL);
  GPIO_PinModeSet(USART_PORT,  USART_CS_PIN,  gpioModePushPull,  1);
  USART->CMD = USART_CMD_CLEARRX | USART_CMD_CLEARTX;
  return SL_SUCCESS;
}



sl_status_t sl_wfx_host_deinit_bus( void )
{

  RTOS_ERR err;


  OSSemDel(&spiSem,OS_OPT_DEL_ALWAYS,&err);
  // Stop DMAs.
  DMADRV_StopTransfer(rxDMACh);
  DMADRV_StopTransfer(txDMACh);
  DMADRV_FreeChannel(txDMACh);
  DMADRV_FreeChannel(rxDMACh);
  DMADRV_DeInit();
  USART_Reset(USART);
  return SL_SUCCESS;
}


// -------------- SPI -------

sl_status_t sl_wfx_host_spi_cs_assert()
{

  GPIO_PinOutClear(USART_PORT,  USART_CS_PIN);
  return SL_SUCCESS;

}

sl_status_t sl_wfx_host_spi_cs_deassert()
{

  GPIO_PinOutSet(USART_PORT,  USART_CS_PIN);
  return SL_SUCCESS;

}

static bool RxDMAComplete(unsigned int channel,
                          unsigned int sequenceNo,
                          void *userParam)
{
	RTOS_ERR err;
	OSSemPost(&spiSem,OS_OPT_POST_1,&err);
	return true;
}

void receiveDMA (uint8_t* buffer, uint16_t buffer_length)
{

// Start receive DMA.
  DMADRV_PeripheralMemory(rxDMACh,
		                  RX_DMA_SIGNAL,
                          (void*)buffer,
						  (void *)&(USART->RXDATA),
                          true,
						  buffer_length,
						  dmadrvDataSize1,
                          RxDMAComplete,
                          NULL);

  // Start transmit DMA.
  DMADRV_MemoryPeripheral(txDMACh,
		                  TX_DMA_SIGNAL,
						  (void *)&(USART->TXDATA),
                          (void *)&(dummyTxValue),
                          false,
						  buffer_length,
						  dmadrvDataSize1,
                          NULL,
                          NULL);
}

void transmitDMA (uint8_t* buffer, uint16_t buffer_length)
{

	  // Receive DMA runs only to initiate callback
	  // Start receive DMA.
	  DMADRV_PeripheralMemory(rxDMACh,
			                  RX_DMA_SIGNAL,
	                          &dummyRx,
							  (void *)&(USART->RXDATA),
	                          false,
							  buffer_length,
							  dmadrvDataSize1,
	                          RxDMAComplete,
	                          NULL);
	// Start transmit DMA.
	  DMADRV_MemoryPeripheral(txDMACh,
			                  TX_DMA_SIGNAL,
							  (void *)&(USART->TXDATA),
	                          (void*)buffer,
	                          true,
							  buffer_length,
							  dmadrvDataSize1,
	                          NULL,
	                          NULL);
}

sl_status_t sl_wfx_host_spi_transfer_no_cs_assert(sl_wfx_host_bus_tranfer_type_t type,
        uint8_t *header,
        uint16_t header_length,
        uint8_t *buffer,
        uint16_t buffer_length)
{
  const bool is_read = ( type == SL_WFX_BUS_READ );
  RTOS_ERR err;
  err.Code = RTOS_ERR_NONE;
  while(!(USART->STATUS & USART_STATUS_TXBL))
  {
  }
  USART->CMD = USART_CMD_CLEARRX | USART_CMD_CLEARTX;

  if(header_length > 0)
  {


          for(uint8_t *buffer_ptr = header; header_length > 0; --header_length, ++buffer_ptr)
          {
        	  USART->TXDATA = (uint32_t)(*buffer_ptr);

              while(!(USART->STATUS & USART_STATUS_TXC))
              {
              }

          }
          while(!(USART->STATUS & USART_STATUS_TXBL))
          {
          }

  }
  if (buffer_length > 0)
  {
	  USART->CMD = USART_CMD_CLEARRX | USART_CMD_CLEARTX;
	  OSSemSet(&spiSem,0,&err);
      if (is_read)
      {
	      receiveDMA(buffer,buffer_length);
      }
      else
      {
          transmitDMA(buffer,buffer_length);
      }
      OSSemPend(&spiSem,0,OS_OPT_PEND_BLOCKING,0,&err);
  }
  // TODO: Enable Power Save Mode

  if (err.Code == RTOS_ERR_NONE) {
    return SL_SUCCESS;
  } else {
    return SL_ERROR;
  }
}



sl_status_t sl_wfx_host_enable_platform_interrupt( void )
{
  // Used interrupt #10 because #8 and #9 are used by the push buttons on the SLSTK3701A
  GPIO_ExtIntConfig(BSP_EXP_SPI_WIRQPORT, BSP_EXP_SPI_WIRQPIN, BSP_EXP_SPI_IRQ, true, false, true);
  return SL_SUCCESS;
}


sl_status_t sl_wfx_host_disable_platform_interrupt( void )
{
  GPIO_IntDisable(BSP_EXP_SPI_IRQ);
  return SL_SUCCESS;
}
sl_status_t sl_wfx_host_enable_spi (void)
{
	if (spiEnabled == false)
	{
	  SLEEP_SleepBlockBegin(sleepEM2);
	  spiEnabled = true;
	}
	return SL_SUCCESS;
}

sl_status_t sl_wfx_host_disable_spi (void)
{
	if (spiEnabled == true)
	{

	  spiEnabled = false;
	  SLEEP_SleepBlockEnd(sleepEM2);
	}
	return SL_SUCCESS;
}



/********************************************************************************************************
 ********************************************************************************************************
 *                                       DEPENDENCIES & AVAIL CHECK(S)
 ********************************************************************************************************
 *******************************************************************************************************/
#endif
