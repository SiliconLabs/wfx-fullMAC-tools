# Set Up a WGM160P Wi-Fi® Module Radio Board and a Silicon Labs Wireless Starter Kit baseboard

1. Plug the WGM160P Wi-Fi® Module Radio Board in the Silicon Labs Wireless Starter Kit baseboard
2. Connect the Silicon Labs Wireless Starter Kit baseboard to your PC using the USB cable. The board should appear as a device named "JLink CDC UART"
3. Open Putty or the serial terminal chosen and connect to the COM port of the Silicon Labs Wireless Starter Kit baseboard using 115200 bps for the speed
4. Flash the demo hex file using the Simplicity Studio flash programmer, either the provided hex files can be used or the hex file generated after compilation.
    * The hex files can be found in the **binaries** folder of the project
    * To compile the project in Simplicity Studio, import the existing project file to the workspace
5. Once the binary file transferred, log traces appears on the serial terminal

## Removing Gecko OS from the WGM160P

In the case, Simplicity Studio throws an error message in the Flash Programmer window or if flashing your application doesn't work, it could mean that Gecko OS has been previously installed on the chip.
To use this example on a WGM160P it is necessary to [**remove Gecko OS**](../../wgm160p/bootloader/README.md) and configure Simplicity Studio to communicate with the WGM160P as a bare metal EFM32 device.

## Importing the Example Project into Simplicity Studio

1. Clone the GitHub repository to your computer
2. In Simplicity Studio choose **Import...** from the File menu
3. At the bottom of the dialog that appears click **More Import Options...**
4. Under General select **Existing Projects into Workspace**
5. Click **Next** and browse to the location of the cloned repository. Then click **Finish**
