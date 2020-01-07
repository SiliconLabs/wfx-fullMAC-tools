# Set Up a WGM160P Wi-Fi® Module Radio Board and a Silicon Labs Wireless Starter Kit baseboard

1. Plug the WGM160P Wi-Fi® Module Radio Board in the Silicon Labs Wireless Starter Kit baseboard
3. Connect the Silicon Labs Wireless Starter Kit baseboard to your PC using the USB cable. The board should appear as a device named 
"JLink CDC UART"
4. Open Putty or the serial terminal chosen and connect to the COM port of the Silicon Labs Wireless Starter Kit baseboard using 115200 bps for the speed
5. Flash the demo hex file using the Simplicity Studio flash programmer or alternatively, compile the project using the Simplicity Studio IDE.
    * The hex files can be found in the **binaries** folder of the project.
    * To compile the project in Simplicity Studio, import the existing project file to the workspace ([**steps below**](#importing-the-example-project-into-simplicity-studio)).
6. Once the binary file transferred, log traces appears on the serial terminal.

## Removing Gecko OS from the WGM160P

In the case, Simplicity Studio throws an error message in the Flash Programmer window, it could mean that Gecko OS has been previously installed on the chip.
To use the WGM160P with this example it is necessary to follow the steps below to remove Gecko OS and configure Simplicity Studio to communicate with the WGM160P as a bare metal EFM32 device.

1. Open the Simplicity Commander tool by typing "commander" in the Simplicity Studio search box or by browsing to C:\SiliconLabs\SimplicityStudio\v4\developer\adapter_packs\commander.
![fig2](https://github.com/SiliconLabs/wfx-fullMAC-tools/blob/master/Examples/SiliconLabs/mcu_examples/WGM160P_micriumos_lwip_wfx200/readme_files/commander.png?raw=true)
2. Click **Connect** in commander to connect to the adapter and then to connect to the target. 
3. Then press the **Unlock debug access** button following by the **Erase chip** button.
4. Finally, copy the default bootloader bl-uart-geckoG1-v2.07.hex (located in the **readme_files** folder) to the WGM160P by clicking the **Browse...** button to select the binary file and then press the **Flash** button to load it to the WGM160P. 
5. Power cycle the WGM160P STK by unplugging the USB cable and then reconnecting.

## Importing the Example Project into Simplicity Studio

The WGM160P STK example shares code with the GG11 STK example so make sure to first import the GG11 STK example before importing this project.

1. Clone the GitHub repository to your computer
2. In Simplicity Studio choose **Import...** from the File menu
3. At the bottom of the dialog that appears click **More Import Options...**
4. Under General select **Existing Projects into Workspace**
5. Click **Next** and browse to the location of the cloned repository. Then click **Finish**
