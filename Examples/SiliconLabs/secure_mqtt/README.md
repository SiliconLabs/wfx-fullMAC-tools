# Secure MQTT Example

The purpose of this application is to provide an example of secure MQTT client implementation using a WFx chip and the FMAC driver.
The lwIP stack and Mbed TLS libraries being already used by WFx projects, the natural choice has been to use the MQTT client implementation
provided by the lwIP stack and the TLS layer implementation provided by Mbed TLS. A comforting reason is that Mbed TLS is intended to be easily 
integrated to the lwIP stack.

## Requirements

### Hardware Prerequisites

One of the supported platforms listed below is required to run the example:

* [**EFM32 Giant Gecko GG11 Starter Kit (SLSTK3701A)**](https://www.silabs.com/products/development-tools/mcu/32-bit/efm32-giant-gecko-gg11-starter-kit) with
  [**WF200 Wi-Fi® Expansion Kit (SLEXP8022A)**](https://www.silabs.com/products/development-tools/wireless/wi-fi/wf200-expansion-kit) or
  [**WFM200S Wi-Fi® Expansion Kit (SLEXP8023A)**](https://www.silabs.com/products/development-tools/wireless/wi-fi/wfm200-expansion-kit)
* [**WGM160P Wi-Fi® Module Starter Kit**](https://www.silabs.com/products/development-tools/wireless/wi-fi/wgm160p-wifi-module-starter-kit)

Additionally, a PC is required to configure the board and it can also be used to load a binary file on the board, to compile the Simplicity Studio project or to run a local MQTT broker.

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

The application is expecting a double authentication between the client (i.e. the device) and the server (i.e. the MQTT broker) which is the most secured and the most used by cloud services.
This is why the certificate of a Certification Authority (CA) signing the server certificate, a device certificate signed by your own CA and a device private key are requested during the start.
The expected format for these information is the x509 PEM format.

These information can either be:

* Retrieved from the cloud service used
* Generated using the [**OpenSSL Toolkit**](https://www.openssl.org/)
* Retrieved from this repository (**only for a local test and debug**)

As mentioned above, certificates and keys examples are provided in this repository to ease the first steps of starting this example in a local environment, meaning
running a MQTT broker on your PC. Please refer to the [**Mosquitto Broker**](#mosquitto-broker) section to start a local MQTT broker.

> **These certificates and keys are provided only for test and debug on a local environment, and should not be used in production or outside of this example.**

<br>
## Local Mosquitto Broker

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

<br>
## Azure IoT Hub

> To test this example with an Azure IoT Hub server, an Azure IoT Hub account is required. First create an account if you don't already have one.

### Configuration

#### CA Certificate

The first thing to do, if it is not already done, is to add your Certification Authority (CA) certificate to your Azure IoT Hub account.
This certificate will be used to authenticate the device certificate, itself signed by this CA certificate.

If you don't already have a CA certificate, you can either:

* Create one from a known certification authority (e.g. [**Let's Encrypt**](https://letsencrypt.org/)). This option is recommended
in the case where the device certificate will be sent to third parties. Indeed, having a CA certificate, itself signed by a well known
certification authority enforces the trust.
* Create one yourself using the [**OpenSSL Toolkit**](https://www.openssl.org/) for instance. This option is sufficient if the device
certificate is used by "internal" services. The Azure IoT Hub service can be considered as "internal" since you manage the CA certificate
used to authenticate the devices.

The instructions below describe how to add your CA certificate to a Azure IoT Hub account:

* Click on **[Add]** from the **[Settings -> Certificates]** page.
* Enter a certificate name.
* Retrieve the CA certificate from your filesystem.
* Click on **[Save]**.

Now that the CA certificate is added, it should appear in the **[Settings -> Certificates]** page with an **_Unverified_** status.
Azure IoT Hub service requires a *Proof a Possession* of the CA certificate to change the status to **_Verified_**. This consist
into challenging you to create a dummy certificate signed by this CA certificate to make sure you have total access to this
CA certificate and private key.

To realize the *Proof of Possession* follow the instructions below:

* Select your CA certificate in the **[Settings -> Certificates]** page.
* Click on **[Generate Verification Code]** in the Certificate Details window.
* Copy the Verification Code.
* Create a new key
* Create a signature request to sign this new key by the CA certificate and enter the Verification Code generated by Azure IoT Hub as Common Name of this request.
* Create the verification certificate with the signature request.
* Upload the verification certificate from the Certificate Details window.
* Finally click on **[Verify]**.

Your CA certificate status should change to a **_Verified_** status.

#### Device Creation

* Click on **[New]** from the **[Explorers -> IoT devices]** page.
* Enter a Device ID.

> Take care to enter a Device ID corresponding to the Common Name field of the device certificate.

* Select **[X.509 CA Signed]**
* Finalize the device creation by clicking on **[Save]**.

### Device Certificate/Key Creation

As for the CA certificate, the Azure IoT Hub let you manage the generation of device certificates and private keys,
the [**OpenSSL Toolkit**](https://www.openssl.org/) provides the means to generate them.

### MQTT communication

| Name             | Value                                      | Comment                                                |
|------------------|--------------------------------------------|--------------------------------------------------------|
| Broker address   | xxxxxxxxxxxx.azure-devices.net             | Imposed                                                |
| Port             | 8883                                       | Imposed                                                |
| Username         | xxxxxxxxxxxx.azure-devices.net/_DeviceId_  | Imposed, e.g. xxxxxxxxxxxx.azure-devices.net/efm32gg11 |
| Password         |                                            | Empty                                                  |
| Publish Topic    | devices/_DeviceId_/messages/events/        | Imposed, e.g. devices/efm32gg11/messages/events/       |
| Subscribe Topic  | devices/_DeviceId_/messages/devicebound/#  | Imposed, e.g. devices/efm32gg11/messages/devicebound/# |

<br>
## AWS IoT Core Service

> To test this example with an AWS server, an AWS account is required. First create an account if you don't already have one.

### Configuration

#### Policy Creation

The policies define the actions allowed or denied to a device (aka Thing in AWS) or a group of devices.

Follow the instructions below to create a policy:

* Click on **[Create]** in **[Secure -> Policies]** page of the IoT Core Service.
* Enter a name describing the policy.
* Add the operation(s) you want to allow or deny. AWS autofills the ARN field with a template, adjust it to fit your needs.
* Finalize the policy creation by clicking on **[Create]**.

Here is an example of what a policy looks like:

```
{
  "Version": "2012-10-17",
  "Statement": [
    {
      "Effect": "Allow",
      "Action": "iot:Connect",
      "Resource": "arn:aws:iot:eu-west-3:xxxxxxxxxxxx:client/${iot:Connection.Thing.ThingName}"
    },
    {
      "Effect": "Allow",
      "Action": "iot:Publish",
      "Resource": "arn:aws:iot:eu-west-3:xxxxxxxxxxxx:topic/${iot:Connection.Thing.ThingName}/data"
    },
    {
      "Effect": "Allow",
      "Action": "iot:Subscribe",
      "Resource": "arn:aws:iot:eu-west-3:xxxxxxxxxxxx:topicfilter/${iot:Connection.Thing.ThingName}/rx"
    },
    {
      "Effect": "Allow",
      "Action": "iot:Receive",
      "Resource": "arn:aws:iot:eu-west-3:xxxxxxxxxxxx:topic/${iot:Connection.Thing.ThingName}/rx"
    }
  ]
}
```

This policy allows only MQTT clients, with an Id corresponding to an existing thing name, to:

* connect to the MQTT broker.
* publish data on the topic **_ThingName_/data**.
* subscribe and allow to receive messages from the topic **_ThingName_/data**.


#### Thing Creation

* Click on **[Create]** in **[Manage -> Things]** page and **[Create a single thing]** in the next page. Only a name is required in this menu.
* Generate the device certificate and keys by selecting the option suiting your use case.

> You can either let AWS generate them, this option is the easiest and quickest especially if you don't have the knowledges about certificate and key generation.
Other options are useful if you already have a device private key, or even your own Certification Authority (CA) allow you to manage all the sensitive information.
These options will require the use of the [**OpenSSL Toolkit**](https://www.openssl.org/).

* **[Download]** the generate certificate and private key from the resulting page.
* **[Activate]** the device from the same page.
* **[Attach a policy]** (without it the thing is basically useless).
* Finalize the thing creation by clicking on **[Register Thing]**.

### AWS certificate

The Amazon server certificate (**Amazon Root CA 1**) is necessary to the device the authenticate the server.
This certificate can be retrieve at [**https://www.amazontrust.com/repository/**](https://www.amazontrust.com/repository/).

### MQTT communication

| Name             | Value                                                                       | Comment                                                |
|------------------|-----------------------------------------------------------------------------|--------------------------------------------------------|
| Broker address   | Available in the **[Settings -> Custom endpoint]** section of Azure IoT Hub | Imposed                                                |
| Port             | 8883                                                                        | Imposed                                                |
| Username         | None                                                                        |                                                        |
| Password         | None                                                                        |                                                        |
| Publish Topic    | Depends on the policies attached to the thing                               | e.g. efm32gg11/data                                    |
| Subscribe Topic  | Depends on the policies attached to the thing                               | e.g. efm32gg11/rx                                      |
 