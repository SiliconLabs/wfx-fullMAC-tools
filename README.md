![Silicon Labs Wi-Fi](https://prnewswire2-a.akamaihd.net/p/1893751/sp/189375100/thumbnail/entry_id/1_bxpjsgr1/def_height/400/def_width/400/version/100011/type/1)

# Silicon Labs Wi-Fi FMAC tools

This repository contains tools and examples to use **Silicon Labs Wi-Fi FMAC driver** (FMAC standing for Full-MAC). This example codes are meant to be evaluated with hosts connected to the [WF200 Wi-Fi Expansion Kit](https://www.silabs.com/products/development-tools/wireless/wi-fi/wf200-expansion-kit) or the [WFM200 Wi-Fi Expansion Kit](https://www.silabs.com/products/development-tools/wireless/wi-fi/wfm200-expansion-kit). It can also be ran in the [WGM160P Wi-Fi Module Starter Kit](https://www.silabs.com/products/development-tools/wireless/wi-fi/wgm160p-wifi-module-starter-kit).

## Repository content

* Examples: contains applications using the FMAC driver on different platforms
  * Silicon Labs MCU
    * commissioning: Commissioning example using Micrium OS, LwIP and the combo mode running on the EFM32 Giant Gecko GG11 Starter Kit (more information about the example [here](./Examples/SiliconLabs/commissioning/SLSTK3701A/README.md)) and on the WGM160P Wi-Fi Module Radio Board (more information about the example [here](./Examples/SiliconLabs/commissioning/WGM160P/README.md))
    * secure_mqtt: MQTT over TLS example using Micrium OS, LwIP (MQTT, DHCP, DNS clients), Mbed TLS running on the EFM32 Giant Gecko GG11 Starter Kit and WGM160P (more information about the example [here](./Examples/SiliconLabs/secure_mqtt/README.md))
  * STM32
    * commissioning: Commissioning example using LwIP and the combo mode running on the NUCLEO-F429ZI board (more information about the bare metal example [here](./Examples/STM32/commissioning/F429ZI_bare_metal/README.md) and the FreeRTOS example [here](./Examples/STM32/commissioning/F429ZI_freertos/README.md))
* Tools: contains generic tools
  * pds_compress: Python script to compress Platform Data Set files
  * RF test agent: Bare metal application running on SLSTK3701A and NUCLEO-F429ZI platforms and allowing the execution of basic RF tests (more information about the tool [here](./Tools/RF_test_agent/README.md))


## FMAC driver source code

The latest Wi-Fi FMAC driver version in the [wfx-fullMAC-driver repository](https://github.com/SiliconLabs/wfx-fullMAC-driver)

## FMAC driver documentation

The latest Wi-Fi FMAC driver documentation in the [Silabs Wi-Fi documentation](https://docs.silabs.com/wifi/wf200/rtos/latest/index).
