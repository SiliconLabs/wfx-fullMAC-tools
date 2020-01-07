# Secure MQTT Example

The purpose of this application is to provide an example of secure MQTT client implementation using a WFx chip and the FMAC driver.
The lwIP stack and Mbed TLS libraries being already used by WFx projects, the natural choice has been to use the MQTT client implementation
provided by the lwIP stack and the TLS layer implementation provided by Mbed TLS. A comforting reason is that Mbed TLS is intended to be easily 
integrated to the LwIP stack.

## Requirements

### Hardware Prerequisites

One of the supported platforms listed below is required to run the example:

* [**EFM32 Giant Gecko GG11 Starter Kit (SLSTK3701A)**](https://www.silabs.com/products/development-tools/mcu/32-bit/efm32-giant-gecko-gg11-starter-kit) with
  [**WF200 Wi-Fi® Expansion Kit (SLEXP8022A)**](https://www.silabs.com/products/development-tools/wireless/wi-fi/wf200-expansion-kit) or
  [**WFM200S Wi-Fi® Expansion Kit (SLEXP8023A)**](https://www.silabs.com/products/development-tools/wireless/wi-fi/wfm200-expansion-kit)
* [**WGM160P Wi-Fi® Module Starter Kit**](https://www.silabs.com/products/development-tools/wireless/wi-fi/wgm160p-wifi-module-starter-kit)

Additionally a PC is required to configure the board and it can also be used to load a binary file on the board, to compile the Simplicity Studio project or to run a local MQTT broker.

### Software Prerequisites

* The required software includes Simplicity Studio and the Gecko SDK Suite (32-bit MCU, Micrium OS Kernel, and lwIP)
* The example projects available in this repository
* A Serial terminal to communicate with the board. For example, [**Tera Term**](https://osdn.net/projects/ttssh2/releases/) or [**Putty**](https://www.putty.org/)
* A MQTT broker either integrated to a cloud platform like [**AWS**](https://aws.amazon.com/) or [**Azure IoT**](https://azure.microsoft.com/), or local for the tests like a [**Mosquitto Broker**](https://mosquitto.org/) 

*The Micrium OS Kernel is designed to run on Silicon Labs devices only and it is free of charge. Lightweight IP (lwIP) is an open-source TCP/IP stack licensed under the BSD license. Mbed TLS is a cryptographic library licensed under Apache-2.0 license.*

## Set Up your Kit

Please follow the instructions related to the platform suiting your case:

* [**EFM32 Giant Gecko GG11 Starter Kit setup**](SLSTK3701A/slstk3701a-setup.md)
* [**WGM160P Wi-Fi® Module Starter Kit setup**](WGM160P/wgm160p-setup.md)

## Start the Example

1. Once the binary file transferred, you should be prompted on the serial terminal. 
2. Enter the SSID and Passkey of the Wi-Fi Access Point you want your product to connect.
3. Wait for the Wi-Fi connection establishment.
4. **[Optional]** Press Enter within 5 seconds to load a new set of TLS certificates/keys.

	* Copy/Paste each certificate/key (x509 PEM format) in the terminal.
	* Validate each item by pressing Enter.

	> The certificates/keys are then stored at the end of the internal Flash memory, allowing you to skip this step between reboots.

5. Enter the MQTT broker address, both IP and Domain address are supported.
6. Once the example correctly started, a message containing the LED states is sent every second on the topic related to the platform used: `efm32gg11/leds/state` or `wgm160p/leds/state`.

Additionally:

* Pressing one of the push buttons also sends a message, containing the name of the button pushed, on the topic related to the platform used: `efm32gg11/button/event` or `wgm160p/button/event`.
The associated LED is toggled allowing to control that the application state.
* The device is subscribed to the topic related to the platform used: `efm32gg11/leds/set` or `wgm160p/leds/set`, allowing to change the LED states remotely by sending a JSON message
containing the name of LED to change the state (i.e. `LED0` or `LED1`) and the wanted state (i.e. `On` or `Off`). Here is an example of message accepted by the application `{"name":"LED0","state":"On"}`.

	> Keep in mind to escape the quotes in the message to send if you use a Shell, making the real message to send: `{\"name\":\"LED0\",\"state\":\"On\"}`.

## TLS Security

The application is expecting a double authentication between the client (i.e. the device) and the server (i.e. the MQTT broker) which is the most secured and the most used by cloud services,
which why the certificate of a Certification Authority (CA), a device certificate signed by the CA and a device private key are requested during the start. The expected format for these
information is the x509 PEM format.

These information can either be:

* Retrieved from the cloud service used
* Generated using the [**OpenSSL Toolkit**](https://www.openssl.org/)
* Retrieved from this repository (**only for a local test and debug**)

As mentioned above, certificates and keys examples are provided in this repository to ease the first steps of starting this example in a local environment, meaning
running a MQTT broker on your PC. Please refer to the [**Mosquitto Broker**](#mosquitto-broker) section to start a local MQTT broker.

> **These certificates and keys are provided only for test and debug on a local environment, and should not be used in production or outside of this example.**

## Mosquitto Broker

A MQTT broker can be easily launched on your computer by using for instance the Mosquitto broker.

### Configuration adaptation

A configuration file example (*.\security_files\mosquitto_tls_exmaples.conf.sample*) is provided in the repository, the following operations are still required to adapt the configuration to your installation:

1. Copy/paste the file.
2. Rename it, for instance *.\security_files\mosquitto_tls_exmaples.conf*.
3. Open it and set the according paths to files contained in the **security_files** project folder.

Now the configuration file is set, the MQTT broker can be launched.

### MQTT Broker Launch

1. Open a new shell at the **security_files** folder location.
2. Run the command:

**Windows:** `& 'C:\Program Files\mosquitto\mosquitto.exe' -c .\mosquitto_tls_examples.conf`

**Linux:** `mosquitto -c .\mosquitto_tls_examples.conf`

### Traffic Monitoring

A MQTT client, subscribed to all topics, can be launched for a monitoring purpose.

> This not recommended in a production environment but fits a test environment with a small traffic.

1. Open a new shell at the **security_files** folder location.
2. Run the following command suiting your use case to launch a MQTT client monitoring the traffic:

**Windows:** `& 'C:\Program Files\mosquitto\mosquitto_sub.exe' -h localhost -t "#" -v --cafile .\ca.crt --cert .\mosquitto_client.crt --key .\mosquitto_client.key`

**Linux:** `mosquitto_sub -h localhost -t "#" -v --cafile .\ca.crt --cert .\mosquitto_client.crt --key .\mosquitto_client.key`

### Test the MQTT Broker

Complementarily to the [**Traffic Monitoring**](#traffic-monitoring), a new MQTT client can be executed to send a message on a topic and ensure that the MQTT broker dispatches the topic the other clients subscribed
to this topic, in this case the MQTT client monitoring the traffic.

1. Open a new shell at the **security_files** folder location.
2. Run the following command suiting your use case:

**Windows:** `& 'C:\Program Files\mosquitto\mosquitto_pub.exe' -h localhost -t "test/broker" -m "Hello World!" --cafile .\ca.crt --cert .\mosquitto_client.crt --key .\mosquitto_client.key`

**Linux:** `mosquitto_pub -h localhost -t "test/broker" -m "Hello World!" --cafile .\ca.crt --cert .\mosquitto_client.crt --key .\mosquitto_client.key`
 