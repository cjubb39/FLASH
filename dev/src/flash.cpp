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
	tick_req.write(false);
	do { wait(); }
	while (!init_done.read());

	while(true) {
		for (i = 0 ; i < WAIT_PER_TICK; ++i) {
			wait();
		}

		next_process.write(queue[cur_task].pid);
		tick_req.write(true);
		do { wait(); }
		while (!tick_grant.read());
		tick_req.write(false);
		next_process.write(PID_POISON);

		cur_task = (cur_task + 1) % end_queue;
	}
}

void flash::schedule() {
	change_grant.write(false);
	do { wait(); }
	while (!init_done.read());

	while (true) {
		do { wait(); }
		while (!change_req.read());
		change_grant.write(true);
		queue[end_queue].pid   = change_pid.read();
		queue[end_queue].pri   = change_pri.read();
		queue[end_queue].state = change_state.read();
		++end_queue;
		do { wait(); }
		while (change_req.read());
		change_grant.write(false);
		wait();
	}
}

