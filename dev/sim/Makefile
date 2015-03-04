TARGET_ARCH := linux64
WAVE_VIEWER := simvision
CC := g++

LIBS   := -lsystemc
LIBDIR = -L. -L$(SYSTEMC)/lib-$(TARGET_ARCH)

INCDIR = -I../src -I../tb -I$(SYSTEMC)/include -I$(TLM)/include/tlm

CFLAGS := #-Wall

TARGET := flash 
VPATH := ../src:../tb
CPP_SRCS := flash.cpp flash_tb.cpp sc_main.cpp
HDRS := flash.h flash_tb.h
OBJS = $(CPP_SRCS:.cpp=.o)
OBJS += $(C_SRCS:.c=.o)

.SUFFIXES: .c .cpp .cc .o
$(TARGET): $(OBJS)
	$(CC) ${INCDIR} ${LIBDIR} -o $@ $(OBJS) ${LIBS}

.PHONY: clean run wave test

.cpp.o:  $< .h
	$(CC) $(CFLAGS) ${INCDIR} -c $<

.c.o:
	$(CC) $(CFLAGS) ${INCDIR} -c $<

$(OBJS): $(HDRS)

clean:
	rm -f *.o $(TARGET) *.dsn *.trn *.vcd
	rm -f ../output/*
	if [ -d .simvision ]; then rm -r .simvision; fi

run: $(TARGET)
	./$(TARGET)

test: run
	@ ./test.sh

wave:
	$(WAVE_VIEWER) $(TARGET).vcd