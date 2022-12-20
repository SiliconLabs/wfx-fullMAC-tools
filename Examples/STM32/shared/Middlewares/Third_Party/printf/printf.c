/*******************
 *
 * Copyright 1998-2017 IAR Systems AB.
 *
 * This is a template implementation of the "__write" function used by
 * the standard library.  Replace it with a system-specific
 * implementation.
 *
 * The "__write" function should output "size" number of bytes from
 * "buffer" in some application-specific way.  It should return the
 * number of characters written, or _LLIO_ERROR on failure.
 *
 * If "buffer" is zero then __write should perform flushing of
 * internal buffers, if any.  In this case "handle" can be -1 to
 * indicate that all handles should be flushed.
 *
 * The template implementation below assumes that the application
 * provides the function "MyLowLevelPutchar".  It should return the
 * character written.
 *
 ********************/

#include <LowLevelIOInterface.h>
#include "stm32f4xx_hal.h"

#pragma module_name = "?__write"

extern UART_HandleTypeDef huart3;

HAL_StatusTypeDef MyLowLevelPutchar(const unsigned char* pchar);

/*
 * If the __write implementation uses internal buffering, uncomment
 * the following line to ensure that we are called with "buffer" as 0
 * (i.e. flush) when the application terminates.
 */

size_t __write(int handle, const unsigned char * buffer, size_t size)
{
  /* Remove the #if #endif pair to enable the implementation */
#if 1

  size_t nChars = 0;

  if (buffer == 0)
  {
    /*
     * This means that we should flush internal buffers.  Since we
     * don't we just return.  (Remember, "handle" == -1 means that all
     * handles should be flushed.)
     */
    return 0;
  }
          
  /* This template only writes to "standard out" and "standard err",
   * for all other file handles it returns failure. */
  if (handle != _LLIO_STDOUT && handle != _LLIO_STDERR)
  {
    return _LLIO_ERROR;
  }

  for (; nChars < size; nChars++)
  {
    if (MyLowLevelPutchar(buffer) == HAL_ERROR)
    {
      return _LLIO_ERROR;
    }
    buffer++;
  }

  return nChars;

#else
  /* Always return error code when implementation is disabled. */
  return _LLIO_ERROR;
#endif
}

HAL_StatusTypeDef MyLowLevelPutchar(const unsigned char* pchar)
{
  HAL_StatusTypeDef retval = HAL_UART_Transmit(&huart3, (uint8_t*)pchar, sizeof(*pchar), 1000);

  return retval;
}