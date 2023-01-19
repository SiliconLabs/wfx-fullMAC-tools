# Secured MQTT Example

This example project provides an example of a secured MQTT client implementation using a WFx chip and the FMAC driver.
The lwIP stack and Mbed TLS third-party libraries being already used by WFx projects, the natural choice has been to use the MQTT client implementation provided by the lwIP stack and the TLS layer implementation provided by Mbed TLS. A comforting reason is that Mbed TLS is intended to be easily integrated to the lwIP stack. Additionally, the Non-Volatile Memory driver, NVM3, provided in the SDK, is used in order to maintain the configuration between reboots and thus simplify the example use.

## Requirements

### Hardware Prerequisites

One of the supported platforms listed below is required to run the example:

* [**EFM32 Giant Gecko GG11 Starter Kit (SLSTK3701A)**](https://www.silabs.com/products/development-tools/mcu/32-bit/efm32-giant-gecko-gg11-starter-kit) with
  [**WF200 Wi-Fi® Expansion Kit (SLEXP8022A)**](https://www.silabs.com/products/development-tools/wireless/wi-fi/wf200-expansion-kit) or
  [**WFM200S Wi-Fi® Expansion Kit (SLEXP8023A)**](https://www.silabs.com/products/development-tools/wireless/wi-fi/wfm200-expansion-kit)
* [**EFR32MG Zigbee and Thread Starter Kit (SLWSTK6000B)**](https://www.silabs.com/development-tools/wireless/zigbee/efr32mg-zigbee-thread-starter-kit) with
  [**WF200 Wi-Fi® Expansion Kit (SLEXP8022A)**](https://www.silabs.com/products/development-tools/wireless/wi-fi/wf200-expansion-kit) or
  [**WFM200S Wi-Fi® Expansion Kit (SLEXP8023A)**](https://www.silabs.com/products/development-tools/wireless/wi-fi/wfm200-expansion-kit)
* [**WGM160P Wi-Fi® Module Starter Kit**](https://www.silabs.com/products/development-tools/wireless/wi-fi/wgm160p-wifi-module-starter-kit)

Additionally, a PC is required to configure the board and it can also be used to load a binary file on the board, to compile the Simplicity Studio project or to run a local MQTT broker.

### Software Prerequisites

The required software includes:

* Simplicity Studio v5 and the Gecko SDK Suite (4.0.0 or newer)
* The example project and the Wi-Fi Full MAC driver (available in the Gecko Platform SDK)
* A serial terminal to communicate with the board. For example, [**Tera Term**](https://osdn.net/projects/ttssh2/releases/) or [**Putty**](https://www.putty.org/)
* An MQTT broker either integrated to Cloud platforms like [**AWS IoT Core**](https://aws.amazon.com/) or [**Azure IoT Hub**](https://azure.microsoft.com/), or local for the tests like a [**Mosquitto Broker**](https://mosquitto.org/) 

*The Micrium OS Kernel is designed to run on Silicon Labs devices only and it is free of charge. Lightweight IP (lwIP) is an open-source TCP/IP stack licensed under the BSD license. Mbed TLS is a cryptographic library licensed under Apache-2.0 license.*

## Install Simplicity Studio 5 and the SDKs

Simplicity Studio 5 is a free software suite needed to start developing your application. To install Simplicity Studio 5, please follow this [**procedure**](https://docs.silabs.com/simplicity-studio-5-users-guide/latest/ss-5-users-guide-getting-started/install-ss-5-and-software) by selecting the options **[Install by connecting device(s)]** and **[Auto]**.

## Set Up your Kit and Get the Example

Please follow the instructions related to the platform suiting your case:

* [**EFM32 Giant Gecko GG11 Starter Kit setup** or **EFR32MG Zigbee and Thread Starter Kit setup**](https://docs.silabs.com/wifi/wf200/content-source/getting-started/silabs/ssv5/gg11/wifi-commissioning-micriumos/getting-started)
* [**WGM160P Wi-Fi® Module Starter Kit setup**](https://docs.silabs.com/wifi/wf200/content-source/getting-started/silabs/ssv5/wgm160p/wifi-commissioning-micriumos/getting-started)

Note that at step 6, the example name is **Platform - Wi-Fi Secured MQTT on Micrium OS kernel**

## Start the Example

1. Please follow this [**instruction**](https://docs.silabs.com/wifi/wf200/content-source/getting-started/silabs/ssv5/gg11/wifi-commissioning-micriumos/getting-started#start-the-example) to compile, flash the kit with our example. Note that at step 5, the binary file is **secured_mqtt.hex**.
2. Setup Wi-Fi station, input the SSID, password and security mode of your Wi-Fi Access Point, using corresponding **wifi set** commands as below
    ```
    > wifi set station.ssid <AP_name>
    > wifi set station.passkey <AP_password>
    > wifi set station.security WPA1/WPA2
    ```
3. Establish the Wi-Fi connection, using 
    ```
    > wifi connect
    ```
    > **Note:** Make sure to successfully setup your desired broker before going to the next steps. Follow [**_this guide_**](#local-mosquitto-broker) to setup a local broker. For Cloud IoT brokers, see [**_Azure instructions_**](#azure-iot-hub) to configure Azure IoT Hub or [**_AWS instructions_**](#aws-iot-core-service) for AWS IoT Core services.

4. Enter the MQTT broker address, both IP and Domain address are supported, using 
    ```
    > mqtt set broker.address <broker_address>
    ```
5. Enter the port used by the MQTT broker, using 
    ```
    > mqtt set broker.port <port>
    ```

6. Enter the MQTT client ID of your device, using 
    ```
    > mqtt set client_id <client ID>
    ```
    > **Note:** Client ID is especially essential when communicating with a cloud service because this element is often used to identify the device. Therefore, the client ID and the device name registred on the Cloud IoT services should be the same.

7. Enter the publish topic name, using 
    ```
    > mqtt set publish_topic.name <publish topic name>
    ```
8.  Enter the subscribe topic name, using 
    ```
    > mqtt set subscribe_topic.name <subscribe topic name>
    ```
10. Set a username and password if required by the MQTT broker, using 
    ``` 
    > mqtt set broker.username <username> 
    > mqtt set broker.password <password (optional)>
    ```
    > **Note:** For Azure IoT Hub, the user name is required with a specific format.
11. Set or update the TLS certificates and device private key (please refer to [**TLS Security**](#tls-security) section for more information), using 
    ```
    > mqtt set tls.certification_authority
    > mqtt set tls.device_certificate
    > mqtt set tls.device_private_key
    ```
    > **Note:** Copy & paste the certificate line-by-line to the console, and enter a dot '.' at the last line to complete. Following, is an example:
    ```
    > mqtt set tls.device_private_key
    [Enter device private key line by line, followed by . (dot) on a separate line]
    Certificate> -----BEGIN RSA PRIVATE KEY-----
    Certificate> MIIEpAIBAAKCAQEA0PBZH7iqxuveNFC0tRyTWAmiRfeNRgDhYJMFaRHvrsKbWVxH
    Certificate> 7e5j633JN83gsfYmaskbq1h2BcdSix8nWk/hJJttb0BoUG1mjR4cVWJanmu3XTcy
    Certificate> 4rSeRPe8/6cCLnCIbKwVifnowb9YMUxu04JtpRgnU67GxbmPH4r8w3Y/3elhT7gr
    Certificate> xzOuBkItwxRupzK6gkTj9r+nqOFOTnLsDyAz/ST8yZ4bIAjkadwXXJZewnVc7U7f
    Certificate> Avs4C1ZiCBVqz9869Msj7U6Uj5iuY9rM8+ADh0LOiBgGrf2xELANChbsIfE3lriC
    Certificate> K5vgSowGlsSceZ33g/0fkuxllbQyqlUsJBwhbsUkOSwqa+5TFzisIA==
    Certificate> -----END RSA PRIVATE KEY-----
    Certificate> .
    ```
1.  Establish the secure MQTT TLS connection, using 
    ```
    > mqtt connect
    ```
2.  Publish a message, using 
    ``` 
    > mqtt publish <message>
    ```
3.  The content of the messages on the subscribe topic, either from the Cloud or from another MQTT client will be displayed on the serial interface
4.  Save the Wi-Fi configuration to non-volatile memory, using 
    ```
    > wifi save
    ```
5.  Save the MQTT configuration to non-volatile memory, using 
    ```
    > mqtt save
    ```

## TLS Security

The application is expecting a double authentication between the client (i.e. the device) and the server (i.e. the MQTT broker) which is the most secured and the most used by cloud services.
This is why the **certificate of a Certification Authority (CA)** signing the server certificate, a **device certificate** signed by your own CA and a **device private key** are requested to run the example successfully.
The expected format for these information is the **x509 PEM format**.

These information can either be:

* Retrieved from the cloud service used
* Generated using the [**OpenSSL Toolkit**](https://www.openssl.org/)
* Retrieved from this repository (**only for a local test and debug**)

<br>

## Azure IoT Hub

> To test this example with an Azure IoT Hub server, an Azure IoT Hub account is required. First create an account if you don't already have one. Azure offers free accounts with $200 credit for 30 days in addition to free services, you can try this for testing. [Start free here.](https://azure.microsoft.com/en-us/free/) 

### Broker Configuration

#### CA Certificate

The first thing to do, if it is not already done, is to add your Certification Authority (CA) certificate to your Azure IoT Hub account.
This certificate will be used to authenticate the device certificate, itself signed by this CA certificate.

If you don't already have a CA certificate, you can either:

* Create one from a known certification authority (e.g. [**Let's Encrypt**](https://letsencrypt.org/)). This option is recommended in the case where the device certificate will be sent to third parties. Indeed, having a CA certificate, itself signed by a well known certification authority enforces the trust.
* Create one yourself using the [**OpenSSL Toolkit**](https://www.openssl.org/) for instance. This option is sufficient if the device certificate is used by "internal" services. The Azure IoT Hub service can be considered as "internal" since you manage the CA certificate used to authenticate the devices.
    + For testing purposes only, using *openssl utility* on Linux Shell to create a CA certificate of the device with a dummy password can be refered as following examples.

      > **Note:** Press enter to skip questions. The common name field can be "deviceCA". The output is the *"deviceCA.pem"* file which will be uploaded to Azure IoT Hub in the next steps.

      ```
      $ openssl genrsa -passout pass:1111 -des3  -out deviceCA-private.key 2048
      $ openssl req -x509 -new -nodes -passin pass:1111 -key deviceCA-private.key -sha256 -days 3650 -out deviceCA.pem
      ```
    
The instructions below describes how to add your CA certificate to an Azure IoT Hub account:

* Click on **[Add]** from the **[Security settings -> Certificates]** page.
* Enter a certificate name.
* Retrieve the CA certificate from your filesystem. (e.g: deviceCA.pem)
* Click on **[Save]**.

Now that the CA certificate is added, it should appear in the **[Security settings -> Certificates]** page with an **_Unverified_** status.
Azure IoT Hub service requires a *Proof a Possession (PoP)* of the CA certificate to change the status to **_Verified_**. This consist into challenging you to create a dummy certificate signed by this CA certificate to make sure you have total access to this CA certificate and private key.

To realize the *Proof of Possession* follow the instructions below:

* Select your CA certificate in the **[Settings -> Certificates]** page.
* Click on **[Generate Verification Code]** in the Certificate Details window.
* Copy the Verification Code.
* Create a new key
* Create a signature request to sign this new key by the CA certificate and enter the Verification Code generated by Azure IoT Hub as Common Name of this request.
* Create the verification certificate with the signature request.
* Upload the verification certificate from the **Certificates**  window.
* Finally click on **[Verify]**.

Your CA certificate status should change to a **_Verified_** status.

For instance, the generation of *PoP* using *openssl utility* on Linux Shell can be demonstrated as below.

> **Note:** Remember to copy the verification code from Azure IoT Hub and paste the code as the answer to *"Common Name"* field when you run the following commands.

```
$ openssl genrsa -out verifyCA.key 2048
$ openssl req -new -key verifyCA.key -out verifyCA.csr
$ openssl x509 -req -in verifyCA.csr -passin pass:1111 -CA deviceCA.pem -CAkey deviceCA-private.key -CAcreateserial -days 3650 -sha256 -out verifyCA.pem
```

 
  Then, upload the verification certificate corresponding to the verification code to Azure IoT Hub then press **[Verify]** button to complete the process.

<p align="center">
  <img src="resources/azure_CA_pop.png" width="70%" title="CA verification process of Azure IoT Hub" alt="CA verification process of Azure IoT Hub"/>
</p>

#### Device Creation

* Click on **[Add devices]** from the **[Device management -> Devices]** page.
* Enter a **Device ID**. (*e.g: wgm160p*)

> Take care to enter a Device ID corresponding to the **_Common Name_** field of the device certificate.

* Select **[X.509 CA Signed]**
* Finalize the device creation by clicking on **[Save]**.

### Device Certificate/Private Key Generation

As for the CA certificate, the Azure IoT Hub lets you manage the generation of device certificates and private keys, the [**OpenSSL Toolkit**](https://www.openssl.org/) provides the means to generate them.

The below example shows some steps to generate private key & device certificate for testing purposes.

> **Note**: Make sure to enter the same device ID that was registered on Azure IoT Hub (*e.g: _wgm160p_*) to the **_Common Name_** field when creating the *Certificate Signing Request (CSR)* and the other fields can be left as default.

```
$ openssl genrsa -out device-private.key 2048
$ openssl req -new -key device-private.key -out device.csr
$ openssl x509 -req -in device.csr -passin pass:1111 -CA deviceCA.pem -CAkey deviceCA-private.key -CAcreateserial -days 3650 -sha256 -out deviceCert.crt
$ openssl x509 -in deviceCert.crt -out deviceCert.pem -outform PEM
```

The generated files, device certificate (*deviceCert.pem*) and device private key (*device-private.key*), will be supplied to the device before it tries to connect to Azure IoT Hub broker.

### Azure Certificate

The Azure IoT Hub certificate (****_Baltimore CyberTrust Root_****) is necessary for the device to authenticate the server.
This certificate can be downloaded at [*https://www.digicert.com/kb/digicert-root-certificates.htm*](https://www.digicert.com/kb/digicert-root-certificates.htm). 

In the future, there might be some critical changes of this CA certificate to **_DigiCert Global G2_** certificate and devices that don't have *DigiCert Global G2* certificate won't be able to connect to Azure IoT Hub anymore. [Please refer to this blog post](https://techcommunity.microsoft.com/t5/internet-of-things-blog/azure-iot-tls-critical-changes-are-almost-here-and-why-you/ba-p/2393169) and [this document for more details.](https://docs.microsoft.com/en-us/azure/security/fundamentals/tls-certificate-changes)

### MQTT Communication to Azure IoT Hub

| Name             | Value                                      | Comment                                                |
|------------------|--------------------------------------------|--------------------------------------------------------|
| Broker address   | xxxxxxxxxxxx.azure-devices.net             | Imposed                                                |
| Port             | 8883                                       | Imposed                                                |
| Client ID        | *DeviceID*                                 | Imposed, e.g. wgm160p                                  |
| Username         | xxxxxxxxxxxx.azure-devices.net/_DeviceId_  | Imposed, e.g. xxxxxxxxxxxx.azure-devices.net/wgm160p   |
| Password         |                                            | Empty                                                  |
| Publish Topic    | devices/_DeviceId_/messages/events/        | Imposed, e.g. devices/wgm160p/messages/events/         |
| Subscribe Topic  | devices/_DeviceId_/messages/devicebound/#  | Imposed, e.g. devices/wgm160p/messages/devicebound/#   |

<br>

## AWS IoT Core Service

> To test this example with an AWS server, an AWS account is required. First create an account if you don't already have one. [Get started here.](https://aws.amazon.com/iot-core/)

### Broker Configuration

#### Policy Creation

The policies define the actions allowed or denied to a device (aka Thing in AWS) or a group of devices.

Follow the instructions below to create a policy:

* Click on **[Create]** in **[Secure -> Policies]** page of the AWS IoT Core Service.
* Enter a name describing the policy. (_e.g._ efm32gg11_policy)
* Under **[Add statements]** section, add the operations you want to allow or deny. AWS autofills the ARN field with a template, adjust it to fit your needs. Then, check on the corresponding **_Allow_/_Deny_** box.  (_e.g._ Action: iot:Connect; Resource ARN: arn:aws:iot:us-west-2:572603184724:client/efm32gg11)
* Click on the **[Add statement]** button to add more operations such as iot:Publish/Subscribe/Receive.
* Finalize the policy creation by clicking on **[Create]**.

Here is an example of what a specific policy looks like:

```
{
  "Version": "2012-10-17",
  "Statement": [
    {
      "Effect": "Allow",
      "Action": "iot:Connect",
      "Resource": "arn:aws:iot:us-west-2:572603184724:client/efm32gg11"
    },
    {
      "Effect": "Allow",
      "Action": "iot:Publish",
      "Resource": "arn:aws:iot:us-west-2:572603184724:topic/efm32gg11/data"
    },
    {
      "Effect": "Allow",
      "Action": "iot:Subscribe",
      "Resource": "arn:aws:iot:us-west-2:572603184724:topicfilter/efm32gg11/rx"
    },
    {
      "Effect": "Allow",
      "Action": "iot:Receive",
      "Resource": "arn:aws:iot:us-west-2:572603184724:topic/efm32gg11/rx"
    }
  ]
}
```

This policy allows only MQTT clients, with an **ID** corresponding to an existing thing name (e.g. **_efm32gg11_**), to:

* connect to the MQTT broker.
* publish data **only** on the topic **_ThingName_/data**.
* subscribe and allow to receive messages **only** from the topic **_ThingName_/rx**.

> Notice that this Policy is quite restrictive especially concerning the allowed publish and subscribe topics.
Feel free to write your own suiting best your use case.

#### Thing Creation

* Click on **[Create things]** in **[Manage -> Things]** page and **[Create a single thing]** in the next page. Enter a thing name required in the next menu (e.g. **_efm32gg11_**).
    > **Note:** The thing name must match with the client ID of device
* Generate the device certificate and keys by selecting the option suiting your use case.

> You can either let AWS generate them, this option is the easiest and quickest especially if you don't have the knowledges about certificate and key generation.
Other options are useful if you already have a device private key, or even your own Certification Authority (CA) allow you to manage all the sensitive information.
These options will require the use of the [**OpenSSL Toolkit**](https://www.openssl.org/).

In case of choosing "_Auto-generate a new certificate (recommended)_" option:

* **[Attach a policy]**: Select the created policy in the previous step (e.g. efm32gg11_policy). **Note**: Without this policy the thing is basically useless.
* Finalize the thing creation by clicking on **[Create Thing]**.
* **[Download]** the generated device certificate, private key & Root CA certificate from the resulting page. 
* Make sure to **[Activate]** the device from the same page.

### AWS Certificate

The Amazon server certificate (**Amazon Root CA 1**) is necessary for the device to authenticate the server.
This certificate can be retrieved at [**https://www.amazontrust.com/repository/**](https://www.amazontrust.com/repository/) or in the previous download step.

### MQTT Communication to AWS IoT Core

| Name             | Value                                                                       | Comment                                                |
|------------------|-----------------------------------------------------------------------------|--------------------------------------------------------|
| Broker address   | Available in the **[Settings -> Device data endpoint]** section of AWS IoT  | Imposed                                                |
| Port             | 8883                                                                        | Imposed                                                |
| Client ID        | Thing name                                                                  | Imposed, e.g. efm32gg11                                |
| Username         | None                                                                        |                                                        |
| Password         | None                                                                        |                                                        |
| Publish Topic    | Depends on the policies attached to the thing                               | e.g. efm32gg11/data                                    |
| Subscribe Topic  | Depends on the policies attached to the thing                               | e.g. efm32gg11/rx                                      |

### Test MQTT TLS Connection to Cloud IoT Brokers
Prior to directly deploying the program & testing MQTT TLS connection of your devices to Cloud IoT brokers (Azure, AWS), it would be a good idea to use some MQTT client software (e.g. MQTT.fx) to validate the configuration of the Cloud IoT brokers. 

#### AWS IoT Core
The image below presents the MQTT.fx client profile for testing the MQTT TLS connection to the AWS IoT core broker.
> **Note:** Make sure that the client ID is the same as the registered thing name on the AWS IoT core broker. Most of connection issues resulted from this mismatch or certificates creation.

<p align="center">
  <img src="resources/aws_mqttfx_profile.png" width="50%" title="MQTT.fx client profile for testing the connection with AWS IoT Core" alt="MQTT.fx client profile connecting to AWS IoT Core"/>
</p>


It's also desired to test the dispatching message operations of the broker.

MQTT.fx client receives messages by subscribing to the **"efm32gg11/rx"** topic. On the AWS IoT broker, go to **[MQTT test client]** under **[Test]** section and send a message to the topic. Then MQTT.fx will receive the message from AWS IoT broker as the following image.

<p align="center">
  <img src="resources/aws_mqttfx_sub.PNG" width="80%" title="MQTT.fx client receives messages from AWS IoT Core on the allowed subscribed topic" alt="MQTT.fx client subcribe to a topic with AWS IoT Core">
</p>

In addition, MQTT.fx client can publish messages to the AWS IoT broker via the allowed **"efm32gg11/data"** topic.

<p align="center">
  <img src="resources/aws_mqttfx_pub.png" width="80%" title="MQTT.fx client publishes messages to AWS IoT Core on the allowed pulish topic" alt="MQTT.fx client publish to a topic with AWS IoT Core">
</p>

#### Azure IoT Hub
In the same manner, MQTT.fx client also is used to test the MQTT TLS connection to Azure IoT broker and the profile setting shown as below:

<p align="center">
  <img src="resources/azure_mqttfx_profile.PNG" width="50%"  title="MQTT.fx client profile for testing the connection with Azure IoT Hub" alt="MQTT.fx Azure profile">
</p>

> **Note:**  Make sure to input the right format of *"User Name"* with *"xxxxxxxxxxxx.azure-devices.net/wgm160p"* under **_"User Credentials"_** tab.

After obtaining the successful connection state to Azure IoT Hub, we would like to monitor messages sent from the client. However, Azure IoT Hub is not full-featured MQTT broker, so that device can't directly subscribe to device topics from other devices, it is recommeded to send messages from Azure Cloud through IoT solutions back-end to the connected devices. [You can learn more about Azure MQTT support here](https://docs.microsoft.com/en-us/azure/iot-hub/iot-hub-mqtt-support).

Therefore, our test aims at monitoring the device-to-cloud (D2C) & cloud-to-device (C2D) messages. [Learn more here.](https://docs.microsoft.com/en-us/azure/iot-hub/iot-hub-devguide-messaging)

On the Azure IoT Hub page, open **Azure Cloud shell** with *Bash terminal* and enter the following command to monitor deivce-to-cloud messages:
```
$ az iot hub monitor-events --output table --device-id <DeviceID> --hub-name <IoT Hub name>
```
MQTT.fx client sends a message to Azure Cloud IoT Hub via the published topic *(e.g: devices/wgm160p/messages/events/)*, it's expected to see the received message on Azure Cloud Shell.

<p align="center">
  <img src="resources/azure_mqttfx_pub.PNG" width="80%" title="MQTT.fx client sends messages to Azure IoT Hub on the pulished topic" alt="MQTT.fx publish to Azure"/>
</p>

MQTT.fx client can receive messages from Azure Cloud IoT Hub via the subscribed topic *(e.g. devices/wgm160p/messages/devicebound/#)*. On the Azure IoT Hub page, click on **[Device managements] -> [Devices]**, select the device wanted to send a message; then open **[Message to Device]** to input and send the message as the image below:

<p align="center">
  <img src="resources/azure_mqttfx_sub.PNG" width="80%" title="MQTT.fx client receives messages from Azure IoT Hub on the subscribed topic" alt="MQTT.fx subscribe to Azure">
</p>

After successfully testing the configuration of Cloud IoT brokers, it's ready for deploying your program on the IoT devices.
<br>

## Local Mosquitto Broker

> Please see [**Known issue**](#known-issues) section

As mentioned in the **TLS Security** section, certificates and keys examples are provided in this repository to ease the first steps of starting this example in a local environment,
meaning running a MQTT broker on your PC. This section describes how to start a local MQTT broker.

> **The certificates and keys provided by this repository are only to use during tests and debug sessions on a local environment, and should not be used in production or outside of this example.**

### Configuration Adaptation

A configuration file example (*./resources/security_files/mosquitto_tls_examples.conf.sample*) is provided in the repository, the following operations are still required to adapt the configuration to your installation:

1. Copy/paste the file.
2. Rename it, for instance *./resources/security_files/mosquitto_tls_examples.conf*.
3. Open it and set the according paths to files contained in the **security_files** project folder.

Now the configuration file is set, the MQTT broker can be launched.

### MQTT Broker Launch

1. Open a new shell at the [**security_files**](./resources/security_files/) folder location.
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

Complementarily to the **Traffic Monitoring**, a new MQTT client can be executed to send a message on a topic and ensure that the MQTT broker dispatches the topic the other clients subscribed
to this topic, in this case the MQTT client monitoring the traffic.

1. Open a new shell at the **security_files** folder location.
2. Run the following command suiting your use case:

**Windows:** `& 'C:\Program Files\mosquitto\mosquitto_pub.exe' -h localhost -t "test/broker" -m "Hello World!" --cafile .\ca.crt --cert .\mosquitto_client.crt --key .\mosquitto_client.key`

**Linux:** `mosquitto_pub -h localhost -t "test/broker" -m "Hello World!" --cafile .\ca.crt --cert .\mosquitto_client.crt --key .\mosquitto_client.key`

## Known Issues
The tests with the local broker could be failed with the following message: 
> Client <unknown> disconnected: Protocol Error

We are working on it to solve this issue soon!

## NVM3

NVM3 is the name of the driver provided in the SDK and used in this example to keep the configurations between reboots, for more information about NVM3
please refer to [AN1135 Using Third Generation NonVolatile Memory (NVM3) Data Storage](https://www.silabs.com/documents/public/application-notes/an1135-using-third-generation-nonvolatile-memory.pdf).
 