Platform Data Set	{#wf200_pds}  
============

One single command is used to configure the WF200 according to its environment (XTAL, GPIOs, FEM). It allows a decorrelation of the host driver code from the HW platforms it is using.
This command takes an encoded string containing all the platform information called **Platform Data Set** or **PDS**. You can refer to ::wf200_send_configuration to see how the PDS is sent from the host to WF200.
