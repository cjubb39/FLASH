CC=gcc
CFLAGS?=
CFLAGS+=-g -Wall $(E)

exec=context_switch_cycles

$(exec):

.PHONY: clean
clean:
	rm -rf *.o *.gch *~ $(exec)

.PHONY: all
all: clean $(exec)

.PHONY: run
run: $(exec)
	perf stat ./$(exec)
