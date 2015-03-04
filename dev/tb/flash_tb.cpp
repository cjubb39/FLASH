#include "flash_tb.h"

void flash_tb::source() {
	flash_rst.write(false);
	wait();
	flash_rst.write(true);
	wait(); wait();

	/* send new process */
	unsigned i;
	flash_task_t task;
	for (i = 0; i < TASKS_TO_SEND; ++i) {
		task.pid = i + 1;
		task.pri = i + 1;
		task.state = 2 * (i + 1);

		change_req.write(true);
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

	while (true) {
		wait();
	}
}

/* readback */
void flash_tb::sink() {
	wait();

	unsigned i;
	flash_pid_t pid;
	for (i = 0; i < TASKS_TO_READ; ++i) {
		do { wait(); }
		while (!tick_req.read());
		tick_grant.write(true);
	  pid = next_process.read();	
		do { wait(); }
		while (tick_req.read());
		tick_grant.write(false);

		cout << "asked to run: " << pid << endl;
		wait();
	}

	sc_stop();
}

