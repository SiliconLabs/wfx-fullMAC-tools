![Silicon Labs Wi-Fi](https://prnewswire2-a.akamaihd.net/p/1893751/sp/189375100/thumbnail/entry_id/1_bxpjsgr1/def_height/400/def_width/400/version/100011/type/1)

**Warning: Do not use the GitHub feature "Download ZIP" to retrieve the source code, use Git instead.**

# Silicon Labs Wi-Fi FMAC tools

This repository contains tools and examples to use with [**Silicon Labs Wi-Fi FMAC driver**](https://github.com/SiliconLabs/wfx-fullMAC-driver) (FMAC standing for Full-MAC).
These example codes are meant to be evaluated with hosts connected to the [WF200 Wi-Fi Expansion Kit](https://www.silabs.com/products/development-tools/wireless/wi-fi/wf200-expansion-kit) or
the [WFM200 Wi-Fi Expansion Kit](https://www.silabs.com/products/development-tools/wireless/wi-fi/wfm200-expansion-kit).
It can also be ran on the [WGM160P Wi-Fi Module Starter Kit](https://www.silabs.com/products/development-tools/wireless/wi-fi/wgm160p-wifi-module-starter-kit).
Unless otherwise specified in the directory, all examples are considered to be EXPERIMENTAL QUALITY which implies that the code provided in the repos has not been formally tested and is provided as-is. It is not suitable for production environments.

## FMAC driver

This repository is built around the [wfx-fullMAC-driver](https://github.com/SiliconLabs/wfx-fullMAC-driver) repository which is included within through a Git submodule.
While this method has many advantages concerning the maintainability, it also has the disadvantages to make **Git mandatory** (since the GitHub feature "Download ZIP" doesn't include the submodule sources)
which can be overwhelming for beginners.
  
> If you are not familiar with Git, please refer to the section _Get started_ of this page.

## Repository content

#### Examples

It contains applications using the FMAC driver on different platforms.

  * **Silicon Labs MCU Examples** (**Gecko SDK v2.7.0 and MicriumOS Kernel v5.8.0 or newer required**)
    * **EFM32GG11 (SLSTK3701A) + WF(M)200**
        * [*commissioning (MicriumOS)*](./Examples/SiliconLabs/commissioning/micrium_os/SLSTK3701A/README.md): Commissioning example using LwIP, the combo mode and running on Micrium OS. This example is a superset of the **SLSTK3701A_micrium_lwip_wfx** present in the Gecko SDK.
        * [*commissioning (Bare Metal)*](./Examples/SiliconLabs/commissioning/bare_metal/SLSTK3701A/README.md): Commissioning example using LwIP, the combo mode and running on Bare Metal (No OS).
        * [*secure_mqtt*](./Examples/SiliconLabs/secure_mqtt/README.md): MQTT over TLS example using Micrium OS, LwIP (MQTT, DHCP, DNS clients), Mbed TLS and NVM3.
        * [*uart_cli*](./Examples/SiliconLabs/uart_cli/README.md): Example providing a UART command line interface to interact with the Wi-Fi FMAC driver and LwIP APIs.
    * **WGM160P**
        * [*commissioning (MicriumOS)*](./Examples/SiliconLabs/commissioning/micrium_os/WGM160P/README.md): Commissioning example using LwIP, the combo mode and running on Micrium OS.
        * [*commissioning (Bare Metal)*](./Examples/SiliconLabs/commissioning/bare_metal/WGM160P/README.md): Commissioning example using LwIP, the combo mode and running on Bare Metal (No OS).
        * [*secure_mqtt*](./Examples/SiliconLabs/secure_mqtt/README.md): MQTT over TLS example using Micrium OS, LwIP (MQTT, DHCP, DNS clients), Mbed TLS and NVM3.
        * [*uart_cli*](./Examples/SiliconLabs/uart_cli/README.md): Example providing a UART command line interface to interact with the Wi-Fi FMAC driver and LwIP APIs.
    * **EFR32MG12 (BRD4161A) + WF(M)200**
        * [*multiprotocol*](./Examples/SiliconLabs/multiprotocol/bare_metal/README.md): Example using the BLE and Wi-Fi interfaces on Bare Metal (No OS), this example is based on the commissioning example. 
    * **EFR32xG21 (BRD4180A) + WF(M)200**
        * [*multiprotocol*](./Examples/SiliconLabs/multiprotocol/bare_metal/README.md): Example using the BLE and Wi-Fi interfaces on Bare Metal (No OS), this example is based on the commissioning example.
  * **Third-Party MCU Examples**
    * **STM32F4 (NUCLEO-F429ZI)**
        * [*commissioning (FreeRTOS)*](./Examples/STM32/commissioning/F429ZI_freertos/README.md): Commissioning example using LwIP, the combo mode and running on FreeRTOS.
        * [*commissioning (Bare Metal)*](./Examples/STM32/commissioning/F429ZI_bare_metal/README.md): Commissioning example using LwIP, the combo mode and running on Bare Metal (No OS).
        * [*uart_cli*](./Examples/STM32/uart_cli/README.md): Example providing a UART command line interface to interact with the Wi-Fi FMAC driver and LwIP APIs.

#### Tools

It contains generic tools to help development around the Wi-Fi FMAC driver.

  * *pds_compress*: Python script to compress Platform Data Set files.
  * [*RF test agent*](./Tools/RF_test_agent/README.md): Bare metal application running on SLSTK3701A and NUCLEO-F429ZI platforms and allowing the execution of basic RF tests. **This feature is also integrated in the uart_cli example.**

## Get started

As mentionned above Git is mandatory, here are few steps to get started with the repository using Git:

1. Install Git, tools can be found [here](https://git-scm.com/downloads)
2. Retrieve the source code by cloning the repository on your computer using the command `git clone --recurse-submodules https://github.com/SiliconLabs/wfx-fullMAC-tools.git`
3. Later on, to retrieve changes published on the repository enter the command `git pull origin` followed by the command `git submodule update` inside the repository folder on your computer

These steps are the minimum ones to get started, don't hesitate to get more information about git to be more familiar with this tool.