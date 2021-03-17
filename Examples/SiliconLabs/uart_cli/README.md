# UART CLI Example

The purpose of this application is to provide a UART Command Line Interface (CLI) to allow interaction with part of the Wi-Fi FMAC driver and the LwIP APIs.

## Requirements

### Hardware Prerequisites

The example requires the hardware components below:

* [**EFM32 Giant Gecko GG11 Starter Kit (SLSTK3701A)**](https://www.silabs.com/products/development-tools/mcu/32-bit/efm32-giant-gecko-gg11-starter-kit) with
  [**WF200 Wi-Fi® Expansion Kit (SLEXP8022A)**](https://www.silabs.com/products/development-tools/wireless/wi-fi/wf200-expansion-kit) or
  [**WFM200S Wi-Fi® Expansion Kit (SLEXP8023A)**](https://www.silabs.com/products/development-tools/wireless/wi-fi/wfm200-expansion-kit)
* [**WGM160P Wi-Fi® Module Starter Kit**](https://www.silabs.com/products/development-tools/wireless/wi-fi/wgm160p-wifi-module-starter-kit)
* A mini USB cable
* A PC is required to configure the board and it can also be used to load a binary file on the board, to compile the Simplicity Studio project or run a serial terminal

### Software Prerequisites

The required software includes:

* The example projects available in this repository
* A Serial terminal to communicate with the board. For example, [**Tera Term**](https://osdn.net/projects/ttssh2/releases/) or [**Putty**](https://www.putty.org/)
* Optionally [**Simplicity Studio 4**](https://www.silabs.com/products/development-tools/software/simplicity-studio)

## Set up your Kit

Please follow the instructions related to the platform suiting your case:

* [**EFM32 Giant Gecko GG11 Starter Kit setup**](../shared/doc/slstk3701a/slstk3701a-setup.md)
* [**WGM160P Wi-Fi® Module Starter Kit setup**](../shared/doc/wgm160p/wgm160p-setup.md)

## Start the Example

1. **Open a Serial Terminal** application, such as PuTTY, configure it to the COM Port Number you found in the previous step, and set the UART settings to **115200 bps, 8 data bits, 1 stop bit and no parity**.
2. Reset the board using the **'RESET' button**. You should see the following output.

		Type "help" to view a list of registered commands.
		@

3. The uart_cli project is ready to receive commands. You can start by the 'help' command to list available commands.
	The 'help' list below is provided as an indication. To access an up-to-date list, use the 'help' command on the target.

		help                     : Lists all the registered commands
		cpu-reset                : Reset the host CPU
		cli-version              : Provide the version of the registered modules
		get                      : Get a parameter value
								Usage: get <param_name>
		set                      : Set a parameter value
								Usage: set <param_name> <param_value>
		wifi-reboot              : Reboot the Wi-Fi chip
		network-up               : Connect to the Wi-Fi access point with the information stored in wlan parameters
		nup                      : Alias of network-up
		network-down             : Disconnect from the Wi-Fi access point
		ndo                      : Alias of network-down
		scan                     : Perform a Wi-Fi scan
		wlan-pm                  : Enable/disable the Power Mode on the WLAN interface of the Wi-Fi chip
								NOTE: WLAN must be up
								Usage: wlan-pm <mode> [interval]
								mode: 0(awake), 1(wake-up on beacons), 2(wake-up on DTIMs)
								interval: number of beacons/DTIMs to skip while asleep
		wlan-ps                  : Enable/disable the Power Save on the WLAN interface of the Wi-Fi chip
								Usage: wlan-ps <state>
								state: 0(OFF), 1(ON)
		wlan-rssi                : Get the RSSI of the WLAN interface
		softap-up                : Start the SoftAP interface using the information stored in softap parameters
		sup                      : Alias of softap-up
		softap-down              : Stop the SoftAP interface
		sdo                      : Alias of softap-down
		softap-rssi              : Get the RSSI of a station connected to the SoftAP
								Usage: softap-rssi <sta_mac>
		softap-client-list       : Get the list of clients connected to the SoftAP
		iperf                    : Start a TCP iPerf test as a client or a server
								Usage: iperf <-c ip [-t dur] [-p port] [-k] | -s>
								ip: iPerf server address to connect to (IPv4 format)
								dur: test duration in client mode (default 10s)
								port: server port to connect to (default 5001)
								-k: execute the command in foreground
		iperf-server-stop        : Stop the running iPerf server
		iperf-client-stop        : Stop the running iPerf client
		ip-stats                 : Display the IP stack statistics
		ping                     : Send ICMP ECHO_REQUEST to network hosts
								Usage: ping [-n nb] <ip>
								ip: address to ping (IPv4 format)
								nb: number of requests to send (default 3)
		wfx_test_agent           : Send a command to the RF Test Agent
								Usage: wfx_test_agent <cmd> [cmd_args]


Additionally, you can display the specific command help by entering:

```
@ [command] help
```
