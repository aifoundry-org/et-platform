#subdir-ccflags-y += -I$(src)/include

obj-m += esperanto.o

ifeq (, $(findstring PCIE_LOOPBACK_DRIVER, $(EXTRA_CFLAGS)))
esperanto-y := et_io.o \
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
	       et_pci_dev.o \
	       et-soc1-pcie.o
else
esperanto-y := et_io_loopback.o \
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
	       et_pci_dev.o \
	       et-soc1-pcie_loopback.o
endif
