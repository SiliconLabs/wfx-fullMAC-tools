/***************************************************************************//**
 * @file
 * @brief Network Wi-Fi Driver for the WFX over SDIO
 *******************************************************************************
 * # License
 * <b>Copyright 2019 Silicon Laboratories Inc. www.silabs.com</b>
 *******************************************************************************
 *
 * The licensor of this software is Silicon Laboratories Inc.  Your use of this
 * software is governed by the terms of Silicon Labs Master Software License
 * Agreement (MSLA) available at
 * www.silabs.com/about-us/legal/master-software-license-agreement.
 * The software is governed by the sections of the MSLA applicable to Micrium
 * Software.
 *
 ******************************************************************************/

/*
*********************************************************************************************************
*********************************************************************************************************
*                                       DEPENDENCIES & AVAIL CHECK(S)
*********************************************************************************************************
*********************************************************************************************************
*/

#include  <rtos_description.h>

#if (defined(RTOS_MODULE_IO_SD_AVAIL))

#if (!defined(RTOS_MODULE_IO_AVAIL))

#error IO SD module requires IO module. Make sure it is part of your project and that \
        RTOS_MODULE_IO_AVAIL is defined in rtos_description.h.

#endif


/*
*********************************************************************************************************
*********************************************************************************************************
*                                               INCLUDES
*********************************************************************************************************
*********************************************************************************************************
*/

#include  <stdio.h>

#include  <cpu/include/cpu.h>
#include  <common/include/lib_mem.h>
#include  <common/include/rtos_err.h>
#include  <common/source/rtos/rtos_utils_priv.h>
#include  <common/source/kal/kal_priv.h>


#include  <io/include/sd.h>
#include  <io/source/sd/sd_io_fnct_priv.h>

#include  <sl_wfx_constants.h>

#include  "net_drv_wifi_wfx_host_sdio_fnct.h"

/*
*********************************************************************************************************
*********************************************************************************************************
*                                               LOCAL DEFINES
*********************************************************************************************************
*********************************************************************************************************
*/

#define  RTOS_MODULE_CUR                    RTOS_CFG_MODULE_IO

#define  NET_DEV_SDIO_FNCT_BUF_LEN                 48u
#define  NET_DEV_SDIO_FNCT_BLK_LEN                  4u
#define  NET_DEV_SDIO_FNCT_BLK_QTY                 64u


/*
*********************************************************************************************************
*********************************************************************************************************
*                                       LOCAL GLOBAL VARIABLES
*********************************************************************************************************
*********************************************************************************************************
*/

KAL_SEM_HANDLE  NetDev_SDIO_FnctSem;

SD_BUS_HANDLE   NetDev_SDIO_FnctBusHandle;

SD_FNCT_HANDLE  NetDev_SDIO_FnctHandle;

CPU_INT08U      NetDev_SDIO_FnctBuf[NET_DEV_SDIO_FNCT_BUF_LEN];
CPU_INT08U      NetDev_SDIO_FnctRefBuf[NET_DEV_SDIO_FNCT_BUF_LEN];

CPU_INT08U      NetDev_SDIO_FnctBlkBuf[NET_DEV_SDIO_FNCT_BLK_LEN * NET_DEV_SDIO_FNCT_BLK_QTY];
CPU_INT08U      NetDev_SDIO_FnctRefBlkBuf[NET_DEV_SDIO_FNCT_BLK_LEN * NET_DEV_SDIO_FNCT_BLK_QTY];


/*
*********************************************************************************************************
*********************************************************************************************************
*                                           LOCAL DATA TYPES
*********************************************************************************************************
*********************************************************************************************************
*/


/*
*********************************************************************************************************
*********************************************************************************************************
*                                               LOCAL TABLES
*********************************************************************************************************
*********************************************************************************************************
*/


/*
*********************************************************************************************************
*********************************************************************************************************
*                                       LOCAL FUNCTION PROTOTYPES
*********************************************************************************************************
*********************************************************************************************************
*/

static void (*NetDev_SDIO_FnctCardIntCallback)(void);

static  CPU_BOOLEAN  NetDev_SDIO_FnctProbe  (SD_BUS_HANDLE     bus_handle,
                                             SD_FNCT_HANDLE    fnct_handle,
                                             CPU_INT08U        sdio_fnct_if_code,
                                             void            **pp_fnct_data);

static  void         NetDev_SDIO_FnctConn   (SD_FNCT_HANDLE    fnct_handle,
                                             void             *p_fnct_data);

static  void         NetDev_SDIO_FnctInt    (SD_FNCT_HANDLE    fnct_handle,
                                             void             *p_fnct_data);

static  void         NetDev_SDIO_FnctDisconn(SD_FNCT_HANDLE    fnct_handle,
                                             void             *p_fnct_data);


/*
*********************************************************************************************************
*                                           LOCAL FUNCTIONS
*********************************************************************************************************
*/

const  SD_IO_FNCT_DRV_API  NetDev_SDIO_FnctDrvAPI = {
    .CardFnctProbe   = NetDev_SDIO_FnctProbe,
    .CardFnctConn    = NetDev_SDIO_FnctConn,
    .CardFnctDisconn = NetDev_SDIO_FnctDisconn,
    .CardFnctInt     = NetDev_SDIO_FnctInt
};


/*
*********************************************************************************************************
*********************************************************************************************************
*                                           GLOBAL FUNCTIONS
*********************************************************************************************************
*********************************************************************************************************
*/

void  NetDev_SDIO_FnctTask (void *p_arg);

void  NetDev_SDIO_FnctInit(RTOS_ERR  *p_err)
{
    KAL_TASK_HANDLE  task_handle;


    NetDev_SDIO_FnctCardIntCallback = DEF_NULL;

    SD_IO_FnctDrvReg(&NetDev_SDIO_FnctDrvAPI, p_err);

    NetDev_SDIO_FnctSem = KAL_SemCreate("Net Dev SDIO Fnct Sem", DEF_NULL, p_err);
    if (RTOS_ERR_CODE_GET(*p_err) != RTOS_ERR_NONE) {
        return;
    }

    task_handle = KAL_TaskAlloc("Net Dev SDIO Fnct Task", DEF_NULL, 512u, DEF_NULL, p_err);
    if (RTOS_ERR_CODE_GET(*p_err) != RTOS_ERR_NONE) {
        return;
    }

    KAL_TaskCreate(task_handle, NetDev_SDIO_FnctTask, DEF_NULL, 3u, DEF_NULL, p_err);
    if (RTOS_ERR_CODE_GET(*p_err) != RTOS_ERR_NONE) {
        return;
    }
}


/*
*********************************************************************************************************
*********************************************************************************************************
*                                           LOCAL FUNCTIONS
*********************************************************************************************************
*********************************************************************************************************
*/

void NetDev_SDIO_FnctIntReg (void* callback)
{
	NetDev_SDIO_FnctCardIntCallback = callback;
}


static CPU_BOOLEAN NetDev_SDIO_FnctProbe (SD_BUS_HANDLE     bus_handle,
                                          SD_FNCT_HANDLE    fnct_handle,
                                          CPU_INT08U        sdio_fnct_if_code,
                                          void            **pp_fnct_data)
{
    NetDev_SDIO_FnctBusHandle = bus_handle;
    NetDev_SDIO_FnctHandle = fnct_handle;

    return (DEF_OK);
}


static void NetDev_SDIO_FnctConn (SD_FNCT_HANDLE   fnct_handle,
                                  void            *p_fnct_data)
{
    RTOS_ERR  err;

    KAL_SemPost(NetDev_SDIO_FnctSem, KAL_OPT_POST_NONE, &err);
}


static void NetDev_SDIO_FnctDisconn(SD_FNCT_HANDLE   fnct_handle,
                                    void            *p_fnct_data)
{

}


CPU_INT08U NetDev_SDIO_FnctRdByte (CPU_INT32U  reg_addr,
                                   RTOS_ERR   *p_err)
{
    CPU_INT08U  byte;

	byte = SD_IO_FnctRdByte(NetDev_SDIO_FnctBusHandle,
	                        NetDev_SDIO_FnctHandle,
	                        reg_addr,
	                        p_err);

	return (byte);
}


void NetDev_SDIO_FnctWrByte (CPU_INT32U  reg_addr,
                             CPU_INT08U  byte,
                             RTOS_ERR   *p_err)
{
	SD_IO_FnctWrByte(NetDev_SDIO_FnctBusHandle,
	                 NetDev_SDIO_FnctHandle,
	                 reg_addr,
	                 byte,
	                 p_err);
}


void NetDev_SDIO_FnctRd (CPU_INT32U   reg_addr,
                         CPU_INT08U  *p_buf,
                         CPU_INT16U   buf_len,
                         CPU_BOOLEAN  fixed_addr,
                         RTOS_ERR    *p_err)
{
	SD_IO_FnctRd(NetDev_SDIO_FnctBusHandle,
	             NetDev_SDIO_FnctHandle,
	             reg_addr,
	             p_buf,
	             buf_len,
	             fixed_addr,
	             p_err);
}


void NetDev_SDIO_FnctWr (CPU_INT32U    reg_addr,
                         CPU_INT08U   *p_buf,
                         CPU_INT16U    buf_len,
                         CPU_BOOLEAN   fixed_addr,
                         RTOS_ERR     *p_err)
{
	SD_IO_FnctWr(NetDev_SDIO_FnctBusHandle,
	             NetDev_SDIO_FnctHandle,
                 reg_addr,
	             p_buf,
	             buf_len,
	             fixed_addr,
	             p_err);
}

void NetDev_SDIO_FnctRdBlk (CPU_INT32U    reg_addr,
                            CPU_INT08U   *p_buf,
                            CPU_INT16U    blk_nbr,
                            CPU_BOOLEAN   fixed_addr,
                            RTOS_ERR     *p_err)
{
	SD_IO_FnctRdBlk(NetDev_SDIO_FnctBusHandle,
	                NetDev_SDIO_FnctHandle,
	                reg_addr,
	                p_buf,
	                blk_nbr,
	                fixed_addr,
	                p_err);

}

void NetDev_SDIO_FnctWrBlk (CPU_INT32U   reg_addr,
                            CPU_INT08U  *p_buf,
                            CPU_INT16U   blk_nbr,
                            CPU_BOOLEAN  fixed_addr,
                            RTOS_ERR    *p_err)
{
	SD_IO_FnctWrBlk(NetDev_SDIO_FnctBusHandle,
  	                NetDev_SDIO_FnctHandle,
                    reg_addr,
	                p_buf,
	                blk_nbr,
	                fixed_addr,
	                p_err);
}

static void NetDev_SDIO_FnctInt (SD_FNCT_HANDLE  fnct_handle,
                                 void           *p_fnct_data)
{
	if (NetDev_SDIO_FnctCardIntCallback != DEF_NULL) {
	    NetDev_SDIO_FnctCardIntCallback();
	}
}

void NetDev_SDIO_FnctIntEnDis (CPU_BOOLEAN  enable,
                               RTOS_ERR    *p_err)
{
	SD_IO_FnctIntEnDis(NetDev_SDIO_FnctBusHandle,
                       NetDev_SDIO_FnctHandle,
                       enable,
                       p_err);
}

void NetDev_SDIO_FnctTask (void *p_arg)
{
    RTOS_ERR     err;
    CPU_BOOLEAN  is_blk_mode_ok;

    while (1) {

        KAL_SemPend(NetDev_SDIO_FnctSem, KAL_OPT_PEND_NONE, 0, &err);

        Mem_Set(NetDev_SDIO_FnctBuf, 0xFF, NET_DEV_SDIO_FNCT_BUF_LEN);
        Mem_Set(NetDev_SDIO_FnctBlkBuf, 0xFF, NET_DEV_SDIO_FNCT_BLK_LEN * NET_DEV_SDIO_FNCT_BLK_QTY);

        is_blk_mode_ok = SD_IO_IsBlkOperSupported(NetDev_SDIO_FnctBusHandle, &err);
        if (RTOS_ERR_CODE_GET(err) != RTOS_ERR_NONE) {
            CPU_SW_EXCEPTION(;);
        }

        if (is_blk_mode_ok) {
            SD_IO_FnctBlkSizeSet(NetDev_SDIO_FnctBusHandle, NetDev_SDIO_FnctHandle, SL_WFX_SDIO_BLOCK_SIZE, &err);
            if (RTOS_ERR_CODE_GET(err) != RTOS_ERR_NONE) {
                CPU_SW_EXCEPTION(;);
            }

            SD_IO_FnctBlkSizeSet(NetDev_SDIO_FnctBusHandle, SD_IO_FNCT_0, SL_WFX_SDIO_BLOCK_SIZE, &err);
            if (RTOS_ERR_CODE_GET(err) != RTOS_ERR_NONE) {
                CPU_SW_EXCEPTION(;);
            }
        }
    }
}

/*
*********************************************************************************************************
*********************************************************************************************************
*                                   DEPENDENCIES & AVAIL CHECK(S) END
*********************************************************************************************************
*********************************************************************************************************
*/

#endif  /* (defined(RTOS_MODULE_IO_SD_AVAIL)) */
