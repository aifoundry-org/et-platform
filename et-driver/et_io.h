#include <linux/io.h>

/* For performace prefer atomic over non-atomic */
#ifndef ioread64
#ifdef readq
#define ioread64 readq
#endif
#endif

#ifndef iowrite64
#ifdef writeq
#define iowrite64 writeq
#endif
#endif

#include <linux/io-64-nonatomic-lo-hi.h>

// It assumes that the iomem addresses represent mapping of u64 aligned device
// addresses
void et_iowrite(void __iomem *dst, loff_t offset, u8 *src, size_t count);
void et_ioread(void __iomem *src, loff_t offset, u8 *dst, size_t count);
