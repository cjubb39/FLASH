#ifndef _FLASH_TB_H_
#define _FLASH_TB_H_

#include "systemc.h"
#include <stdint.h>

#include "flash_sched.h"

#define TASKS_TO_SEND 128
#define TASKS_TO_READ 1024

SC_MODULE(flash_tb) {
	sc_in<bool> clk;
	sc_in<bool> rst;
	sc_out<bool> flash_rst;

	sc_in<bool> operational;

	/* schedule requests */
	sc_out<bool>        sched_req;
	sc_in <bool>        sched_grant;
	sc_in <flash_pid_t> next_process;

	/* tick command */
	sc_in <bool> tick_req;
	sc_out<bool> tick_grant;

	/* process update request */
	sc_out<bool>          change_req;
	sc_in <bool>          change_grant;
	sc_out<flash_pid_t>   change_pid;
	sc_out<flash_pri_t>   change_pri;
	sc_out<flash_state_t> change_state;

	void source();
	void sink();

	SC_CTOR(flash_tb) {
		SC_CTHREAD(source, clk.pos());
		reset_signal_is(rst, false);
		
		SC_CTHREAD(sink, clk.pos());
		reset_signal_is(rst, false);
	}
};
#endif
