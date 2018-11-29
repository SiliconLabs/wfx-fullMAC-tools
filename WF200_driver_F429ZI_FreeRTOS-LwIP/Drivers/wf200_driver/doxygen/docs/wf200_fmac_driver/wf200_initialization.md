Initialization and configuration {#wf200_initialization}  
============

It is recommended to reset WF200 before running the initialization phase described below.
The initialization and configuration is managed by ::wf200_init. WF200 initialization phase is as follow:
* Bus configuration (SPI or SDIO)
* WF200 initialization
* Firmware download
* Startup indication reception
* PDS configuration

## 1. BUS configuration
The bus configuration is handled by ::wf200_init_bus. It has a specific implementation depending on the bus used.

###SPI bus

During the SPI initialization, the host follows the steps below:
* Reset WF200
* Read the \ref config_register "configuration register" to make sure the communication with WF200 is available.
* Write the \ref config_register "configuration register" with the SPI bus configuration settings preferred (e.g. word mode, high frequency SPI enabled...).

###SDIO bus

During the SPI initialization, the host follows the steps below:
* Send SDIO command 0 \warning It is required for the host to wait a response from this command before issuing another command.
* Send SDIO command 8
* Send SDIO command 3
* Send SDIO command 7 
* Enable function 1/cmd 53 in \ref CCCR_register "CCCR register".
* Enable interrupt.
* Configure the SDIO bus to function in 4-bit mode.
* Switch to high-speed mode (up to 50MHz).
* Set the block size used by the host in the \ref FBR_register "FBR register".

## 2. WF200 initialization
The WF200 initialization is handled by ::wf200_init_chip. During this phase, the host interacts with WF200 registers described in \ref wf200_registers.
* The first step performed is to set the correct values in the General purpose registers. Those values are linked to the crystal used by the hardware.
* Set the wake-up bit (bit 12) in the \ref control_register "control register".
* Wait for the wlan_rdy bit (bit 13) to be set in the \ref control_register "control register".
* Check if the access mode bit (bit 10) in the \ref config_register "configuration register" is at 1 (direct access mode).

## 3. Firmware download 

First of all, the WF200 clock is enabled in ::wf200_download_run_bootloader. An optional test to verify the SRAM access is performed.

The firmware download is handled by ::wf200_download_run_firmware.
A firmware binary needs to be loaded to the chip at power-up. The binary is signed and encrypted. This binary file is stored in a c table in wfm_wf200_XX.h (XX depending on the keyset used by WF200).
Below is a diagram listing the steps followed by ::wf200_download_run_firmware to load the firmware.
\msc 
  hscale = "1"; 

  a [label="Host"],b [label="WF200"];

  a box a [label = "READY"]; 
  b box b [label = "NOT READY"]; 
  b=>a [ label = "INFO_READY"]; 
  a=>b [ label = "INFO_READ"]; 
  b=>a [ label = "READY"];
  a=>b [ label = "UPLOAD_PENDING"]; 
  b box b [label = "DOWNLOAD_PENDING"]; 
  b box b [label = "DOWNLOAD_COMPLETE"]; 
  b=>a [ label = "AUTH_OK"];
  a=>b [ label = "OK_TO_JUMP"]; 
\endmsc

After the host notifies the "OK_TO_JUMP" state, the WF200 will issue a startup indication when ready. 

## 4. Startup indication
If the initialization proccess has been successful up to now the host should receive the startup indication from WF200. 

\todo describe startup indication info when the doxygen struture def is available.

## 5. PDS configuration
Once the startup indication received, the host can send to WF200 the **PDS configuration** (Platform Data Set). The PDS contains information regarding the WF200 environment. Below are some examples:
* IO configuration
* XTAL tune
* TX power control
* RF output configuration
* ...

The PDS is sent using a dedicated function ::wf200_send_configuration.
The PDS presents itself in the FMAC driver as several string tables found in wf200_pds.c. Those tables are the results of the compression of a more complete and lisible file. 
You can find more information on the PDS in a dedicated page \ref wf200_pds.

