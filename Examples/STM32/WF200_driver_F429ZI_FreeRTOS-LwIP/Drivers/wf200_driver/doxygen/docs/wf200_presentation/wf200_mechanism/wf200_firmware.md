WF200 Firmware	{#wf200_firmware}  
============

During the \ref wf200_initialization phase, the host has to send a signed and encrypted firmware to WF200. In the FMAC driver, the firmware information can be provided by the host through the following functions:
* **wf200_host_get_firmware_data** (declaration in wf200_host_api.h)
* **wf200_host_get_firmware_size** (declaration in wf200_host_api.h)

Depending on the host implementation, the firmware can be stored in different formats. It can be for example stored in a simple c array or be part of a more complex file system. 
The firmware download phase is handled through the ::wf200_download_run_firmware function. In the FMAC driver, the firmware files are provided in a c array format in wfm_wf200_C0.h.

In addition to the actual WF200 firmware, the file contains a keyset, signature and hash to authenticate the firmware.
![WF200 firmware structure](@ref wf200_firmware.png)