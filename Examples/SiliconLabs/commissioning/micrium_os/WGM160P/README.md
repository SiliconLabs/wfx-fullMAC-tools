# Commissioning MicriumOS Example on WGM160P

To use the example, follow the [**demonstration quick start guide**](https://docs.silabs.com/wifi/wf200/content-source/getting-started/silabs/wgm160/getting-started).

## Removing Gecko OS from the WGM160P

In the case, Simplicity Studio throws an error message in the Flash Programmer window or if flashing your application doesn't work, it could mean that Gecko OS has been previously installed on the chip.
To use this example on a WGM160P it is necessary to [**remove Gecko OS**](../../../shared/wgm160p/bootloader/README.md) and configure Simplicity Studio to communicate with the WGM160P as a bare metal EFM32 device.
