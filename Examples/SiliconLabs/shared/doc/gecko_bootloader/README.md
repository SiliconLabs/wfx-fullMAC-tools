# Gecko Bootloader

The Gecko Bootloader is an advanced bootloader providing primordial features as the Secure Boot and the ability to update Over The Air (OTA) several key components (Bootloader, Secure Element, Application).
This bootloader can be required for applications running the Bluetooth stack for instance. More information about the bootloader in the following documents:

* [UG103.6 Bootloader Fundamentals](https://www.silabs.com/documents/public/user-guides/ug103-06-fundamentals-bootloading.pdf)
* [UG266 Silicon Labs Gecko Bootloader User’s Guide](https://www.silabs.com/documents/public/user-guides/ug266-gecko-bootloader-user-guide.pdf)

The projects in this repository requiring the Gecko Bootloader, includes a default binary in their **binaries** folder to ease the launch of the example. Keep in mind that the version provided is specific to a target and may not be up-to-date.
The Gecko Bootloader source code up-to-date and specific to a target can be retrieved from Simplicity Studio,
the steps to follow are described in the section **6. Getting Started with the Gecko Bootloader** of [UG266 Silicon Labs Gecko Bootloader User’s Guide](https://www.silabs.com/documents/public/user-guides/ug266-gecko-bootloader-user-guide.pdf).
