# STM32/FreeRTOS/LwIP WFx FMAC driver example

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
6. Drag-and-drop or copy the WF200_driver_F429ZI_FreeRTOS_xxx.bin file to the "NODE_F429ZI" mass storage. The file can be
found under WF200_driver_F429ZI_FreeRTOS-LwIP/binaries. The "xxx" are to be replaced by the bus (SPI/SDIO) you selected
through the WF200 expansion kit bus switch. 
Alternatively, you can use your preferred IDE to compile and flash the project into the NUCLEO board.
7. Once the binary file transferred, you should be prompted on the serial terminal. 
8. Press "enter", otherwise the example will start by default in SoftAp mode with the name "WF200_AP" with the passkey
"12345678". If you have pressed "enter", reply to the information to configure the example as wanted.
9. Once the example correctly started:
    * In SoftAp mode, connect with a Wi-Fi device to the "WF200_AP" access point (default name). You can open a browser
    and go to the web page 192.168.0.1 (again value by default).
    * In station mode, use a device in the same network to access the web page. Enter the IP address displayed on the 
    serial terminal in a web browser. 
10. In both cases, a web page hosted in the STM32 is displayed. It gives control to board LEDs.
11. Finally, using the same IP address, you can start an iPerf TCP test between the WF200 and another device. First 
install [iPerf](https://iperf.fr/) on your machine. Once installed, call the command below:

iperf -c [IP address displayed] -i 1

