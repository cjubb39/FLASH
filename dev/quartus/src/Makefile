MODULES= flash_rtl.v \
				 flash_wrapper.sv

MFE_OUTPUT= flash_tb.exe

.PHONY: all
all: mfebench

help:
	@echo "The following make targets are supported:" ;\
	echo " mfebench  - builds the testbench";\
	echo " wave   - convert vcd to vpd";\
	echo " clean  - cleans testbench and intermediate files" ;\
	echo " help   - show this information";\

mfebench: 
	vcs -full64 -PP -sverilog +define+SV +define+VPD -notice $(MODULES) -o $(MFE_OUTPUT) 

wave:
	vcs -vpd2vcd vcdplus.vpd waveform.vcd
	gtkwave waveform.vcd &

clean:
	rm -rf *.o *~ *.vpd sim* csrc DVEfiles *daidir *exe *.key *.exe.vdb
