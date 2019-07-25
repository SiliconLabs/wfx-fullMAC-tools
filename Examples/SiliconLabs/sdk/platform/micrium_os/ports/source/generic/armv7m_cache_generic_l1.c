/***************************************************************************//**
 * @file
 * @brief CPU - Cache Controller Driver - ARMv7-M L1 Cache
 *******************************************************************************
 * # License
 * <b>Copyright 2018 Silicon Laboratories Inc. www.silabs.com</b>
 *******************************************************************************
 *
 * The licensor of this software is Silicon Laboratories Inc.  Your use of this
 * software is governed by the terms of Silicon Labs Master Software License
 * Agreement (MSLA) available at
 * www.silabs.com/about-us/legal/master-software-license-agreement.  This
 * software is distributed to you in Source Code format and is governed by the
 * sections of the MSLA applicable to Source Code.
 *
 ******************************************************************************/

/****************************************************************************************************//**
 * @note     (1) This driver targets the following:
 *               Core      : ARMv7M Cortex-M with L1 cache
 *               Toolchain : Any
 *******************************************************************************************************/

/********************************************************************************************************
 ********************************************************************************************************
 *                                               INCLUDE FILES
 ********************************************************************************************************
 *******************************************************************************************************/

#include  <cpu/include/cpu_cache.h>
#include  <cpu/include/cpu.h>

#include  <common/include/lib_def.h>

#ifdef __cplusplus
extern  "C" {
#endif

/********************************************************************************************************
 ********************************************************************************************************
 *                                               LOCAL DEFINES
 ********************************************************************************************************
 *******************************************************************************************************/

//                                                                 ------ CACHE CONTROL IDENTIFICATION REGISTERS ------
#define  SCS_CLIDR      (*((CPU_REG32 *)(0xE000ED78u)))
#define  SCS_CTR        (*((CPU_REG32 *)(0xE000ED7Cu)))
#define  SCS_CCSIDR     (*((CPU_REG32 *)(0xE000ED80u)))
#define  SCS_CCSELR     (*((CPU_REG32 *)(0xE000ED84u)))
//                                                                 ----------- CACHE MAINTENANCE OPERATIONS -----------
#define  SCS_ICIALLU    (*((CPU_REG32 *)(0xE000EF50u)))         // Invalidate I-cache to PoU
#define  SCS_ICIMVAU    (*((CPU_REG32 *)(0xE000EF58u)))         // Invalidate I-cache to PoU by MVA
#define  SCS_DCIMVAC    (*((CPU_REG32 *)(0xE000EF5Cu)))         // Invalidate D-cache to PoC by MVA
#define  SCS_DCISW      (*((CPU_REG32 *)(0xE000EF60u)))         // Invalidate D-cache by Set/Way
#define  SCS_DCCMVAU    (*((CPU_REG32 *)(0xE000EF64u)))         // Clean D-cache to PoU by MVA
#define  SCS_DCCMVAC    (*((CPU_REG32 *)(0xE000EF68u)))         // Clean D-cache to PoC by MVA
#define  SCS_DCCSW      (*((CPU_REG32 *)(0xE000EF6Cu)))         // Clean D-cache by Set/Way
#define  SCS_DCCIMVAC   (*((CPU_REG32 *)(0xE000EF70u)))         // Clean and invalidate D-cache by MVA
#define  SCS_DCCISW     (*((CPU_REG32 *)(0xE000EF74u)))         // Clean and invalidate D-cache by Set/Way
#define  SCS_BPIALL     (*((CPU_REG32 *)(0xE000EF78u)))         // Invalidate Branch predictor

/********************************************************************************************************
 ********************************************************************************************************
 *                                       LOCAL FUNCTION PROTOTYPES
 ********************************************************************************************************
 *******************************************************************************************************/

static CPU_INT32U CPU_DCache_LineSizeGet(void);

/********************************************************************************************************
 ********************************************************************************************************
 *                                       LOCAL GLOBAL VARIABLES
 ********************************************************************************************************
 *******************************************************************************************************/

static CPU_INT32U CPU_Cache_Linesize;                           // Cache line size.

/********************************************************************************************************
 ********************************************************************************************************
 *                                           GLOBAL FUNCTIONS
 ********************************************************************************************************
 *******************************************************************************************************/

/****************************************************************************************************//**
 *                                               CPU_Cache_Init()
 *
 * @brief    Initialize CPU cache module.
 *******************************************************************************************************/
void CPU_Cache_Init(void)
{
  CPU_Cache_Linesize = CPU_DCache_LineSizeGet();
}

/****************************************************************************************************//**
 *                                           CPU_DCache_RangeInv()
 *
 * @brief    Invalidates a range of the data cache.
 *
 * @param    addr_start  Start address of the region to invalidate.
 *
 * @param    len         Size of the region to invalidate in bytes.
 *******************************************************************************************************/
void CPU_DCache_RangeInv(void     *addr_start,
                         CPU_ADDR len)
{
  //                                                               Align the address according to the line size.
  addr_start = (void *)((CPU_ADDR)addr_start & ~(CPU_Cache_Linesize - 1u));

  CPU_MB();

  while (len > CPU_Cache_Linesize) {
    SCS_DCIMVAC = (CPU_ADDR)addr_start;
    addr_start = (void *)((CPU_ADDR)addr_start + CPU_Cache_Linesize);
    len -= CPU_Cache_Linesize;
  }

  if (len > 0u) {
    SCS_DCIMVAC = (CPU_ADDR)addr_start;
  }

  CPU_MB();
}

/****************************************************************************************************//**
 *                                           CPU_DCache_RangeFlush()
 *
 * @brief    Flushes a range of the data cache to the main memory.
 *
 * @param    addr_start  Start address of the region to flush.
 *
 * @param    len         Size of the region to invalidate in bytes.
 *******************************************************************************************************/
void CPU_DCache_RangeFlush(void     *addr_start,
                           CPU_ADDR len)
{
  //                                                               Align the address according to the line size.
  addr_start = (void *)((CPU_ADDR)addr_start & ~(CPU_Cache_Linesize - 1u));

  CPU_MB();

  while (len > CPU_Cache_Linesize) {
    SCS_DCCMVAC = (CPU_ADDR)addr_start;
    addr_start = (void *)((CPU_ADDR)addr_start + CPU_Cache_Linesize);
    len -= CPU_Cache_Linesize;
  }

  if (len > 0u) {
    SCS_DCCMVAC = (CPU_ADDR)addr_start;
  }

  CPU_MB();
}

/********************************************************************************************************
 ********************************************************************************************************
 *                                               LOCAL FUNCTIONS
 ********************************************************************************************************
 *******************************************************************************************************/

/****************************************************************************************************//**
 *                                           CPU_DCache_LineSizeGet()
 *
 * @brief    Returns the cache line size.
 *
 * @return   The size of a cache line, in bytes.
 *
 * @note     (1) Line Size = 2^(CCSIDR[2:0] + 2)
 *******************************************************************************************************/
static CPU_INT32U CPU_DCache_LineSizeGet(void)
{
  return (1u << ((SCS_CCSIDR & 0x7u) + 2u));
}

#ifdef __cplusplus
}
#endif
