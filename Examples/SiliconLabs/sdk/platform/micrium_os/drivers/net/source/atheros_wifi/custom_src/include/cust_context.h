//                                                                ------------------------------------------------------------------------------
//                                                                 Copyright (c) Qualcomm Atheros, Inc.
//                                                                 All rights reserved.
//                                                                 Redistribution and use in source and binary forms, with or without modification, are permitted (subject to
//                                                                 the limitations in the disclaimer below) provided that the following conditions are met:
//
//                                                                 � Redistributions of source code must retain the above copyright notice, this list of conditions and the
//                                                                   following disclaimer.
//                                                                 � Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the
//                                                                   following disclaimer in the documentation and/or other materials provided with the distribution.
//                                                                 � Neither the name of nor the names of its contributors may be used to endorse or promote products derived
//                                                                   from this software without specific prior written permission.
//
//                                                                 NO EXPRESS OR IMPLIED LICENSES TO ANY PARTY'S PATENT RIGHTS ARE GRANTED BY THIS LICENSE. THIS SOFTWARE IS
//                                                                 PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING,
//                                                                 BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
//                                                                 DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
//                                                                 INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
//                                                                 SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
//                                                                 ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
//                                                                 ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//                                                                ------------------------------------------------------------------------------
//                                                                ==============================================================================
//                                                                 Author(s): ="Atheros"
//                                                                ==============================================================================
#ifndef _CUST_CONTEXT_H_
#define _CUST_CONTEXT_H_
/* PORT_NOTE: This file is intended to define the A_CUSTOM_DRIVER_CONTEXT
 *  structure. This structure is the same as the pCxt object which can be
 *  found throughout the driver source.  It is the main driver context which
 *  is expected to change from system to system.  It keeps a pointer to the
 *  complimentary A_DRIVER_CONTEXT which is the common context that should
 *  remain constant from system to system. The porting effort requieres that
 *  the A_CUSTOM_DRIVER_CONTEXT be populated with system specific elements. */

#include  <net/include/net.h>
#include  <net/include/net_if_wifi.h>
#include "../../../net_drv_wifi_qca400x_priv.h"
/* PORT_NOTE: Include any System header files here to resolve any types used
 *  in A_CUSTOM_DRIVER_CONTEXT */
#if 0
typedef struct _a_custom_driver_context{
  /* PORT_NOTE: UtilityMutex is a mutex used to protect resources that
   *  might be accessed by multiple task/threads in a system.
   *  It is used by the ACQUIRE/RELEASE macros below.  If the target
   *  system is single threaded then this mutex can be omitted and
   *  the macros should be blank.*/
#if DRIVER_CONFIG_MULTI_TASKING
  A_MUTEX_T       UtilityMutex;
#endif
  /* PORT_NOTE: rxFreeQueue stores pre-allocated rx net-buffers. If dynamic heap
   *  allocation is preferred for RX net-buffers then this option should be disabled. */
#if DRIVER_CONFIG_IMPLEMENT_RX_FREE_QUEUE
  A_NETBUF_QUEUE_T  rxFreeQueue;
#endif
  /* PORT_NOTE: connectStateCB allows an app to get feedback/notification
   *  as the WIFI connection state changes. If used it should be populated
   *  with a function pointer by an IOCTL or system equivalent. */
  A_VOID          *connectStateCB;
  /* PORT_NOTE: pScanOut stores scan results in a buffer so that they
   *  can be retrieved aysnchronously by a User task. */
  A_UINT8*    pScanOut;   /* callers buffer to hold results. */
  A_UINT16    scanOutSize;/* size of pScanOut buffer */
  A_UINT16    scanOutCount;/* current fill count of pScanOut buffer */
#if DRIVER_CONFIG_MULTI_TASKING
  /* PORT_NOTE: These 2 events are used to block & wake their respective
   *  tasks. */
  LWEVENT_STRUCT  driverWakeEvent;   /* wakes the driver thread */
  LWEVENT_STRUCT  userWakeEvent;   /* wakes blocked user threads */
#endif
  /* PORT_NOTE: These three elements are used to provide access
   *  to the chip HW interface.  Use system appropriate element types. */
  FILE_PTR            spi_cxt;            /* used for device SPI access */
  LWGPIO_STRUCT_PTR   int_cxt;           /* used for device interrupt */
#if WLAN_CONFIG_ENABLE_CHIP_PWD_GPIO
  LWGPIO_STRUCT_PTR   pwd_cxt;    /* used for PWD line */
#endif
  /* PORT_NOTE: These 2 elements provide pointers to system context.
   *  The pDriverParam is intended provide definition for info such as
   *  which GPIO's to use for the SPI bus.  The pUpperCxt should be
   *  populated with a system specific object that is required when the
   *  driver calls any system API's. */
  A_VOID          *pDriverParam;   /* param struct containing initialization params */
  A_VOID          *pUpperCxt;   /* back pointer to the MQX enet object. */
  /* PORT_NOTE: pCommonCxt stores the drivers common context. It is
   *  accessed by the common source via GET_DRIVER_COMMON() below. */
  A_VOID      *pCommonCxt;   /* the common driver context link */
  A_BOOL      DriverShutdown;   /* used to shutdown the driver */
  A_UINT32    line;
  A_UINT8     securityType;   /* used to satisfy MQX ENET_GET_MEDIACTL_SEC_TYPE */
  A_VOID*     promiscuous_cb;   /* used to feed rx frames in promiscuous mode. */
  A_UINT8     extended_scan;   /* used to indicate whether an extended scan is in progress */
#if MANUFACTURING_SUPPORT
  A_UINT32        testCmdRespBufLen;
#endif
}A_CUSTOM_DRIVER_CONTEXT;
#endif
/* MACROS used by common driver code to access custom context elements */
#define GET_DRIVER_CXT(pCxt)                       ((NET_DEV_DATA*)(((NET_IF*)(pCxt))->Dev_Data))
#define GET_DRIVER_COMMON(pCxt)                    ((A_DRIVER_CONTEXT*)(GET_DRIVER_CXT((pCxt))->CommonCxt))

#define GET_DRIVER_HCD_CXT(pCxt) ((A_CUSTOM_HCD_CXT*)(pCxt))   //custom struct

#if DRIVER_CONFIG_MULTI_TASKING
#define RXBUFFER_ACCESS_INIT(pCxt)                   A_MUTEX_INIT(&(GET_DRIVER_CXT(pCxt)->UtilityMutex))
#define RXBUFFER_ACCESS_ACQUIRE(pCxt)                A_MUTEX_ACQUIRE(&(GET_DRIVER_CXT(pCxt)->UtilityMutex))
#define RXBUFFER_ACCESS_RELEASE(pCxt)                A_MUTEX_RELEASE(&(GET_DRIVER_CXT(pCxt)->UtilityMutex))
#define RXBUFFER_ACCESS_DESTROY(pCxt)                A_MUTEX_DELETE(&(GET_DRIVER_CXT(pCxt)->UtilityMutex))

#define TXQUEUE_ACCESS_INIT(pCxt)                   (A_OK)/* because its the same mutex as RXBUFFER_ACCESS this is NOP */
#define TXQUEUE_ACCESS_ACQUIRE(pCxt)                 A_MUTEX_ACQUIRE(&(GET_DRIVER_CXT(pCxt)->UtilityMutex))
#define TXQUEUE_ACCESS_RELEASE(pCxt)                 A_MUTEX_RELEASE(&(GET_DRIVER_CXT(pCxt)->UtilityMutex))
#define TXQUEUE_ACCESS_DESTROY(pCxt) /* because its the same mutex as RXBUFFER_ACCESS this is NOP */

#define IRQEN_ACCESS_INIT(pCxt)                     (A_OK)/* because its the same mutex as RXBUFFER_ACCESS this is NOP */
#define IRQEN_ACCESS_ACQUIRE(pCxt)                   A_MUTEX_ACQUIRE(&(GET_DRIVER_CXT(pCxt)->UtilityMutex))
#define IRQEN_ACCESS_RELEASE(pCxt)                   A_MUTEX_RELEASE(&(GET_DRIVER_CXT(pCxt)->UtilityMutex))
#define IRQEN_ACCESS_DESTROY(pCxt) /* because its the same mutex as RXBUFFER_ACCESS this is NOP */

#define DRIVER_SHARED_RESOURCE_ACCESS_INIT(pCxt)    (A_OK)/* because its the same mutex as RXBUFFER_ACCESS this is NOP */
#define DRIVER_SHARED_RESOURCE_ACCESS_ACQUIRE(pCxt)  A_MUTEX_ACQUIRE(&(GET_DRIVER_CXT(pCxt)->UtilityMutex))
#define DRIVER_SHARED_RESOURCE_ACCESS_RELEASE(pCxt)  A_MUTEX_RELEASE(&(GET_DRIVER_CXT(pCxt)->UtilityMutex))
#define DRIVER_SHARED_RESOURCE_ACCESS_DESTROY(pCxt) /* because its the same mutex as RXBUFFER_ACCESS this is NOP */
#else /* DRIVER_CONFIG_MULTI_TASKING */
#define RXBUFFER_ACCESS_INIT(pCxt) (A_OK)
#define RXBUFFER_ACCESS_ACQUIRE(pCxt)
#define RXBUFFER_ACCESS_RELEASE(pCxt)
#define RXBUFFER_ACCESS_DESTROY(pCxt)

#define TXQUEUE_ACCESS_INIT(pCxt) (A_OK)/* because its the same mutex as RXBUFFER_ACCESS this is NOP */
#define TXQUEUE_ACCESS_ACQUIRE(pCxt)
#define TXQUEUE_ACCESS_RELEASE(pCxt)
#define TXQUEUE_ACCESS_DESTROY(pCxt)
#define IRQEN_ACCESS_INIT(pCxt) (A_OK)/* because its the same mutex as RXBUFFER_ACCESS this is NOP */
#define IRQEN_ACCESS_ACQUIRE(pCxt)
#define IRQEN_ACCESS_RELEASE(pCxt)
#define IRQEN_ACCESS_DESTROY(pCxt)

#define DRIVER_SHARED_RESOURCE_ACCESS_INIT(pCxt) (A_OK)/* because its the same mutex as RXBUFFER_ACCESS this is NOP */
#define DRIVER_SHARED_RESOURCE_ACCESS_ACQUIRE(pCxt)
#define DRIVER_SHARED_RESOURCE_ACCESS_RELEASE(pCxt)
#define DRIVER_SHARED_RESOURCE_ACCESS_DESTROY(pCxt)
#endif /* DRIVER_CONFIG_MULTI_TASKING */

#endif /* _CUST_CONTEXT_H_ */
