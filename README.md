# WFx Wi-Fi FMAC tools

This repository contains tools and examples to use the **WFx Wi-Fi FMAC driver**
(FMAC standing for Full-MAC). The WF200 is a Wi-Fi transceiver and this code is
meant to be run with the WF200 Wi-Fi Expansion Kit. See [this page](https://www.silabs.com/products/development-tools/wireless/wi-fi/wf200-expansion-kit)
for details on the WF200 Wi-Fi Expansion Kit.

## Repository content

* Examples: contains implementations of the FMAC driver on different platforms
  * Silicon Labs MCU
    * SLSTK3701A_micriumos_lwip_wfx200: Example using MicriumOS and LwIP running
    on the EFM32 Giant Gecko GG11 Starter Kit (more information in the SLSTK3701A_micriumos_lwip_wfx200/README.md)
  * STM32
    * WF200_driver_F429ZI_BM-LwIP: Bare metal example with LwIP running on the
    NUCLEO-F429ZI board (more information in the WF200_driver_F429ZI_BM-LwIP/README.md)
    * WF200_driver_F429ZI_FreeRTOS-LwIP: Example using FreeRTOS and LwIP running
    on the NUCLEO-F429ZI board (more information in the WF200_driver_F429ZI_FreeRTOS-LwIP/README.md)
* Tools: contains generic tools
  * pds_compress: Python script to compress Platform Data Set files

## FMAC driver source code

The latest WFx Wi-Fi FMAC driver version is located [here](https://github.com/SiliconLabs/wfx-fullMAC-driver)

## FMAC driver documentation

The latest WFx Wi-Fi FMAC driver documentation is located [here](https://docs.silabs.com/wifi/wf200/rtos/latest/index).
