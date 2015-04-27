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
		if (process_list[i].pid == pid) {
			if(process_list[i].active)
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

	SC_FIX_t num, den, tret;
	uint64_t ret;
	num = time * NICE_0_LOAD;
	den = vr_weight[pri];

#ifdef CTOS_FP
	tret = sld::udiv_func<64,64,64,64,64,64>(num, den);
	ret = tret.range().to_uint64();
#else
	ret = num / den;
#endif
	return ret;
}


void flash::process_change() {
	flash_change_t req_type;
	flash_pid_t task_pid;
	flash_pri_t task_pri;
	flash_state_t task_state;
	int task_index;
	int i;

	flash_task_t old_task, new_task;

INIT_PROCESS_LIST:
	for (i = 0; i < TASK_QUEUE_SIZE; ++i) {
		process_list[i].active = 0;
	}

	change_grant.write(false);
	/* end reset beh */
	wait();

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
			//new_task.start_time = 0;
			new_task.vr = 0;
			new_task.pr = 0;
			new_task.running = 0;

			process_list[task_index].pid = new_task.pid;
			process_list[task_index].pri = new_task.pri;
			process_list[task_index].state = new_task.state;
			process_list[task_index].active = new_task.active;
			process_list[task_index].vr = new_task.vr;
			process_list[task_index].pr = new_task.pr;
			process_list[task_index].running = new_task.running;
			//cerr << "new process " << task_index << endl;;
		} else {
			/* no new tasks, no new tasks, no new tasks, no no new */
			cout << "REAL UPDATE" << endl;

			task_index = lookup_process(task_pid);
			/* TODO check -1 return */
			if (task_index == -1) {
				continue;
			}

			/* don't load whole struct for CtoS reasons. We get extra reads/writes */
			new_task.state = old_task.state = process_list[task_index].state;
			//new_task.pri = old_task.pri = process_list[task_index].pri;
			new_task.active = old_task.active = process_list[task_index].active;
			wait();

			if (req_type & FLASH_CHANGE_PRI) {
				new_task.pri = task_pri;
				process_list[task_index].pri = new_task.pri;
			}

			if (req_type & FLASH_CHANGE_STATE) {
				new_task.state = task_state;
				process_list[task_index].state = new_task.state;
				if (task_state & EXIT_TRACE) {
					new_task.active = 0;
					process_list[task_index].active = new_task.active;
				}
			}

		} // end no new tasks

		wait();

		/* make sure run queue is consistent */
		if (old_task.state && !new_task.state && new_task.active) {
			add_task_to_run_queue(task_index);
		} else if (!old_task.state && new_task.state || !new_task.active) {
			remove_task_from_run_queue(task_index);
		}
	}
}

flash_pid_t /*flash_task_t*/ flash::get_next_task() {
	size_t i;
	int possible_next_task_index;
	uint64_t lowest_vr, delta, delta_w, cur_time, start_time;
	uint64_t vr_tmp, pr_tmp;
	int cur;
	flash_pri_t proc_pri;

	/* update current task's runtime */
	cur = current_task_index.read();
	wait();
	cur_time = time.read();
	start_time = process_list[cur].start_time;
	proc_pri = process_list[cur].pri;

	wait();
	delta = cur_time - start_time;
	delta_w = calculate_virtual_runtime(delta, proc_pri);

	wait();
	vr_tmp = process_list[cur].vr;
	vr_tmp += delta_w;
	pr_tmp = process_list[cur].pr;
	pr_tmp += delta;
	wait();
	process_list[cur].vr = vr_tmp;
	process_list[cur].pr = pr_tmp;


	lowest_vr = -1; /* unsigned */
GET_NEXT_TASK_L1:
	for(i = 0; i < RUN_QUEUE_SIZE; ++i) {
		flash_task_t tmp_task;
		int process_index;

		process_index = runnable_list[i];
		wait();
		if (process_index != INDEX_POISON) {

			tmp_task = process_list[process_index];
			//cerr << "TMP; LOW:: " << tmp_task.vr << "; " << lowest_vr << endl;
			if (tmp_task.vr < lowest_vr) {
				//cerr << "new lower task" << endl;
				possible_next_task_index = process_index;
				lowest_vr = tmp_task.vr;
			}
			wait();
		}
	}

	if (cur != possible_next_task_index) {
		process_list[cur].running = 0;
	}

#ifdef VERBOSE
	cerr << "GNT:: next " << possible_next_task_index << "; "  << process_list[possible_next_task_index].vr << "; " << process_list[possible_next_task_index].pr << endl;
#endif

	uint64_t new_start_time = time.read();
	wait();

	process_list[possible_next_task_index].start_time = new_start_time;//time.read();
	process_list[possible_next_task_index].running = 1;
	current_task_index.write(possible_next_task_index);

	return process_list[possible_next_task_index].pid;
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
		next_proc_pid = get_next_task();//.pid;
		wait();

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
