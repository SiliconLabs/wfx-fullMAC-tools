/**************************************************************************//**
 * Copyright 2019, Silicon Laboratories Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *    http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *****************************************************************************/

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include "sl_wfx_cli_generic.h"

#include "lwip/stats.h"
#include "lwip/tcpip.h"
#include "lwip/raw.h"
#include "lwip/icmp.h"
#include "lwip/inet.h"
#include "lwip/inet_chksum.h"
#include "lwip/ip.h"
#include "lwiperf.h"

// X.x.x: Major version of the IP CLI
#define SL_WFX_CLI_IP_VERSION_MAJOR      4
// x.X.x: Minor version of the IP CLI
#define SL_WFX_CLI_IP_VERSION_MINOR      0
// x.x.X: Revision of the IP CLI
#define SL_WFX_CLI_IP_VERSION_REVISION   0

// Provides the version of the IP CLI
#define SL_WFX_CLI_IP_VERSION   SL_WFX_CLI_GEN_MODULE_VERSION(SL_WFX_CLI_IP_VERSION_MAJOR, \
                                                              SL_WFX_CLI_IP_VERSION_MINOR, \
                                                              SL_WFX_CLI_IP_VERSION_REVISION)

// Provides the version of the IP CLI as string
#define SL_WFX_CLI_IP_VERSION_STRING     SL_WFX_CLI_GEN_MODULE_VERSION_STRING(SL_WFX_CLI_IP_VERSION_MAJOR, \
                                                                              SL_WFX_CLI_IP_VERSION_MINOR, \
                                                                              SL_WFX_CLI_IP_VERSION_REVISION)

#define IPERF_DEFAULT_DURATION_SEC          10
#define IPERF_DEFAULT_PORT                5001

#define IPERF_CLIENT_MODE                    0
#define IPERF_SERVER_MODE                    1

#define PING_DEFAULT_REQ_NB                  3
#define PING_DEFAULT_INTERVAL_SEC            1
#define PING_DEFAULT_RCV_TMO_SEC             1
#define PING_DEFAULT_DATA_SIZE              32

#ifndef PING_ID
#define PING_ID                         0xAFAF
#endif

static void *iperf_server_session = NULL;
static void *iperf_client_session = NULL;
static bool iperf_client_is_foreground_mode = false;

static uint32_t last_client_bytes_transferred = 0;
static uint32_t last_client_ms_duration = 0;
static uint32_t last_client_bandwidth_kbitpsec = 0;

static uint16_t ping_nb_packet_received = 0;
static uint16_t ping_nb_packet_sent = 0;
static uint32_t ping_echo_total_time = 0;
static uint32_t ping_echo_min_time = 0xFFFFFFFF;
static uint32_t ping_echo_max_time = 0;
static uint16_t ping_seq_num = 0;
static uint32_t ping_time = 0;


static void lwip_iperf_results (void *arg,
                                enum lwiperf_report_type report_type,
                                const ip_addr_t* local_addr,
                                uint16_t local_port,
                                const ip_addr_t* remote_addr,
                                uint16_t remote_port,
                                uint32_t bytes_transferred,
                                uint32_t ms_duration,
                                uint32_t bandwidth_kbitpsec)
{
  int mode = (int) arg;

  (void)report_type;
  (void)local_addr;
  (void)local_port;
  (void)remote_addr;
  (void)remote_port;

  if (mode == IPERF_CLIENT_MODE) {
    last_client_bytes_transferred = bytes_transferred;
    last_client_ms_duration = ms_duration;
    last_client_bandwidth_kbitpsec = bandwidth_kbitpsec;

    printf("\r\nIperf Client Report:\r\n" );
    printf("Interval %d.%ds\r\n",(int)(ms_duration/1000),(int)(ms_duration%1000));
    printf("Bytes transferred %d.%dM\r\n",(int)(bytes_transferred/1024/1024),(int)((((bytes_transferred/1024)*1000)/1024)%1000));
    printf("%d.%d Mbps\r\n\r\n",(int)(bandwidth_kbitpsec/1024),(int)(((bandwidth_kbitpsec*1000)/1024)%1000));

    if (iperf_client_is_foreground_mode) {
      // Give back the hand to the shell
      sl_wfx_cli_generic_resume();
    }
  } else {
    // Server stopped, display the last client report
    printf("\r\nIperf Last Client Report:\r\n" );
    printf("Interval %d.%ds\r\n",(int)(last_client_ms_duration/1000),(int)(last_client_ms_duration%1000));
    printf("Bytes transferred %d.%dM\r\n",(int)(last_client_bytes_transferred/1024/1024),(int)((((last_client_bytes_transferred/1024)*1000)/1024)%1000));
    printf("%d.%d Mbps\r\n\r\n",(int)(last_client_bandwidth_kbitpsec/1024),(int)(((last_client_bandwidth_kbitpsec*1000)/1024)%1000));
  }
}

static int iperf_cmd_cb (int argc,
                         char **argv,
                         char *output_buf,
                         uint32_t output_buf_len)
{
  unsigned long duration = IPERF_DEFAULT_DURATION_SEC;
  unsigned long remote_port = IPERF_DEFAULT_PORT;
  ip_addr_t remote_address;
  char *endptr = NULL;
  char *ip_str = NULL;
  char *msg = NULL;
  int res = -1;
  bool is_server = false;
  bool parsing_error = true;

  if (argc >= 2) {
    // Parse the mode
    if ((strncmp(argv[1], "-s", 2) == 0)
        && (argv[1][2] == '\0')) {
      // Server mode
      is_server = true;

      if (argc == 2) {
        parsing_error = false;
      }

    } else if ((strncmp(argv[1], "-c", 2) == 0)
               && (argv[1][2] == '\0')) {
      // Client mode
      switch (argc) {
        case 8:
        case 7:
        case 6:
        case 5:
        case 4:
          for (int i=3; i<argc; ) {
            // Reset the flag
            parsing_error = true;

            if ((strncmp(argv[i], "-t", 2) == 0)
                && (argv[i][2] == '\0')) {
              duration = strtoul(argv[i+1], &endptr, 0);
              if ((*endptr == '\0') ||
                  (*endptr == ' ')) {
                parsing_error = false;
                i += 2;
              } else {
                break;
              }
            } else if ((strncmp(argv[i], "-p", 2) == 0)
                && (argv[i][2] == '\0')) {
              remote_port = strtoul(argv[i+1], &endptr, 0);
              if ((*endptr == '\0') ||
                  (*endptr == ' ')) {
                parsing_error = false;
                i += 2;
              } else {
                break;
              }
            } else if ((strncmp(argv[i], "-k", 2) == 0)
                && (argv[i][2] == '\0')) {
              iperf_client_is_foreground_mode = true;
              parsing_error = false;
              i++;
            } else {
              // Unknown option
              break;
            }
          }

          if (parsing_error == true) {
            // Reset to the default client mode
            iperf_client_is_foreground_mode = false;
            break;
          }
          //no break

        case 3:
          ip_str = argv[2];
          parsing_error = false;
          break;
      }
    }
  }

  if (parsing_error) {
    msg = (char *)invalid_command_msg;
    return -1;
  }

  if (is_server) {
    // Server mode
    if (iperf_server_session != NULL) {
      // An iPerf server is already running, kill it first
      printf("A server is running, stop it first\r\n");
      res = -1;
    } else {
      // Reset session values
      last_client_bytes_transferred = 0;
      last_client_ms_duration = 0;
      last_client_bandwidth_kbitpsec = 0;

      LOCK_TCPIP_CORE();
      iperf_server_session = lwiperf_start_tcp_server_default(lwip_iperf_results,
                                                              (void *)IPERF_SERVER_MODE);
      UNLOCK_TCPIP_CORE();

      if (iperf_server_session != NULL) {
        strncpy(output_buf, "iPerf TCP server started\r\n", output_buf_len);
        res = 0;
      } else {
        strncpy(output_buf, "iPerf TCP server error\r\n", output_buf_len);
        res = -1;
      }
    }

  } else {
    // Client mode
    // Try parsing the server IP address to connect
    res = ipaddr_aton(ip_str, &remote_address);
    if (res != 0) {

      LOCK_TCPIP_CORE();
      iperf_client_session = lwiperf_start_tcp_client(&remote_address,
                                                      remote_port,
                                                      LWIPERF_CLIENT,
                                                      (uint32_t)duration,
                                                      lwip_iperf_results,
                                                      (void *)IPERF_CLIENT_MODE);
      UNLOCK_TCPIP_CORE();

      if (iperf_client_session != NULL) {
        sl_wfx_cli_generic_config_wait(0);

        printf("iPerf TCP client started on server %s\r\n", ip_str);

        if (iperf_client_is_foreground_mode) {
          // Wait until the test is done
          sl_wfx_cli_generic_wait(((uint32_t)duration + 1 /*security*/) * 1000);

          // Reset to the default client mode
          iperf_client_is_foreground_mode = false;
        }
        res = 0;
      } else {
        strncpy(output_buf, "iPerf TCP client error\r\n", output_buf_len);
        res = -1;
      }
    } else {
      // Parsing error
      msg = (char *)invalid_command_msg;
      res = -1;
    }
  }

  if (msg != NULL) {
    // Format the output message
    strncpy(output_buf, msg, output_buf_len);
    if (output_buf_len > 0) {
      output_buf[output_buf_len - 1] = '\0';
    }
  }

  return res;
}

static const sl_wfx_cli_generic_command_t iperf_cmd =
{
  "iperf",
  "iperf                    : Start a TCP iPerf test as a client or a server\r\n"
  "                         Usage: iperf <-c ip [-t dur] [-p port] [-k] | -s>\r\n"
  "                           ip: iPerf server address to connect to (IPv4 format)\r\n"
  "                           dur: test duration in client mode (default 10s)\r\n"
  "                           port: server port to connect to (default 5001)\r\n"
  "                           -k: execute the command in foreground\r\n",
  iperf_cmd_cb,
  -1
};

static int iperf_server_stop_cmd_cb (int argc,
                                     char **argv,
                                     char *output_buf,
                                     uint32_t output_buf_len)
{
  if (iperf_server_session != NULL) {
    printf("Stop server\r\n");

    LOCK_TCPIP_CORE();
    lwiperf_abort(iperf_server_session);
    UNLOCK_TCPIP_CORE();

    iperf_server_session = NULL;
  }

  return 0;
}

static const sl_wfx_cli_generic_command_t iperf_server_stop_cmd =
{
  "iperf-server-stop",
  "iperf-server-stop        : Stop the running iPerf server\r\n",
  iperf_server_stop_cmd_cb,
  0
};

static int iperf_client_stop_cmd_cb (int argc,
                                     char **argv,
                                     char *output_buf,
                                     uint32_t output_buf_len)
{
  if (iperf_client_session != NULL) {
    printf("Stop client\r\n");

    LOCK_TCPIP_CORE();
    lwiperf_abort(iperf_client_session);
    UNLOCK_TCPIP_CORE();

    iperf_client_session = NULL;
  }

  return 0;
}

static const sl_wfx_cli_generic_command_t iperf_client_stop_cmd =
{
  "iperf-client-stop",
  "iperf-client-stop        : Stop the running iPerf client\r\n",
  iperf_client_stop_cmd_cb,
  0
};

#if LWIP_RAW
static u8_t ping_recv (void *arg,
                       struct raw_pcb *pcb,
                       struct pbuf *p,
                       const ip_addr_t *addr)
{
  struct icmp_echo_hdr *iecho;
  LWIP_UNUSED_ARG(arg);
  LWIP_UNUSED_ARG(pcb);

  LWIP_ASSERT("p != NULL", p != NULL);

  if ((p->tot_len >= (PBUF_IP_HLEN + sizeof(struct icmp_echo_hdr)))
      && (pbuf_remove_header(p, PBUF_IP_HLEN) == 0)) {

    iecho = (struct icmp_echo_hdr *)p->payload;
    if ((iecho->id == PING_ID) && (iecho->seqno == lwip_htons(ping_seq_num))) {
      // Update statistic variables
      ping_nb_packet_received++;

      uint32_t duration = sys_now() - ping_time;
      ping_echo_total_time += duration;
      if (duration < ping_echo_min_time) {
        ping_echo_min_time = duration;
      }
      if (duration > ping_echo_max_time) {
        ping_echo_max_time = duration;
      }

      printf("Reply from %s: bytes=%d, time=%lums\r\n", ipaddr_ntoa(addr),
                                                        p->len,
                                                        duration);
      pbuf_free(p);

      // Eat the packet
      return 1;
    }

    // Packet not eaten, restore the original one
    pbuf_add_header(p, PBUF_IP_HLEN);
  }

  // Don't eat the packet
  return 0;
}

static int ping_send (struct raw_pcb *raw,
                      const ip_addr_t *addr,
                      uint16_t data_size)
{
  struct pbuf *p;
  struct icmp_echo_hdr *iecho;
  size_t ping_size, i;
  int res = -1;

  ping_size = sizeof(struct icmp_echo_hdr) + data_size;

  p = pbuf_alloc(PBUF_IP, (u16_t)ping_size, PBUF_RAM);
  if (p != NULL) {
    iecho = (struct icmp_echo_hdr *)p->payload;

    ICMPH_TYPE_SET(iecho, ICMP_ECHO);
    ICMPH_CODE_SET(iecho, 0);
    iecho->chksum = 0;
    iecho->id     = PING_ID;
    iecho->seqno  = lwip_htons(++ping_seq_num);

    // Fill the additional data buffer with some data
    for (i=0; i<data_size; i++) {
      ((char*)iecho)[sizeof(struct icmp_echo_hdr) + i] = (char)i;
    }

    iecho->chksum = inet_chksum(iecho, ping_size);

    res = raw_sendto(raw, p, addr);
    if (res == 0) {
      ping_time = sys_now();
      ping_nb_packet_sent++;
    }

    pbuf_free(p);
  }

  return res;
}

static int ping_cmd_cb (int argc,
                        char **argv,
                        char *output_buf,
                        uint32_t output_buf_len)
{
  char *msg = NULL;
  char *ip_str = NULL;
  struct raw_pcb *ping_pcb;
  ip_addr_t ping_address;
  int req_nb = PING_DEFAULT_REQ_NB;
  int res = -1;
  bool parsing_error = true;

  switch (argc) {
    case 4:
      if ((strncmp(argv[1], "-n", 2) == 0)
          && (argv[1][2] == '\0')) {

        req_nb = atoi(argv[2]);
        if (req_nb <= 0) {
          // Wrong value, restore the default one
          req_nb = PING_DEFAULT_REQ_NB;
          printf("Invalid number of requests, default applied\r\n");
        }

        ip_str = argv[3];
        parsing_error = false;
      }
      break;

    case 2:
      ip_str = argv[1];
      parsing_error = false;
      break;
  }

  if (parsing_error) {
    msg = (char *)invalid_command_msg;
    return -1;
  }

  // Try parsing the server IP address to connect
  res = ipaddr_aton(ip_str, &ping_address);

  if (res != 0) {
    // Allocate a new resource
    ping_pcb = raw_new(IP_PROTO_ICMP);
    if (ping_pcb != NULL) {
      // Configure the IP stack
      raw_recv(ping_pcb, ping_recv, NULL);
      raw_bind(ping_pcb, IP_ADDR_ANY);

      printf("Pinging %s with %d bytes of data:\r\n",
             ip_str, PING_DEFAULT_DATA_SIZE);

      // Reset the internal variables
      ping_seq_num = 0;
      ping_nb_packet_sent = 0;
      ping_nb_packet_received = 0;
      ping_echo_total_time = 0;
      ping_echo_min_time = 0xFFFFFFFF;
      ping_echo_max_time = 0;

      while (req_nb-- > 0) {
        // Send the ping request
        res = ping_send(ping_pcb, &ping_address, PING_DEFAULT_DATA_SIZE);
        if (res != 0) {
          msg = (char *)command_error_msg;
          res = -1;
          break;
        }

        sys_msleep(PING_DEFAULT_INTERVAL_SEC * 1000);
      }

      if (res == 0) {
        // Display statistics
        printf("\r\nPing statistics for %s:\r\n", ip_str);
        printf("\tPackets: Sent = %u, Received = %u, Lost = %u (%u%% loss),\r\n",
               ping_nb_packet_sent,
               ping_nb_packet_received,
               ping_nb_packet_sent-ping_nb_packet_received,
               ((ping_nb_packet_sent-ping_nb_packet_received)*100)/ping_nb_packet_sent);
        printf("Approximate round trip times in milli-seconds:\r\n");
        printf("\tMinimum = %lums, Maximum = %lums, Average = %lums\r\n",
               ping_echo_min_time,
               ping_echo_max_time,
               ping_echo_total_time/ping_nb_packet_received);
      }

    } else {
      msg = (char *)command_error_msg;
      res = -1;
    }
  } else {
    // Parsing error
    msg = (char *)invalid_command_msg;
    res = -1;
  }

  if (msg != NULL) {
    // Format the output message
    strncpy(output_buf, msg, output_buf_len);
    if (output_buf_len > 0) {
      output_buf[output_buf_len - 1] = '\0';
    }
  }

  return res;
}

static const sl_wfx_cli_generic_command_t ping_cmd =
{
  "ping",
  "ping                     : Send ICMP ECHO_REQUEST to network hosts\r\n"
  "                         Usage: ping [-n nb] <ip>\r\n"
  "                           ip: address to ping (IPv4 format)\r\n"
  "                           nb: number of requests to send (default 3)\r\n",
  ping_cmd_cb,
  -1
};
#endif //LWIP_RAW

static int ip_stats_cmd_cb (int argc,
                            char **argv,
                            char *output_buf,
                            uint32_t output_buf_len)
{
  stats_display();
  return 0;
}

static const sl_wfx_cli_generic_command_t ip_stats_cmd =
{
  "ip-stats",
  "ip-stats                 : Display the IP stack statistics\r\n",
  ip_stats_cmd_cb,
  0
};

int sl_wfx_cli_ip_init (void)
{
  int res;

  // Add IP command to the CLI
  res = sl_wfx_cli_generic_register_cmd(&iperf_cmd);
  res = sl_wfx_cli_generic_register_cmd(&iperf_server_stop_cmd);
  res = sl_wfx_cli_generic_register_cmd(&iperf_client_stop_cmd);
  res = sl_wfx_cli_generic_register_cmd(&ip_stats_cmd);
#if LWIP_RAW
  res = sl_wfx_cli_generic_register_cmd(&ping_cmd);
#endif

  // Register the IP CLI module
  res |= sl_wfx_cli_generic_register_module("IP CLI  ",
                                            SL_WFX_CLI_IP_VERSION_STRING);

  return res;
}
