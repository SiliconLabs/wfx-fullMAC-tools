WF200 hardware overview	{#wf200_hardware}  
============

The WF200 can be connected to a host using two different busses, SPI or SDIO. The selected bus has implications on the physical connection between the host and WF200. Below are described two standard connection patterns for each bus configuration.
For more detailed schematics, refer to WF200 datasheet.

##SPI
![WF200 host connection diagram using SPI](@ref wf200_spi.png)

##SDIO
![WF200 host connection diagram using SDIO](@ref wf200_sdio.png)
*The WIRQ pin (pin 24) can be used as an alternative interrupt to SDIO_DAT1 (pin29).