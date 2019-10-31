/**************************************************************************//**
 * Copyright 2018, Silicon Laboratories Inc.
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

#ifndef HOST_RESOURCES_H_
#define HOST_RESOURCES_H_

#include <stdint.h>

int host_init(void);

int host_dma_start(char *rx_buffer, int rx_buffer_size);
int host_dma_stop(void);
int host_dma_get_nb_bytes_received(int *nb_rx_bytes);

int host_led_toggle(unsigned int led_nb);
int host_led_set_state(unsigned int led_nb, int state);

void host_delay(uint32_t nb_ticks);

#endif
