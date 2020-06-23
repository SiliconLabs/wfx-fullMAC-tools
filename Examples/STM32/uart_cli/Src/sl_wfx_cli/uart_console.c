/**************************************************************************//**
 * Copyright 2018, Silicon Laboratories Inc.
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

/* Standard includes. */
#include "string.h"
#include "cmsis_os.h"
#include "stm32f4xx_hal.h"
#include "FreeRTOS_CLI.h"
#include "sl_wfx_cli.h"

/* Dimensions the buffer into which input characters are placed. */
#define cmdMAX_INPUT_SIZE 512

/*-----------------------------------------------------------*/

/*
 * The task that implements the command console processing.
 */
static void prvUARTCommandConsoleTask(void const * pvParameters );

/*-----------------------------------------------------------*/

/* Holds the handle of the task that implements the UART command console. */
static const char * const pcWelcomeMessage = (char *) "Type \"help\" to view a list of registered commands.\r\n";
static const char * const pcNewLine = (char *) "\r\n";
static const char * const pcEndOfCommandOutputString = (char *) "$";

/*-----------------------------------------------------------*/
osThreadId UARTCmdTaskHandle;
extern UART_HandleTypeDef huart3;
extern SemaphoreHandle_t uart3Semaphore;

void vUARTCommandConsoleStart( void )
{
  osThreadDef(UARTCmdTask, prvUARTCommandConsoleTask, osPriorityLow, 0, 512);
  UARTCmdTaskHandle = osThreadCreate(osThread(UARTCmdTask), NULL);
}
/*-----------------------------------------------------------*/

static void prvUARTCommandConsoleTask(void const * pvParameters )
{
  char *output_string, *end_line_pos;
  static char rxed_char[cmdMAX_INPUT_SIZE]; 
  static char input_string[cmdMAX_INPUT_SIZE];
  static uint32_t input_string_size, rxed_size;
  portBASE_TYPE xReturned;
  
  /* Obtain the address of the output buffer.  Note there is no mutual
  exclusion on this buffer as it is assumed only one command console
  interface will be used at any one time. */
  output_string = FreeRTOS_CLIGetOutputBuffer();
  
  /* Send the welcome message*/
  
  printf(pcWelcomeMessage);
  printf("Wi-Fi CLI version      %s\r\n", SL_WFX_CLI_VERSION_STRING);
  
  for( ;; )
  {
    if(xSemaphoreTake( uart3Semaphore, portMAX_DELAY ) == pdTRUE)
    {
      memset(rxed_char, 0x00, cmdMAX_INPUT_SIZE);
      HAL_UART_Receive_DMA(&huart3, (uint8_t *) rxed_char, cmdMAX_INPUT_SIZE);
      xSemaphoreGive(uart3Semaphore);
    }
    
    /*Wait for idle line detection*/
    ulTaskNotifyTake( pdTRUE, portMAX_DELAY );
    
    /* Retrieve the number of received characters */
    rxed_size = sizeof(rxed_char) - huart3.hdmarx->Instance->NDTR;
    
    if(rxed_size != 0){
      /* Handle backspace case */
      if(strchr(rxed_char, '\b') && (rxed_size == 1))
      {
        if(input_string_size > 0)
        {
          if(xSemaphoreTake( uart3Semaphore, portMAX_DELAY ) == pdTRUE)
          {
            HAL_UART_Transmit_DMA(&huart3, "\b \b", 3);
          }
          input_string_size--;
          input_string[input_string_size] = 0;
        }
      }else{
        strncat(input_string, rxed_char, rxed_size);
        input_string_size += rxed_size;
        /* Echo the characters received */
        if(xSemaphoreTake( uart3Semaphore, portMAX_DELAY ) == pdTRUE)
        {
          HAL_UART_Transmit_DMA(&huart3, (uint8_t *) rxed_char, rxed_size);
        }
      }
      
      do{
        end_line_pos = strchr(input_string, '\r');
        if(end_line_pos != NULL)
        {
          *end_line_pos = '\0';
          printf(pcNewLine);
          do{
            /* Get the string to write to the UART from the command
            interpreter. */
            xReturned = FreeRTOS_CLIProcessCommand(input_string,
                                                   output_string,
                                                   configCOMMAND_INT_MAX_OUTPUT_SIZE);
            
            if(xSemaphoreTake( uart3Semaphore, portMAX_DELAY ) == pdTRUE)
            {
              HAL_UART_Transmit(&huart3,
                                (uint8_t *) output_string,
                                strlen(output_string),
                                portMAX_DELAY);
              xSemaphoreGive(uart3Semaphore); 
            }
          }while(xReturned != pdFALSE);
          printf(pcEndOfCommandOutputString);
          memset(input_string, 0x00, cmdMAX_INPUT_SIZE);
          input_string_size = 0;
        }
      }while(end_line_pos != NULL);
      
      HAL_UART_AbortReceive(&huart3);
    }
  }
}
