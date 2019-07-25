/***************************************************************************//**
 * @file
 * @brief Common - RTOS Utilities
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
 * @defgroup COMMON_UTILS Utilities API
 * @ingroup  COMMON
 * @brief      Utilities API
 *
 * @addtogroup COMMON_UTILS
 * @{
 *******************************************************************************************************/

/********************************************************************************************************
 ********************************************************************************************************
 *                                               MODULE
 ********************************************************************************************************
 *******************************************************************************************************/

#ifndef  _RTOS_UTILS_H_
#define  _RTOS_UTILS_H_

/********************************************************************************************************
 ********************************************************************************************************
 *                                               INCLUDE FILES
 ********************************************************************************************************
 *******************************************************************************************************/

#include  <cpu/include/cpu.h>
#include  <common/include/lib_utils.h>
#include  <common/include/rtos_opt_def.h>
#include  <common/include/rtos_path.h>
#include  <rtos_cfg.h>

/********************************************************************************************************
 ********************************************************************************************************
 *                                           DEFAULT CONFIGURATION
 *
 * Note(s) : (1) These default defines are there to simplify the application writing and assume that if no
 *               RTOS_MODULE_CUR has been defined, that the current module is part of the application. To
 *               avoid defaulting to the application, RTOS_MODULE_CUR should be #defined to another module
 *               before including this file.
 ********************************************************************************************************
 *******************************************************************************************************/

#ifdef  RTOS_CFG_RTOS_ASSERT_DBG_FAILED_END_CALL
#define  RTOS_ASSERT_DBG_FAILED_END_CALL(ret_val)       RTOS_CFG_RTOS_ASSERT_DBG_FAILED_END_CALL(ret_val)
#else
#define  RTOS_ASSERT_DBG_FAILED_END_CALL(ret_val)       while (1) {; }
#endif

#ifdef  RTOS_CFG_RTOS_ASSERT_CRITICAL_FAILED_END_CALL
#define  RTOS_ASSERT_CRITICAL_FAILED_END_CALL(ret_val)  RTOS_CFG_RTOS_ASSERT_CRITICAL_FAILED_END_CALL(ret_val)
#else
#define  RTOS_ASSERT_CRITICAL_FAILED_END_CALL(ret_val)  CPU_SW_EXCEPTION(ret_val)
#endif

//                                                                 See Note #1.
#ifdef  RTOS_MODULE_CUR
#define  APP_RTOS_MODULE_CUR                            RTOS_MODULE_CUR
#else
#define  APP_RTOS_MODULE_CUR                            RTOS_CFG_MODULE_APP
#endif

/********************************************************************************************************
 ********************************************************************************************************
 *                                                   DEFINES
 ********************************************************************************************************
 *******************************************************************************************************/

/********************************************************************************************************
 *                                               ASSERT DEFINES
 ********************************************************************************************************
 * Note(s) : (1) The 'ret_val' parameter cannot have parentheses added to it when receiving it as a
 *               parameter, since in the case of a void return value, ';' is given as parameter, to be
 *               directly added after a potential 'return' call.
 *******************************************************************************************************/

/****************************************************************************************************//**
 *                           APP_RTOS_ASSERT_CRITICAL() / APP_RTOS_ASSERT_DBG()
 *
 * @brief    Assert given expression. In case of failure, calls
 *           RTOS_CFG_RTOS_ASSERT_CRITICAL_FAILED_END_CALL(ret_val) or
 *           RTOS_CFG_RTOS_ASSERT_DBG_FAILED_END_CALL(ret_val) if defined, CPU_SW_EXCEPTION(ret_val)
 *           if not.
 *
 * @param    expr        Expression to assert. If expression is false, the assert fail call will be
 *                       made.
 *
 * @param    ret_val     Value that would be returned from the function, ';' if void.
 *
 * @note     (1) Usage of assert is as follows:
 *                              @verbatim
 *                   APP_RTOS_ASSERT_CRITICAL((p_buf != DEF_NULL), ;);
 *                   APP_RTOS_ASSERT_DBG((p_buf != DEF_NULL), DEF_NULL);
 *                              @endverbatim
 * @{
 *******************************************************************************************************/

#define  APP_RTOS_ASSERT_CRITICAL(expr, ret_val)    if ((expr) == 0) { \
    RTOS_ASSERT_CRITICAL_FAILED_END_CALL(ret_val);                     \
}

#define  APP_RTOS_ASSERT_DBG(expr, ret_val)         if ((DEF_BIT_IS_SET_ANY(RTOS_CFG_ASSERT_DBG_ARG_CHK_EXT_MASK, APP_RTOS_MODULE_CUR)) != 0) { \
    if ((expr) == 0) {                                                                                                                          \
      RTOS_ASSERT_DBG_FAILED_END_CALL(ret_val);                                                                                                 \
    }                                                                                                                                           \
}

///< @}

/****************************************************************************************************//**
 *                       APP_RTOS_ASSERT_CRITICAL_FAIL() / APP_RTOS_ASSERT_DBG_FAIL()
 *
 * @brief    Checks if assert is enabled for current module. Calls END_CALL as configured by user. No
 *           check is made.
 *
 * @param    ret_val     Value that would be returned from the function, ';' if void.
 *
 * @{
 *******************************************************************************************************/

#define  APP_RTOS_ASSERT_CRITICAL_FAIL(ret_val)             APP_RTOS_ASSERT_CRITICAL(0u, ret_val)

#define  APP_RTOS_ASSERT_DBG_FAIL(ret_val)                  APP_RTOS_ASSERT_DBG(0u, ret_val)

///< @}

/****************************************************************************************************//**
 ********************************************************************************************************
 * @}                                          MODULE END
 ********************************************************************************************************
 *******************************************************************************************************/

#endif // End of rtos utils module include.
