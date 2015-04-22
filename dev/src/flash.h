#ifndef _FLASH_H_
#define _FLASH_H_

#define SC_INCLUDE FX
#include "systemc.h"
#include <stdint.h>

#include "flash_sched.h"

/* fixed point for division */
#ifdef __CTOS__
#define CTOS_FP
#endif

#ifdef CTOS_FP
#include <ctos_fx.h>
#define SC_FIX_t ctos_sc_dt::sc_fixed<64,64>
#else
#define SC_FIX_t uint64_t
#endif

#include "math/div.h"

#ifndef TASK_QUEUE_SIZE
#error "TASK_QUEUE_SIZE not defined"
#endif

#ifndef RUN_QUEUE_SIZE
#error "RUN_QUEUE_SIZE not defined"
#endif

#ifndef WAIT_PER_TICK
#error "WAIT_PER_TICK not defined"
#endif

#define PID_POISON (-1)
#define INDEX_POISON (-1)

//#define VERBOSE

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
	void timer();

	SC_CTOR(flash) {
		SC_CTHREAD(initialize, clk.pos());
		reset_signal_is(rst, false);

		SC_CTHREAD(tick, clk.pos());
		reset_signal_is(tick_rst, false);

		SC_CTHREAD(process_change, clk.pos());
		reset_signal_is(rst, false);

		SC_CTHREAD(schedule, clk.pos());
		reset_signal_is(rst, false);

		SC_CTHREAD(timer, clk.pos());
		reset_signal_is(rst, false);
	}

	private:
	sc_signal<bool> init_done;
	sc_signal<bool> tick_req_internal;
	sc_signal<bool> tick_grant_internal;
	sc_signal<bool> tick_rst;

	sc_signal<int> current_task_index;
	sc_signal<uint64_t> time;

	flash_task_t get_next_task();
	int lookup_process(flash_pid_t);
	int find_empty_slot();
	int add_task_to_run_queue(int process_index);
	int remove_task_from_run_queue(int process_index);
	uint64_t inline calculate_virtual_runtime(uint64_t, flash_pri_t);

	flash_task_t process_list[TASK_QUEUE_SIZE];
	int runnable_list[RUN_QUEUE_SIZE];

};
#endif
