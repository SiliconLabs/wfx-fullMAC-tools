# Multiprotocol Bare Metal Example

The purpose of this application is to provide a Bare Metal example using multiple protocols to do something as simple as toggling LEDs.

BLE and Wi-Fi protocols are used in this example to communicate with it, **neither the Dynamic Multiprotocol feature (DMP) nor a coexistence mechanism are used**.
The BLE interface is used to send advertisements/beacons allowing to connect to the device and toggle the LEDs via the Wireless Gecko application.
A Wi-Fi SoftAP interface is also provided by the device allowing to connect and access a Web Page and toggle the LEDs from it for instance.

## Requirements

### Hardware Prerequisites

One of the supported platforms listed below is required to run the example:

* [**EFR32xG21 Wireless Gecko Starter Kit (SLWSTK6006A)**](https://www.silabs.com/products/development-tools/wireless/efr32xg21-wireless-starter-kit) or
  [**EFR32™ Mighty Gecko Wireless Starter Kit (SLWSTK6000B)**](https://www.silabs.com/products/development-tools/wireless/mesh-networking/mighty-gecko-starter-kit)
* [**WF200 Wi-Fi® Expansion Kit (SLEXP8022A)**](https://www.silabs.com/products/development-tools/wireless/wi-fi/wf200-expansion-kit) or
  [**WFM200S Wi-Fi® Expansion Kit (SLEXP8023A)**](https://www.silabs.com/products/development-tools/wireless/wi-fi/wfm200-expansion-kit)

Additionally, this example requires:

* a PC to configure the board, load a binary file on the board, compile the Simplicity Studio project, visualize the traces or access the Web Page
* a Smartphone to interact with the BLE interface and access the Web Page

### Software Prerequisites

* The required software includes Simplicity Studio, the Gecko SDK Suite (32-bit MCU and lwIP) and the Bluetooth SDK
* The example projects available in this repository
* The **Wireless Gecko** application available on [**Google Play**](https://play.google.com/store/apps/details?id=com.siliconlabs.wirelessgecko) and [**App Store**](https://apps.apple.com/us/app/wireless-gecko/id1315784335)
* A Serial terminal to communicate with the board. For example, [**Tera Term**](https://osdn.net/projects/ttssh2/releases/) or [**Putty**](https://www.putty.org/)
* A Web browser 

## Set Up your Kit

Please follow the instructions related to the platform suiting your case:

* [**EFR32xG21 Wireless Gecko Starter Kit setup**](../../shared/doc/efr32xg21/efr32xg21-wfx-setup.md)
* [**EFR32 Mighty Gecko Wireless Starter Kit setup**](../../shared/doc/efr32mg12/efr32mg12-wfx-setup.md)

## Start the Example

1. Once the binary file transferred, the example starts sending BLE advertisements/beacons, launches a SoftAP interface named _**multiprotocol_softap**_
and displays the LED and the interface connection states on the LCD screen of the board (except for EFR32xG21 devices)
2. Enable the Bluetooth on your smartphone
3. Launch the Wireless Gecko application and start the **Connected Lighting Demo**
4. Select your BLE platform from the list of detected devices, it should be named **MPxxxx**.
If several devices are displayed, the two last bytes of the BLE MAC address, composing the BLE name, can be retrieved from the log traces displayed at boot
5. Your smartphone is now connected to the device using the Bluetooth interface.
Two dots are displayed on each side of the Bluetooth logo on the LCD screen to indicate this state of connection.
6. Touch the light bulb displayed by the application on the smartphone and watch the board LEDs toggle.
The LED state is also toggled on the LCD screen and an arrow appears shortly next to the Bluetooth logo to indicate the source of the light toggle.
7. Enable the Wi-Fi on your smartphone
8. Connect to the Access Point _**multiprotocol_softap**_ provided by the device, with the password **changeme**.
9. Open a Web browser on your smartphone and go to [http://10.10.0.1/](http://10.10.0.1/). If a popup appears due to a lack of Internet connectivity on the Wi-Fi interface
make sure to request to stay connected, otherwise the Web page may not be displayed in your browser. 
10. The displayed Web page gives the possibility to toggle the LEDs as the Wireless Gecko application, and it also gives more like:
    * Shutdown an active BLE connection
    * Enable/disable the BLE advertisements
    * Shutdown active connections with the Wi-Fi Softap
    * Enable/Disable the Wi-Fi Softap interface
    * Scan and connect to Access Points surrounding the device
    * Display various information about the BLE and Wi-Fi interfaces and their connections
11. LEDs can also be toggled from the push button PB0 on the board (except for EFR32xG21 devices)

This example is actually a combination of two already existing examples, for more information about the Wireless Gecko interactions, please refer to the [DMP Light Demo](https://www.silabs.com/documents/public/quick-start-guides/qsg155-dynamic-multiprotocol-demo-quick-start-guide.pdf) and
and for more information about the Web page interactions, please refer to the [Wi-Fi commissioning example](https://docs.silabs.com/wifi/wf200/content-source/getting-started/silabs/ssv4/gg11/wifi-commissioning-micriumos/interacting-with-the-example).
