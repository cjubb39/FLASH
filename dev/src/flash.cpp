#include "flash.h"

void flash::tick() {
	unsigned i;
	wait();

	while(true) {
		for (i = 0 ; i < WAIT_PER_TICK; ++i) {
			wait();
		}

		next_process.write(queue[i].pid);
		tick_req.write(true);
		do { wait(); }
		while (!tick_grant.read());
		tick_req.write(false);
		next_process.write(PID_POISON);
	}
}

void flash::schedule() {
	operational.write(false);	
	wait();

	operational.write(true);
	while(true) {
		wait();
	}
}

