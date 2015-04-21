#include "flash.h"

void flash::timer() {
	time.write(0);
	wait();

TIMER_LOOP:
	while (true) {
		uint64_t tmp;
		tmp = time.read();
		++tmp;
		time.write(tmp);

		wait();
	}
}

void flash::initialize() {
	int i;

	init_done.write(false);
	wait();

INIT_RUN_LIST:
	for (i = 0; i < RUN_QUEUE_SIZE; ++i) {
		runnable_list[i] = INDEX_POISON;
	}

INIT_PROCESS_LIST:
	for (i = 0; i < TASK_QUEUE_SIZE; ++i) {
		process_list[i].active = 0;
	}

	init_done.write(true);

INIT_INFINITE_LOOP:
	while(true) {
		wait();
	}
}

void flash::tick() {
	unsigned i = 0;
	tick_req_internal.write(false);
	do { wait(); }
	while (!init_done.read());

TICK_LOOP:
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

int flash::lookup_process(flash_pid_t pid) {
	int i;

LOOKUP_PROCESS_LOOP:
	for (i = 0; i < TASK_QUEUE_SIZE; ++i) {
		if (process_list[i].pid == pid && process_list[i].active) {
			return i;
		}
	}
	return -1; /* no match */
}

int flash::find_empty_slot() {
	int i;

FIND_EMPTY_LOOP:
	for (i = 0; i < TASK_QUEUE_SIZE; ++i) {
		if (!process_list[i].active) {
			return i;
		}
	}
	return -1; /* full */
}

int flash::add_task_to_run_queue(int process_index) {
	int i;

ADD_TASK_RL_EXIST_LOOP:
	/* check if exists */
	for (i = 0; i < RUN_QUEUE_SIZE; ++i) {
		if (runnable_list[i] == process_index) {
			return 0;
		}
	}

ADD_TASK_RL_ADD_LOOP:
	/* now try to add */
	for (i = 0; i < RUN_QUEUE_SIZE; ++i) {
		if (runnable_list[i] == INDEX_POISON) {
			runnable_list[i] = process_index;
			return 0;
		}
	}

	return -1;
}

int flash::remove_task_from_run_queue(int process_index) {
	int i;

REMOVE_FROM_RL:
	for (i = 0; i < RUN_QUEUE_SIZE; ++i) {
		if (runnable_list[i] == process_index) {
			runnable_list[i] = INDEX_POISON;
			return 0;
		}
	}
	return -1;
}

uint64_t inline flash::calculate_virtual_runtime(uint64_t time, flash_pri_t pri) {
	/*
		 Lookup table for weight factor for computing virtual runtimes of
		 processes considering their priority.

		 Computed using: 1 << 15 * 1.25^(-nice)
		 USE CASE: time * (NICE_0_LOAD / vr_weight[pri])
	 */
	const uint64_t vr_weight[40] =
	{    378,     472,     590,     737,     922,
		  1153,    1441,    1801,    2252,    2815,
		  3518,    4398,    5498,    6872,    8590,
		 10737,   13422,   16777,   20972,   26214,
		 32768,   40960,   51200,   64000,   80000,
		100000,  125000,  156250,  195313,  244141,
		305176,  381470,  476837,  596046,  745058,
		931323, 1164150, 1455190, 1818990, 2273740};

	return (time * NICE_0_LOAD) / vr_weight[pri];
}


void flash::process_change() {
	flash_change_t req_type;
	flash_pid_t task_pid;
	flash_pri_t task_pri;
	flash_state_t task_state;
	int task_index;

	flash_task_t old_task, new_task;

	change_grant.write(false);
	do { wait(); }
	while (!init_done.read());

	while (true) {
		do { wait(); }
		while (!change_req.read());
		change_grant.write(true);

		req_type = change_type.read();
		task_pid = change_pid.read();
		task_pri = change_pri.read();
		task_state = change_state.read();

		do { wait(); }
		while (change_req.read());
		change_grant.write(false);

		/* end of handshake */

		if (req_type & __FLASH_CHANGE_NEW) {
			/* insert new task */
			task_index = find_empty_slot();
			/* TODO check -1 return */
			if (task_index == -1) {
				continue;
			}

			old_task.state = -1;

			new_task.pid = task_pid;
			new_task.pri = task_pri;
			new_task.state = task_state;
			new_task.active = 1;
			new_task.start_time = 0;
			new_task.vr = 0;
			new_task.pr = 0;
			new_task.running = 0;

			process_list[task_index] = new_task;
			//cerr << "new process " << task_index << endl;;
		} else {
			/* no new tasks, no new tasks, no new tasks, no no new */
			cout << "REAL UPDATE" << endl;

			task_index = lookup_process(task_pid);
			/* TODO check -1 return */
			if (task_index == -1) {
				continue;
			}

			new_task = old_task = process_list[task_index];

			if (req_type & FLASH_CHANGE_PRI) {
				new_task.pri = task_pri;
			}

			if (req_type & FLASH_CHANGE_STATE) {
				new_task.state = task_state;
				if (task_state & EXIT_TRACE) {
					new_task.active = 0;
				}
			}

			process_list[task_index] = new_task;
		} // end no new tasks

		/* make sure run queue is consistent */
		if (old_task.state && !new_task.state && new_task.active) {
			add_task_to_run_queue(task_index);
		} else if (!old_task.state && new_task.state || !new_task.active) {
			remove_task_from_run_queue(task_index);
		}
	}
}

flash_task_t flash::get_next_task() {
	size_t i;
	int possible_next_task_index;
	uint64_t lowest_vr, delta, delta_w;
	int cur;

	/* update current task's runtime */
	cur = current_task_index.read();
	delta = time.read() - process_list[cur].start_time;
	delta_w = calculate_virtual_runtime(delta, process_list[cur].pri);
	process_list[cur].vr += delta_w;
	process_list[cur].pr += delta;


GET_NEXT_TASK_L1:
	lowest_vr = -1; /* unsigned */
	for(i = 0; i < RUN_QUEUE_SIZE; ++i) {
		flash_task_t tmp_task;
		int process_index;

		process_index = runnable_list[i];
		if (process_index == INDEX_POISON) {
			continue;
		}

		tmp_task = process_list[process_index];
		//cerr << "TMP; LOW:: " << tmp_task.vr << "; " << lowest_vr << endl;
		if (tmp_task.vr < lowest_vr) {
			//cerr << "new lower task" << endl;
			possible_next_task_index = process_index;
			lowest_vr = tmp_task.vr;
		}
	}

	if (cur != possible_next_task_index) {
		process_list[cur].running = 0;
	}

#ifdef VERBOSE
	cerr << "GNT:: next " << possible_next_task_index << "; "  << process_list[possible_next_task_index].vr << "; " << process_list[possible_next_task_index].pr << endl;
#endif

	process_list[possible_next_task_index].start_time = time.read();
	process_list[possible_next_task_index].running = 1;
	current_task_index.write(possible_next_task_index);

	return process_list[possible_next_task_index];
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

#ifdef __CTOS__
SC_MODULE_EXPORT(flash)
#endif
