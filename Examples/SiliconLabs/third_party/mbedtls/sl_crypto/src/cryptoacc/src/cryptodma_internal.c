/**
 * @file
 * @brief Provides the API to access the cryptographic engine through the cryptoDMA
 * @copyright Copyright (c) 2016-2018 Silex Insight. All Rights reserved
 */

#include "cryptodma_internal.h"
#include "sxregs.h"
#include "cryptolib_def.h"
#include "sx_errors.h"

static dma_sg_regs32_t * const regs = (dma_sg_regs32_t *) ADDR_CRYPTOMASTER_REGS;

void cryptodma_config_sg(
      struct dma_sg_descr_s * first_fetch_descriptor,
      struct dma_sg_descr_s * first_push_descriptor)
{
   WR_REG32(regs->fetch_addr, (uint32_t) first_fetch_descriptor);
   WR_REG32(regs->push_addr,  (uint32_t) first_push_descriptor);
   WR_REG32(regs->config,      DMA_AXI_CONFIGREG_FETCHER_INDIRECT|DMA_AXI_CONFIGREG_PUSHER_INDIRECT);
}

void cryptodma_config_direct(block_t dest, block_t src, uint32_t length)
{
   uint32_t transfer_len = dest.len<length ? dest.len : length;
   // if destination is a FIFO, size needs to be a multiple of 32-bits.
   if (dest.flags & BLOCK_S_CONST_ADDR)
      transfer_len = roundup_32(transfer_len);

   transfer_len &= DMA_AXI_DESCR_MASK_LENGTH;

   WR_REG32(regs->fetch_addr, (uint32_t) src.addr);
   WR_REG32(regs->fetch_len,  transfer_len | (src.flags & BLOCK_S_FLAG_MASK_DMA_PROPS) | DMA_AXI_DESCR_REALIGN);
   WR_REG32(regs->fetch_tag,  DMA_SG_ENGINESELECT_BYPASS);
   WR_REG32(regs->push_addr,  (uint32_t) dest.addr);
   WR_REG32(regs->push_len,   transfer_len | (dest.flags & BLOCK_S_FLAG_MASK_DMA_PROPS) | DMA_AXI_DESCR_REALIGN);
   WR_REG32(regs->config,     DMA_AXI_CONFIGREG_PUSHER_DIRECT | DMA_AXI_CONFIGREG_FETCHER_DIRECT);
}

void cryptodma_start(void) {
   WR_REG32(regs->int_stat_clr, DMA_AXI_INTENSETREG_ALL_EN);
   WR_REG32(regs->int_en,       DMA_AXI_INTENSETREG_PUSHER_STOPPED_EN);
   WR_REG32(regs->start,        DMA_AXI_STARTREG_FETCH | DMA_AXI_STARTREG_PUSH);
}

void cryptodma_wait(void) {
   // Wait until DMA is done
   #if WAIT_CRYPTOMASTER_WITH_REGISTER_POLLING // polling
      while (RD_REG32(regs->status)&DMA_AXI_STATUSREG_MASK_PUSHER_BUSY);
   #else  // wait interrupt
      #error "Interrupt not supported, polling is the only option"
   #endif
}

uint32_t cryptodma_check_bus_error(void) {
   if (RD_REG32(regs->int_stat_raw)&(DMA_AXI_RAWSTATREG_MASK_FETCHER_ERROR | DMA_AXI_RAWSTATREG_MASK_PUSHER_ERROR)) {
      CRYPTOLIB_PRINTF("CRYPTODMA bus error\n");
      return CRYPTOLIB_DMA_ERR;
   } else {
      return CRYPTOLIB_SUCCESS;
   }
}

/**
 * @brief Check cryptodma fifo status
 * @return CRYPTOLIB_DMA_ERR if bus error occured, CRYPTOLIB_SUCCESS otherwise
 */
static uint32_t cryptodma_check_fifo_empty(void) {
   uint32_t dma_status = RD_REG32(regs->status);
   if (dma_status & (DMA_AXI_STATUSREG_MASK_FIFOIN_NOT_EMPTY|DMA_AXI_STATUSREG_MASK_FIFOOUT_NDATA)) {
      CRYPTOLIB_PRINTF("CRYPTODMA fifo error %08x\n", dma_status);
      return CRYPTOLIB_DMA_ERR;
   } else {
      return CRYPTOLIB_SUCCESS;
   }
}

void cryptodma_check_status(void) {
   if (cryptodma_check_bus_error() | cryptodma_check_fifo_empty()) {
      TRIGGER_HARDFAULT_FCT();
   }
}

void cryptodma_reset(void) {
   WR_REG32(regs->config, DMA_AXI_CONFIGREG_SOFTRESET);
   WR_REG32(regs->config, 0);                                          // Clear soft-reset
   while (RD_REG32(regs->status) & DMA_AXI_STATUSREG_MASK_SOFT_RESET); // Wait for soft-reset deassertion
}




#define DMA_SG_DEBUG 0

#if DMA_SG_DEBUG
void debug_print_sg(struct dma_sg_descr_s * desc_in)
{
   CRYPTOLIB_PRINTF("\n-------------------------------------------------------\n");
   CRYPTOLIB_PRINTF("descriptor        : dataptr  nextptr  length   tag\n");
   volatile struct dma_sg_descr_s * desc_ptr = desc_in;
   uint32_t i = 0;
   while(desc_ptr != DMA_AXI_DESCR_NEXT_STOP)
   {
      if(i>50)
         TRIGGER_HARDFAULT_FCT();

      CRYPTOLIB_PRINTF("desc #%02d @%08X: %08X %08X %08X %08X\n", i, desc_ptr, desc_ptr->addr, desc_ptr->next_descr, desc_ptr->length_irq, desc_ptr->tag);
      desc_ptr = desc_ptr->next_descr;
      i++;
   }
   CRYPTOLIB_PRINTF("-------------------------------------------------------\n");
}
#endif

#ifndef MAP_DESCRIPTOR_HOOKS

void map_descriptors(struct dma_sg_descr_s *first_fetch_descriptor,
   struct dma_sg_descr_s *first_push_descriptor,
   struct dma_sg_descr_s **mapped_in,
   struct dma_sg_descr_s **mapped_out)
{
   *mapped_in = first_fetch_descriptor;
   *mapped_out = first_push_descriptor;
}

void unmap_descriptors(struct dma_sg_descr_s *out_descs)
{
   (void)out_descs;
}
#endif

void cryptodma_run_sg(struct dma_sg_descr_s * first_fetch_descriptor, struct dma_sg_descr_s * first_push_descriptor)
{
   struct dma_sg_descr_s *mapped_in, *mapped_out;
   #if DMA_SG_DEBUG
      debug_print_sg(first_fetch_descriptor);
      debug_print_sg(first_push_descriptor);
   #endif

   map_descriptors(first_fetch_descriptor, first_push_descriptor, &mapped_in, &mapped_out);
   cryptodma_config_sg(mapped_in, mapped_out);
   cryptodma_start();
   cryptodma_wait();
   cryptodma_check_status();
   unmap_descriptors(first_push_descriptor);
}

struct dma_sg_descr_s* write_desc_always(
      struct dma_sg_descr_s *descr,
      volatile void *addr,
      const uint32_t length,
      const uint32_t flags,
      const uint32_t tag)
{
   struct dma_sg_descr_s *nextdescr = descr + 1;
   descr->next_descr = nextdescr;
   descr->addr = addr;

   // Build flags[31:28] and length[27:0]
   descr->length_irq = length & DMA_AXI_DESCR_MASK_LENGTH;
   if(addr)
      descr->length_irq |= (flags & ~DMA_AXI_DESCR_MASK_LENGTH);
   else
      descr->length_irq |= DMA_AXI_DESCR_DISCARD;

   descr->tag = tag;
   return nextdescr;
}

void realign_desc(struct dma_sg_descr_s * d)
{
   d->length_irq |= DMA_AXI_DESCR_REALIGN;
}

void set_last_desc(struct dma_sg_descr_s * d)
{
   d->next_descr = DMA_AXI_DESCR_NEXT_STOP;
   d->tag |= DMA_SG_TAG_ISLAST;
   d->length_irq |= DMA_AXI_DESCR_REALIGN;
}

void set_desc_invalid_bytes(
      struct dma_sg_descr_s *d,
      const uint32_t n_bytes)
{
   // Check it is the last data descriptor
   CRYPTOLIB_ASSERT(!(d->tag & DMA_SG_TAG_ISCONFIG) &&
         (d->tag & DMA_SG_TAG_ISLAST), "Descriptor is not tagged as last data");

   // Check the engine supports padding
   switch(d->tag & DMA_SG_ENGINESELECT_MASK) {
   case DMA_SG_ENGINESELECT_BA411E:
   case DMA_SG_ENGINESELECT_BA413:
   case DMA_SG_ENGINESELECT_BA417:
   {
      uint32_t pad = (n_bytes & DMA_SG_TAG_PADDING_MASK);
      d->tag |= pad << DMA_SG_TAG_PADDING_OFFSET;
      d->length_irq += pad;
      break;
   }
   default:
   CRYPTOLIB_ASSERT(0, "Crypto engine does not support invalid bytes/ is not implemented");
      break;
   }
}

