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
	struct miscdevice misc_ops_dev;
	bool is_ops_open;
	spinlock_t ops_open_lock;	/* serializes access to is_ops_open */
	void __iomem *dir;
	struct et_mapped_region regions[OPS_MEM_REGION_TYPE_NUM];
	struct et_ops_dir_vqueue dir_vq;
	struct et_squeue **hpsq_pptr;
	struct et_squeue **sq_pptr;
	struct et_cqueue **cq_pptr;
	struct et_vq_common vq_common;
	struct rb_root dma_rbtree;
	struct mutex dma_rbtree_mutex;	/* serializes access to dma_rbtree */
};

struct et_mgmt_dev {
	struct miscdevice misc_mgmt_dev;
	bool is_mgmt_open;
	spinlock_t mgmt_open_lock;	/* serializes access to is_mgmt_open */
	void __iomem *dir;
	struct et_mapped_region regions[MGMT_MEM_REGION_TYPE_NUM];
	struct et_mgmt_dir_vqueue dir_vq;
	struct et_squeue **sq_pptr;
	struct et_cqueue **cq_pptr;
	struct et_vq_common vq_common;
};

struct et_pci_dev {
	u8 dev_index;
	bool is_recovery_mode;
	struct pci_dev *pdev;
	struct dev_config cfg;
	struct et_ops_dev ops;
	struct et_mgmt_dev mgmt;
	struct list_head bar_region_list;
};

// clang-format on

int et_map_bar(struct et_pci_dev *et_dev,
	       const struct et_bar_mapping *bm_info,
	       void __iomem **mapped_addr_ptr);
void et_unmap_bar(void __iomem *mapped_addr);

#endif
