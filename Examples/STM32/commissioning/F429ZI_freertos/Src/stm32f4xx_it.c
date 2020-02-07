/**
  ******************************************************************************
  * @file    stm32f4xx_it.c
  * @brief   Interrupt Service Routines.
  ******************************************************************************
  *
  * COPYRIGHT(c) 2018 STMicroelectronics
  *
  * Redistribution and use in source and binary forms, with or without modification,
  * are permitted provided that the following conditions are met:
  *   1. Redistributions of source code must retain the above copyright notice,
  *      this list of conditions and the following disclaimer.
  *   2. Redistributions in binary form must reproduce the above copyright notice,
  *      this list of conditions and the following disclaimer in the documentation
  *      and/or other materials provided with the distribution.
  *   3. Neither the name of STMicroelectronics nor the names of its contributors
  *      may be used to endorse or promote products derived from this software
  *      without specific prior written permission.
  *
  * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
  * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
  * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
  * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
  * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
  * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
  * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
  * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
  * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
  * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
  *
  ******************************************************************************
  */
#include "stm32f4xx_hal.h"
#include "stm32f4xx.h"
#include "stm32f4xx_it.h"
#include "cmsis_os.h"

#include "semphr.h"
#include "sl_wfx.h"
#include "sl_wfx_host_pin.h"

/* External variables --------------------------------------------------------*/
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
/******************************************************************************/
/*            Cortex-M4 Processor Interruption and Exception Handlers         */ 
/******************************************************************************/

/**
* @brief This function handles System tick timer.
*/
void SysTick_Handler(void)
{
  osSystickHandler();
}

/******************************************************************************/
/* STM32F4xx Peripheral Interrupt Handlers                                    */
/* Add here the Interrupt Handlers for the used peripherals.                  */
/* For the available peripheral interrupt handler names,                      */
/* please refer to the startup file (startup_stm32f4xx.s).                    */
/******************************************************************************/

/**
* @brief This function handles TIM1 update interrupt and TIM10 global interrupt.
*/
void TIM1_UP_TIM10_IRQHandler(void)
{
  HAL_TIM_IRQHandler(&htim1);
}

/**
* @brief This function handles EXTI line[15:10] interrupts.
*/
void EXTI15_10_IRQHandler(void)
{
  BaseType_t xHigherPriorityTaskWoken = pdFALSE;
  
#ifdef SL_WFX_USE_SPI
  if (__HAL_GPIO_EXTI_GET_IT(SL_WFX_IRQ_GPIO_SPI) != RESET) {
    vTaskNotifyGiveFromISR( busCommTaskHandle, &xHigherPriorityTaskWoken );
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
  portYIELD_FROM_ISR( xHigherPriorityTaskWoken );
}

#ifdef SL_WFX_USE_SPI
/**
* @brief This function handles SPI1 global interrupt.
*/
void SPI1_IRQHandler(void)
{
  HAL_SPI_IRQHandler(&hspi1);
}

/**
* @brief This function handles DMA2 stream0 global interrupt.
*/
void DMA2_Stream0_IRQHandler(void)
{
  HAL_DMA_IRQHandler(&hdma_spi1_rx);
}

/**
* @brief This function handles DMA2 stream3 global interrupt.
*/
void DMA2_Stream3_IRQHandler(void)
{
  HAL_DMA_IRQHandler(&hdma_spi1_tx);
}

void HAL_SPI_TxCpltCallback(SPI_HandleTypeDef *hspi)
{
  BaseType_t xHigherPriorityTaskWoken = pdFALSE;
  xSemaphoreGiveFromISR(spiDMASemaphore, &xHigherPriorityTaskWoken);  
  portYIELD_FROM_ISR( xHigherPriorityTaskWoken );
}

void HAL_SPI_RxCpltCallback(SPI_HandleTypeDef *hspi)
{
  BaseType_t xHigherPriorityTaskWoken = pdFALSE;
  xSemaphoreGiveFromISR(spiDMASemaphore, &xHigherPriorityTaskWoken);  
  portYIELD_FROM_ISR( xHigherPriorityTaskWoken );
}
#endif /* SL_WFX_USE_SPI */

#ifdef SL_WFX_USE_SDIO
/**
* @brief This function handles SDIO global interrupt.
*/
void SDIO_IRQHandler(void)
{
  BaseType_t xHigherPriorityTaskWoken = pdFALSE;
  if(__SDIO_GET_FLAG(SDIO, SDIO_IT_SDIOIT)){
    /*Receive SDIO interrupt on SDIO_DAT1 from Ineo*/
    __SDIO_CLEAR_FLAG(SDIO, SDIO_FLAG_SDIOIT);
    vTaskNotifyGiveFromISR( busCommTaskHandle, &xHigherPriorityTaskWoken );
  }
  if(__SDIO_GET_FLAG(SDIO, SDIO_IT_DATAEND)){
    /*SDIO transfer over*/
    __SDIO_CLEAR_FLAG(SDIO, SDIO_IT_DATAEND);
    xSemaphoreGiveFromISR( sdioDMASemaphore, &xHigherPriorityTaskWoken );
  }
  portYIELD_FROM_ISR( xHigherPriorityTaskWoken );
}

/**
  * @brief This function handles DMA2 stream3 global interrupt.
  */
void DMA2_Stream3_IRQHandler(void)
{
  HAL_DMA_IRQHandler(&hdma_sdio_rx);
}

/**
  * @brief This function handles DMA2 stream6 global interrupt.
  */
void DMA2_Stream6_IRQHandler(void)
{
  HAL_DMA_IRQHandler(&hdma_sdio_tx);
}
#endif /* SL_WFX_USE_SDIO */

/**
* @brief This function handles USART3 global interrupt.
*/
void USART3_IRQHandler(void)
{
  uint32_t isrflags   = READ_REG(huart3.Instance->SR);

  HAL_UART_IRQHandler(&huart3);
  /*Character received*/
  if((isrflags & USART_SR_RXNE) != RESET)
  {
    /*Notify UARTCmdTask that data is available*/
    vTaskNotifyGiveFromISR( UARTInputTaskHandle, pdFALSE );
  }
}

void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart){
  /*Release the UART binary semaphore*/
  xSemaphoreGiveFromISR(uart3Semaphore, pdFALSE);  
}

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
  /*Release the UART binary semaphore*/
  xSemaphoreGiveFromISR(uart3Semaphore, pdFALSE);  
}
/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
