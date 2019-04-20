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

#include "sl_wfx.h"
#include "bus/sl_wfx_bus.h"
#include "sl_wfx_host_api.h"

#define MAX_RETRIES 3
#define CONFIG_PREFETCH_BIT (1 << 13)

sl_status_t sl_wfx_reg_read_16(sl_wfx_register_address_t address, uint16_t *value_out)
{
  uint8_t tmp[4];
  sl_status_t result = sl_wfx_reg_read(  address, tmp, sizeof( tmp ) );

  *value_out = sl_wfx_unpack_16bit_little_endian( tmp );

  return result;
}

sl_status_t sl_wfx_reg_write_16(sl_wfx_register_address_t address, uint16_t value_in)
{
  uint8_t tmp[4];

  sl_wfx_pack_16bit_little_endian( tmp, value_in );
  tmp[2] = 0;
  tmp[3] = 0;

  return sl_wfx_reg_write(address, tmp, sizeof( tmp ));
}

sl_status_t sl_wfx_reg_read_32(sl_wfx_register_address_t address, uint32_t *value_out)
{
  sl_status_t result = sl_wfx_reg_read(address, value_out, sizeof(*value_out));

  *value_out = sl_wfx_htole32(*value_out);

  return result;
}

sl_status_t sl_wfx_reg_write_32(sl_wfx_register_address_t address, uint32_t value_in)
{
  value_in = sl_wfx_htole32(value_in);

  return sl_wfx_reg_write(address, &value_in, sizeof(value_in));
}

sl_status_t sl_wfx_data_read(void *buffer, uint32_t length)
{
  sl_status_t result;

  result = sl_wfx_reg_read(SL_WFX_IN_OUT_QUEUE_REG_ID, buffer, length);

  return result;
}

sl_status_t sl_wfx_data_write(const void *buffer, uint32_t length)
{
  sl_status_t result;

  result = sl_wfx_reg_write(SL_WFX_IN_OUT_QUEUE_REG_ID, buffer, length);

  return result;
}

sl_status_t sl_wfx_apb_write(uint32_t address, const void *buffer, uint32_t length)
{
  sl_status_t result;
  if (length / 2 >= 0x1000) {
    return SL_BAD_ARG;
  }

  result = sl_wfx_reg_write_32(SL_WFX_SRAM_BASE_ADDR_REG_ID, address);

  if (result == SL_SUCCESS) {
    result = sl_wfx_reg_write(SL_WFX_SRAM_DPORT_REG_ID, buffer, length);
  }

  return result;
}

sl_status_t sl_wfx_apb_write_32(uint32_t address, uint32_t value_in)
{
  sl_status_t result;

  result = sl_wfx_reg_write_32(SL_WFX_SRAM_BASE_ADDR_REG_ID, address);

  if (result == SL_SUCCESS) {
    result = sl_wfx_reg_write_32(SL_WFX_SRAM_DPORT_REG_ID, value_in);
  }

  return result;
}

sl_status_t sl_wfx_apb_read_32(uint32_t address, uint32_t *value_out)
{
  uint32_t value32;
  sl_status_t result;

  // write address
  result = sl_wfx_reg_write_32(SL_WFX_SRAM_BASE_ADDR_REG_ID, address);

  // set the "prefetch" bit
  result = sl_wfx_reg_read_32(SL_WFX_CONFIG_REG_ID, &value32);
  result = sl_wfx_reg_write_32(SL_WFX_CONFIG_REG_ID, value32 | CONFIG_PREFETCH_BIT);

  // and wait for the prefetch bit to clear
  for (uint32_t i = 0; i < 20; i++) {
    result = sl_wfx_reg_read_32(SL_WFX_CONFIG_REG_ID, &value32);
    if ((value32 & CONFIG_PREFETCH_BIT) == 0) {
      break;
    }

    sl_wfx_host_wait(1);
  }

  // and data is ready
  result = sl_wfx_reg_read_32(SL_WFX_SRAM_DPORT_REG_ID, value_out);

  return result;
}
