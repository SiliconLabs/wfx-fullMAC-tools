/**************************************************************************//**
 * Copyright 2021, Silicon Laboratories Inc.
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

#include "stm32f4xx_hal.h"
#include "stm32f4xx.h"
#include "stm32f4xx_it.h"
#include "cmsis_os.h"
#include "semphr.h"
#include "sl_wfx.h"
#include "sl_wfx_host.h"
#include "sl_wfx_host_pin.h"
#include "sl_wfx_host_events.h"

#ifdef SL_WFX_USE_SPI
extern SPI_HandleTypeDef hspi1;
extern DMA_HandleTypeDef hdma_spi1_tx;
extern DMA_HandleTypeDef hdma_spi1_rx;
extern SemaphoreHandle_t spiDMASemaphore;
#endif /* SL_WFX_USE_SPI */

#ifdef SL_WFX_USE_SDIO
extern DMA_HandleTypeDef hdma_sdio_rx;
extern DMA_HandleTypeDef hdma_sdio_tx;
extern SemaphoreHandle_t sdioDMASemaphore;
#endif /* SL_WFX_USE_SDIO */

extern TIM_HandleTypeDef htim1;
extern osThreadId busCommTaskHandle;
extern UART_HandleTypeDef huart3;
extern osThreadId UARTCmdTaskHandle;
extern SemaphoreHandle_t uart3Semaphore;
extern osThreadId UARTInputTaskHandle;

/**************************************************************************//**
 * Handle System tick timer
 *****************************************************************************/
void SysTick_Handler (void) {
  osSystickHandler();
}

/**************************************************************************//**
 * Handle System tick timer
 *****************************************************************************/
void TIM1_UP_TIM10_IRQHandler (void) {
  HAL_TIM_IRQHandler(&htim1);
}

/**************************************************************************//**
 * Handle EXTI line[15:10] interrupts. Wi-FI SPI interrupt.
 *****************************************************************************/
void EXTI15_10_IRQHandler (void) {
  BaseType_t xHigherPriorityTaskWoken = pdFALSE;
  
#ifdef SL_WFX_USE_SPI
  if (__HAL_GPIO_EXTI_GET_IT(SL_WFX_IRQ_GPIO_SPI) != RESET) {
    xSemaphoreGiveFromISR(sl_wfx_wake_up_sem, &xHigherPriorityTaskWoken);
    xEventGroupSetBitsFromISR(sl_wfx_event_group,
                              SL_WFX_RX_PACKET_AVAILABLE,
                              &xHigherPriorityTaskWoken);
  }
#endif /* SL_WFX_USE_SPI */

  if (__HAL_GPIO_EXTI_GET_IT(GPIO_PIN_13) != RESET) {
    HAL_GPIO_TogglePin(LD3_GPIO_Port, LD3_Pin);
    HAL_GPIO_TogglePin(LD2_GPIO_Port, LD2_Pin);
    HAL_GPIO_TogglePin(SL_WFX_LED0_PORT, SL_WFX_LED0_GPIO);
    HAL_GPIO_TogglePin(SL_WFX_LED1_PORT, SL_WFX_LED1_GPIO);
  }
  
  HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_13);
  HAL_GPIO_EXTI_IRQHandler(SL_WFX_IRQ_GPIO_SPI);
  portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}

#ifdef SL_WFX_USE_SPI
/**************************************************************************//**
 * Handle SPI1 global interrupt
 *****************************************************************************/
void SPI1_IRQHandler (void) {
  HAL_SPI_IRQHandler(&hspi1);
}

/**************************************************************************//**
 * Handle DMA2 stream0 global interrupt
 *****************************************************************************/
void DMA2_Stream0_IRQHandler (void) {
  HAL_DMA_IRQHandler(&hdma_spi1_rx);
}

/**************************************************************************//**
 * Handle DMA2 stream3 global interrupt
 *****************************************************************************/
void DMA2_Stream3_IRQHandler (void) {
  HAL_DMA_IRQHandler(&hdma_spi1_tx);
}

/**************************************************************************//**
 * SPI transmit complete callback
 *****************************************************************************/
void HAL_SPI_TxCpltCallback (SPI_HandleTypeDef *hspi) {
  BaseType_t xHigherPriorityTaskWoken = pdFALSE;
  xSemaphoreGiveFromISR(spiDMASemaphore, &xHigherPriorityTaskWoken);  
  portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}

/**************************************************************************//**
 * SPI receive complete callback
 *****************************************************************************/
void HAL_SPI_RxCpltCallback (SPI_HandleTypeDef *hspi) {
  BaseType_t xHigherPriorityTaskWoken = pdFALSE;
  xSemaphoreGiveFromISR(spiDMASemaphore, &xHigherPriorityTaskWoken);  
  portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}
#endif /* SL_WFX_USE_SPI */

#ifdef SL_WFX_USE_SDIO
/**************************************************************************//**
 * Handle SDIO global interrupt
 *****************************************************************************/
void SDIO_IRQHandler (void) {
  BaseType_t xHigherPriorityTaskWoken = pdFALSE;
  
  if (((SDIO->MASK & SDIO_IT_SDIOIT) == SDIO_IT_SDIOIT)
      && (__SDIO_GET_FLAG(SDIO, SDIO_FLAG_SDIOIT))) {
    /*Receive SDIO interrupt on SDIO_DAT1 from WF200*/
    __SDIO_CLEAR_FLAG(SDIO, SDIO_FLAG_SDIOIT);
    xSemaphoreGiveFromISR(sl_wfx_wake_up_sem, 
                          &xHigherPriorityTaskWoken);
    xEventGroupSetBitsFromISR(sl_wfx_event_group,
                              SL_WFX_RX_PACKET_AVAILABLE,
                              &xHigherPriorityTaskWoken);
  }
  
  if (((SDIO->MASK & SDIO_IT_DATAEND) == SDIO_IT_DATAEND)
      && (__SDIO_GET_FLAG(SDIO, SDIO_FLAG_DATAEND))) {
    /*SDIO transfer over*/
    __SDIO_CLEAR_FLAG(SDIO, SDIO_IT_DATAEND);
    xSemaphoreGiveFromISR(sdioDMASemaphore, &xHigherPriorityTaskWoken);
  }
  portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}

/**************************************************************************//**
 * Handle DMA2 stream3 global interrupt
 *****************************************************************************/
void DMA2_Stream3_IRQHandler (void) {
  HAL_DMA_IRQHandler(&hdma_sdio_rx);
}

/**************************************************************************//**
 * Handle DMA2 stream6 global interrupt
 *****************************************************************************/
void DMA2_Stream6_IRQHandler (void) {
  HAL_DMA_IRQHandler(&hdma_sdio_tx);
}
#endif /* SL_WFX_USE_SDIO */

/**************************************************************************//**
 * Handle DMA1 stream1 global interrupt
 *****************************************************************************/
void DMA1_Stream1_IRQHandler(void) {
  HAL_DMA_IRQHandler(huart3.hdmarx);
}

/**************************************************************************//**
 * Handle DMA1 stream3 global interrupt
 *****************************************************************************/
void DMA1_Stream3_IRQHandler (void) {
  HAL_DMA_IRQHandler(huart3.hdmatx);
}

/**************************************************************************//**
 * Handle USART3 interrupt
 *****************************************************************************/
void USART3_IRQHandler (void) {
  uint32_t isrflags = READ_REG(huart3.Instance->SR);

  HAL_UART_IRQHandler(&huart3);
  /*Character received*/
  if ((isrflags & USART_SR_RXNE) != RESET) {
    /*Notify UARTCmdTask that data is available*/
    vTaskNotifyGiveFromISR(UARTInputTaskHandle, pdFALSE);
  }
}

/**************************************************************************//**
 * UART transmit complete callback
 *****************************************************************************/
void HAL_UART_TxCpltCallback (UART_HandleTypeDef *huart) {
  /*Release the UART binary semaphore*/
  xSemaphoreGiveFromISR(uart3Semaphore, pdFALSE);  
}

/**************************************************************************//**
 * UART receive complete callback
 *****************************************************************************/
void HAL_UART_RxCpltCallback (UART_HandleTypeDef *huart) {
  /*Release the UART binary semaphore*/
  xSemaphoreGiveFromISR(uart3Semaphore, pdFALSE);  
}
