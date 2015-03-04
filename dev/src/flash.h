#ifndef _FLASH_H_
#define _FLASH_H_

#include "systemc.h"

SC_MODULE(flash) {
	sc_in<bool> clk;
	sc_in<bool> rst;

	sc_out<bool> operational;

	void schedule();

	SC_CTOR(flash) {
		SC_CTHREAD(schedule, clk.pos());
		reset_signal_is(rst, false);
	}

};
#endif

