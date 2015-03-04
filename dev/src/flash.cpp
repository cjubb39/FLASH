#include "flash.h"

void flash::schedule() {
	operational.write(false);	
	wait();

	operational.write(true);
	while(true) {
		wait();
	}
}

