#include "flash.h"

void flash::initialize() {
	init_done.write(false);
	wait();
	cur_task = 0;
	end_queue = 0;
	init_done.write(true);

	while(true) {
		wait();
	}
}

void flash::tick() {
	unsigned i = 0;
	tick_req_internal.write(false);
	do { wait(); }
	while (!init_done.read());

	while(true) {
		for (i = 0 ; i < WAIT_PER_TICK; ++i) {
			wait();
		}

		tick_req_internal.write(true);
		do { wait(); }
		while (!tick_grant_internal.read());
		tick_req_internal.write(false);
	}
}

void flash::process_change() {
	flash_change_t req_type;
	change_grant.write(false);
	do { wait(); }
	while (!init_done.read());

	while (true) {
		do { wait(); }
		while (!change_req.read());
		change_grant.write(true);
		req_type = change_type.read();

		if (req_type & FLASH_CHANGE_PID)
			queue[end_queue].pid   = change_pid.read();
		if (req_type & FLASH_CHANGE_PRI)
			queue[end_queue].pri   = change_pri.read();
		if (req_type & FLASH_CHANGE_STATE)
			queue[end_queue].state = change_state.read();

		++end_queue;
		do { wait(); }
		while (change_req.read());
		change_grant.write(false);
		wait();
	}
}

void flash::schedule() {
	flash_pid_t next_proc_pid;

	tick_rst.write(false);
	tick_grant_internal.write(false);
	tick_req.write(false);
	do { wait(); }
	while (!init_done.read());
	tick_rst.write(true);

	while (true) {
		do { wait(); }
		while (!tick_req_internal.read() && !sched_req.read());

		/* decide what's next */
		next_proc_pid = queue[cur_task].pid;
		cur_task = (cur_task + 1) % end_queue;

		if (tick_req_internal.read()) {
			/* four phase handshake (internal) */
			tick_grant_internal.write(true);
			do { wait(); }
			while (tick_req_internal.read());
			tick_grant_internal.write(false);

			/* four phase external */
			tick_req.write(true);
			next_process.write(next_proc_pid);
			do { wait(); }
			while (!tick_grant.read());
			tick_req.write(false);
			do { wait(); }
			while (tick_grant.read());

		} else { /* sched_req.read() */
			/* reset tick */
			tick_rst.write(false);

			sched_grant.write(true);
			next_process.write(next_proc_pid);
			do { wait(); }
			while (sched_req.read());
			sched_grant.write(false);

			/* turn start tick again */
			tick_rst.write(true);
		}
	}
}

