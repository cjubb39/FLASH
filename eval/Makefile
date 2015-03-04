CC=gcc
CFLAGS=-g -Wall

exec=context_switch_cycles

$(exec):

.PHONY: clean
clean:
	rm -rf *.o *.gch *~

.PHONY: all
all: clean $(exec)

.PHONY: run
run: $(exec)
	perf stat ./$(exec)
