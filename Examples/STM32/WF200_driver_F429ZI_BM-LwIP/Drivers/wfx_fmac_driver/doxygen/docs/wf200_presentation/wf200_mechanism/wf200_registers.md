WF200 registers	{#wf200_registers}  
============

This section presents the registers exposed by WF200 to the host. The section focuses on the registers involved in the WF200 FMAC driver.
Those registers can be accessed both using SPI or SDIO. 

## SPI 
In SPI, the MSB is used to define the following data as read or write (1: read, 0: write). The next three bits are the actual register address, and all other bits define the length of the following transmission.  
Each command word is 16 bits long.

|**Bit number**  | 15  | 14 downto 12 | 11 downto 0 |
|----------------|-----|--------------|-------------|
| **Function**   | R/W | Address      | Length      |

Each command for this operational mode is formatted as described below.
![SPI access to WF200 registers](@ref spi_register_communication.png)
For each command, there can be data in or data out, and data can be any multiple of 16 bit blocks.

## SDIO
In SDIO, these registers are defined for command 53 (::wf200_host_sdio_transfer_cmd53). The addresses below are used as the first 6 bits of SDIO command address field. 

# 1. Address Map

| Name                  | SPI Address | SDIO Address | Description                                                                                                                                                                                                 |
|-----------------------|--------------|--------------|-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| Config                | 0x00         | 0x00         | Configuration register                                                                                                                                                                                      |
| Control               | 0x01         | 0x04         | Control register                                                                                                                                                                                            |
| Input /Output_channel | 0x02         | 0x08         | Input/output queue port                                                                                                                                                                                     |
| Ahb port              | 0x03         | 0x0C         | Direct AHB port access                                                                                                                                                                                      |
| Address               | 0x04         | 0x10         | AHB: Full 32-bit address SRAM: Memory address offset (0=first SRAM address)                                                                                                                                 |
| Sram_data_port        | 0x05         | 0x14         | Direct Shared RAM APB  port access                                                                                                                                                                          |
| General purpose       | 0x06         | 0x18         | General purpose registers: extended indirect mode to configure various parameters for SYXO, RFBIAS, CLKMULT, EMUOSC, HDREG…                                                                                 |
| Frame_output_channel  | 0x07         | N/A          | **SPI Only**: Used to read data in current output queue without giving length in SPI command. When the Host sends this read command, HIF provides 16-bit length information (in bytes) before sending data. |

# 2. CONFIG Register
\anchor config_register
| Name   | Size (bits) | SPI address | SDIO address |
|--------|-------------|-------------|--------------|
| CONFIG | 32          | 0x00        | 0x00         |

| Bit   | Reset             | Read/Write | description                                                                                                                                                                                                                                                                                                                        |
|-------|-------------------|------------|------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| 31-24 | 0x1               | R          | Device identification information                                                                                                                                                                                                                                                                                                  |
| 23-20 |                   |            | Unused                                                                                                                                                                                                                                                                                                                             |
| 19    | 0x0               | R/W        | SPI: Unused SDIO: Disable CRC check on data. 0: Normal CRC check behaviour. 1: Disable CRC check on data transfers (CRC result assumed always correct).                                                                                                                                                                            |
| 18    | 0x0               | R/W        | SPI: Dout_posedge_enable (1=enable, 0=disable) SDIO: Dout_posedge_enable (1=enable, 0=disable) This in intended to be used with 50MHz SDIO clock speed.                                                                                                                                                                            |
| 17:16 | 0x0               | R/W        | Irq_enable 10 = wlan_rdy enable 01 = data-irq enable 00 = both irq’s disabled 11 = both irq’s enabled                                                                                                                                                                                                                              |
| 15    | 0x0               | R/W        | SPI: 0 (normal functional mode) SDIO: Disable DAT1 interrupt mechanism 0: Normal DAT1 interrupt behaviour. 1: Disable DAT1 interrupts. Interrupt to host are still available on WIRQ pin.                                                                                                                                          |
| 14    | 0x1               | R/W        | Cpu_reset (1 = cpu reset, 0 = cpu non_reset)                                                                                                                                                                                                                                                                                       |
| 13    | 0x0               | R/W        | Direct_pre_fetch  apb (1 = channel busy, 0 = channel non-busy)                                                                                                                                                                                                                                                                     |
| 12    | 0x1               | R/W        | Cpu_clk_disable (1 = disable clk, 0 = enable clk)                                                                                                                                                                                                                                                                                  |
| 11    | 0x0               | R/W        | Direct_pre_fetch_ahb  (1 = channel busy, 0 = channel non-busy)                                                                                                                                                                                                                                                                     |
| 10    | 0x1               | R/W        | Direct_access_mode apb/ahb (0 = queue mode, 1 = direct access mode)                                                                                                                                                                                                                                                                |
| 9:8   | 0x0 SPI  0x2 SDIO | R/W  R     | Mode0 (“00”) : 4 bytes are sent : B1,B0,B3,B2 Mode1 (“01”) : 4 bytes are sent : B3,B2,B1,B0. This only make sense if the HOST works in 32-bit mode (SW-controlled) Mode2 (“10”) : 4 bytes are sent : B0,B1,B2,B3 Note : In SPI the config register access will always be in word_mode 0, regardless of the real value of word mode |
| 7     | 0x0  0x0          | R/W  R     | SPI : CSN-framing disable (1= spi_cs is not checked  , 0=spi_cs is checked)  SDIO: Err 7  (1 = host misses CRC error, 0 = no error)                                                                                                                                                                                                |
| 6     | 0x0               | R          | Err 6  (1 = host tries to send data with no hif input queue entry programmed, 0 = no error)                                                                                                                                                                                                                                        |
| 5     | 0x0               | R          | Err 5  (1 = host tries to send data larger than hif input buffer, 0 = no error)                                                                                                                                                                                                                                                    |
| 4     | 0x0               | R          | Err 4  (1 = host tries to send data when hif buffers overrun, 0 = no error)                                                                                                                                                                                                                                                        |
| 3     | 0x0               | R          | Err 3  (1 = host tries to read data with no hif output queue entry programmed, 0 = no error)                                                                                                                                                                                                                                       |
| 2     | 0x0               | R          | Err 2  (1 = host tries to read data less than output message length, 0 = no error)                                                                                                                                                                                                                                                 |
| 1     | 0x0               | R          | Err 1  (1 = host tries to read data when hif buffers underrun, 0 = no error)                                                                                                                                                                                                                                                       |
| 0     | 0x0  0x0          | R  R       | SPI: Err 0  (1 = CSN Framing error ,0=no error)  SDIO:Err 0 (1= Buffer number mismatch, 0= no error)                                                                                                                        

# 3. CONTROL Register        
\anchor control_register
| Name    | Size (bits) | SPI Address | SDIO Address |
|---------|-------------|-------------|--------------|
| CONTROL | 16          | 0x01        | 0x4          |

| Bit   | Reset | Read/Write | description                                                                                                                  |
|-------|-------|------------|------------------------------------------------------------------------------------------------------------------------------|
| 15:14 | 0x0   | R          | Frame type information : 11 = Wi-Fi data packet 10 = Wi-Fi management packet 01 = any other indication 00 = any confirmation |
| 13    | 0x0   | R          | Wlan_rdy (1= wlan is ready, 0 = wlan not ready)                                                                              |
| 12    | 0x0   | R/W        | Wlan_wup (1= init wake up , 0 = do not init wakeup)                                                                          |
| 11:0  | 0x0   | R          | Next output Queue item length                                                                                                |

# 4. Card Common Control Register (SDIO specific)
\anchor CCCR_register
| Name | Size (Bytes) | Address |
|------|--------------|---------|
| CCCR | 256          | 0x00000 |

| Offset    | Reset  | Read/Write | description                                 |
|-----------|--------|------------|---------------------------------------------|
| 0x00      | 0x11   | R/O        | CCCR/SDIO Revision                          |
| 0x01      | 0x0    | R/O        | SD Specification Revision                   |
| 0x02      | 0x0    | R/W        | I/O Function Enable                         |
| 0x03      | 0x0    | R/O        | I/O Function Ready                          |
| 0x04      | 0x0    | R/W        | Interrupt Enable (7 functions and 1 master) |
| 0x05      | 0x0    | R/O        | Interrupt Pending for each function         |
| 0x06      | 0x0    | W/O        | I/O Function Abort                          |
| 0x07      | 0x0    | R/W*       | Bus Interface Control                       |
| 0x08      | 0x1F   | R/W*       | Card Capability                             |
| 0x09-0x0B | 0x1000 | R/O        | Common CIS Pointer                          |
| 0x0C      | 0x0    | R/W*       | Bus Suspend                                 |
| 0x0D      | 0x0    | R/W*       | Function Select                             |
| 0x0E      | 0x0    | R/O        | Function Execution Flags                    |
| 0x0F      | 0x0    | R/O        | Function Ready Flags                        |
| 0x10-0x11 | 0x0    | R/W        | Function0 Block Size                        |
| 0x12      | 0x0    | R/W*       | Power Control                               |
| 0x13-0xEF |        |            | unused – Reserved for Future Use            |
| 0xF0-0xFF |        |            | unused – Reserved for Vendors               |
R/W*:  There are also R/O bits

# 5. Function Basic Registers (FBR) of Function1 (SDIO specific)
\anchor FBR_register
| Register Name | Size (Bytes) | Address |
|---------------|--------------|---------|
| FBR           | 256          | 0x00100 |

| Offset    | Reset  | Read/Write | description                                         |
|-----------|--------|------------|-----------------------------------------------------|
| 0x00      | 0x0    | R/O        | Function1 Standard SDIO Function Interface Code     |
| 0x01      | 0x0    | R/O        | Function1 Standard SDIO Function Interface Code     |
| 0x02      | 0x0    | R/W*       | Power Selection                                     |
| 0x03-0x08 |        |            | unused - RFU                                        |
| 0x0B-0x09 | 0x2000 | R/O        | Pointer to Function1 Card Information Pointer (CIS) |
| 0x0C-0x0E | 0x0    | R/O        | Pointer to Function1 Code Storage Area (CSA)        |
| 0x0F      | 0x0    | R/O        | Data Access Window to CSA                           |
| 0x10-0x11 | 0x0    | R/W        | Function1 Block Size                                |
| 0x12-0xFF |        |            | unused - RFU                                        |
R/W*:  There are also R/O bits