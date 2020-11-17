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

#ifdef FREERTOS

#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include "cmsis_os.h"
#include "stm32f4xx_hal.h"
#include "FreeRTOS_CLI.h"
#include "sl_wfx_host_events.h"
#include "sl_wfx_cli_generic.h"

static int sl_wfx_cli_generic_freertos_init(void *config);
static int sl_wfx_cli_generic_freertos_register_cmd(sl_wfx_cli_generic_command_t const *cmd);
static int sl_wfx_cli_generic_freertos_config_wait(uint32_t event_flag);
static int sl_wfx_cli_generic_freertos_wait(uint32_t timeout_ms);
static int sl_wfx_cli_generic_freertos_resume(void);

sl_wfx_cli_generic_functions_t sl_wfx_cli_gen_funcs = {
  sl_wfx_cli_generic_freertos_init,
  sl_wfx_cli_generic_freertos_register_cmd,
  sl_wfx_cli_generic_freertos_config_wait,
  sl_wfx_cli_generic_freertos_wait,
  sl_wfx_cli_generic_freertos_resume
};

// -----------------------------------------------------------------------------
// Local defines

#ifndef SL_WFX_CLI_GEN_TASK_STK_SIZE
#define SL_WFX_CLI_GEN_TASK_STK_SIZE       512u
#endif

/* Dimensions the buffer into which input characters are placed. */
#ifndef SL_WFX_CLI_GEN_INPUT_BUF_SIZE
#define SL_WFX_CLI_GEN_INPUT_BUF_SIZE      128u
#endif

// -----------------------------------------------------------------------------
// Local structures

typedef struct sl_wfx_cli_gen_int_cmd_t {
  CLI_Command_Definition_t cmd;
  sl_wfx_cli_generic_command_callback_t callback;
} sl_wfx_cli_gen_int_cmd_t;

// -----------------------------------------------------------------------------
// Local global variables

// Shell Task Stack
osThreadId UARTCmdTaskHandle;
extern UART_HandleTypeDef huart3;
extern SemaphoreHandle_t uart3Semaphore;

static sl_wfx_cli_gen_int_cmd_t sl_wfx_cli_gen_cmd_registered[SL_WFX_CLI_GEN_NB_MAX_CMD] = {0};
static int nb_command_registered = 0;
static SemaphoreHandle_t event_sem;

// -----------------------------------------------------------------------------
// Local functions

static sl_wfx_cli_gen_int_cmd_t* freertos_cmd_search (const char *cmd_name)
{
  sl_wfx_cli_gen_int_cmd_t *cmd = NULL;

  // Iterate through the command list
  for (int i=0; i<nb_command_registered; i++) {
    if ((strncmp(cmd_name,
                 sl_wfx_cli_gen_cmd_registered[i].cmd.pcCommand,
                 strlen(sl_wfx_cli_gen_cmd_registered[i].cmd.pcCommand)) == 0)
        && ((cmd_name[strlen(sl_wfx_cli_gen_cmd_registered[i].cmd.pcCommand)] == '\0')
            || (cmd_name[strlen(sl_wfx_cli_gen_cmd_registered[i].cmd.pcCommand)] == ' '))) {
      cmd = &sl_wfx_cli_gen_cmd_registered[i];
      break;
    }
  }

  return cmd;
}

static int freertos_count_cmd_param (const char *cmd_line)
{
  int nb_param = 0;
  bool is_last_char_space = false;
  bool is_quote_open = false;

  // Count the number of space delimited words in cmd_line.
  while (*cmd_line != 0x00) {
    if (*cmd_line == '\"') {
      // Count quoted args as one parameter (already counted with the preceeding space)
      is_quote_open = !is_quote_open;
      is_last_char_space = false;
    } else if (!is_quote_open) {
      if (*cmd_line == ' ') {
        if (!is_last_char_space) {
          nb_param++;
        }
        is_last_char_space = true;
      } else {
        is_last_char_space = false;
      }
    }

    cmd_line++;
  }

  // If the command string ended with spaces, then there will have been too
  // many parameters counted.
  if (is_last_char_space) {
    nb_param--;
  }

  return nb_param;
}

static int freertos_get_cmd_param (char *input, char **param, int *len)
{
  char *param_start_ptr = NULL;
  char *cmd_ptr = input;
  char *token;
  int ret = 1;
  bool is_param_end = false;
  bool is_quote_open = false;
  
  *param = NULL;
  
  do {
    token = strpbrk(cmd_ptr, " \"");
    
    if (token == NULL) {
      if (strlen(cmd_ptr) > 0) {
        // Manage the last parameter
        *param = cmd_ptr;
        *len = strlen(cmd_ptr);
        cmd_ptr += strlen(cmd_ptr);
      }
      is_param_end = true;
      
    } else {
      if (token == cmd_ptr) {              
        // Opening quote should always be preceeded by a space
        if (*token == '\"') {
          is_quote_open = !is_quote_open;
          if (!is_quote_open) {
            // Closing quote, preceeded by a quote or a space
            *param = param_start_ptr;
            *len = token - param_start_ptr;
            is_param_end = true;
          }
        }
        
        // Discard successive delimiters
        cmd_ptr++;
        // Keep the next character as potential start of the next param
        param_start_ptr = cmd_ptr;
        
      } else if (*token == '\"') {
        
        if (is_quote_open) {
          // Closing quote
          *param = param_start_ptr;
          *len = token - param_start_ptr;
          is_quote_open = false;
          is_param_end = true;
        } //else quote in middle of a word, discard it
        cmd_ptr = token + 1;
        
      } else if (*token == ' ') {
        // Only take spaces into account outside quotes
        if (!is_quote_open) {
          *param = param_start_ptr;
          *len = token - param_start_ptr;
          is_param_end = true;
        }
        cmd_ptr = token + 1;
      }
    }
  } while (!is_param_end);
  
  if (is_quote_open) {
    // Invalid command
    ret = -1;
  } else if (*cmd_ptr == '\0') {
    // All parameters treated
    ret = 0;
  }
  
  return ret;
}

static BaseType_t freertos_wrapper_cb (char *pcWriteBuffer,
                                       size_t xWriteBufferLen,
                                       const char *pcCommandString)
{
  sl_wfx_cli_gen_int_cmd_t *sl_wfx_cli_gen_cmd;
  char **argv;
  char *cmd_ptr;
  char *msg = NULL;
  char *param = NULL;
  int len;
  int param_nb = 0;
  int param_cnt = 0;
  bool is_error = false;
  bool is_param_nb_error = false;
  BaseType_t ret = -1;
  
  // Retrieve the command information
  sl_wfx_cli_gen_cmd = freertos_cmd_search(pcCommandString);
  if ((sl_wfx_cli_gen_cmd != NULL)
      && (sl_wfx_cli_gen_cmd->callback != NULL)) {
    
    len = strlen(sl_wfx_cli_gen_cmd->cmd.pcCommand);
    
    // Retrieve the number of parameters in the command line
    param_cnt = freertos_count_cmd_param(pcCommandString + len);
    
    // Allocate memory resources
    argv = pvPortMalloc(sizeof(char *) * (param_cnt+1));
    if (argv != NULL) {
      argv[0] = pvPortMalloc(len+1/*NUL*/);
      if (argv[0] != NULL) {
        memcpy(argv[0], sl_wfx_cli_gen_cmd->cmd.pcCommand, len+1/*NUL*/);
        
        // Parse the command line parameters (one by one) by searching for
        // the space and quotes characters
        cmd_ptr = (char *)pcCommandString + len;

        do {          
          ret = freertos_get_cmd_param(cmd_ptr, &param, &len);
          if (ret >= 0) {
            if (param != NULL) {
              // Parameter limits found, extract it
              
              // Allocate the space required
              argv[param_nb+1] = pvPortMalloc(len+1/*NUL*/);
              if (argv[param_nb+1] == NULL) {
                // Allocation error
                msg = (char *)alloc_error_msg;
                is_error = true;
                break;
              }
              
              memcpy(argv[param_nb+1], param, len);
              argv[param_nb+1][len] = '\0';
              param_nb++;
              
              cmd_ptr = param + len;
            } //else no parameter found
            
          } else {
            msg = (char *)invalid_command_msg;
            is_error = true;
          }
        } while (ret > 0);
        
        ret = -1;
        
        // Execute the command callback
        if (!is_error) { 
          if ((sl_wfx_cli_gen_cmd->cmd.cExpectedNumberOfParameters < 0)
              || (sl_wfx_cli_gen_cmd->cmd.cExpectedNumberOfParameters == param_cnt)) {
                ret = (BaseType_t)sl_wfx_cli_gen_cmd->callback(param_cnt+1,
                                                               argv,
                                                               pcWriteBuffer,
                                                               xWriteBufferLen);
          } else {
            is_param_nb_error = true;
          }
          
          if (ret != 0) {
            if ((param_cnt > 0)
                && ((strncmp(argv[1], "help", 4) == 0) && (argv[1][4] == '\0'))) {
              // Command help requested, display it
              printf(sl_wfx_cli_gen_cmd->cmd.pcHelpString);
              // Erase the potential error message set by a module
              pcWriteBuffer[0] = '\0';
              ret = 0;
            
            } else if (pcWriteBuffer[0] == '\0') {
              // Ensure to display an error message
              if (is_param_nb_error) {
                msg = (char *)invalid_command_msg;
              } else {
                msg = (char *)command_error_msg;
              }
              ret = 0;
            
            } else {
              ret = 0;
            }
          }
        }
        
        // Free resources
        vPortFree(argv[0]);
        for (int i=0; i<param_nb; i++) {
          vPortFree(argv[i+1]);
        }
      } else {
        msg = (char *)invalid_command_msg;
      }
      
      // Free resources
      vPortFree(argv);
    } else {
      msg = (char *)invalid_command_msg;
    }
  }

  if (msg != NULL) {  
    strncpy(pcWriteBuffer, msg, xWriteBufferLen);
    if (xWriteBufferLen > 0) {
      pcWriteBuffer[xWriteBufferLen -1] = '\0';
    }
  }

  return ret;
}

static void prvUARTCommandConsoleTask(void const * pvParameters )
{
  char *output_string, *end_line_pos;
  static char rxed_char[SL_WFX_CLI_GEN_INPUT_BUF_SIZE]; 
  static char input_string[SL_WFX_CLI_GEN_INPUT_BUF_SIZE];
  static uint32_t input_string_size = 0, rxed_size = 0, rxed_prev_size = 0;
  portBASE_TYPE xReturned;
  
  // Obtain the address of the output buffer.  Note there is no mutual
  // exclusion on this buffer as it is assumed only one command console
  // interface will be used at any one time.
  output_string = FreeRTOS_CLIGetOutputBuffer();
  
  // Send the welcome message
  printf(welcome);
  printf(prompt);
  
  // Start UART reception
  if (xSemaphoreTake( uart3Semaphore, portMAX_DELAY ) == pdTRUE) {
    HAL_UART_Receive_DMA(&huart3,
                         (uint8_t *) rxed_char,
                         SL_WFX_CLI_GEN_INPUT_BUF_SIZE);
    xSemaphoreGive(uart3Semaphore);
  }
  
  for ( ;; ) {    
    // Wait for idle line detection
    ulTaskNotifyTake( pdTRUE, portMAX_DELAY );
    
    // Retrieve the number of received characters
    rxed_size = sizeof(rxed_char) - huart3.hdmarx->Instance->NDTR;
    
    if (rxed_size != 0) {
      if ((rxed_char[rxed_size-1] == '\b' /*BS*/) ||
          (rxed_char[rxed_size-1] == 0x7F /*DEL*/)) {
        if (strlen(input_string) > 0) {
          // Handle backspace case  
          if (xSemaphoreTake( uart3Semaphore, portMAX_DELAY ) == pdTRUE) {
            HAL_UART_Transmit_DMA(&huart3, "\b \b", 3);
          }
          input_string[strlen(input_string)-1] = 0;
        }
      } else {
        // Echo the characters received
        if (xSemaphoreTake( uart3Semaphore, portMAX_DELAY ) == pdTRUE) {
          HAL_UART_Transmit_DMA(&huart3,
                                (uint8_t *)&rxed_char[rxed_prev_size],
                                rxed_size-rxed_prev_size);
        }
        strncat(&input_string[input_string_size],
                &rxed_char[rxed_prev_size],
                rxed_size-rxed_prev_size);
      }
      
      // Look for an end of line
      end_line_pos = strpbrk(input_string, "\r\n");
      if ((end_line_pos != NULL) ||
          (rxed_size == SL_WFX_CLI_GEN_INPUT_BUF_SIZE)) {
        if (end_line_pos != NULL) {
          *end_line_pos = '\0';
        } else {
          // Buffer full, substitute the input to ensure an error when
          // processing the command
          strcpy(input_string, "error");
        }
        printf(newline);
        
        if (strlen(input_string) > 0) {
          do {
            // Get the string to write to the UART from the command interpreter.
            xReturned = FreeRTOS_CLIProcessCommand(input_string,
                                                   output_string,
                                                   configCOMMAND_INT_MAX_OUTPUT_SIZE);
            
            if(xSemaphoreTake( uart3Semaphore, portMAX_DELAY ) == pdTRUE)
            {
              HAL_UART_Transmit(&huart3,
                                (uint8_t *) output_string,
                                strlen(output_string),
                                portMAX_DELAY);
              // Artificially flush the output buffer 
              output_string[0] = '\0';
              xSemaphoreGive(uart3Semaphore); 
            }
          } while(xReturned != pdFALSE); 
        }
        
        // Artificially flush the input buffer
        input_string[0] = '\0';
        rxed_size = 0;
        
        // Start UART reception
        if (xSemaphoreTake( uart3Semaphore, portMAX_DELAY ) == pdTRUE) {
          HAL_UART_AbortReceive(&huart3);
          HAL_UART_Receive_DMA(&huart3,
                               (uint8_t *) rxed_char,
                               SL_WFX_CLI_GEN_INPUT_BUF_SIZE);
          xSemaphoreGive(uart3Semaphore);
        }
        
        printf(prompt);
      }
      
      rxed_prev_size = rxed_size;
    }
  }
}

static int cpu_reset_cmd_cb (int argc,
                             char **argv,
                             char *output_buf,
                             uint32_t output_buf_len)
{
  printf("CPU Reset\r\n");
  HAL_NVIC_SystemReset();
  
  return 0;
}

static const sl_wfx_cli_generic_command_t cpu_reset_cmd =
{
  "cpu-reset",
  "cpu-reset                : Reset the host CPU\r\n",
  cpu_reset_cmd_cb,
  0
};

static int sl_wfx_cli_generic_freertos_init (void *config)
{
  int res = SL_WFX_CLI_ERROR;
  
  // Align the internal counter with the shell sublayer
  nb_command_registered = 1;  //help command automatically added

  // Start the Shell task
  osThreadDef(UARTCmdTask,
              prvUARTCommandConsoleTask,
              osPriorityLow,
              0,
              SL_WFX_CLI_GEN_TASK_STK_SIZE);
  UARTCmdTaskHandle = osThreadCreate(osThread(UARTCmdTask), config);
  
  if (UARTCmdTaskHandle != NULL) {
    osSemaphoreDef(EventSem);
    event_sem = osSemaphoreCreate(osSemaphore(EventSem), 1);
    if (event_sem != NULL) {
      res = sl_wfx_cli_generic_freertos_register_cmd(&cpu_reset_cmd);
    } else {
      osThreadTerminate(UARTCmdTaskHandle);
    }
  }
  
  return res;
}

static int sl_wfx_cli_generic_freertos_register_cmd (sl_wfx_cli_generic_command_t const *cmd)
{
  int ret = SL_WFX_CLI_ERROR;
  BaseType_t err;
  // Create a FreeRTOS command
  CLI_Command_Definition_t freertos_cmd = { .pcCommand = cmd->name,
                                            .pcHelpString = cmd->help,
                                            .pxCommandInterpreter = freertos_wrapper_cb,
                                            .cExpectedNumberOfParameters  = cmd->nb_param };
  
  // Check that there is some place left
  if (nb_command_registered < SL_WFX_CLI_GEN_NB_MAX_CMD) {
    // Fill the internal command information
    memcpy(&sl_wfx_cli_gen_cmd_registered[nb_command_registered].cmd,
           &freertos_cmd,
           sizeof(CLI_Command_Definition_t));
    
    sl_wfx_cli_gen_cmd_registered[nb_command_registered].callback = cmd->callback;
    
    // Register the command in the FreeRTOS CLI
    err = FreeRTOS_CLIRegisterCommand(&sl_wfx_cli_gen_cmd_registered[nb_command_registered].cmd);
    if (err == pdPASS) {
      nb_command_registered++;
      ret = SL_WFX_CLI_ERROR_NONE;
    }
  } else {
    ret = SL_WFX_CLI_ERROR_NB_MAX_COMMANDS;
  }

  return ret;
}

static void event_notification_callback (uint32_t event_flag)
{
  sl_wfx_host_events_unnotify(event_flag, (void *)&event_sem);
  sl_wfx_cli_generic_freertos_resume();
}

static int sl_wfx_cli_generic_freertos_config_wait (uint32_t event_flag)
{
  int res = 0;
  
  if ((event_flag & SL_WFX_EVENT_ALL_FLAGS) != 0) {
    res = sl_wfx_host_events_notify(event_flag,
                                    event_notification_callback,
                                    (void *)&event_sem);
  }
  
  if (res == 0) {
    // Take the semaphore
    osSemaphoreWait(event_sem, 100);
    res = SL_WFX_CLI_ERROR_NONE;
  } else {
    res = SL_WFX_CLI_ERROR;
  }

  return res;
}

static int sl_wfx_cli_generic_freertos_wait (uint32_t timeout_ms)
{
  int res;

  res = osSemaphoreWait(event_sem, timeout_ms);
  if (res == osOK) {
    res = SL_WFX_CLI_ERROR_NONE;
  } else {
    // Presume it is a timeout error
    res = SL_WFX_CLI_ERROR_TIMEOUT;
  }

  return res;
}

static int sl_wfx_cli_generic_freertos_resume (void)
{
  int res;
  
  res = osSemaphoreRelease(event_sem);
  if (res == osOK) {
    res = SL_WFX_CLI_ERROR_NONE;
  } else {
    res = SL_WFX_CLI_ERROR;
  }
  
  return res;
}

#endif //FREERTOS


