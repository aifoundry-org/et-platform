#include "pcie_init.h"

#define jump_bl2                                                        \
{                                                                       \
          __asm__ (" .option push \n");                                 \
          __asm__ (" .option norelax \n");                              \
          __asm__ (" # BL2's entry point is located @ 0x40400000 \n");  \
          __asm__ (" la ra, 0x40400000 \n");                            \
          __asm__ (" .option pop \n");                                  \
          __asm__ (" # Jump to BL2 \n");                                \
          __asm__ (" jr ra\n");                                         \
}

void fast_rom(void);

void  fast_rom(void) 
{
    PCIe_release_pshire_from_reset();
    //In Fast boot mode - we just have to make sure Pshire PLL bypass it switch from Ref Clock to PLL    
    iowrite32(R_PCIE_ESR_BASEADDR + PSHIRE_PSHIRE_CTRL_ADDRESS,
                  PSHIRE_PSHIRE_CTRL_PLL0_BYP_SET(0));
    PCIe_init(false /*expect_link_up*/);

    jump_bl2;
}
