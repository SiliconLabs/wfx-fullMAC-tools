# WGM160P Commissioning Example

To use the example, follow the [**demonstration quick start guide**](https://docs.silabs.com/wifi/wf200/content-source/getting-started/silabs/wgm160/getting-started).

## Removing Gecko OS from the WGM160P

In the case, Simplicity Studio throws an error message in the Flash Programmer window or if flashing your application doesn't work, it could mean that Gecko OS has been previously installed on the chip.
To use the WGM160P with this example it is necessary to follow the steps below to remove Gecko OS and configure Simplicity Studio to communicate with the WGM160P as a bare metal EFM32 device.

1. Connect the WGM160P STK board to your PC using the USB cable. 
2. Launch Simplicity Studio.
3. Open the Simplicity Commander tool by typing "commander" in the Simplicity Studio search box or by browsing to C:\SiliconLabs\SimplicityStudio\v4\developer\adapter_packs\commander.
![fig2](https://github.com/SiliconLabs/wfx-fullMAC-tools/blob/master/Examples/SiliconLabs/mcu_examples/WGM160P_micriumos_lwip_wfx200/readme_files/commander.png?raw=true)
4. Click "Connect" in commander to connect to the adapter and then to connect to the target. 
5. Then press the "Unlock debug access" button following by the "Erase chip" button.
6. Finally, copy the default bootloader bl-uart-geckoG1-v2.07.hex (located in the readme_files folder) to the WGM160P by clicking the "Browse..." button to select the binary file and then press the "Flash" button to load it to the WGM160P. 
7. Power cycle the WGM160P STK by unplugging the USB cable and then reconnecting.