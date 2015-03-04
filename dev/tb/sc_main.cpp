#include "systemc.h"

#include "flash.h"
#include "flash_tb.h"
#include "flash_sched.h"

int sc_main(int, char**) {
	sc_clock        clk ("clk", 10, SC_NS);	
	sc_signal<bool> rst;
	sc_signal<bool> flash_rst;

	sc_signal<bool>     operational;

	/* schedule requests */
	sc_signal<bool>        sched_req;
	sc_signal<bool>        sched_grant;
	sc_signal<flash_pid_t> next_process;

	/* tick command */
	sc_signal<bool> tick_req;
	sc_signal<bool> tick_grant;

	/* process update request */
	sc_signal<bool>          change_req;
	sc_signal<bool>          change_grant;
	sc_signal<flash_pid_t>   change_pid;
	sc_signal<flash_pri_t>   change_pri;
	sc_signal<flash_state_t> change_state;

	flash    dut("dut");
	flash_tb tb("tb");

	dut.clk(clk);
	dut.rst(flash_rst);
	dut.operational(operational);
	dut.sched_req(sched_req);
	dut.sched_grant(sched_grant);
	dut.next_process(next_process);
	dut.tick_req(tick_req);
	dut.tick_grant(tick_grant);
	dut.change_req(change_req);
	dut.change_grant(change_grant);
	dut.change_pid(change_pid);
	dut.change_pri(change_pri);
	dut.change_state(change_state);

	tb.clk(clk);
	tb.rst(rst);
	tb.flash_rst(flash_rst);
	tb.operational(operational);
	tb.sched_req(sched_req);
	tb.sched_grant(sched_grant);
	tb.next_process(next_process);
	tb.tick_req(tick_req);
	tb.tick_grant(tick_grant);
	tb.change_req(change_req);
	tb.change_grant(change_grant);
	tb.change_pid(change_pid);
	tb.change_pri(change_pri);
	tb.change_state(change_state);


	rst.write(false);
	sc_start(30, SC_NS);
	rst.write(true);

	sc_start();

	return 0;
}
