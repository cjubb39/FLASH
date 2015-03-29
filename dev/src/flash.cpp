#include "flash.h"

void flash::initialize() {
	int i;

	init_done.write(false);
	wait();

	for (i = 0; i < FLASH_MAX_PRI; ++i) {
		cur_task[i] = 0;
		end_queue[i] = 0;
	}
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
	flash_pri_t task_pri, new_task_pri;
	flash_pid_t task_pid;
	size_t insertion_point;
	int i;

	change_grant.write(false);
	do { wait(); }
	while (!init_done.read());

	while (true) {
		do { wait(); }
		while (!change_req.read());
		change_grant.write(true);
		req_type = change_type.read();
		task_pid = change_pid.read();


		if (req_type & __FLASH_CHANGE_NEW) {
			task_pri = change_pri.read();
			insertion_point = end_queue[task_pri];
			printf("%lu %lu %lu\n", task_pri, task_pid, insertion_point);
			queue[task_pri][insertion_point].pid    = task_pid;
			queue[task_pri][insertion_point].pri    = change_pri.read();
			queue[task_pri][insertion_point].state  = change_state.read();
			queue[task_pri][insertion_point].active = 1;

			MODULAR_INCR(end_queue[task_pri], TASK_QUEUE_SIZE);
		} else {
			cout << "REAL UPDATE" << endl;
			/* no new task */
			for (task_pri = 0; task_pri < FLASH_MAX_PRI; ++task_pri) {
				for (i = 0; i < TASK_QUEUE_SIZE; ++i) {
					if (!queue[task_pri][i].active)
						continue;

					/* found a match */
					if (queue[task_pri][i].pid == task_pid) {
						if (req_type & FLASH_CHANGE_STATE) {
							queue[task_pri][i].state = change_state.read();
						}

						if (req_type & FLASH_CHANGE_PRI) {
							new_task_pri = change_pri.read();
							insertion_point = end_queue[new_task_pri];

							/* copy entry and make old one inactive */
							queue[new_task_pri][insertion_point] = queue[task_pri][i];
							queue[task_pri][i].active    = 0;
							
							/* update end_queue on new priority list */
							MODULAR_INCR(end_queue[new_task_pri], TASK_QUEUE_SIZE);
						}

						break;
					} // end find match
				}
			}
		} // end no new tasks

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
		next_proc_pid = get_next_task().pid;

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

