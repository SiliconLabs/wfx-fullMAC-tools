# Wi-Fi CLI Example

This example project provides a Command Line Interface (CLI) to interact with the Wi-Fi FMAC driver and the LwIP APIs.

## Requirements

### Hardware Prerequisites

One of the supported platforms listed below is required to run the example:

* [**EFM32 Giant Gecko GG11 Starter Kit (SLSTK3701A)**](https://www.silabs.com/products/development-tools/mcu/32-bit/efm32-giant-gecko-gg11-starter-kit) with
  [**WF200 Wi-Fi® Expansion Kit (SLEXP8022A)**](https://www.silabs.com/products/development-tools/wireless/wi-fi/wf200-expansion-kit) or
  [**WFM200S Wi-Fi® Expansion Kit (SLEXP8023A)**](https://www.silabs.com/products/development-tools/wireless/wi-fi/wfm200-expansion-kit)
* [**EFR32MG Zigbee and Thread Starter Kit (SLWSTK6000B)**](https://www.silabs.com/development-tools/wireless/zigbee/efr32mg-zigbee-thread-starter-kit) with
  [**WF200 Wi-Fi® Expansion Kit (SLEXP8022A)**](https://www.silabs.com/products/development-tools/wireless/wi-fi/wf200-expansion-kit) or
  [**WFM200S Wi-Fi® Expansion Kit (SLEXP8023A)**](https://www.silabs.com/products/development-tools/wireless/wi-fi/wfm200-expansion-kit)
* [**WGM160P Wi-Fi® Module Starter Kit**](https://www.silabs.com/products/development-tools/wireless/wi-fi/wgm160p-wifi-module-starter-kit)

Additionally, a PC is required to configure the board and it can also be used to load a binary file on the board, to compile the Simplicity Studio project or run a serial terminal.

### Software Prerequisites

The required software includes:

* Simplicity Studio v5 and the Gecko SDK Suite (4.0.0 or newer)
* The example project and the Wi-Fi Full MAC driver (available in the Gecko Platform SDK)
* (Optional) A Serial terminal to communicate with the board. For example, [**Tera Term**](https://osdn.net/projects/ttssh2/releases/) or [**Putty**](https://www.putty.org/)

## Install Simplicity Studio 5 and the Gecko SDK

Simplicity Studio 5 is a free software suite needed to start developing your application. To install Simplicity Studio 5, please follow this [**procedure**](https://docs.silabs.com/simplicity-studio-5-users-guide/latest/ss-5-users-guide-getting-started/install-ss-5-and-software) by selecting the options **[Install by connecting device(s)]** and **[Auto]**.

## Set up your Kit and Get the Example

Please follow the instructions related to the platform suiting your case:

* [**EFM32 Giant Gecko GG11 Starter Kit setup** or **EFR32MG Zigbee and Thread Starter Kit setup**](https://docs.silabs.com/wifi/wf200/content-source/getting-started/silabs/ssv5/gg11/wifi-commissioning-micriumos/getting-started)
* [**WGM160P Wi-Fi® Module Starter Kit setup**](https://docs.silabs.com/wifi/wf200/content-source/getting-started/silabs/ssv5/wgm160p/wifi-commissioning-micriumos/getting-started)

Note that at step 6, select **Platform - Wi-Fi CLI on Micrium OS kernel** instead of **Platform - Wi-Fi Commissioning Micrium OS Kernel**.

## Start the Example

1. Please follow this [**instruction**](https://docs.silabs.com/wifi/wf200/content-source/getting-started/silabs/ssv5/gg11/wifi-commissioning-micriumos/getting-started#start-the-example)
Note that at step 5, the hex file name is **wifi_cli_micriumos.hex**

2. Reset the board using the **'RESET' button**. You should see the following output.

        FMAC Driver version    x.x.x
        WF200 Firmware version x.x.x
        WF200 initialization successful
        Wi-Fi CLI Application Example
        
        >


3. The Wi-Fi cli project is ready to receive commands. You can start by the 'help' command to list available commands.

	The 'help' list below is provided as an indication. To access an up-to-date list, use the 'help' command on the target.

        help
        lwip                          lwip CLI commands
        reset                         Reset the host CPU
                                      [*] reset
        ping                          Send ICMP ECHO_REQUEST to network hosts
                                      [*] [-n nb] <ip>
        iperf                         Start a TCP iPerf test as a client or a server
                                      [*] iperf <-c ip [-t dur] [-p port] [-k] | -s>
        iperf_server_stop             Stop the running iPerf server
                                      [*] iperf_stop_server
        iperf_client_stop             Stop the running iPerf client
                                      [*] iperf_stop_client
        wifi                          Wifi CLI commands

Additionally, you can display a specific command help by entering:

```
@ [command] help
```
