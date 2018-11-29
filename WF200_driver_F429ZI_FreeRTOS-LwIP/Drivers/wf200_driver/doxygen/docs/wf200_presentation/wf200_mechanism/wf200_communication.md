API messages	{#wf200_communication}  
============

## Protocol
WF200 uses a classic **request/confirmation** and **indication** protocol to communicate with the host as shown below.

![WF200 communication mechanism](@ref wf200_communication.png)

## HOST to WF200
A message sent by the host is a **request** and is always answered back with a **confirmation**. This **confirmation** systematically informs the host of the status of the **request**.

## WF200 to HOST
On top of sending back confirmations, WF200 is sending **indication** messages (e.g. packet reveiced, connection established...). The host is notified through an interrupt that information is available. 
Depending on the \ref config_register "configuration register" setting, the interrupt is issued on the standard GPIO (GPIO 29 \ref wf200_hardware) or on WIRQ pin (pin 24). 

## Message header format
Message header is a 32bit word starting every messages sent or received by WF200. Below is a diagram to present the header content.
![WF200 message header](@ref wf200_header.png)

* Message length (16 bits): the size of the message including the header size.
* Message info (16 bits): content below
	* MsgId (8 bits): message ID, refer to \ref requests, confirmation or \ref indications for the ID values.
	* MsgInfo (8 bits): content below
		* Reserved (1 bit)
		* IntId (2 bits): Interface concerned by the message. For the FMAC driver, station interface is 0 and softap is 1.
		* Reserved (3 bits)
		* SecLink (2 bits): Always 00 for non-encrypted messages. In secure link mode, the standard header is replaced by a specific one.
		
\todo Add link to the header structures in wfx_fm_api.h
