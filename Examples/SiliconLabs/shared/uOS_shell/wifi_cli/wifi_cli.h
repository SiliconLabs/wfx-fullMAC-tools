/**************************************************************************//**
* @file wifi_cli.h
* @brief WiFi Command Line Interface based on Micrium OS + Shell via UART
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
#ifndef WIFI_CLI_H
#define WIFI_CLI_H


void WIFI_CLI_Init(void);

void WIFI_CLI_CfgDialog(void);

#endif // WIFI_CLI_H
