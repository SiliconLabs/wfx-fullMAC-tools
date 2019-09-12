#ifdef  SL_WFX_USE_SDIO
#include  <rtos_description.h>
#include "sl_wfx.h"
#include "sl_wfx_host_api.h"
#include "sl_wfx_bus.h"


#include "em_gpio.h"
#include "em_usart.h"
#include "em_cmu.h"
#include "em_ldma.h"
#include "em_bus.h"


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <kernel/include/os.h>
#include <common/include/rtos_utils.h>
#include <common/include/rtos_err.h>
#include <common/source/kal/kal_priv.h>

#include <common/include/rtos_err.h>

#include <io/include/sd.h>
#include <io/include/sd_card.h>

#include "net_drv_wifi_wfx_host_sdio_fnct.h"
#include "wfx_host_cfg.h"
#include "sleep.h"
#include "wfx_task.h"

#ifndef SL_WIFI_CFG_SD_CONTROLLER_NAME
#define SL_WIFI_CFG_SD_CONTROLLER_NAME             "sd0"
#endif

static SD_BUS_HANDLE SD_BusHandle = 0;
static bool sdioEnabled = false;
#ifdef SLEEP_ENABLED
static bool useWIRQ = false;
#endif
/****************************************************************************************************//**
 *                                     sl_wfx_host_init_bus()
 *
 * @brief    Initializes the communications bus.
 *
 * @return   sl_status_t    Error code of the operation as defined in sl_status.h.
 *******************************************************************************************************/
sl_status_t sl_wfx_host_init_bus(void)
{

  RTOS_ERR err;

  GPIO_PinOutSet(WFX_HOST_CFG_RESET_PORT, WFX_HOST_CFG_RESET_PIN);
#ifdef SLEEP_ENABLED
  SLEEP_SleepBlockBegin(sleepEM2);
#endif
  sdioEnabled = true;
  // Check to see if the bus was already added
  if (SD_BusHandleGetFromName(SL_WIFI_CFG_SD_CONTROLLER_NAME) == SD_BusHandleNull)
  {
    (void)SD_BusAdd(SL_WIFI_CFG_SD_CONTROLLER_NAME, &err);
    if (RTOS_ERR_CODE_GET(err) != RTOS_ERR_NONE) {
      return SL_ERROR;
    }
  }

  NetDev_SDIO_FnctInit(&err);

  if (SD_BusHandle == SD_BusHandleNull) {
    SD_BusHandle = SD_BusHandleGetFromName(SL_WIFI_CFG_SD_CONTROLLER_NAME);

    SD_BusStart(SD_BusHandle, &err);

    if (RTOS_ERR_CODE_GET(err) != RTOS_ERR_NONE) {
      return SL_ERROR;
    }
  }

  return SL_SUCCESS;
}

/****************************************************************************************************//**
 *                                     sl_wfx_host_deinit_bus()
 *
 * @brief    De-initializes the communications bus.
 *
 * @return   sl_status_t    Error code of the operation as defined in sl_status.h.
 *******************************************************************************************************/
sl_status_t sl_wfx_host_deinit_bus(void)
{
  RTOS_ERR err;


  if (SD_BusHandle != SD_BusHandleNull) {
    SD_BusStop(SD_BusHandle, &err);
    if (RTOS_ERR_CODE_GET(err) != RTOS_ERR_NONE) {
      return SL_ERROR;
    }
    SD_BusHandle = SD_BusHandleNull;
  }

  return SL_SUCCESS;
}


static sl_status_t sdio_io_write_direct(uint8_t function, uint32_t address, uint8_t* data)
{
	RTOS_ERR    err;
    sl_status_t result;

    NetDev_SDIO_FnctWrByte( address,
				         *data,
				         &err);

    if (RTOS_ERR_CODE_GET(err) == RTOS_ERR_NONE) {
    	result = SL_SUCCESS;
    } else {
    	result = SL_ERROR;
    }

    return result;
}


static sl_status_t sdio_io_read_direct(uint8_t function, uint32_t address, uint8_t* data)
{
    RTOS_ERR    err;
	sl_status_t result;

    *data = NetDev_SDIO_FnctRdByte(address, &err);

    if (RTOS_ERR_CODE_GET(err) == RTOS_ERR_NONE) {
    	result = SL_SUCCESS;
    } else {
    	result = SL_ERROR;
    }

    return result;
}


static sl_status_t sdio_io_write_extended( uint8_t function, uint32_t address, uint8_t* data, uint32_t data_length )
{
    RTOS_ERR    err;
	sl_status_t result;
    uint32_t    block_count;

    if (data_length >= 512) {
    	block_count = ( data_length / SL_WFX_SDIO_BLOCK_SIZE ) + ( ( ( data_length % SL_WFX_SDIO_BLOCK_SIZE ) == 0 ) ? 0 : 1 );
        NetDev_SDIO_FnctWrBlk(address,
                           data,
                           block_count,
                           1,
                           &err);
    } else {
        NetDev_SDIO_FnctWr(address,
                        data,
                        data_length,
                        1,
                        &err);
    }

    if (RTOS_ERR_CODE_GET(err) == RTOS_ERR_NONE) {
    	result = SL_SUCCESS;
    } else {
    	result = SL_ERROR;
    }

    return result;
}


static sl_status_t sdio_io_read_extended( uint8_t function, uint32_t address, uint8_t* data, uint32_t data_length )
{
    RTOS_ERR    err;
	sl_status_t result;
	uint32_t    block_count;

    if (data_length >= 512) {
    	block_count = ( data_length / SL_WFX_SDIO_BLOCK_SIZE ) + ( ( ( data_length % SL_WFX_SDIO_BLOCK_SIZE ) == 0 ) ? 0 : 1 );

        NetDev_SDIO_FnctRdBlk(address,
                           data,
                           block_count,
                           1,
                           &err);
    } else {
        NetDev_SDIO_FnctRd(address,
                        data,
                        data_length,
                        1,
                        &err);
    }

    if (RTOS_ERR_CODE_GET(err) == RTOS_ERR_NONE) {
    	result = SL_SUCCESS;
    } else {
    	result = SL_ERROR;
    }

    return result;
}


sl_status_t sl_wfx_host_sdio_transfer_cmd52( sl_wfx_host_bus_tranfer_type_t type, uint8_t function, uint32_t address, uint8_t* buffer )
{
  sl_status_t status;

  if (type == SL_WFX_BUS_WRITE){
    status = sdio_io_write_direct(function, address, buffer);
  } else {
    status = sdio_io_read_direct(function, address, buffer);
  }

  return status;
}


sl_status_t sl_wfx_host_sdio_transfer_cmd53( sl_wfx_host_bus_tranfer_type_t type, uint8_t function, uint32_t address, uint8_t* buffer, uint16_t buffer_length )
{
  sl_status_t status;

  if (type == SL_WFX_BUS_WRITE){
    status = sdio_io_write_extended(function, address, buffer, buffer_length);
  } else {
    status = sdio_io_read_extended(function, address, buffer, buffer_length);
  }

  return status;
}


sl_status_t sl_wfx_host_sdio_enable_high_speed_mode(void)
{
    sl_status_t result;
    uint8_t     value_u8;

    result = sdio_io_read_direct(0, 0x13, &value_u8);
    value_u8 |= 0x2; // Set EHS to 1
    result = sdio_io_write_direct(0, 0x13, &value_u8);
    SDIO->HOSTCTRL1 |= SDIO_HOSTCTRL1_HSEN; // Enable HS mode at the host
    // Swap SDIOCLK source to USHFRCO. Now clocked at 50MHz
    //enable wakeup
    //SDIO->HOSTCTRL1 |= SDIO_HOSTCTRL1_WKUPEVNTENONCARDINT;
    return result;
}


static void sdio_irq_callback(void* arg)
{
	RTOS_ERR err;
	OSFlagPost(&wf200_evts, WF200_EVENT_FLAG_RX,OS_OPT_POST_FLAG_SET,&err);
}



/****************************************************************************************************//**
 *                                  sl_wfx_host_enable_platform_interrupt()
 *
 * @brief    Enable interrupts on the host side.
 *
 * @return   sl_status_t    Error code of the operation as defined in sl_status.h.
 *******************************************************************************************************/
sl_status_t sl_wfx_host_enable_platform_interrupt(void)
{
#ifdef SLEEP_ENABLED
    if (useWIRQ)
    {
        //GPIO_ExtIntConfig(WFX_HOST_CFG_WIRQPORT, WFX_HOST_CFG_WIRQPIN, WFX_HOST_CFG_IRQ, true, false, true);
        return SL_SUCCESS;
    }
    else
#endif
    {

        RTOS_ERR err;
        NetDev_SDIO_FnctIntReg(sdio_irq_callback);

        NetDev_SDIO_FnctIntEnDis(1, &err);
        SDIO->IEN |= SDIO_IFCR_CARDINT;
        SDIO->IFENC |= SDIO_IFENC_CARDINTEN;

        if (RTOS_ERR_CODE_GET(err) == RTOS_ERR_NONE) {
          return SL_SUCCESS;
        }
    }


    return SL_ERROR;

}

/****************************************************************************************************//**
 *                                 sl_wfx_host_disable_platform_interrupt()
 *
 * @brief    Disable interrupts on the host side.
 *
 * @return   sl_status_t    Error code of the operation as defined in sl_status.h.
 *******************************************************************************************************/
sl_status_t sl_wfx_host_disable_platform_interrupt(void)
{
#ifdef SLEEP_ENABLED
  if (useWIRQ)
  {
      GPIO_IntDisable(WFX_HOST_CFG_IRQ);
      return SL_SUCCESS;
  }
  else
#endif
  {
      RTOS_ERR err;

      SDIO->IEN &= ~(SDIO_IFCR_CARDINT);
      SDIO->IFENC &= ~(SDIO_IFENC_CARDINTEN);
      NetDev_SDIO_FnctIntEnDis(0, &err);

      if (RTOS_ERR_CODE_GET(err) == RTOS_ERR_NONE) {
        return SL_SUCCESS;
      }
  }
  return SL_ERROR;

}
#ifdef SLEEP_ENABLED
sl_status_t sl_wfx_host_switch_to_wirq (void)
{
	uint32_t value32;

	GPIO_ExtIntConfig(WFX_HOST_CFG_WIRQPORT, WFX_HOST_CFG_WIRQPIN, WFX_HOST_CFG_IRQ, true, false, true);
	sl_wfx_reg_read_32(SL_WFX_CONFIG_REG_ID, &value32);
	value32 |= (1<<15);
    sl_wfx_reg_write_32(SL_WFX_CONFIG_REG_ID, value32);
	useWIRQ = true;
	return SL_SUCCESS;
}

sl_status_t sl_wfx_host_enable_sdio (void)
{
	if (sdioEnabled == false)
	{
	  SLEEP_SleepBlockBegin(sleepEM2);
	  CMU_OscillatorEnable(cmuOsc_AUXHFRCO, true, true);
	  // Re-enable SDIO clock
	  CMU_ClockEnable(cmuClock_SDIO, true);
	  while ((CMU->STATUS & CMU_STATUS_SDIOCLKENS) == 0UL);
	  SDIO->CLOCKCTRL |= SDIO_CLOCKCTRL_INTCLKEN;
	  while((SDIO->CLOCKCTRL & SDIO_CLOCKCTRL_INTCLKSTABLE) == 0);
	  SDIO->CLOCKCTRL |= SDIO_CLOCKCTRL_SDCLKEN;
	  sdioEnabled = true;
	}
	return SL_SUCCESS;
}

sl_status_t sl_wfx_host_disable_sdio (void)
{
	if (sdioEnabled == true)
	{
	  // Disable internal SDIO peripheral clock
	  SDIO->CLOCKCTRL &= ~(_SDIO_CLOCKCTRL_SDCLKEN_MASK | _SDIO_CLOCKCTRL_INTCLKEN_MASK);
	  // Disable MCU clock tree SDIO clock
	  CMU_ClockEnable(cmuClock_SDIO, false);
	  while ((CMU->STATUS & CMU_STATUS_SDIOCLKENS) != 0);
	  CMU_OscillatorEnable(cmuOsc_AUXHFRCO, false, true);
	  sdioEnabled = false;
	  SLEEP_SleepBlockEnd(sleepEM2);
	}
	return SL_SUCCESS;
}

#endif

#endif
