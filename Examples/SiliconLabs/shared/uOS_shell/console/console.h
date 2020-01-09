/**************************************************************************//**
* @file console.h
* @brief Console management based on Micrium OS + Shell
* @version 1.0.0
******************************************************************************
* # License
* <b>Copyright 2018 Silicon Labs, Inc. http://www.silabs.com</b>
*******************************************************************************
*
* This file is licensed under the Silabs License Agreement. See the file
* "Silabs_License_Agreement.txt" for details. Before using this software for
* any purpose, you must agree to the terms of that agreement.
*
******************************************************************************/
#ifndef CONSOLE_H
#define CONSOLE_H

#ifdef __cplusplus
extern "C" {
#endif

void console_get_input(char *buf, uint32_t size, CPU_BOOLEAN echo);
void console_get_input_tmo(char *buf, uint32_t size, uint8_t timeout_sec, CPU_BOOLEAN echo);

#ifdef __cplusplus
}
#endif
#endif // CONSOLE_H
