extern "C" {
#include <csp/csp.h>

#include "util.h"
#include "io.h"
}

#include "flash-wrapper.hpp"
#include "flash_sched.h"

#define FLASH_DMA_IN_COUNT 4
#define FLASH_DMA_OUT_COUNT 4
#define FLASH_DMA_IN_SIZE (sizeof(flash_task_t) * FLASH_DMA_IN_COUNT)
#define FLASH_DMA_OUT_SIZE (sizeof(flash_task_t) * FLASH_DMA_OUT_COUNT)
#define FLASH_DMA_SIZE (FLASH_DMA_IN_SIZE + FLASH_DMA_OUT_SIZE)

void flash_wrapper::ioread32(struct io_req *req, struct io_rsp *rsp)
{
	int reg = req->local_offset >> 2;

	obj_dbg(&flash->dev.obj, "%s: size %d offset 0x%x\n", __func__, req->size, req->local_offset);

	switch (reg) {
	case FLASH_REG_CMD:
		rsp->val = status_reg;
		break;
	case FLASH_REG_SRC:
		rsp->val = dma_phys_addr_src;
		break;
	case FLASH_REG_DST:
		rsp->val = dma_phys_addr_dst;
		break;
	case FLASH_REG_MAX_SIZE:
		rsp->val = FLASH_DMA_SIZE;
		break;
	case FLASH_REG_ID:
		rsp->val = flash->dev.id;
		break;
	default:
		BUG();
	}
}

void flash_wrapper::iowrite32(const struct io_req *req, struct io_rsp *rsp)
{
	int reg = req->local_offset >> 2;

	obj_dbg(&flash->dev.obj, "%s: size %d offset 0x%x val 0x%x\n",
		__func__, req->size, req->local_offset, req->val);

	rsp->val = req->val;

	switch (reg) {
	case FLASH_REG_CMD:
	    if (req->val == FLASH_CMD_RESET) {
	    	rst_dut.write(false);
	    	wait();
	    	rst_dut.write(true);
	    }
		else
		    BUG();
		status_reg = req->val;
		break;
	case FLASH_REG_SRC:
		dma_phys_addr_src = req->val;
		break;
	case FLASH_REG_DST:
		dma_phys_addr_dst = req->val;
		break;
	default:
		BUG();
	}
}

void flash_wrapper::copy_from_dram(u64 index, unsigned length)
{
	obj_dbg(&flash->dev.obj, "%s\n", __func__);
	/* Byte address */
	out_phys_addr.put(dma_phys_addr_src + (index * sizeof(flash_task_t)));
	/* Number of DMA token (templated type). u16 for flash */
	out_len.put(length);
	out_write.put(false);
	out_start.put(true);
	write_to_device();
}

void flash_wrapper::write_to_device()
{
	
}

void flash_wrapper::copy_to_dram(u64 index, unsigned length)
{
	obj_dbg(&flash->dev.obj, "%s\n", __func__);
	out_phys_addr.put(dma_phys_addr_dst + (index * sizeof(flash_task_t)));
	out_len.put(length);
	out_write.put(true);
	out_start.put(true);
	write_to_dma();
}

void flash_wrapper::write_to_dma()
{

}

void flash_wrapper::start()
{
	// RESET DUT
	rst_dut.write(false);
	wait();
	rst_dut.write(true);

	for (;;) {
		wait();

		obj_dbg(&flash->dev.obj, "CTL start\n");
		drive();
		obj_dbg(&flash->dev.obj, "FLASH done\n");
	}
}

void flash_wrapper::drive()
{
	for (;;) {
		do {
			wait();
		} while (!rd_request.read() && !wr_request.read());
		if (rd_request.read()) {
			unsigned index = rd_index.read();
			unsigned length = rd_length.read();

			rd_tran_cnt++;
			rd_byte += length * sizeof(flash_task_t);

			rd_grant.write(true);

			do { wait(); }
			while (rd_request.read());
			rd_grant.write(false);
			wait();

			copy_from_dram((u64) index, length);
		} else {
			// WRITE REQUEST
			unsigned index = wr_index.read();
			unsigned length = wr_length.read();

			wr_tran_cnt++;
			wr_byte += length * sizeof(flash_task_t);

			wr_grant.write(true);

			do { wait(); }
			while (wr_request.read());
			wr_grant.write(false);
			wait();

			copy_to_dram((u64) index, length);
		}
	}
}

void flash_wrapper::io()
{
	struct io_req req;
	struct io_rsp rsp;

	for (;;) {
		/*
		 * Most of the time the channel will be empty; we speed things
		 * up by just peeking directly at the queue, avoiding syscalls.
		 * We do this every 10 cycles to simulate faster.
		 */
		wait(10);
		if (csp_channel_is_empty(flash->dev.io_req))
			continue;

		if (unlikely(io_recv_req(flash->dev.io_req, &req)))
			die_errno(__func__);

		//cout << req.write << " - " << std::hex << req.local_offset << " ===" << std::dec << (int)req.size << "===" << endl;

		BUG_ON(req.size != 4); /* XXX */

		rsp.local_offset = req.local_offset;
		rsp.size = req.size;

		if (req.write)
			iowrite32(&req, &rsp);
		else
			ioread32(&req, &rsp);
		if (unlikely(io_send_rsp(req.rsp_chan, &rsp)))
			die_errno(__func__);
	}
}

