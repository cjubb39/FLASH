#include "flash_tb.h"

/* load processes */
void flash_tb::load() {
	dut_init_done.write(false);
	wait();
	flash_rst.write(true);
	wait(); wait();

	/* send new process */
	unsigned i;
	flash_task_t task;
#undef TASKS_TO_SEND
#define TASKS_TO_SEND 32
	for (i = 0; i < TASKS_TO_SEND; ++i) {
		task.pid = i + 1;
		task.pri = (i + 1) % (FLASH_MAX_PRI - 1) + 1;

		switch(i%4) {
			case 0:
				task.state = 0;
				break;
			case 1:
				task.state = 1;
				break;
			case 2:
				task.state = 2;
				break;
			case 3:
				task.state = 4;
				break;
		}
		task.state = 0;

		change_req.write(true);
		change_type.write(FLASH_CHANGE_NEW);
		change_pid.write(task.pid);
		change_pri.write(task.pri);
		change_state.write(task.state);
		do { wait(); }
		while (!change_grant.read());
		change_req.write(false);
		do { wait(); }
		while (change_grant.read());
	}

	cout << "done sending proc" << endl;

	dut_init_done.write(true);

	for(i = 0; i < 10000; ++i) {
		wait();
	}
	change_req.write(true);
#if 1
	change_type.write(FLASH_CHANGE_STATE);
	change_pid.write(7);
	change_state.write(TASK_INTERRUPTIBLE);
#else
	change_type.write(FLASH_CHANGE_PRI);
	change_pid.write(7);
	change_pri.write(FLASH_MAX_PRI);
#endif
	do { wait(); }
	while (!change_grant.read());
	change_req.write(false);
	do { wait(); }
	while (change_grant.read());
	cout << "ACTION ON TB" << endl;

	while (true) {
		wait();
	}
}

/* sched requests */
void flash_tb::source() {
	sched_done.write(false);
	do { wait(); }
	while (!dut_init_done.read());

	unsigned i, j;
	flash_pid_t pid;

	for (i = 0; i < TASKS_TO_READ; ++i) {
		sched_req.write(true);
		do { wait(); }
		while (!sched_grant.read());
		sched_req.write(false);
		pid = next_process.read();
		do { wait(); }
		while (sched_grant.read());

		cout << "SCHED: asked to run: " << pid << endl;
		wait();
		for (j = 0; j < 1024; ++j)
			wait();
	}

	cout << sc_time_stamp() << endl;
	sc_stop();
}

/* handle tick */
void flash_tb::sink() {
	tick_done.write(false);
	wait();

	unsigned i;
	flash_pid_t pid;
	while (true) {
		do { wait(); }
		while (!tick_req.read());
		tick_grant.write(true);
	  pid = next_process.read();	
		do { wait(); }
		while (tick_req.read());
		tick_grant.write(false);

		cout << "TICK:  asked to run: " << pid << endl;
		wait();
	}
}

