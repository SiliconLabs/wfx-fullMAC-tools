# STM32/Bare Metal/LwIP WFx FMAC driver example

## Hardware prerequisites
This demonstration example runs the WFx FMAC driver meant to communicate with the WFx Silicon Labs Wi-Fi solution. To 
use the example properly, below are the necessary hardware:
* [the WF200 Wi-Fi® Expansion Kit SLEXP8022A](https://www.silabs.com/products/development-tools/wireless/wi-fi/wf200-expansion-kit)
* The WFx Arduino/NUCLEO interposer
* A NUCLEO-F429ZI development board
* A micro USB cable
* A PC to load a binary file in the NUCLEO board or to compile the IAR/TrueSTUDIO projects. In addition, it can be used 
to test the example if it is equipped with a Wi-Fi adapter.

## Steps to start the demonstration
Install the following software pieces:
* [ST-Link driver for the OS you are using](https://www.st.com/en/development-tools/stsw-link008.html)
* A Serial terminal to communicate with the NUCLEO-F429ZI board. [Putty is a valid option](https://www.putty.org/)
* Optionally, an IDE to recompile the example: IAR or TrueSTUDIO

Once you have the above resources, follow the steps described below:
1. Connect the WF200 expansion kit to the WFx interposer
2. Connect the WFx interposer to the NUCLEO-F429ZI board
3. Make sure the two switches on the WF200 expansion kit are on the correct position:
    * "On Board LDO" for the power switch
    * SPI or SDIO for the bus switch depending on the bus you want to test
4. Connect the NUCLEO board to your PC using the USB cable. The board should appear as a mass storage device named 
"NODE_F429ZI"
5. Open Putty or the serial terminal chosen and connect to the COM port of the NUCLEO port using 115200 bps for the speed
6. Drag-and-drop or copy the WF200_driver_F429ZI_BM-LwIP_xxx.bin file to the "NODE_F429ZI" mass storage. The file can be
found under WF200_driver_F429ZI_BM-LwIP/binaries. The "xxx" are to be replaced by the bus (SPI/SDIO) you selected
through the WF200 expansion kit bus switch. 
Alternatively, you can use your preferred IDE to compile and flash the project into the NUCLEO board.
7. Once the binary file transferred, you should be prompted on the serial terminal.
8. By default the example starts in the SoftAp mode using the SSID and passkey below:
    * SSID: WF200_AP
    * Passkey: 12345678
9. Make sure your Wi-Fi interface is configured to use static IP address and is part of the same network as the address
192.168.0.1 (192.168.0.2 for example). To use the example in station mode, you need to recompile the project using IAR 
or TrueSTUDIO by changing the example configuration in the header file "demo_config.h".
10. In both cases, a web page hosted in the STM32 is displayed. It gives control to board LEDs. In addition, pressing
the User Button on the NUCLEO board will start a Wi-Fi scan sequence.
11. Finally, using the same IP address, you can start an iPerf TCP test between the WF200 and another device. First 
install [iPerf](https://iperf.fr/) on your machine. Once installed, call the command below:

iperf -c [IP address displayed] -i 1

