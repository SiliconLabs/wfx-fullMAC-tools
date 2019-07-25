/***************************************************************************//**
 * @file
 * @brief CPU - Microsoft Win32 Emulation Port
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
 * @note     (1) This port targets the following:
 *                   Core      : Win32
 *                   Mode      :
 *                   Toolchain : Visual Studio
 *
 * @note     (2) This port is an emulation support port.
 *******************************************************************************************************/

/********************************************************************************************************
 ********************************************************************************************************
 *                                               INCLUDE FILES
 ********************************************************************************************************
 *******************************************************************************************************/

#define    MICRIUM_SOURCE
#include  "win32_cpu_port.h"
#include  <cpu/include/cpu.h>

#define  _WIN32_WINNT  0x0600
#define   WIN32_LEAN_AND_MEAN

#include  <windows.h>
#include  <stdio.h>

#ifdef _MSC_VER
#include  <intrin.h>

#pragma intrinsic(_BitScanForward)
#pragma intrinsic(_BitScanReverse)
#endif

#ifdef __cplusplus
extern  "C" {
#endif

/********************************************************************************************************
 ********************************************************************************************************
 *                                       LOCAL GLOBAL VARIABLES
 ********************************************************************************************************
 *******************************************************************************************************/

#if   (CPU_CFG_CRITICAL_METHOD_WIN32 == WIN32_CRITICAL_SECTION)
static CRITICAL_SECTION CriticalSection;
#elif (CPU_CFG_CRITICAL_METHOD_WIN32 == WIN32_MUTEX)
static HANDLE CriticalSection;
#endif

/********************************************************************************************************
 ********************************************************************************************************
 *                                       LOCAL FUNCTION PROTOTYPES
 ********************************************************************************************************
 *******************************************************************************************************/

#ifdef CPU_CFG_MSG_TRACE_EN
static int CPU_Printf(char *p_str, ...);
#endif

/********************************************************************************************************
 ********************************************************************************************************
 *                                           GLOBAL FUNCTIONS
 ********************************************************************************************************
 *******************************************************************************************************/

/****************************************************************************************************//**
 *                                               CPU_IntInit()
 *
 * @brief    This function initializes the critical section.
 *
 * @note     (1) CPU_IntInit() MUST be called prior to use any of the CPU_IntEn(), and CPU_IntDis()
 *               functions.
 *******************************************************************************************************/
void CPU_PortInit(void)
{
#if   (CPU_CFG_CRITICAL_METHOD_WIN32 == WIN32_MUTEX)
#ifdef CPU_CFG_MSG_TRACE_EN
  DWORD  last_err;
  LPTSTR p_msg;
#endif

  CriticalSection = CreateMutex(NULL, FALSE, NULL);
  if (CriticalSection == NULL) {
#ifdef CPU_CFG_MSG_TRACE_EN
    last_err = GetLastError();

    FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
                  NULL,
                  last_err,
                  MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                  (LPTSTR)&p_msg,
                  0,
                  NULL);

    CPU_Printf("Error: Initialize Critical Section failed: %s.\n", p_msg);

    LocalFree(p_msg);
#endif
    return;
  }
#elif (CPU_CFG_CRITICAL_METHOD_WIN32 == WIN32_CRITICAL_SECTION)
  InitializeCriticalSection(&CriticalSection);
#endif
}

/****************************************************************************************************//**
 *                                               CPU_IntEnd()
 *
 * @brief    This function terminates the critical section.
 *******************************************************************************************************/
void CPU_IntEnd(void)
{
#if   (CPU_CFG_CRITICAL_METHOD_WIN32 == WIN32_MUTEX)
  CloseHandle(CriticalSection);
#elif (CPU_CFG_CRITICAL_METHOD_WIN32 == WIN32_CRITICAL_SECTION)
  DeleteCriticalSection(&CriticalSection);
#endif
}

/****************************************************************************************************//**
 *                                               CPU_IntDis()
 *
 * @brief    This function disables interrupts for critical sections of code.
 *******************************************************************************************************/
void CPU_IntDis(void)
{
#if   (CPU_CFG_CRITICAL_METHOD_WIN32 == WIN32_MUTEX)
  DWORD ret;
#ifdef  CPU_CFG_MSG_TRACE_EN
  DWORD  last_err;
  LPTSTR p_msg;
#endif

  ret = WaitForSingleObject(CriticalSection, INFINITE);

  switch (ret) {
    case WAIT_OBJECT_0:
    default:
      break;

#ifdef  CPU_CFG_MSG_TRACE_EN
    case WAIT_ABANDONED:
      CPU_Printf("cpu_c.c: Enter Critical Section failed: Mutex abandoned by owning thread.\n");
      break;

    case WAIT_TIMEOUT:
      CPU_Printf("cpu_c.c: Enter Critical Section failed: Time-out interval elapsed.\n");
      break;

    case WAIT_FAILED:
      last_err = GetLastError();

      FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
                    NULL,
                    last_err,
                    MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                    (LPTSTR)&p_msg,
                    0,
                    NULL);

      CPU_Printf("cpu_c.c: Enter Critical Section failed: %s.\n", p_msg);

      LocalFree(p_msg);
      break;
#endif
  }
#elif (CPU_CFG_CRITICAL_METHOD_WIN32 == WIN32_CRITICAL_SECTION)
  EnterCriticalSection(&CriticalSection);
#endif
}

/****************************************************************************************************//**
 *                                               CPU_IntEn()
 *
 * @brief    This function enables interrupts after critical sections of code.
 *******************************************************************************************************/
void CPU_IntEn(void)
{
#if   (CPU_CFG_CRITICAL_METHOD_WIN32 == WIN32_MUTEX)
#ifdef  CPU_CFG_MSG_TRACE_EN
  DWORD  last_err;
  LPTSTR p_msg;
#endif

  if (ReleaseMutex(CriticalSection) == 0u) {
#ifdef  CPU_CFG_MSG_TRACE_EN
    last_err = GetLastError();

    FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
                  NULL,
                  last_err,
                  MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                  (LPTSTR)&p_msg,
                  0,
                  NULL);

    CPU_Printf("cpu_c.c: Exit Critical Section failed: %s.\n", p_msg);

    LocalFree(p_msg);
#endif
  }
#elif (CPU_CFG_CRITICAL_METHOD_WIN32 == WIN32_CRITICAL_SECTION)
  LeaveCriticalSection(&CriticalSection);
#endif
}

/****************************************************************************************************//**
 *                                           CPU_CntLeadZeros()
 *
 * @brief    Count the number of contiguous, most-significant, leading zero bits in a data value.
 *
 * @param    val     Data value to count leading zero bits.
 *
 * @return   Number of contiguous, most-significant, leading zero bits in 'val', if NO error(s).
 *           0,                                                                  otherwise.
 *
 * @note     (1) Supports the 8, 16 and 32-bits data value sizes :
 *               See also 'cpu_def.h  CPU WORD CONFIGURATION  Note #1'.
 *               - (a) For  8-bit values :
 *                       @verbatim
 *                               b07  b06  b05  b04  b03  b02  b01  b00    # Leading Zeros
 *                               ---  ---  ---  ---  ---  ---  ---  ---    ---------------
 *                               1    x    x    x    x    x    x    x            0
 *                               0    1    x    x    x    x    x    x            1
 *                               0    0    1    x    x    x    x    x            2
 *                               0    0    0    1    x    x    x    x            3
 *                               0    0    0    0    1    x    x    x            4
 *                               0    0    0    0    0    1    x    x            5
 *                               0    0    0    0    0    0    1    x            6
 *                               0    0    0    0    0    0    0    1            7
 *                               0    0    0    0    0    0    0    0            8
 *                       @endverbatim
 *               - (b) For 16-bit values :
 *                       @verbatim
 *                       b15  b14  b13  ...  b04  b03  b02  b01  b00    # Leading Zeros
 *                       ---  ---  ---       ---  ---  ---  ---  ---    ---------------
 *                           1    x    x         x    x    x    x    x            0
 *                           0    1    x         x    x    x    x    x            1
 *                           0    0    1         x    x    x    x    x            2
 *                           :    :    :         :    :    :    :    :            :
 *                           :    :    :         :    :    :    :    :            :
 *                           0    0    0         1    x    x    x    x           11
 *                           0    0    0         0    1    x    x    x           12
 *                           0    0    0         0    0    1    x    x           13
 *                           0    0    0         0    0    0    1    x           14
 *                           0    0    0         0    0    0    0    1           15
 *                           0    0    0         0    0    0    0    0           16
 *                       @endverbatim
 *               - (c) For 32-bit values :
 *                       @verbatim
 *                       b31  b30  b29  ...  b04  b03  b02  b01  b00    # Leading Zeros
 *                       ---  ---  ---       ---  ---  ---  ---  ---    ---------------
 *                           1    x    x         x    x    x    x    x            0
 *                           0    1    x         x    x    x    x    x            1
 *                           0    0    1         x    x    x    x    x            2
 *                           :    :    :         :    :    :    :    :            :
 *                           :    :    :         :    :    :    :    :            :
 *                           0    0    0         1    x    x    x    x           27
 *                           0    0    0         0    1    x    x    x           28
 *                           0    0    0         0    0    1    x    x           29
 *                           0    0    0         0    0    0    1    x           30
 *                           0    0    0         0    0    0    0    1           31
 *                           0    0    0         0    0    0    0    0           32
 *                       @venderbatim
 *               See also 'CPU COUNT LEAD ZEROs LOOKUP TABLE  Note #1'.
 *
 * @note     (2) MUST be implemented in cpu_a.asm if and only if CPU_CFG_LEAD_ZEROS_ASM_PRESENT
 *               is #define'd in 'cpu_cfg.h' or 'cpu.h'.
 *******************************************************************************************************/

#ifdef  CPU_CFG_LEAD_ZEROS_ASM_PRESENT
#ifdef  _MSC_VER
CPU_DATA CPU_CntLeadZeros(CPU_DATA val)
{
  DWORD clz;

  if (val == 0u) {
    return (32u);
  }

  _BitScanReverse(&clz, (DWORD)val);

  return (31u - (CPU_DATA)clz);
}
#endif
#endif

/****************************************************************************************************//**
 *                                           CPU_CntTrailZeros()
 *
 * @brief    Count the number of contiguous, least-significant, trailing zero bits in a data value.
 *
 * @param    val     Data value to count trailing zero bits.
 *
 * @return   Number of contiguous, least-significant, trailing zero bits in 'val'.
 *
 * @note     (1) Supports the 8, 16, 32 and 64-bits data value sizes :
 *               - (a) For  8-bit values :
 *                           @verbatim
 *                           b07  b06  b05  b04  b03  b02  b01  b00    # Trailing Zeros
 *                           ---  ---  ---  ---  ---  ---  ---  ---    ----------------
 *                           x    x    x    x    x    x    x    1            0
 *                           x    x    x    x    x    x    1    0            1
 *                           x    x    x    x    x    1    0    0            2
 *                           x    x    x    x    1    0    0    0            3
 *                           x    x    x    1    0    0    0    0            4
 *                           x    x    1    0    0    0    0    0            5
 *                           x    1    0    0    0    0    0    0            6
 *                           1    0    0    0    0    0    0    0            7
 *                           0    0    0    0    0    0    0    0            8
 *                           @endverbatim
 *               - (b) For 16-bit values :
 *                           @verbatim
 *                       b15  b14  b13  b12  b11  ...  b02  b01  b00    # Trailing Zeros
 *                       ---  ---  ---  ---  ---       ---  ---  ---    ----------------
 *                           x    x    x    x    x         x    x    1            0
 *                           x    x    x    x    x         x    1    0            1
 *                           x    x    x    x    x         1    0    0            2
 *                           :    :    :    :    :         :    :    :            :
 *                           :    :    :    :    :         :    :    :            :
 *                           x    x    x    x    1         0    0    0           11
 *                           x    x    x    1    0         0    0    0           12
 *                           x    x    1    0    0         0    0    0           13
 *                           x    1    0    0    0         0    0    0           14
 *                           1    0    0    0    0         0    0    0           15
 *                           0    0    0    0    0         0    0    0           16
 *                           @endverbatim
 *               - (c) For 32-bit values :
 *                           @verbatim
 *                       b31  b30  b29  b28  b27  ...  b02  b01  b00    # Trailing Zeros
 *                       ---  ---  ---  ---  ---       ---  ---  ---    ----------------
 *                           x    x    x    x    x         x    x    1            0
 *                           x    x    x    x    x         x    1    0            1
 *                           x    x    x    x    x         1    0    0            2
 *                           :    :    :    :    :         :    :    :            :
 *                           :    :    :    :    :         :    :    :            :
 *                           x    x    x    x    1         0    0    0           27
 *                           x    x    x    1    0         0    0    0           28
 *                           x    x    1    0    0         0    0    0           29
 *                           x    1    0    0    0         0    0    0           30
 *                           1    0    0    0    0         0    0    0           31
 *                           0    0    0    0    0         0    0    0           32
 *                           @endverbatim
 *               - (d) For 64-bit values :
 *                           @verbatim
 *                       b63  b62  b61  b60  b59  ...  b02  b01  b00    # Trailing Zeros
 *                       ---  ---  ---  ---  ---       ---  ---  ---    ----------------
 *                           x    x    x    x    x         x    x    1            0
 *                           x    x    x    x    x         x    1    0            1
 *                           x    x    x    x    x         1    0    0            2
 *                           :    :    :    :    :         :    :    :            :
 *                           :    :    :    :    :         :    :    :            :
 *                           x    x    x    x    1         0    0    0           59
 *                           x    x    x    1    0         0    0    0           60
 *                           x    x    1    0    0         0    0    0           61
 *                           x    1    0    0    0         0    0    0           62
 *                           1    0    0    0    0         0    0    0           63
 *                           0    0    0    0    0         0    0    0           64
 *                          @endverbatim
 *                   See also 'cpu_def.h  CPU WORD CONFIGURATION  Note #1'.
 *
 * @note     (2) For non-zero values, the returned number of contiguous, least-significant, trailing
 *               zero bits is also equivalent to the bit position of the least-significant set bit.
 *
 * @note     (3) 'val' SHOULD be validated for non-'0' PRIOR to all other counting zero calculations :
 *               - (a) CPU_CntTrailZeros()'s final conditional statement calculates 'val's number of
 *                     trailing zeros based on its return data size, 'CPU_CFG_DATA_SIZE', & 'val's
 *                     calculated number of lead zeros ONLY if the initial 'val' is non-'0' :
 *                     @verbatim
 *                       if (val != 0u) {
 *                           nbr_trail_zeros = ((CPU_CFG_DATA_SIZE * DEF_OCTET_NBR_BITS) - 1u) - nbr_lead_zeros;
 *                       } else {
 *                           nbr_trail_zeros = nbr_lead_zeros;
 *                       }
 *                     @endverbatim
 *                     Therefore, initially validating all non-'0' values avoids having to conditionally
 *                     execute the final statement.
 *******************************************************************************************************/

#ifdef  CPU_CFG_TRAIL_ZEROS_ASM_PRESENT
#ifdef  _MSC_VER
CPU_DATA CPU_CntTrailZeros(CPU_DATA val)
{
  DWORD ctz;

  if (val == 0u) {
    return (32u);
  }

  _BitScanForward(&ctz, (DWORD)val);

  return ((CPU_DATA)ctz);
}
#endif
#endif

/********************************************************************************************************
 ********************************************************************************************************
 *                                               LOCAL FUNCTIONS
 ********************************************************************************************************
 *******************************************************************************************************/

/****************************************************************************************************//**
 *                                               CPU_Printf()
 *
 * @brief    This function is analog of printf.
 *
 * @param    p_str   Pointer to format string output.
 *
 * @return   Number of characters written.
 *******************************************************************************************************/

#ifdef  CPU_CFG_MSG_TRACE_EN
static int CPU_Printf(char *p_str, ...)
{
  va_list param;
  int     ret;

  va_start(param, p_str);
  ret = vprintf_s(p_str, param);
  va_end(param);

  return (ret);
}
#endif

#ifdef __cplusplus
}
#endif
