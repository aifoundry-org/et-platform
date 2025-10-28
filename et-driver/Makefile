MODULE_DIR := $(CURDIR)
KDIR := /lib/modules/$(shell uname -r)/build
KMAKE := $(MAKE) -C $(KDIR) M=$(MODULE_DIR)
ET_MODULE_VERSION := $(or $(shell cat VERSION 2>/dev/null), $(shell cat $(src)/VERSION 2>/dev/null), unknown)
CFLAGS_MODULE+=-DET_MODULE_VERSION=\"$(ET_MODULE_VERSION)\"
#subdir-ccflags-y += -I$(src)/include

obj-m += et-soc1.o

ifeq (, $(findstring PCIE_LOOPBACK_DRIVER, $(EXTRA_CFLAGS)))
et-soc1-y := et_io.o \
	       et_sysfs_mgmt_vq_stats.o \
	       et_sysfs_ops_vq_stats.o \
	       et_sysfs_mem_stats.o \
	       et_sysfs_err_stats.o \
	       et_sysfs_soc_reset.o \
	       et_sysfs.o \
	       et_fw_update.o \
	       et_circbuffer.o \
	       et_event_handler.o \
	       et_vqueue.o \
	       et_dma.o \
	       et_p2pdma.o \
	       et_pci_dev.o \
	       et-soc1-pcie.o
else
et-soc1-y := et_io_loopback.o \
	       et_sysfs_mgmt_vq_stats.o \
	       et_sysfs_ops_vq_stats.o \
	       et_sysfs_mem_stats.o \
	       et_sysfs_err_stats.o \
	       et_sysfs_soc_reset.o \
	       et_sysfs.o \
	       et_fw_update.o \
	       et_circbuffer_loopback.o \
	       et_event_handler.o \
	       et_vqueue_loopback.o \
	       et_dma.o \
	       et_p2pdma_loopback.o \
	       et_pci_dev.o \
	       et-soc1-pcie_loopback.o
endif

all: modules

modules:
	+$(KMAKE) modules

modules_install:
	+$(KMAKE) modules_install

clean:
	+$(KMAKE) clean

help:
	+$(KMAKE) help

show-version:
	@echo "Current version: $(ET_MODULE_VERSION)"

dkms:
	sudo dkms add .
	sudo dkms install --force et-soc1/$(ET_MODULE_VERSION)
	sudo modprobe et-soc1

dkms-remove:
	-sudo modprobe -r et-soc1
	sudo dkms remove et-soc1/$(ET_MODULE_VERSION) --all

akms:
	doas akms install .
	doas modprobe et-soc1 

akms-remove:
	doas modprobe -r et-soc1 
	doas akms remove et-soc1 
