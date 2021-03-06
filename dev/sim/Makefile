TARGET_ARCH := linux64
WAVE_VIEWER := simvision
CC := g++

LIBS   := -lsystemc
LIBDIR = -L. -L$(SYSTEMC)/lib-$(TARGET_ARCH)

CTOS_ROOT := $(CTOS_PATH)
CTOS_INCLUDES := -I$(CTOS_ROOT)/share/ctos/include \
	-I$(CTOS_ROOT)/share/ctos/include/ctos_fx \
	-I$(CTOS_ROOT)/share/ctos/include/ctos_tlm \
	-I$(CTOS_ROOT)/share/ctos/include/ctos_flex_channels
SLD_MATH_DIR=/opt/zynq-math/src/

INCDIR = -I../src -I../tb -I$(SYSTEMC)/include -I$(TLM)/include/tlm \
				 -I$(SLD_MATH_DIR) $(CTOS_INCLUDES)

CFLAGS ?= -g -DCTOS_FP
include Makefile.params # after CFLAGS defined

PARAMS_FILE=.params

TARGET := flash 
VPATH := ../src:../tb
CPP_SRCS := flash.cpp flash_tb.cpp sc_main.cpp
HDRS := flash.h flash_tb.h flash_sched.h
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

$(OBJS): $(HDRS) $(PARAMS_FILE)

.PHONY: force
$(PARAMS_FILE): force
	@echo '$(CFLAGS)' | cmp -s $@ || echo '$(CFLAGS)' > $@


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
