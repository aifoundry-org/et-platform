RV_CXX = $(TOOLCHAIN)/bin/riscv64-unknown-elf-g++
RV_LD = $(TOOLCHAIN)/bin/riscv64-unknown-elf-ld
RV_OBJCOPY = $(TOOLCHAIN)/bin/riscv64-unknown-elf-objcopy
RV_FLAGS = -O3 -std=c++11 -nostdlib -fPIC -fno-rtti -fno-exceptions -fvisibility=hidden -fno-builtin -Wl,-Ttext-segment,0x8100000000 -mcmodel=medany -I ../include -I ${INC_DIR}
ETT_INCLUDE_HEADERS = $(wildcard ../include/*.h) # dependencies from kernel_args.h
OUTDIR ?= .


.PHONY: all
all : $(OUTDIR)/etlibdevice.o $(OUTDIR)/net_test $(OUTDIR)/bootrom


$(OUTDIR)/etlibdevice.o : Makefile sys_inc.h etlibdevice.h etlibdevice.cc
	$(RV_CXX) $(RV_FLAGS) -c etlibdevice.cc -o $@


$(OUTDIR)/net_test : Makefile etblas_kernels.cc thread_init.cc crt.S kernel_args.h $(ETT_INCLUDE_HEADERS) sys_inc.h etlibdevice.h $(OUTDIR)/etlibdevice.o
	$(RV_CXX) $(RV_FLAGS) etblas_kernels.cc thread_init.cc crt.S $(OUTDIR)/etlibdevice.o -z noexecstack -o $@ #-Wl,--export-dynamic


$(OUTDIR)/bootrom.o : Makefile bootrom.S
	$(RV_CXX) -c bootrom.S -o $(OUTDIR)/bootrom.o

$(OUTDIR)/bootrom.elf : Makefile $(OUTDIR)/bootrom.o
	$(RV_LD) -Ttext 0x8000001000 -o $(OUTDIR)/bootrom.elf $(OUTDIR)/bootrom.o

$(OUTDIR)/bootrom : Makefile $(OUTDIR)/bootrom.elf
	$(RV_OBJCOPY) -O binary $(OUTDIR)/bootrom.elf $(OUTDIR)/bootrom
