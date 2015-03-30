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

int flash::lookup_process(flash_pid_t pid, flash_pri_t *pri,
		size_t *position) {
	flash_pri_t task_pri;
	size_t i;

	for (task_pri = 0; task_pri < FLASH_MAX_PRI; ++task_pri) {
		for (i = 0; i < TASK_QUEUE_SIZE; ++i) {
			if (queue[task_pri][i].pid == pid) {
				*pri = task_pri;
				*position = i;
				return 1;
			}
		}
	}
	return 0; /* no match */
}


void flash::process_change() {
	flash_change_t req_type;
	flash_pri_t task_pri, new_task_pri;
	flash_pid_t task_pid;
	size_t insertion_point;
	bool lookup_found, condense_queue_check;
	size_t i;

	change_grant.write(false);
	do { wait(); }
	while (!init_done.read());

	while (true) {
		do { wait(); }
		while (!change_req.read());
		change_grant.write(true);
		req_type = change_type.read();
		task_pid = change_pid.read();

		condense_queue_check = false;

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
			/* no new task */
			cout << "REAL UPDATE" << endl;

			lookup_found = lookup_process(task_pid, &task_pri, &i);

			if (lookup_found && queue[task_pri][i].active) {
				if (req_type & FLASH_CHANGE_STATE) {
					queue[task_pri][i].state = change_state.read();
				}

				if (req_type & FLASH_CHANGE_PRI) {
					new_task_pri = change_pri.read();
					insertion_point = end_queue[new_task_pri];

					/* copy entry and make old one inactive */
					queue[new_task_pri][insertion_point] = queue[task_pri][i];
					queue[task_pri][i].active    = 0;

					/* consistency */
					task_pri = new_task_pri;
					i = insertion_point;
				}

				/* check if process is now dead */
				if (queue[task_pri][i].state & EXIT_TRACE) {
					queue[task_pri][i].active = 0;
					condense_queue_check = true;
				}

				/* cleanup and condense */
				if (req_type & FLASH_CHANGE_PRI) {
					MODULAR_INCR(end_queue[task_pri], TASK_QUEUE_SIZE);
					condense_queue_check = true;
				}

				if (condense_queue_check) {
					condense_queue();
				}
			} // end find match
		} // end no new tasks

		do { wait(); }
		while (change_req.read());
		change_grant.write(false);
		wait();
	}
}

void flash::condense_queue() {
	flash_pri_t pri;
	size_t i, fill_marker;

	for (pri = 0 ; pri < FLASH_MAX_PRI; ++pri) {
		for (i = cur_task[pri], fill_marker = i, cur_task[pri] = -1;
				i != end_queue[pri];
				MODULAR_INCR(i, TASK_QUEUE_SIZE)) {
			/* condense the queue */
			if (queue[pri][i].active) {
				if (cur_task[pri] == -1) cur_task[pri] = i;
				queue[pri][fill_marker] = queue[pri][i];
				MODULAR_INCR(fill_marker, TASK_QUEUE_SIZE);
			}
		}
		end_queue[pri] = fill_marker;
	}
}

flash_task_t flash::get_next_task() {
	flash_task_t next_task, to_check;
	flash_pri_t pri, highest_priority;
	uint32_t i;

	next_task.active = 0;

	for (pri = 0; pri < FLASH_MAX_PRI; ++pri) {
		if (end_queue[pri] == cur_task[pri])
			continue;

		for (; cur_task[pri] != end_queue[pri];) {
			to_check = queue[pri][cur_task[pri]];
			MODULAR_INCR(cur_task[pri], end_queue[pri]);
			if (to_check.active) {
				next_task = to_check;
				break;
			} else {
				cout << "SKIPPING INACTIVE RECORD" << endl;
			}
		}
	}

	return next_task;
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

