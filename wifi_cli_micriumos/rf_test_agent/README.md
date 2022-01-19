# RF test agent tool

This repository contains a multi-target application to execute basic
RF tests (TX Carrier Wave tone, TX modulated, RX, ...) on a WFx chip. The application is a bridge between a test system,
typically a computer, and a WFx chip. It forwards commands received from the
MCU UART to the WFx chip, using the [wfx-fullMAC-driver](https://github.com/SiliconLabs/wfx-fullMAC-driver), and sends back the output traces through the MCU UART. It is meant to be used in conjunction with the Python Test Feature tools available in the [wfx-common-tools](https://github.com/SiliconLabs/wfx-common-tools) repository.

## UART configuration

The UART configuration is the following:

| Parameter | Value  |
|-----------|--------|
| Baudrate  | 115200 |
| Byte size | 8      |
| Parity    | N      |
| Stop bits | 1      |

## Commands

Below, the command list provided by the RF test agent:


 1. [write_test_data](#write_test_data)
 2. [read_rx_stats](#read_rx_stats)
 3. [read_agent_version](#read_agent_version)
 4. [read_driver_version](#read_driver_version)
 5. [read_fw_version](#read_fw_version)
  

### Command format

Each command must be:

* prefixed with `wifi test`

### write_test_data

Command executing the RF tests.

Usage: `wifi test write_test_data <compressed_pds>`

> Note: please refer to the [wfx-pds](https://github.com/SiliconLabs/wfx-pds)
repository and its [README](https://github.com/SiliconLabs/wfx-pds/blob/API3.0/README.md) file for information and PDS examples.

Example: RX test mode

```
wifi test write_test_data  "{i:{a:B,b:2,f:3E8,c:{a:0,b:0,c:0,d:44},d:{a:BB8,b:0,c:0,d:15,e:0,f:4},e:{}}}"
Send PDS OK
```


### read_rx_stats

Command to retrieve the latest RX stats.

> Note: only usable after putting the WFx chip in RX mode.

Usage: `wifi test read_rx_stats`

Example:

```
wifi test read_rx_stats
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

Usage: `wifi test read_agent_version`

Example:

```
wifi test read_agent_version
1.0.0
```

### read_driver_version

Command to retrieve the version of the WFx fullMAC driver.

Usage: `wifi test read_driver_version`

Example:

```
wifi test read_driver_version
2.2.0
```

### read_fw_version

Command to retrieve the version of the firmware used by the WFx chip.

Usage: `wifi test read_fw_version`

Example:

```
wifi test read_fw_version
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