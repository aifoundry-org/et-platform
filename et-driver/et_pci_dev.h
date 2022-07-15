#ifndef __ET_PCI_DEV_H
#define __ET_PCI_DEV_H

#include <linux/kernel.h>
#include <linux/list.h>
#include <linux/miscdevice.h>
#include <linux/mutex.h>
#include <linux/pci.h>
#include <linux/spinlock.h>
#include <linux/workqueue.h>

#include "et_ioctl.h"
#include "et_sysfs_stats.h"
#include "et_vqueue.h"

// clang-format off
struct et_bar_region {
	struct list_head list;
	bool is_mgmt;
	u8 bar;
	u8 region_type;
	u64 region_start;
	u64 region_end;
};

enum et_iomem_r {
	IOMEM_R_DIR_OPS = 0,
	IOMEM_R_DIR_MGMT,
	IOMEM_REGIONS
};

struct et_bar_mapping {
	u32 bar;
	u64 bar_offset;
	u64 size;
};

extern const struct et_bar_mapping DIR_MAPPINGS[];

struct et_mapped_region {
	bool is_valid;			/*
					 * compulsory regions must be valid,
					 * non-compulsory regions may or may
					 * not be valid
					 */
	struct et_dir_reg_access access;
	void __iomem *mapped_baseaddr;
	u64 soc_addr;
	u64 size;
};

enum et_msi_vec_idx {
	ET_MGMT_SQ_VEC_IDX = 0,
	ET_MGMT_CQ_VEC_IDX,
	ET_OPS_SQ_VEC_IDX,
	ET_OPS_CQ_VEC_IDX,
	ET_MAX_MSI_VECS
};

struct et_ops_dev {
        atomic_t state;                 /*
                                         * state is readable without holding
                                         * a mutex hence atomic.
                                         */
        struct mutex state_chng_mutex;  /*
                                         * Mutex should be held when making a
                                         * state transition, to capture the
                                         * critical section related to changes
                                         * in state.
                                         */
	struct miscdevice misc_dev;
	bool miscdev_created;
	bool is_open;
	spinlock_t open_lock;		/* serializes access to is_open */
	void __iomem *dir;
	struct et_mapped_region regions[OPS_MEM_REGION_TYPE_NUM];
	struct et_ops_dir_vqueue dir_vq;
	struct et_vq_data vq_data;
	struct et_mem_stats mem_stats;
};

struct et_mgmt_dev {
	atomic_t state;			/*
					 * state is readable without holding
					 * a mutex hence atomic.
					 */
	struct mutex state_chng_mutex;	/*
					 * Mutex should be held when making a
					 * state transition, to capture the
					 * critical section related to changes
					 * in state.
					 */
	struct miscdevice misc_dev;
	bool miscdev_created;
	bool is_open;
	spinlock_t open_lock;		/* serializes access to is_open */
	void __iomem *dir;
	struct et_mapped_region regions[MGMT_MEM_REGION_TYPE_NUM];
	struct et_mgmt_dir_vqueue dir_vq;
	struct et_vq_data vq_data;
	struct et_err_stats err_stats;
};

struct et_pci_dev {
	u8 dev_index;
	bool is_initialized;
	bool is_err_reporting;
	struct pci_dev *pdev;
	struct dev_config cfg;
	struct et_ops_dev ops;
	struct et_mgmt_dev mgmt;
	struct list_head bar_region_list;
	u32 bar_cfgs[6];
	struct workqueue_struct *reset_workqueue;
	struct work_struct isr_work;
};

// clang-format on

void et_save_bars(struct et_pci_dev *et_dev);
void et_restore_bars(struct et_pci_dev *et_dev);
int et_map_bar(struct et_pci_dev *et_dev,
	       const struct et_bar_mapping *bm_info,
	       void __iomem **mapped_addr_ptr);
void et_unmap_bar(void __iomem *mapped_addr);

int et_mgmt_dev_init(struct et_pci_dev *et_dev,
		     u32 timeout_secs,
		     bool miscdev_create);
void et_mgmt_dev_destroy(struct et_pci_dev *et_dev, bool miscdev_destroy);
int et_ops_dev_init(struct et_pci_dev *et_dev,
		    u32 timeout_secs,
		    bool miscdev_create);
void et_ops_dev_destroy(struct et_pci_dev *et_dev, bool miscdev_destroy);

#endif
