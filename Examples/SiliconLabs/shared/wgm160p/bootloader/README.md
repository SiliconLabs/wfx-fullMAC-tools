# Removing Gecko OS from the WGM160P

1. Connect the WGM160P STK board to your PC using the USB cable. 
2. Launch Simplicity Studio.
3. Open the Simplicity Commander tool by typing "commander" in the Simplicity Studio search box or by browsing to `C:\SiliconLabs\SimplicityStudio\v4\developer\adapter_packs\commander`.
![fig2](commander.png?raw=true)
4. Click **[Connect]** in commander to connect to the adapter and then to connect to the target. 
5. Then press the **[Unlock debug access]** button following by the **[Erase chip]** button.
6. Finally, copy the default bootloader `bl-uart-geckoG1-v2.07.hex` (located in this folder) to the WGM160P by clicking the **[Browse...]** button to select the binary file
and then press the **[Flash]** button to load it to the WGM160P. 
7. Power cycle the WGM160P STK by unplugging the USB cable and then reconnecting.
