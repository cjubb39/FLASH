#ifndef _FLASH_H_
#define _FLASH_H_

#include "systemc.h"
#include <stdint.h>

#include "flash_sched.h"

typedef struct {
	flash_pid_t pid;
	flash_pri_t pri;
	flash_state_t state;
} flash_task_t;

#ifndef TASK_QUEUE_SIZE
#define TASK_QUEUE_SIZE 128
#endif

#ifndef WAIT_PER_TICK
#define WAIT_PER_TICK 32
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
	sc_in <bool>          change_req;
	sc_out<bool>          change_grant;
	sc_in <flash_pid_t>   change_pid;
	sc_in <flash_pri_t>   change_pri;
	sc_in <flash_state_t> change_state;

	void schedule();
	void tick();

	SC_CTOR(flash) {
		SC_CTHREAD(tick, clk.pos());
		reset_signal_is(rst, false);

		SC_CTHREAD(schedule, clk.pos());
		reset_signal_is(rst, false);
	}

	private:
	uint32_t cur_task;
	flash_task_t queue[TASK_QUEUE_SIZE];

};
#endif

