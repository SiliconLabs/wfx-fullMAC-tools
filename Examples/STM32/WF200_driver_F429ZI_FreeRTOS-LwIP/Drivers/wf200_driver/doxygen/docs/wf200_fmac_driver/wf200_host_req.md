Host requirements	{#wf200_host_req}  
============

A microcontroller host can use the FMAC driver to access the Wi-Fi capabilities of WF200. To do so, the host has to respect several requirements.

## Bus communication
The first requirement of the host is to support either SPI or SDIO communication busses.

### SPI bus
WF200 supports SPI 8-bit up to 50MHz. In addition to the SPI connections (MISO, MOSI, clock), the host has also to provide two GPIOs to manage the chip select and interrupt functionalities. You can refer to \ref wf200_hardware to connect the host to WF200. 
The chip select is active in the reset state and the interrupt is detected on a rising edge. In case of power save features support, the host has to manage an additional GPIO (GPIO_WUP) for the wake-up functionality.

### SDIO bus
WF200 supports SDIO 4-bit up to 50MHz. By default, the interrupt occurs on the SDIO data line 1. This can be changed using the PDS (\ref wf200_pds) enabling the interrupt on the GPIO_WIRQ pin. This option can be useful in case of power save support. 
WF200 is aligned with the SDIO specification ver 2.0. In case of power save features support, the host has to manage an additional GPIO (GPIO_WUP) for the wake-up functionality.

## Host API functions implementation
The host has to implement a set of functions to enable the driver to access multiple resources (Firmware, GPIOs, memory, busses...). This implementations are dependent on platforms and type of operating system used.
The list of functions to be implemented are located in the wf200_host_api.h file.

A first set of functions is responsible of initializing the host and retrieving the firmware information.
```c
sl_status_t wf200_host_init( void );
sl_status_t wf200_host_get_firmware_data( const uint8_t** data, uint32_t data_size );
sl_status_t wf200_host_get_firmware_size( uint32_t* firmware_size );
sl_status_t wf200_host_deinit( void );
```
The host also needs to provide GPIO access to the FMAC driver.
```c
sl_status_t wf200_host_reset_chip( void );
sl_status_t wf200_host_set_wake_up_pin( uint8_t state );
sl_status_t wf200_host_hold_in_reset( void );
```
In addition, several APIs serve to manage the confirmations and indications coming from WF200...
```c
sl_status_t wf200_host_wait_for_confirmation( uint32_t timeout, void** event_payload_out );
sl_status_t wf200_host_wait( uint32_t wait_time );
sl_status_t wf200_host_post_event( uint32_t event_id, void* event_payload, uint32_t event_payload_length );
```
and allocate or free memory.
```c
sl_status_t wf200_host_allocate_buffer( wf200_buffer_t** buffer, wf200_buffer_type_t type, uint32_t buffer_size, uint32_t wait_duration );
sl_status_t wf200_host_free_buffer( wf200_buffer_t* buffer, wf200_buffer_type_t type );
```
The last required functions to be supported by the host are linked to the chosen communication bus. 
```c
/* WF200 host bus API */
sl_status_t wf200_host_init_bus( void );
sl_status_t wf200_host_enable_platform_interrupt( void );
sl_status_t wf200_host_disable_platform_interrupt( void );
/* WF200 host SPI bus API */
sl_status_t wf200_host_spi_cs_assert( void );
sl_status_t wf200_host_spi_cs_deassert(void );
sl_status_t wf200_host_spi_transfer_no_cs_assert( sl_wifi_bus_tranfer_type_t type, uint8_t* buffer, uint16_t buffer_length );
/* WF200 host SDIO bus API */
sl_status_t wf200_host_sdio_transfer_cmd52( wf200_host_bus_tranfer_type_t type, uint8_t function, uint32_t address, uint8_t* buffer );
sl_status_t wf200_host_sdio_transfer_cmd53( wf200_host_bus_tranfer_type_t type, uint8_t function, uint32_t address, uint8_t* buffer, uint16_t buffer_length );
sl_status_t wf200_host_sdio_enable_high_speed_mode( void );
```

## WF200 input buffers limit
When sending commands and frames to WF200, the host has to manage the number of input buffers available on the WF200 side. The number of available buffers is shared in the startup indication.
The host has to maintain the number of commands/frames not yet acknowledged by a confirmation. It can be done through a simple counter or by using a semaphore. The host API includes a dedicated function hook to manage this limit on the host side.
```c
sl_status_t wf200_host_transmit_frame( wf200_buffer_t* frame );
```
\todo add startup indication ref.

## WF200 receive frame loop

The host is responsible for detecting an interrupt coming from WF200 and calling ::wf200_receive_frame. Depending on the returned status of ::wf200_receive_frame, the host has to either call the function again or stop the communication with WF200. In addition, **the host will also have to manage the priority between the received frames and the packets to be sent from the host**.
The balance between RX and TX is use case dependent. It is up to the host to manage this balance in high throughput situations.

## wf200_host_post_event() implementation
The host has to manage the indications received from WF200. It is done through the wf200_host_post_event() function. The list of possible indications sent by WF200 can be found on the dedicated \ref indications page.
The information reported by each indication is described in wfx_fm_api.h.
