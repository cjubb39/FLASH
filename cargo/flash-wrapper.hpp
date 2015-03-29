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

  // DMA requests interface
  sc_in<unsigned>   rd_index;   // array index
  sc_in<unsigned>   rd_length;
  sc_in<bool>       rd_request; // transaction request
  sc_out<bool>      rd_grant;   // transaction grant

  // DMA requests interface
  sc_in<unsigned>   wr_index;   // array index
  sc_in<unsigned>   wr_length;
  sc_in<bool>       wr_request; // transaction request
  sc_out<bool>      wr_grant;   // transaction grant

  /* DMA */
  put_initiator<unsigned long long> out_phys_addr;
  put_initiator<unsigned long> out_len;
  put_initiator<bool> out_write;
  put_initiator<bool> out_start;

  sc_in<bool> operational;

  /* schedule requests */
  sc_out<bool>        sched_req;
  sc_in<bool>        sched_grant;
  sc_in<flash_pid_t> next_process;

  /* tick command */
  sc_out<bool> tick_req;
  sc_in<bool> tick_grant;

  /* process update request */
  sc_out<bool>          change_req;
  sc_in<bool>           change_grant;
  sc_out<flash_pid_t>   change_pid;
  sc_out<flash_pri_t>   change_pri;
  sc_out<flash_state_t> change_state;

  void iowrite32(const struct io_req *req, struct io_rsp *rsp);
  void ioread32(struct io_req *req, struct io_rsp *rsp);

  void drive();
  void copy_from_dram(u64 index, unsigned length);
  void copy_to_dram(u64 index, unsigned length);

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

  /* accelerator-activity profiling */
  unsigned rd_tran_cnt;
  unsigned long long rd_byte;
  unsigned wr_tran_cnt;
  unsigned long long wr_byte;
};


#endif /* FLASH_WRAPPER_HPP */
