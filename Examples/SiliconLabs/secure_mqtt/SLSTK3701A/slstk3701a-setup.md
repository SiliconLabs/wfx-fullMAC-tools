# Set Up a EFM32 Giant Gecko GG11 Starter Kit (SLSTK3701A) and a WFx Wi-Fi® Expansion Kit

1. Connect the WFx Wi-Fi® Expansion Kit to the EFM32 Giant Gecko GG11 Starter Kit
2. Make sure the two switches on the WF200 expansion kit are on the correct position:
    * "On Board LDO" for the power switch
    * SPI or SDIO for the bus switch depending on the bus you want to test
3. Connect the GG11 STK board to your PC using the USB cable. The board should appear as a device named 
"JLink CDC UART"
4. Open Putty or the serial terminal chosen and connect to the COM port of the GG11 STK board using 115200 bps for the speed
5. Flash the demo hex file using the Simplicity Studio flash programmer or alternatively, compile the project using the Simplicity Studio IDE.
    * The hex files can be found in the **binaries** folder of the project. The hex file bus type should match the bus (SPI/SDIO) you selected through the WFx Wi-Fi® Expansion Kit bus switch.
    * To compile the project in Simplicity Studio, import the existing project file to the workspace ([**steps below**](#importing-the-example-project-into-simplicity-studio)).
6. Once the binary file transferred, log traces appears on the serial terminal. 

## Importing the Example Project into Simplicity Studio

1. Clone the github repository to your computer
2. In Simplicity Studio choose **Import...** from the File menu
3. At the bottom of the dialog that appears click **More Import Options...**
4. Under General select **Existing Projects into Workspace**
5. Click **Next** and browse to the location of the cloned repository. Then click **Finish**
