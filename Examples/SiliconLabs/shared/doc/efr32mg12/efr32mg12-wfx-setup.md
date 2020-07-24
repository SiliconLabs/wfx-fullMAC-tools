# Set Up a EFR32MG12 Wireless Gecko Starter Kit (SLWSTK6000B) and a WFx Wi-Fi® Expansion Kit

1. Connect the WFx Wi-Fi® Expansion Kit to the EFR32MG12 Wireless Gecko Starter Kit
2. Make sure the two switches on the WF(M)200 expansion kit are on the correct position:
    * "On Board LDO" for the power switch
    * SPI for the bus switch
3. Connect the Silicon Labs Wireless Starter Kit baseboard to your PC using the USB cable. The board should appear as a device named "JLink CDC UART"
4. Open Putty or the serial terminal chosen and connect to the COM port of the Silicon Labs Wireless Starter Kit baseboard using 115200 bps for the speed
5. Flash the demo hex file using the Simplicity Studio flash programmer, either the provided hex files can be used or the hex file generated after compilation.
    * The hex files can be found in the **binaries** folder of the project
    * To compile the project in Simplicity Studio, import the existing project file to the workspace
6. Once the binary file transferred, log traces appears on the serial terminal

## Importing the Example Project into Simplicity Studio

1. Clone the github repository to your computer
2. In Simplicity Studio choose **Import...** from the File menu
3. At the bottom of the dialog that appears click **More Import Options...**
4. Under General select **Existing Projects into Workspace**
5. Click **Next** and browse to the location of the cloned repository. Then click **Finish**