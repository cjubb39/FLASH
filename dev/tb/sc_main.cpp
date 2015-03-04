#include "systemc.h"

#include "flash.h"
#include "flash_tb.h"

int sc_main(int, char**) {
	sc_clock        clk ("clk", 10, SC_NS);	
	sc_signal<bool> rst;
	sc_signal<bool> flash_rst;

	sc_signal<bool> operational;

	flash    dut("dut");
	flash_tb tb("tb");

	dut.clk(clk);
	dut.rst(flash_rst);
	dut.operational(operational);

	tb.clk(clk);
	tb.rst(rst);
	tb.flash_rst(flash_rst);
	tb.operational(operational);


	rst.write(false);
	sc_start(30, SC_NS);
	rst.write(true);

	sc_start();

	return 0;
}
