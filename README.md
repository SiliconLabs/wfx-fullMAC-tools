<table border="0">
  <tr>
    <td align="left" valign="middle">
	  <h1>WF200/WFM200/WGM160P<br/>Wi-Fi Sample Applications</h1>
	</td>
	<td align="left" valign="middle">
	  <a href="https://www.silabs.com/wireless/wi-fi">
	    <img src="http://pages.silabs.com/rs/634-SLU-379/images/WGX-transparent.png"  title="Wi-Fi Sample Applications" alt="logo" width="250"/>
	  </a>
	</td>
  </tr>
</table>

> To use Simplicity Studio 4 or third-party MCU based Wi-Fi examples, refer to the `wifi_examples_ssv4` branch

This repository contains examples to use with [**Silicon Labs Wi-Fi FMAC driver**](https://github.com/SiliconLabs/wfx-fullMAC-driver) (FMAC standing for Full-MAC).
These example codes are meant to be evaluated with hosts connected to the [WF200 Wi-Fi Expansion Kit](https://www.silabs.com/products/development-tools/wireless/wi-fi/wf200-expansion-kit) or
the [WFM200 Wi-Fi Expansion Kit](https://www.silabs.com/products/development-tools/wireless/wi-fi/wfm200-expansion-kit).
It can also be run on the [WGM160P Wi-Fi Module Starter Kit](https://www.silabs.com/products/development-tools/wireless/wi-fi/wgm160p-wifi-module-starter-kit).
Unless otherwise specified in the directory, all examples are of EXPERIMENTAL QUALITY which implies that the code provided in the repository has not been formally tested and is provided as-is. It is not suitable for production environments.

# Get Started
## Add the examples to Simplicity Studio 5
1. Download and install [Simplicity Studio 5](https://www.silabs.com/developers/simplicity-studio).

2. On Simplicity Studio 5, go to **Window -> Preferences -> Simplicity Studio -> External Repos**.

3. Click **[Add]**. In the **URI** field, copy & paste the following link `https://github.com/SiliconLabs/wfx-fullMAC-tools.git`

4. Click **[Next]** then **[Finish]** and **[Apply and Close]** .

5. Restart Simplicity Studio 5.

## Get the examples

1. Connect the Silicon Labs Starter Kit and open Simplicity Studio 5.

2. Select the **[Launcher]** perspective.

3. From the **[Debug Adapters]** panel on the left top corner, select your Silicon Labs Starter Kit.

4. Ensure that an SDK is selected in the **[General Information]** tile of the **[Overview]** tab.

5. Select the **[EXAMPLE PROJECTS & DEMOS]** tab in **[Launcher]** perspective.

6. Check **wfx-fullmac-tools** under _**Provider**_ and click **[create]** on the desired project.

## Repository Content

This repository contains applications using the FMAC driver.

  * [*wifi_cli*](./wifi_cli_micriumos/README.md): An application providing a UART command line interface to interact with the Wi-Fi FMAC driver, LwIP stack and NVM3 APIs.

  * [*secured_mqtt*](./secured_mqtt/README.md): An application providing a UART command line interface to establish MQTT over TLS connection using Micrium OS, LwIP (MQTT, DHCP, DNS clients), Mbed TLS and NVM3.

  * [*ethernet_bridge*](./ethernet_bridge/README.md): An application providing a network Bridge feature that allows data streaming between Ethernet and Softap interfaces by using the Micrium OS Network & wfx-fullMAC-driver's APIs.

  * [*multiprotocol_micriumos*](./multiprotocol_micriumos/README.md): An application providing a real-time Micrium OS-based example, which can use multiple protocols (Wi-Fi + BLE) simultaneously to toggle LEDs on the development board via a Webpage (over Wi-Fi) and EFR Connect BLE Mobile App (over BLE). The application demonstrates the combination of wfx-fullMAC-driver, lwIP and Bluetooth stack APIs.
    