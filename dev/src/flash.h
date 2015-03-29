#ifndef _FLASH_H_
#define _FLASH_H_

#include "systemc.h"
#include <stdint.h>

#include "flash_sched.h"

#ifndef TASK_QUEUE_SIZE
#error "TASK_QUEUE_SIZE not defined"
#endif

#ifndef WAIT_PER_TICK
#error "WAIT_PER_TICK not defined"
#endif

#define PID_POISON 0

SC_MODULE(flash) {
	sc_in<bool> clk;
	sc_in<bool> rst;

	sc_out<bool> operational;

	/* schedule requests */
	sc_in <bool>        sched_req;
	sc_out<bool>        sched_grant;
	sc_out<flash_pid_t> next_process;

	/* tick command */
	sc_out<bool> tick_req;
	sc_in <bool> tick_grant;

	/* process update request */
	sc_in <bool>           change_req;
	sc_out<bool>           change_grant;
	sc_in <flash_change_t> change_type;
	sc_in <flash_pid_t>    change_pid;
	sc_in <flash_pri_t>    change_pri;
	sc_in <flash_state_t>  change_state;

	void initialize();
	void tick();
	void process_change();
	void schedule();

	SC_CTOR(flash) {
		SC_CTHREAD(initialize, clk.pos());
		reset_signal_is(rst, false);

		SC_CTHREAD(tick, clk.pos());
		reset_signal_is(tick_rst, false);

		SC_CTHREAD(process_change, clk.pos());
		reset_signal_is(rst, false);

		SC_CTHREAD(schedule, clk.pos());
		reset_signal_is(rst, false);
	}

	private:
	sc_signal<bool> init_done;
	sc_signal<bool> tick_req_internal;
	sc_signal<bool> tick_grant_internal;
	sc_signal<bool> tick_rst;

	flash_task_t get_next_task();

	uint32_t cur_task [FLASH_MAX_PRI];
	uint32_t end_queue[FLASH_MAX_PRI];
	flash_task_t queue[FLASH_MAX_PRI][TASK_QUEUE_SIZE];
};
#endif

