#include "flash_tb.h"

void flash_tb::source() {
	flash_rst.write(false);
	wait();
	flash_rst.write(true);

	/* send new process */


	while (true) {
		wait();
	}
}

void flash_tb::sink() {
	wait();
	while(!operational.read()) {
		wait();
	}

	cout << "operational!" << endl;
	sc_stop();
}

