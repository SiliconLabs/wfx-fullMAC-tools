# WGM160P/MicriumOS/LwIP WFx FMAC driver example

## Hardware prerequisites
This demonstration example runs the WFx FMAC driver meant to communicate with the WFx Silicon Labs Wi-Fi solution. To 
use the example properly, below are the necessary hardware:
* [the WGM160P Wi-Fi Module Starter Kit](https://www.silabs.com/products/development-tools/wireless/wi-fi/wgm160p-wifi-module-starter-kit) (also referred to as the WGM160P STK)
* A micro USB cable
* A PC to load a binary file on the WGM160P STK board or to compile the Simplicity Studio project. In addition, it can be used 
to test the example if it is equipped with a Wi-Fi adapter.

## Software prerequisites
Install the following software:
* [Simplicity Studio](https://www.silabs.com/products/development-tools/software/simplicity-studio)
* A Serial terminal to communicate with the WGM160P STK board. [Putty is a valid option](https://www.putty.org/)

## Removing Gecko OS from the WGM160P
The WGM160P ships with Gecko OS installed. To use the WGM160P with this example it is necessary to follow the steps below to remove Gecko OS and configure Simplicity Studio to communicate with the WGM160P as a bare metal EFM32 device.

1. Connect the WGM160P STK board to your PC using the USB cable. 
2. Launch Simplicity Studio. Right click on the Jlink device under "Debug Adapters". Choose "Device Configuration". If you are prompted to update the board firmware do this now before going on to the next step.
3. Remove "WGM160P Wi-Fi Module Radio Board" from the board list in the device hardware tab. Then add EFM32GG11B820F2048GM64 as the target part. Click "OK".
4. Open the Simplicity Commander tool by typing "commander" in the Simplicity Studio search box or by browsing to C:\SiliconLabs\SimplicityStudio\v4\developer\adapter_packs\commander.
5. Click "Connect" in commander to connect to the adapter and then to connect to the target. 
6. Then press the "Unlock debug access" button following by the "Erase chip" button.
7. Finally, copy the default bootloader bl-uart-geckoG1-v2.07.hex (located in the readme_files folder) to the WGM160P by clicking the "Browse..." button to select the binary file and then press the "Flash" button to load it to the WGM160P. 
8. Power cycle the WGM160P STK by unplugging the USB cable and then reconnecting.

## Steps to start the demonstration
Once you have the above resources, and have removed Gecko OS, follow the steps described below:

1. Connect the WGM160P STK board to your PC using the USB cable. The board should appear as a device named 
"JLink CDC UART"
2. Open Putty or the serial terminal chosen and connect to the COM port of the WGM160P STK board using 115200 bps for the speed
3. Flash the demo hex file using the Simplicity Studio flash programmer or alternatively, compile the project using the Simplicity Studio IDE.
    * The hex files can be found under WGM160P_micriumos_lwip_wfx200/binaries. 
    * To compile the project in Simplicity Studio, import the existing project file to the workspace (steps below).
4. Once the binary file transferred, you should be prompted on the serial terminal. 
5. Press "enter", otherwise the example will start by default in SoftAp mode with the name "WF200_AP" with the passkey
"12345678". If you have pressed "enter", reply to the information to configure the example as wanted.
6. Once the example correctly started:
    * In SoftAp mode, connect with a Wi-Fi device to the "WF200_AP" access point (default name). You can open a browser
    and go to the web page 192.168.0.1 (again value by default).
    * In station mode, use a device in the same network to access the web page. Enter the IP address displayed on the 
    serial terminal in a web browser. 
7. In both cases, a web page hosted in the EFM32 GG11 is displayed. It gives control to board LEDs.
8. Finally, using the same IP address, you can start an iPerf TCP test between the WF200 and another device. First 
install [iPerf](https://iperf.fr/) on your machine. Once installed, call the command below:

iperf -c [IP address displayed] -i 1

## Importing the example project into Simplicity Studio
The WGM160P STK example shares code with the GG11 STK example so make sure to clone the complete repository before importing the project.

1. Clone the github repository to your computer
2. In Simplicity Studio choose "Import..." from the File menu.
3. At the bottom of the dialog that appears click "More Import Options..."
4. Under General select "Existing Projects into Workspace"
5. Click Next and browse to the location of the cloned repository. Then click Finish.