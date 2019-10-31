# RF test agent tool

This repository contains a multi-target application to execute basic
RF tests (TX Carrier Wave tone, TX modulated, RX, ...) on a WFx chip. The application is a bridge between a test system,
typically a computer, and a WFx chip. It forwards commands received from the
MCU UART to the WFx chip, using the [wfx-fullMAC-driver](https://github.com/SiliconLabs/wfx-fullMAC-driver), and sends back the output traces through the MCU UART. It is meant to be used in conjunction with the Python Test Feature tools available in the [wfx-common-tools](https://github.com/SiliconLabs/wfx-common-tools) repository.

The MCU targets currently supported are:

* EFM32GG11 (Platform [SLSTK3701A](https://www.silabs.com/products/development-tools/mcu/32-bit/efm32-giant-gecko-gg11-starter-kit))
* STM32F429 (Platform [NucleoF429ZI](https://www.st.com/en/evaluation-tools/nucleo-f429zi.html))

**Restriction: the SDIO interface is not supported on the EFM32GG11 target.**

## UART configuration

The UART configuration is the following:

| Parameter | Value  |
|-----------|--------|
| Baudrate  | 115200 |
| Byte size | 8      |
| Parity    | N      |
| Stop bits | 1      |

## Commands

Below the command list provided by the RF test agent:

* [write_test_data](#write_test_data)
* [read_rx_stats](#read_rx_stats)
* [read_agent_version](#read_agent_version)
* [read_driver_version](#read_driver_version)
* [read_fw_version](#read_fw_version)

### Command format

Each command must be:

* prefixed with `wfx_test_agent`
* ended with a LF character `\n` (0x0A)

### write_test_data

Command executing the RF tests.

Usage: `wfx_test_agent write_test_data <compressed_pds>`

> Note: please refer to the [wfx-pds](https://github.com/SiliconLabs/wfx-pds)
repository and its [README](https://github.com/SiliconLabs/wfx-pds/blob/API3.0/README.md) file for information and PDS examples.

Example: RX test mode

```
wfx_test_agent write_test_data  "{i:{a:B,b:2,f:3E8,c:{a:0,b:0,c:0,d:44},d:{a:BB8,b:0,c:0,d:15,e:0,f:4},e:{}}}"
Send PDS OK
```


### read_rx_stats

Command to retrieve the latest RX stats.

> Note: only usable after putting the WFx chip in RX mode.

Usage: `wfx_test_agent read_rx_stats`

Example:

```
wfx_test_agent read_rx_stats
Timestamp: 119980938us
Low power clock: frequency 32785Hz, external yes
Num. of frames: 173, PER (x10e4): 4104, Throughput: 158Kbps/s
.     Num. of      PER     RSSI      SNR      CFO
rate   frames  (x10e4)    (dBm)     (dB)    (kHz)
 1M         77      129      -57       32      -25
 2M          0        0        0        0        0
 5.5M        0        0        0        0        0
 11M         3        0      -85        7      -36
 6M          8    10000      -79       11        0
 9M         13    10000      -81       10        5
 12M        21     2857      -78       11      -19
 18M         8    10000      -78       11       15
 24M        12     6666      -76       12        4
 36M         8    10000      -77       12      -32
 48M         9    10000      -80        9        7
 54M        10    10000      -79        6      -13
 MCS0        0        0        0        0        0
 MCS1        0        0        0        0        0
 MCS2        2        0      -64       23        8
 MCS3        0        0        0        0        0
 MCS4        0        0        0        0        0
 MCS5        0        0        0        0        0
 MCS6        0        0        0        0        0
 MCS7        2        0      -67       32      -43
```

### read_agent_version

Command to retrieve the version of the RF test agent.

Usage: `wfx_test_agent read_agent_version`

Example:

```
wfx_test_agent read_agent_version
1.0.0
```

### read_driver_version

Command to retrieve the version of the WFx fullMAC driver.

Usage: `wfx_test_agent read_driver_version`

Example:

```
wfx_test_agent read_driver_version
2.2.0
```

### read_fw_version

Command to retrieve the version of the firmware used by the WFx chip.

Usage: `wfx_test_agent read_fw_version`

Example:

```
wfx_test_agent read_fw_version
3.0.0
```

## WFx common tools

The modification and the compression of a PDS for the different RF tests
can be an heavy task. To ease this sequence, Silicon Labs provides tools in the "test-feature" folder of the [wfx-common-tools](https://github.com/SiliconLabs/wfx-common-tools) repository.

These Python tools provide a set of user friendly commands easing the RF tests.
Theses commands make the translation to [RF test agent commands](#commands) and
if necessary generate a PDS, compress and send it to the RF test agent
application. For more information about these tools, please refer to their
[README](https://github.com/SiliconLabs/wfx-common-tools/blob/master/test-feature/README.md) file.

## MCU upload

### EFM32GG11

The EFM32GG11 target can be easily uploaded with the RF test agent application
using either an IDE (Simplicity Studio or IAR) or the drag and drop feature.

#### Simplicity Studio IDE

Use the **Flash Programmer** tool provided by the IDE, browse the project directory
and use the binary `binaries/SimplicityStudio/RF_test_agent.hex`.

#### IAR IDE

Follow `Project->Download->Download File...` from the IAR IDE, browse the project
directory and use the binary `binaries/IAR/RF_test_agent_SLSTK3701A_SPI.bin`.

#### Drag and drop feature

In a File Explorer, open the Mass Storage directory related to the platform in a first window and the directory `binaries/SimplicityStudio/` (or `binaries/IAR/`)
in an other one. Then drag and drop the choosen binary into the Mass Storage directory, the plaform will be automatically flashed with the new application. 

### STM32F429

The STM32F429 target can be easily uploaded with the RF test agent application
using either an IDE (IAR) or the drag and drop feature.

#### IAR IDE

The upload procedure is the same as [EFM32GG11 upload using IAR IDE](#iar-ide), but the binary to use is `binaries/IAR/RF_test_agent_NUCLEOF429_SPI.bin`
or `binaries/IAR/RF_test_agent_NUCLEOF429_SDIO.bin` (minding the switch of WFx chip interface).

#### Drag and drop feature

The upload procedure is the same as [EFM32GG11 upload using the drag and drop feature](#drag-and-drop-feature), but the binary to use is
`binaries/IAR/RF_test_agent_NUCLEOF429_SPI.bin` or `binaries/IAR/RF_test_agent_NUCLEOF429_SDIO.bin` (minding the switch of WFx chip interface).