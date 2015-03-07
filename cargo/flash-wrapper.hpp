#ifndef FLASH_WRAPPER_HPP
#define FLASH_WRAPPER_HPP

extern "C" {
#include "flash-sync.h"
}

#include <flex_channels.hpp>

#include "flash.h"

SC_MODULE(flash_wrapper) {
  sc_in<bool> clk; // clock
  sc_in<bool> rst; // reset
  sc_out<bool> rst_dut; // reset DUT

  // // DMA requests interface
  // sc_in<unsigned>   rd_index;   // array index
  // sc_in<unsigned>   rd_length;
  // sc_in<bool>       rd_request; // transaction request
  // sc_out<bool>      rd_grant;   // transaction grant

  // // DMA requests interface
  // sc_in<unsigned>   wr_index;   // array index
  // sc_in<unsigned>   wr_length;
  // sc_in<bool>       wr_request; // transaction request
  // sc_out<bool>      wr_grant;   // transaction grant

  // /* DMA */
  // put_initiator<unsigned long long> out_phys_addr;
  // put_initiator<unsigned long> out_len;
  // put_initiator<bool> out_write;
  // put_initiator<bool> out_start;

  sc_out<unsigned> conf_size;
  sc_out<bool>     conf_done;

  // computation complete. Written by store_output
  sc_in<bool> flash_done;

  void iowrite32(const struct io_req *req, struct io_rsp *rsp);
  void ioread32(struct io_req *req, struct io_rsp *rsp);

  // void drive();
  // void copy_from_dram(u64 index, unsigned length);
  // void copy_to_dram(u64 index, unsigned length);

  void io();
  void start();

  typedef flash_wrapper SC_CURRENT_USER_MODULE;
  flash_wrapper(sc_core::sc_module_name, struct flash_sync *flash_)
  {
    SC_CTHREAD(io, clk.pos());
    reset_signal_is(rst, false);

    SC_CTHREAD(start, clk.pos());
    reset_signal_is(rst, false);

    flash = flash_;
  }

private:
  struct flash_sync *flash; /* device driver interface */

  /**
   * We can use variables in this case, because the
   * driver guaramtees that these values won't change
   * while the device is running, therefore there is no
   * race condition between CTHREAD io() and CTHREAD start()
   * even though they share these variables.
   */
  u32 dma_phys_addr_src; /* data-in begin address */
  u32 dma_phys_addr_dst; /* data-out begin address */
  size_t dma_size_src;   /* data-in size (# of T -> see DMA instance) */
  size_t dma_size_dst;   /* data-out size (# of T -> see DMA instance) */

  u32 status_reg;        /* [0] go command */
                         /* [1-3] unused */
                         /* [4-5] see flash-sync.h */

  tlm_fifo<bool> start_fifo; /* handshake between driver and device */
                             /* when configuration is done the device can go */

  /* accelerator-activity profiling */
  unsigned rd_tran_cnt;
  unsigned long long rd_byte;
  unsigned wr_tran_cnt;
  unsigned long long wr_byte;
};


#endif /* FLASH_WRAPPER_HPP */
