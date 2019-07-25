/***************************************************************************//**
 * @file
 * @brief Kernel - Memory Partition Management
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

/********************************************************************************************************
 ********************************************************************************************************
 *                                       DEPENDENCIES & AVAIL CHECK(S)
 ********************************************************************************************************
 *******************************************************************************************************/

#include  <rtos_description.h>

#if (defined(RTOS_MODULE_KERNEL_AVAIL))

/********************************************************************************************************
 ********************************************************************************************************
 *                                               INCLUDE FILES
 ********************************************************************************************************
 *******************************************************************************************************/

#define   MICRIUM_SOURCE
#include "../include/os.h"
#include "os_priv.h"

#ifdef VSC_INCLUDE_SOURCE_FILE_NAMES
const CPU_CHAR *os_mem__c = "$Id: $";
#endif

/********************************************************************************************************
 ********************************************************************************************************
 *                                       DEPRECATED GLOBAL FUNCTIONS
 ********************************************************************************************************
 *******************************************************************************************************/

#if (OS_CFG_MEM_EN == DEF_ENABLED)
/****************************************************************************************************//**
 *                                               OSMemCreate()
 *
 * @brief    Creates a fixed-sized memory partition that will be managed by the Kernel.
 *
 * @param    p_mem       Pointer to a memory partition control block which is allocated in user
 *                       memory space.
 *
 * @param    p_name      Pointer to an ASCII string to provide a name to the memory partition.
 *
 * @param    p_addr      The starting address of the memory partition.
 *
 * @param    n_blks      The number of memory blocks to create from the partition.
 *
 * @param    blk_size    The size (in bytes) of each block in the memory partition.
 *
 * @param    p_err       Pointer to the variable that will receive one of the following error code(s)
 *                       from this function:
 *                           - RTOS_ERR_NONE
 *                           - RTOS_ERR_OS_ILLEGAL_RUN_TIME
 *
 * @note     (1) This function is DEPRECATED and will be removed in a future version of this product.
 *******************************************************************************************************/
void OSMemCreate(OS_MEM      *p_mem,
                 CPU_CHAR    *p_name,
                 void        *p_addr,
                 OS_MEM_QTY  n_blks,
                 OS_MEM_SIZE blk_size,
                 RTOS_ERR    *p_err)
{
  OS_MEM_QTY i;
  OS_MEM_QTY loops;
  CPU_INT08U *p_blk;
  void       **p_link;
  CPU_SR_ALLOC();

  OS_ASSERT_DBG_ERR_PTR_VALIDATE(p_err,; );

#ifdef OS_SAFETY_CRITICAL_IEC61508
  if (OSSafetyCriticalStartFlag == DEF_TRUE) {
    RTOS_ERR_SET(*p_err, RTOS_ERR_OS_ILLEGAL_RUN_TIME);
    return;
  }
#endif

  //                                                               Not allowed to call from an ISR
  OS_ASSERT_DBG_ERR_SET((OSIntNestingCtr == 0u), *p_err, RTOS_ERR_ISR,; );

  //                                                               Must pass a valid address for the memory part.
  OS_ASSERT_DBG_ERR_SET((p_addr != DEF_NULL), *p_err, RTOS_ERR_NULL_PTR,; );

  //                                                               Must have at least 2 blocks per partition
  OS_ASSERT_DBG_ERR_SET((n_blks >= 2u), *p_err, RTOS_ERR_INVALID_ARG,; );

  //                                                               Must contain space for at least a pointer
  OS_ASSERT_DBG_ERR_SET((blk_size >= sizeof(void *)), *p_err, RTOS_ERR_INVALID_ARG,; );

#if (OS_ARG_CHK_EN == DEF_ENABLED)
  {
    CPU_DATA align_msk = sizeof(void *) - 1u;

    if (align_msk > 0u) {
      //                                                           Must be pointer size aligned
      OS_ASSERT_DBG_ERR_SET((((CPU_ADDR)p_addr & align_msk) == 0u), *p_err, RTOS_ERR_INVALID_ARG,; );

      //                                                           Block size must be a multiple address size
      OS_ASSERT_DBG_ERR_SET(((blk_size & align_msk) == 0u), *p_err, RTOS_ERR_INVALID_ARG,; );
    }
  }
#endif

  p_link = (void **)p_addr;                                     // Create linked list of free memory blocks
  p_blk = (CPU_INT08U *)p_addr;
  loops = n_blks - 1u;
  for (i = 0u; i < loops; i++) {
    p_blk += blk_size;
    *p_link = (void *)p_blk;                                    // Save pointer to NEXT block in CURRENT block
    p_link = (void **)(void *)p_blk;                            // Position     to NEXT block
  }
  *p_link = DEF_NULL;                                           // Last memory block points to NULL

  CPU_CRITICAL_ENTER();
#if (OS_OBJ_TYPE_REQ == DEF_ENABLED)
  p_mem->Type = OS_OBJ_TYPE_MEM;                                // Set the type of object
#endif
#if (OS_CFG_DBG_EN == DEF_ENABLED)
  p_mem->NamePtr = p_name;                                      // Save name of memory partition
#else
  (void)&p_name;
#endif
  p_mem->AddrPtr = p_addr;                                      // Store start address of memory partition
  p_mem->FreeListPtr = p_addr;                                  // Initialize pointer to pool of free blocks
  p_mem->NbrFree = n_blks;                                      // Store number of free blocks in MCB
  p_mem->NbrMax = n_blks;
  p_mem->BlkSize = blk_size;                                    // Store block size of each memory blocks

#if (OS_CFG_DBG_EN == DEF_ENABLED)
  OS_MemDbgListAdd(p_mem);
  OSMemQty++;
#endif

  OS_TRACE_MEM_CREATE(p_mem, p_name);

  CPU_CRITICAL_EXIT();
  RTOS_ERR_SET(*p_err, RTOS_ERR_NONE);
}

/****************************************************************************************************//**
 *                                               OSMemGet()
 *
 * @brief    Gets a memory block from a partition.
 *
 * @param    p_mem   Pointer to the memory partition control block.
 *
 * @param    p_err   Pointer to the variable that will receive one of the following error code(s)
 *                   from this function:
 *                       - RTOS_ERR_NONE
 *                       - RTOS_ERR_POOL_EMPTY
 *
 * @return   A pointer to a memory block if no error is detected.
 *           DEF_NULL if an error is detected.
 *
 * @note     (1) This function is DEPRECATED and will be removed in a future version of this product.
 *******************************************************************************************************/
void *OSMemGet(OS_MEM   *p_mem,
               RTOS_ERR *p_err)
{
  void *p_blk;
  CPU_SR_ALLOC();

  OS_ASSERT_DBG_ERR_PTR_VALIDATE(p_err, DEF_NULL);

  OS_TRACE_MEM_GET_ENTER(p_mem);

  //                                                               Must point to a valid memory partition
  OS_ASSERT_DBG_ERR_SET((p_mem != DEF_NULL), *p_err, RTOS_ERR_NULL_PTR, DEF_NULL);

  //                                                               Validate object type
  OS_ASSERT_DBG_ERR_SET((p_mem->Type == OS_OBJ_TYPE_MEM), *p_err, RTOS_ERR_INVALID_TYPE, 0u);

  CPU_CRITICAL_ENTER();
  if (p_mem->NbrFree == 0u) {                                   // See if there are any free memory blocks
    CPU_CRITICAL_EXIT();
    RTOS_ERR_SET(*p_err, RTOS_ERR_POOL_EMPTY);                  // No, Notify caller of empty memory partition
    OS_TRACE_MEM_GET_FAILED(p_mem);
    OS_TRACE_MEM_GET_EXIT(RTOS_ERR_CODE_GET(*p_err));
    return (DEF_NULL);                                          // Return NULL pointer to caller
  }
  p_blk = p_mem->FreeListPtr;                                   // Yes, point to next free memory block
  p_mem->FreeListPtr = *(void **)p_blk;                         // Adjust pointer to new free list
  p_mem->NbrFree--;                                             // One less memory block in this partition
  CPU_CRITICAL_EXIT();
  RTOS_ERR_SET(*p_err, RTOS_ERR_NONE);                          // No error
  OS_TRACE_MEM_GET(p_mem);
  OS_TRACE_MEM_GET_EXIT(RTOS_ERR_CODE_GET(*p_err));
  return (p_blk);                                               // Return memory block to caller
}

/****************************************************************************************************//**
 *                                               OSMemPut()
 *
 * @brief    Returns a memory block to a partition.
 *
 * @param    p_mem   Pointer to the memory partition control block.
 *
 * @param    p_blk   Pointer to the memory block being released.
 *
 * @param    p_err   Pointer to the variable that will receive one of the following error code(s)
 *                   from this function:
 *                       - RTOS_ERR_NONE
 *                       - RTOS_ERR_POOL_FULL
 *
 * @note     (1) This function is DEPRECATED and will be removed in a future version of this product.
 *******************************************************************************************************/
void OSMemPut(OS_MEM   *p_mem,
              void     *p_blk,
              RTOS_ERR *p_err)
{
  CPU_SR_ALLOC();

  OS_ASSERT_DBG_ERR_PTR_VALIDATE(p_err,; );

  OS_TRACE_MEM_PUT_ENTER(p_mem, p_blk);

  //                                                               Must point to a valid memory partition
  OS_ASSERT_DBG_ERR_SET((p_mem != DEF_NULL), *p_err, RTOS_ERR_NULL_PTR,; );

  //                                                               Must release a valid block
  OS_ASSERT_DBG_ERR_SET((p_blk != DEF_NULL), *p_err, RTOS_ERR_NULL_PTR,; );

  //                                                               Validate object type
  OS_ASSERT_DBG_ERR_SET((p_mem->Type == OS_OBJ_TYPE_MEM), *p_err, RTOS_ERR_INVALID_TYPE,; );

  CPU_CRITICAL_ENTER();
  if (p_mem->NbrFree >= p_mem->NbrMax) {                        // Make sure all blocks not already returned
    CPU_CRITICAL_EXIT();
    RTOS_ERR_SET(*p_err, RTOS_ERR_POOL_FULL);
    OS_TRACE_MEM_PUT_FAILED(p_mem);
    OS_TRACE_MEM_PUT_EXIT(RTOS_ERR_CODE_GET(*p_err));
    return;
  }
  *(void **)p_blk = p_mem->FreeListPtr;                         // Insert released block into free block list
  p_mem->FreeListPtr = p_blk;
  p_mem->NbrFree++;                                             // One more memory block in this partition
  CPU_CRITICAL_EXIT();
  RTOS_ERR_SET(*p_err, RTOS_ERR_NONE);                          // Notify caller that memory block was released
  OS_TRACE_MEM_PUT(p_mem);
  OS_TRACE_MEM_PUT_EXIT(RTOS_ERR_CODE_GET(*p_err));
}

/********************************************************************************************************
 ********************************************************************************************************
 *                                           INTERNAL FUNCTIONS
 ********************************************************************************************************
 *******************************************************************************************************/

/****************************************************************************************************//**
 *                                           OS_MemDbgListAdd()
 *
 * @brief    This function is called by OSMemCreate() to add the memory partition to the debug table.
 *
 * @param    p_mem   Pointer to the memory partition control block.
 *
 * @note     (1) This function is INTERNAL to the Kernel and your application MUST NOT call it.
 *******************************************************************************************************/

#if (OS_CFG_DBG_EN == DEF_ENABLED)
void OS_MemDbgListAdd(OS_MEM *p_mem)
{
  p_mem->DbgPrevPtr = DEF_NULL;
  if (OSMemDbgListPtr == DEF_NULL) {
    p_mem->DbgNextPtr = DEF_NULL;
  } else {
    p_mem->DbgNextPtr = OSMemDbgListPtr;
    OSMemDbgListPtr->DbgPrevPtr = p_mem;
  }
  OSMemDbgListPtr = p_mem;
}
#endif

/****************************************************************************************************//**
 *                                               OS_MemInit()
 *
 * @brief    This function is called by the Kernel initialize the memory partition manager. Your
 *           application MUST NOT call this function.
 *
 * @param    p_err   Pointer to the variable that will receive one of the following error code(s) from
 *                   this function:
 *                       - RTOS_ERR_NONE
 *
 * @note     (1) This function is INTERNAL to the Kernel and your application MUST NOT call it.
 *******************************************************************************************************/
void OS_MemInit(RTOS_ERR *p_err)
{
#if (OS_CFG_DBG_EN == DEF_ENABLED)
  OSMemDbgListPtr = DEF_NULL;
  OSMemQty = 0u;
#endif
  RTOS_ERR_SET(*p_err, RTOS_ERR_NONE);
}
#endif

/********************************************************************************************************
 ********************************************************************************************************
 *                                   DEPENDENCIES & AVAIL CHECK(S) END
 ********************************************************************************************************
 *******************************************************************************************************/

#endif // (defined(RTOS_MODULE_KERNEL_AVAIL))
