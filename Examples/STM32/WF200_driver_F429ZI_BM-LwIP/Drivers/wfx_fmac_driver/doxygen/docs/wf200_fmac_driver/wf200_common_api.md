Common API	{#wf200_common_api}  
============

To interact with WF200, the host has access to a set of API which are exposed both in the split MAC case and in the full MAC case. To find the related driver functions, you can refer to the table below or to the \ref COMMON_API group.

## Common API commands

| Commands         | Value | Associated driver function | Description                                             |
|------------------|-------|----------------------------|---------------------------------------------------------|
| HI_CONFIGURATION | 0x09  | ::wf200_send_configuration | Configure the device through the \ref wf200_pds         |
| HI_SHUT_DOWN     | 0x32  | ::wf200_shutdown           | Send a request to shut down the internal power supplies |

For each available command, there is an associated confirmation with the same ID value.

## Common API indications

| Indications      | Value | Associated structure   | Description        |
| ---------------- | ----- | ---------------------- | ------------------ |
| HI_EXCEPTION_IND | 0xe0  | ::HiExceptionIndBody_t | Exception          |
| HI_ERROR_IND     | 0xe4  | ::HiErrorIndBody_t     | Error              |
| HI_STARTUP_IND   | 0xe1  | ::HiStartupIndBody_t   | Startup indication |
| HI_GENERIC_IND   | 0xe3  | ::HiGenericIndBody_t   | Generic status     |

This indications have to be managed by the host depending on the application and use case.
