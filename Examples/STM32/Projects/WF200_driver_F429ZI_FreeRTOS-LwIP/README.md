# STM32/FreeRTOS OS/LwIP Wi-Fi FMAC Driver Example

## Hardware Prerequisites
This demonstration example runs the **Wi-Fi FMAC driver** meant to communicate with the **WF200 Silicon Labs Wi-Fi solution**. To use the example properly, below are the necessary hardware:
* [the WF200 Wi-Fi® Expansion Kit SLEXP8022A](https://www.silabs.com/products/development-tools/wireless/wi-fi/wf200-expansion-kit).
* The Arduino/NUCLEO interposer.
* A NUCLEO-F429ZI development board.
* A micro USB cable.
* A PC to load a binary file in the NUCLEO board or to compile the IAR/TrueSTUDIO projects. In addition, it can be used
to test the example if it is equipped with a Wi-Fi adapter.

## Software Prerequisites
Install the following software pieces:
* [ST-Link driver for the OS you are using](https://www.st.com/en/development-tools/stsw-link008.html)
* A serial terminal to communicate with the NUCLEO-F429ZI board. [Putty is a valid option](https://www.putty.org/)
* Optionally, an IDE to recompile the example: IAR Embedded Workbench or Atollic TrueSTUDIO

## Step-By-Step Wi-Fi Commissioning
The WF200 supports a specific feature called **simultaneous combo mode** which eases the **Wi-Fi commissioning**. The device can **act simultaneously as a Wi-Fi access point and as a Wi-Fi station** connected to an access point. The two interfaces can even be used on different Wi-Fi channels. Through this mode, a user can easily configure a device through the access point interface and connect it as station to a home network for example. This example shows an implementation of this Wi-Fi commissioning. To perform the commissioning, follow the steps below:

### Flash the STM32F4
1. Connect the WF200 expansion kit to the Arduino interposer.
2. Connect the Arduino interposer to the NUCLEO-F429ZI board.
3. **Make sure the two switches on the WF200 expansion kit are on the correct position**:
    * "On Board LDO" for the power switch.
    * SPI or SDIO for the bus switch depending on the bus you want to test;
4. Connect the NUCLEO board to your PC using the USB cable. The board should appear as **a mass storage device named "NODE_F429ZI"**.
5. Drag-and-drop or copy the WF200_driver_F429ZI_FreeRTOS_xxx.bin file to the "NODE_F429ZI" mass storage. The file can be found under **WF200_driver_F429ZI_FreeRTOS-LwIP/binaries**. "xxx" is to be replaced by the bus (SPI/SDIO) you selected through the WF200 expansion kit bus switch.
Alternatively, you can use your preferred IDE to compile and flash the project into the NUCLEO board (IAR Embedded Workbench or Atollic TrueSTUDIO).

### Serial Terminal (Optional)
6. Open Putty or another serial terminal and connect to the NUCLEO COM port using 115200 bps speed.
7. Once the binary file transferred, you should be prompted on the serial terminal.
```
FMAC Driver version    2.2.0
WF200 Firmware version 3.0.0
WF200 initialization successful
Press enter within 5 seconds to configure the demo...
```
8. **Press "enter" if you want to modify the default settings**, otherwise the example will start by default in SoftAp mode with the name **"WF200_AP"** and the passkey **"12345678"**. If you have pressed "enter", reply to the information to configure the example as wanted.
9. If you choose to configure the example as station, the WF200 connects to the specified access point and retrieves a dynamic IP address.
```
Connected
IP address : 192.168.1.247
```
You can **jump to the "Use the Station Interface" section.**

### Wi-Fi Commissioning
9. If you did not connect a serial terminal, the example will **automatically start in Wi-Fi access point** mode with the name **"WF200_AP"** and the passkey **"12345678"** after 5 seconds.
10. Connect a smartphone or a PC to the "WF200_AP" Wi-Fi access point.
11. Verify you successfully connected to "WF200_AP".
12. (Optional) at this point, you can test the **example throughput** by using the **TCP iPerf server** embedded in the example. To do so, the requirements differ if you are using a PC or a smartphone:
  * For PCs, install [iPerf](https://iperf.fr/) on your machine. Once installed, call the command below:
```
iperf -c 10.10.0.1 -i 1
```
 * For Android smartphones, install "Magic iPerf" using the Google Play Store. Open the application and enter the command above without the "iperf". Make sure "iPerf2" is selected. You can start the test by using the "Stopped" switch.
 * For IOS smartphones, install "HE.NET Network Tools" using the Apple Store. Open the application and open the side menu to click on "iperf". Make sure "TCP" is selected. Enter the number of bytes you want to send (e.g., 100 M) and finally enter the IP address in the upper text box (e.g., 10.10.0.1). The iPerf test starts.
13. Open a Web browser and **browse to the "10.10.0.1" Web page**. The Web page can control the LEDs on the board. Additionally, you can start a scan process by **clicking on "Refresh Wi-Fi Scan Results"**.
14. You can select one of the displayed access point to connect the WF200.
15. If security is required, you are prompted to provide the correct passkey.
16. **Click on the "Connect" button**, if the information entered are correct, a pop-up message appears to indicate the connection was successful. Soon after the connection, an IP address is displayed. This is the dynamic IP address of the WF200 station interface.  
17. You were **successful to provision the device using the WF200 simultaneous combo mode**. You can know access to the device using the station interface.

### Use the Station Interface

18. If you were successful at connecting the WF200 to your Wi-Fi network, you can access to the Web page hosted by the device. Open a Web browser and enter the IP address displayed either through the serial terminal or the Web page. If you kept the provisioning device and the associated Web page open, you can observe that the LED states are updated on both ends.
> If you click on the "Disconnect" button, the WF200 disconnects from the Wi-Fi access point. Once disconnected, you cannot reach the Web page from the station interface.
