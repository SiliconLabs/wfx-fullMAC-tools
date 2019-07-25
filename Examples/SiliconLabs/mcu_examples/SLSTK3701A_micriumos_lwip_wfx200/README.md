# EFM32/MicriumOS/LwIP WFx FMAC driver example

## Hardware prerequisites
This demonstration example runs the WFx FMAC driver meant to communicate with the WFx Silicon Labs Wi-Fi solution. To 
use the example properly, below are the necessary hardware:
* [the WF200 Wi-Fi® Expansion Kit SLEXP8022A](https://www.silabs.com/products/development-tools/wireless/wi-fi/wf200-expansion-kit)
* [the EFM32 Giant Gecko GG11 Starter Kit](https://www.silabs.com/products/development-tools/mcu/32-bit/efm32-giant-gecko-gg11-starter-kit) (also referred to as the GG11 STK)
* A micro USB cable
* A PC to load a binary file on the GG11 STK board or to compile the Simplicity Studio project. In addition, it can be used 
to test the example if it is equipped with a Wi-Fi adapter.

## Steps to start the demonstration
Install the following software pieces:
* [Simplicity Studio](https://www.silabs.com/products/development-tools/software/simplicity-studio)
* A Serial terminal to communicate with the GG11 STK board. [Putty is a valid option](https://www.putty.org/)

Once you have the above resources, follow the steps described below:
1. Connect the WF200 expansion kit to the Giant Gecko GG11 starter kit
2. Make sure the two switches on the WF200 expansion kit are on the correct position:
    * "On Board LDO" for the power switch
    * SPI or SDIO for the bus switch depending on the bus you want to test
3. Connect the GG11 STK board to your PC using the USB cable. The board should appear as a device named 
"JLink CDC UART"
4. Open Putty or the serial terminal chosen and connect to the COM port of the GG11 STK board using 115200 bps for the speed
5. Flash the demo hex file using the Simplicity Studio flash programmer or alternatively, compile the project using the Simplicity Studio IDE.
    * The hex files can be found under SLSTK3701A_micriumos_lwip_wfx200/binaries. The hex file bus type should match the bus (SPI/SDIO) you selected
through the WF200 expansion kit bus switch. 
    * To compile the project in Simplicity Studio, import the existing project file to the workspace.
6. Once the binary file transferred, you should be prompted on the serial terminal. 
7. Press "enter", otherwise the example will start by default in SoftAp mode with the name "WF200_AP" with the passkey
"12345678". If you have pressed "enter", reply to the information to configure the example as wanted.
8. Once the example correctly started:
    * In SoftAp mode, connect with a Wi-Fi device to the "WF200_AP" access point (default name). You can open a browser
    and go to the web page 192.168.0.1 (again value by default).
    * In station mode, use a device in the same network to access the web page. Enter the IP address displayed on the 
    serial terminal in a web browser. 
9. In both cases, a web page hosted in the EFM32 GG11 is displayed. It gives control to board LEDs.
10. Finally, using the same IP address, you can start an iPerf TCP test between the WF200 and another device. First 
install [iPerf](https://iperf.fr/) on your machine. Once installed, call the command below:

iperf -c [IP address displayed] -i 1

