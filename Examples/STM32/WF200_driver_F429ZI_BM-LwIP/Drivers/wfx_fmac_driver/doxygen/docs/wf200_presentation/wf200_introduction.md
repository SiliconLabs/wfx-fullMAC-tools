Introduction	{#wf200_introduction}  
============

The WF200 fits well with Linux-based and RTOS based host processors. WF200 supports both the 802.11 split MAC and the 802.11 full MAC architectures. It communicates
with the external host controller over the SPI or SDIO interface.

## LINUX use case 
WF200 acts as a common soft-MAC/half-MAC wireless device.
The firmware exposes a Lower-MAC API compatible with Linux Kernel mac80211 subsystem and the ieee80211_ops structure. Linux distribution IP stack and supplicant are leveraged.

## RTOS/Bare-metal use case
WF200 acts as a full-MAC wireless device and WPA2 personal supplicant. The firmware exposes a full-MAC API at the IP packet level.
It also manages WPA2 personal  authentication process as well as automatic roaming. **This is the particular use case addressed by this documentation.**

![WF200 functional diagram](@ref wf200_diagram.png)
