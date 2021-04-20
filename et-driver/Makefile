#subdir-ccflags-y += -I$(src)/include

obj-m += esperanto.o

esperanto-y := et_io.o et_mmio.o et_circbuffer.o et_event_handler.o et_vqueue.o et_dma.o et_pci_dev.o et-soc1-pcie.o

ifneq (, $(findstring ENABLE_DRIVER_TESTS, $(EXTRA_CFLAGS)))
esperanto-y += et_test.o
endif
