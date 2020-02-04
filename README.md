# WFx Wi-Fi FMAC tools

This repository contains tools and examples to use the **WFx Wi-Fi FMAC driver**
(FMAC standing for Full-MAC). The WF200 is a Wi-Fi transceiver and this code is
meant to be run with the [WF200 Wi-Fi Expansion Kit](https://www.silabs.com/products/development-tools/wireless/wi-fi/wf200-expansion-kit),
the [WFM200 Wi-Fi Expansion Kit](https://www.silabs.com/products/development-tools/wireless/wi-fi/wfm200-expansion-kit) and also the
[WGM160P Wi-Fi Module Starter Kit](https://www.silabs.com/products/development-tools/wireless/wi-fi/wgm160p-wifi-module-starter-kit).

## Repository content

* Examples: contains applications using the FMAC driver on different platforms
  * Silicon Labs MCU
    * commissioning: Commissioning Example using Micrium OS, LwIP and the combo mode running
    on the EFM32 Giant Gecko GG11 Starter Kit (more information about the example [here](./Examples/SiliconLabs/commissioning/SLSTK3701A/README.md))
    and on the WGM160P Wi-Fi Module Radio Board (more information about the example [here](./Examples/SiliconLabs/commissioning/WGM160P/README.md))
	* secure_mqtt: MQTT over TLS example using Micrium OS, LwIP (MQTT, DHCP, DNS clients), Mbed TLS running
    on the EFM32 Giant Gecko GG11 Starter Kit and WGM160P (more information about the example [here](./Examples/SiliconLabs/secure_mqtt/README.md))
  * STM32
    * WF200_driver_F429ZI_BM-LwIP: Commissioning Example on Bare Metal using LwIP and the combo mode running with LwIP running on the
    NUCLEO-F429ZI board (more information about the example [here](./Examples/STM32/Projects/WF200_driver_F429ZI_BM-LwIP/README.md))
    * WF200_driver_F429ZI_FreeRTOS-LwIP: Commissioning Example using FreeRTOS, LwIP and the combo mode running
    on the NUCLEO-F429ZI board (more information about the example [here](./Examples/STM32/Projects/WF200_driver_F429ZI_FreeRTOS-LwIP/README.md))
* Tools: contains generic tools
  * pds_compress: Python script to compress Platform Data Set files
  * RF test agent: Bare Metal application running on SLSTK3701A and NUCLEO-F429ZI platforms and
  allowing the execution of basic RF tests (more information about the tool [here](./Tools/RF_test_agent/README.md))


## FMAC driver source code

The latest WFx Wi-Fi FMAC driver version in the [wfx-fullMAC-driver repository](https://github.com/SiliconLabs/wfx-fullMAC-driver)

## FMAC driver documentation

The latest WFx Wi-Fi FMAC driver documentation in the [Silabs WF200 documentation](https://docs.silabs.com/wifi/wf200/rtos/latest/index).
