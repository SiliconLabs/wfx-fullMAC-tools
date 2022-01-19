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
Unless otherwise specified in the directory, all examples are considered to be EXPERIMENTAL QUALITY which implies that the code provided in the repository has not been formally tested and is provided as-is. It is not suitable for production environments.

# Get Started
## Add the Examples to Simplicity Studio 5

1. Download and install [Simplicity Studio 5](https://www.silabs.com/developers/simplicity-studio)
2. On Simplicity Studio 5, go to **Window -> Preferences -> Simplicity Studio -> External Repos**.
3. Click **[Add]** and enter on **URL** the following link `https://github.com/SiliconLabs/wfx-fullMAC-tools.git` .
4. Click **[Next]** then **[finish]** and **[Apply and Close]** 
5. Restart Simplicity Studio 5.

## Run the Examples

First, select your product under **Debug Adapter** or **My products**. Then, in the **Launcher** perspective under **EXEMPLE PROJECTS & DEMOS**, Check **wfx-fullmac-tools** under **Provider** and click **[create]** on the desired project to import it to **{}Simplicity IDE**.

## Repository Content

This repository contains applications using the FMAC driver.

  * [*wifi_cli*](./wifi_cli_micriumos/README.md): An application providing a UART command line interface to interact with the Wi-Fi FMAC driver, LwIP APIs and NVM3 APIs.
  * [*secured_mqtt*](./secured_mqtt/README.md): An application providing a UART command line interface to establish MQTT over TLS connection using Micrium OS, LwIP (MQTT, DHCP, DNS clients), Mbed TLS and NVM3.
    