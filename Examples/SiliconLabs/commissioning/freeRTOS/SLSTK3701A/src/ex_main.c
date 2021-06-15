/***************************************************************************//**
 * @file
 * @brief Example main
 *******************************************************************************
 * # License
 * <b>Copyright 2019 Silicon Laboratories Inc. www.silabs.com</b>
 *******************************************************************************
 *
 * The licensor of this software is Silicon Laboratories Inc. Your use of this
 * software is governed by the terms of Silicon Labs Master Software License
 * Agreement (MSLA) available at
 * www.silabs.com/about-us/legal/master-software-license-agreement.
 * The software is governed by the sections of the MSLA applicable to Micrium
 * Software.
 *
 ******************************************************************************/

#include <stdio.h>
#include <stdarg.h>

#include "em_cmu.h"
#include "em_emu.h"
#include "em_chip.h"
#include "sleep.h"
#include "bsp.h"
#include "bsp_trace.h"
#include "bspconfig.h"
#include "retargetserial.h"

#include "cmsis_os.h"

#include "demo_config.h"

#include "sl_wfx_task.h"
#include "sl_wfx_host.h"
#include "sl_wfx_host_cfg.h"
#include "sl_wfx_host_events.h"

#ifdef SL_WFX_USE_SECURE_LINK
#include  <mbedtls/threading.h>
extern void wfx_securelink_task_start(void);
#endif

extern osThreadId busCommTaskHandle;


#ifdef SLEEP_ENABLED
static bool sleepCallback(SLEEP_EnergyMode_t emode)
{
#ifdef SL_WFX_USE_SPI
  if (GPIO_PinInGet(SL_WFX_HOST_CFG_SPI_WIRQPORT,  SL_WFX_HOST_CFG_SPI_WIRQPIN))//wf200 messages pending
#else
    if (GPIO_PinInGet(SL_WFX_HOST_CFG_WIRQPORT,  SL_WFX_HOST_CFG_WIRQPIN)) //wf200 messages pending
#endif
    {
      return false;
    }

  return true;
}

static void wakeupCallback(SLEEP_EnergyMode_t emode)
{

}
#endif


static void gpio_setup(void);
/**************************************************************************//**
 * Main function
 *****************************************************************************/
int  main(void)
{
  CHIP_Init();       // Initialize CPU.

  // Set the HFRCO frequency.
  CMU_HFRCOFreqSet(cmuHFRCOFreq_72M0Hz);
  // Init DCDC regulator
  EMU_DCDCInit_TypeDef dcdcInit = EMU_DCDCINIT_STK_DEFAULT;
  // Initialize DCDC regulator
  dcdcInit.dcdcMode = emuDcdcMode_LowNoise;
  EMU_DCDCInit(&dcdcInit);

#ifdef SL_WFX_USE_SPI
  CMU_ClockPrescSet(cmuClock_HFPER, 0);
#endif

  RETARGET_SerialInit();
  RETARGET_SerialCrLf(1);
#ifdef SLEEP_ENABLED
  const SLEEP_Init_t sleepInit =
  {
    .sleepCallback = sleepCallback,
    .wakeupCallback = wakeupCallback,
    .restoreCallback = 0
  };
  SLEEP_InitEx(&sleepInit);
#endif

#ifdef SL_WFX_USE_SECURE_LINK
  // Enable mbedtls FreeRTOS support
#if defined ( MBEDTLS_THREADING_C )
  THREADING_setup();
#endif
#endif

#ifdef SLEEP_ENABLED
  // Don't allow EM3, since we use LF clocks.
  SLEEP_SleepBlockBegin(sleepEM3);
#endif

  gpio_setup();

  BSP_LedsInit();
  // Clear the console and buffer
  printf("\033\143");
  printf("\033[3J");
  printf("WF200 FreeRTOS LwIP Example\n");

  // Start wfx bus communication task.


  wfx_events_task_start();
  wfx_bus_start();
#ifdef SL_WFX_USE_SECURE_LINK
  wfx_securelink_task_start(); // start securelink key renegotiation task
#endif //SL_WFX_USE_SECURE_LINK

  lwip_start();

  // Start scheduler
  osKernelStart();

  // We should never get here as control is now taken by the scheduler

  while (1)
  {
  }

}


/**************************************************************************//**
 * Unified GPIO interrupt handler.
 *****************************************************************************/
static void GPIO_Unified_IRQ(void)
{
  BaseType_t xHigherPriorityTaskWoken;
  /* xHigherPriorityTaskWoken must be initialised to pdFALSE. */
  xHigherPriorityTaskWoken = pdFALSE;
  // Get and clear all pending GPIO interrupts
  uint32_t interrupt_mask = GPIO_IntGet();
  GPIO_IntClear(interrupt_mask);

  // Act on interrupts
  if (interrupt_mask & 0x400) {
    xSemaphoreGiveFromISR(wfx_wakeup_sem, &xHigherPriorityTaskWoken);
#ifdef  SL_WFX_USE_SPI
    xEventGroupSetBitsFromISR(sl_wfx_event_group,
                              SL_WFX_RX_PACKET_AVAILABLE,
                              &xHigherPriorityTaskWoken);
#endif
#ifdef  SL_WFX_USE_SDIO
#ifdef  SLEEP_ENABLED
    xEventGroupSetBitsFromISR(sl_wfx_event_group,
                                  SL_WFX_RX_PACKET_AVAILABLE,
                                  &xHigherPriorityTaskWoken);
#endif
#endif

  }
  if (interrupt_mask & (1 << BSP_GPIO_PB0_PIN)) {
    BSP_LedToggle(0);
  }

  if (interrupt_mask & (1 << BSP_GPIO_PB1_PIN)) {
    BSP_LedToggle(1);
  }
  portYIELD_FROM_ISR( xHigherPriorityTaskWoken );
}

/**************************************************************************//**
 * GPIO even interrupt handler.
 *****************************************************************************/
void GPIO_EVEN_IRQHandler(void)
{
  GPIO_Unified_IRQ();
}

/**************************************************************************//**
 * GPIO odd interrupt handler.
 *****************************************************************************/
void GPIO_ODD_IRQHandler(void)
{
  GPIO_Unified_IRQ();
}

/**************************************************************************//**
 * Configure the GPIO pins.
 *****************************************************************************/
static void gpio_setup(void)
{
  // Enable GPIO clock.
  CMU_ClockEnable(cmuClock_GPIO, true);

  // Configure PB0 and PB1 as inputs (present on the Wireless Radio board in WGM160P case).
  GPIO_PinModeSet(BSP_GPIO_PB0_PORT, BSP_GPIO_PB0_PIN, gpioModeInput, 0);
  GPIO_PinModeSet(BSP_GPIO_PB1_PORT, BSP_GPIO_PB1_PIN, gpioModeInput, 0);
  // Enable interrupts.
  GPIO_IntConfig(BSP_GPIO_PB0_PORT, BSP_GPIO_PB0_PIN, false, true, true);
  GPIO_IntConfig(BSP_GPIO_PB1_PORT, BSP_GPIO_PB1_PIN, false, true, true);

  // Configure WF200 reset pin.
  GPIO_PinModeSet(SL_WFX_HOST_CFG_RESET_PORT, SL_WFX_HOST_CFG_RESET_PIN, gpioModePushPull, 0);
  // Configure WF200 WUP pin.
  GPIO_PinModeSet(SL_WFX_HOST_CFG_WUP_PORT, SL_WFX_HOST_CFG_WUP_PIN, gpioModePushPull, 0);
#ifdef  SL_WFX_USE_SPI
  // GPIO used as IRQ.
  GPIO_PinModeSet(SL_WFX_HOST_CFG_SPI_WIRQPORT, SL_WFX_HOST_CFG_SPI_WIRQPIN, gpioModeInputPull, 0);
#endif
  CMU_OscillatorEnable(cmuOsc_LFXO, true, true);
#ifdef EFM32GG11B820F2048GM64 //WGM160PX22KGA2
  // GPIO used as IRQ
  GPIO_PinModeSet(SL_WFX_HOST_CFG_WIRQPORT,  SL_WFX_HOST_CFG_WIRQPIN,  gpioModeInputPull,  0);
  // SDIO Pull-ups
  GPIO_PinModeSet(gpioPortD,  0,  gpioModeDisabled,  1);
  GPIO_PinModeSet(gpioPortD,  1,  gpioModeDisabled,  1);
  GPIO_PinModeSet(gpioPortD,  2,  gpioModeDisabled,  1);
  GPIO_PinModeSet(gpioPortD,  3,  gpioModeDisabled,  1);
  GPIO_PinModeSet(gpioPortD,  5,  gpioModeDisabled,  1);
  //WF200 LF CLK
  CMU->CTRL      |= CMU_CTRL_CLKOUTSEL0_LFXO;
  CMU->ROUTEPEN  |= CMU_ROUTEPEN_CLKOUT0PEN;
  CMU->ROUTELOC0 |= CMU_ROUTELOC0_CLKOUT0LOC_LOC5;
  GPIO_PinModeSet(LP_CLK_PORT,  LP_CLK_PIN,  gpioModePushPull,  0);
#endif
  // Reset and enable associated CPU interrupt vector.
  NVIC_ClearPendingIRQ(GPIO_ODD_IRQn);
  NVIC_EnableIRQ(GPIO_ODD_IRQn);
  NVIC_ClearPendingIRQ(GPIO_EVEN_IRQn);
  NVIC_SetPriority(GPIO_EVEN_IRQn,1);
  NVIC_EnableIRQ(GPIO_EVEN_IRQn);
}

void vApplicationStackOverflowHook (TaskHandle_t xTask, signed char *pcTaskName) {
  (void)xTask;
  (void)pcTaskName;
  configASSERT(0);
}
/**
 * @brief User defined assertion call. This function is plugged into configASSERT.
 * See FreeRTOSConfig.h to define configASSERT to something different.
 */
void vAssertCalled(const char * pcFile,
	uint32_t ulLine)
{
    /* FIX ME. If necessary, update to applicable assertion routine actions. */

	const uint32_t ulLongSleep = 1000UL;
	volatile uint32_t ulBlockVariable = 0UL;
	volatile char * pcFileName = (volatile char *)pcFile;
	volatile uint32_t ulLineNumber = ulLine;

	(void)pcFileName;
	(void)ulLineNumber;

	printf("vAssertCalled %s, %ld\n", pcFile, (long)ulLine);
	fflush(stdout);

	/* Setting ulBlockVariable to a non-zero value in the debugger will allow
	* this function to be exited. */
	taskDISABLE_INTERRUPTS();
	{
		while (ulBlockVariable == 0UL)
		{
			vTaskDelay( pdMS_TO_TICKS( ulLongSleep ) );
		}
	}
	taskENABLE_INTERRUPTS();
}
