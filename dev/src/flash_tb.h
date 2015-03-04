#ifndef _FLASH_TB_H_
#define _FLASH_TB_H_

#include "systemc.h"

SC_MODULE(flash_tb) {
	sc_in<bool> clk;
	sc_in<bool> rst;
	sc_out<bool> flash_rst;

	sc_in<bool> operational;

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

