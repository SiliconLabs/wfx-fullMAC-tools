/*
 * EVALUATION AND USE OF THIS SOFTWARE IS SUBJECT TO THE TERMS AND
 * CONDITIONS OF THE CONTROLLING LICENSE AGREEMENT FOUND AT LICENSE.md
 * IN THIS SDK. IF YOU DO NOT AGREE TO THE LICENSE TERMS AND CONDITIONS,
 * PLEASE RETURN ALL SOURCE FILES TO SILICON LABORATORIES.
 * (c) Copyright 2018, Silicon Laboratories Inc.  All rights reserved.
 */

/*
 *  Bus low level operations that dependent on the underlying physical bus : spi implementation
 */

/* FreeRTOS includes. */
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"

#include <stdbool.h>
#include "wf200_host_pin.h"

extern SPI_HandleTypeDef hspi1;
SemaphoreHandle_t spiDMASemaphore;

sl_status_t wf200_host_init_bus( void )
{
  //Due to STM32CubeMX, the SPI initialization is done elsewhere
  /*Create semaphore to handle SPI*/
  spiDMASemaphore = xSemaphoreCreateBinary();
  xSemaphoreGive(spiDMASemaphore);  
  return SL_SUCCESS;
}

sl_status_t wf200_host_spi_cs_assert()
{
  HAL_GPIO_WritePin(WF200_CS_PORT_SPI, WF200_CS_GPIO_SPI, GPIO_PIN_RESET);
  return SL_SUCCESS;
}

sl_status_t wf200_host_spi_cs_deassert()
{
  HAL_GPIO_WritePin(WF200_CS_PORT_SPI, WF200_CS_GPIO_SPI, GPIO_PIN_SET);
  return SL_SUCCESS;
}

sl_status_t wf200_host_spi_transfer_no_cs_assert( wf200_host_bus_tranfer_type_t type, uint8_t* buffer, uint16_t buffer_length )
{
  sl_status_t    result  = SL_ERROR;
  const bool     is_read = ( type == WF200_BUS_READ );
  
  /* Wait for the DMA channels to be available */
  if(xSemaphoreTake(spiDMASemaphore, portMAX_DELAY) == pdTRUE )
  {
    if(is_read)
    {
      if(HAL_SPI_Receive_DMA(&hspi1, buffer, buffer_length) == HAL_OK) result = SL_SUCCESS;
    }else{
      if(HAL_SPI_Transmit_DMA(&hspi1, buffer, buffer_length) == HAL_OK) result = SL_SUCCESS;
    }
  }else{
    result = SL_TIMEOUT;
  }
  /* Wait to receive the semaphore back from the DMA. In case of a read function, this means data is ready to be read*/ 
  if(xSemaphoreTake(spiDMASemaphore, portMAX_DELAY) == pdTRUE )
  {     
    xSemaphoreGive(spiDMASemaphore); 
  }
  return result;
}

sl_status_t wf200_host_enable_platform_interrupt( void )
{
  //Done in main due to STM32CubeMX format (To be moved?)
  return SL_SUCCESS;
}


sl_status_t wf200_host_disable_platform_interrupt( void )
{
  //TODO: add deinit from CubeMX template
  return SL_SUCCESS;
}

