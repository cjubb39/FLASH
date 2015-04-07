CARGO ?= /usr/local/share/cargo
QSIM ?= ${CARGO}/qsim
KSRC ?= $(QSIM)/linux
CFLAGS := -O3
CFLAGS += -m32 -static
CFLAGS += -I../.. -I../../dev/src -I../
LIBS = -lm -lrt

all: tarball

check:
ifeq ($(KSRC),)
	$(error 'Path to kernel in env variable KSRC not found. Exiting')
endif
.PHONY: check

flash-mmap: flash-mmap.c
	$(CC) $(CFLAGS) -o $@ $^ $(LIBS)

tarball: flash.tar
.PHONY: tarball

flash.tar: flash-mmap flash.ko runme.sh
	tar cf $@ $^

driver: flash.ko

.PHONY: driver

flash.ko: check
	make -C $(KSRC) M=`pwd`

clean help: check
	$(MAKE) -C $(KSRC) M=`pwd` $@
distclean: clean
	$(RM) flash.tar flash-mmap

.PHONY: all clean help
