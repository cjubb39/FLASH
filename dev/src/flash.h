#ifndef _FLASH_H_
#define _FLASH_H_

#include "systemc.h"
#include <stdint.h>

SC_MODULE(flash) {
	sc_in<bool> clk;
	sc_in<bool> rst;

	sc_out<bool> operational;

	/* schedule requests */
	sc_in <bool>     sched_req;
	sc_out<bool>     sched_grant;
	sc_out<uint32_t> next_process;

	/* tick command */
	sc_out<bool> tick_req;
	sc_in <bool> tick_grant;

	/* process update request */
	sc_in <bool>     change_req;
	sc_out<bool>     change_grant;
	sc_in <uint32_t> change_pid;
	sc_in <uint8_t>  change_pri;
	sc_in <uint8_t>  change_state;

	void schedule();

	SC_CTOR(flash) {
		SC_CTHREAD(schedule, clk.pos());
		reset_signal_is(rst, false);
	}

};
#endif

