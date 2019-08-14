#include "bl2_minion_pll_and_dll.h"
#include "minion_esr_defines.h"

typedef enum
{
    PP_USER       = 0,
    PP_SUPERVISOR = 1,
    PP_MESSAGES   = 2,
    PP_MACHINE    = 3
} esr_protection_t;

typedef enum
{
    REGION_MINION        = 0,    // HART ESR
    REGION_NEIGHBOURHOOD = 1,    // Neighbor ESR
    REGION_TBOX          = 2,    // 
    REGION_OTHER         = 3     // Shire Cache ESR and Shire Other ESR
} esr_region_t;

const uint64_t ESR_MEMORY_REGION = 0x0100000000UL;     // [32]=1

static inline volatile uint64_t* __attribute__((always_inline)) esr_address(esr_protection_t pp, uint8_t shire_id, esr_region_t region, uint32_t address)
{
    volatile uint64_t *p = (uint64_t *) (  ESR_MEMORY_REGION
                         | ((uint64_t)(pp       & 0x03    ) << 30)
                         | ((uint64_t)(shire_id & 0xff    ) << 22)
                         | ((uint64_t)(region   & 0x03    ) << 20)
                         | ((uint64_t)(address  & 0x01ffff) <<  3));
    return p;
}

static inline uint64_t __attribute__((always_inline)) read_esr(esr_protection_t pp, uint8_t shire_id, esr_region_t region, uint32_t address)
{
    volatile uint64_t *p = esr_address(pp, shire_id, region, address);
    return *p;
}

static inline void __attribute__((always_inline)) write_esr(esr_protection_t pp, uint8_t shire_id, esr_region_t region, uint32_t address, uint64_t value)
{
    volatile uint64_t *p = esr_address(pp, shire_id, region, address);
    *p = value;
}

void pll_config(uint32_t shire_id );
//void minion_pll_config ( uint64_t minion_shire_mask);

/*==================== Function Separator =============================*/
void pll_config(uint32_t shire_32id )
{

   uint8_t shire_id = (uint8_t) shire_32id;
   // Select 1 GHz
   write_esr(PP_MACHINE, shire_id, REGION_OTHER, SHIRE_OTHER_CTRL_CLOCKMUX, 0xb);

   // PLL and DLL configuration
   // Assume invalid values initially, then "write" values into PLL config registers
   write_esr(PP_MACHINE, shire_id, REGION_OTHER, SHIRE_OTHER_PLL_AUTO_CONFIG, 0x0);
   write_esr(PP_MACHINE, shire_id, REGION_OTHER, SHIRE_OTHER_DLL_AUTO_CONFIG, 0x0);
   write_esr(PP_MACHINE, shire_id, REGION_OTHER, SHIRE_OTHER_PLL_AUTO_CONFIG, REG_SHIRE_PLL_AUTO_CONFIG_SHADOW1);
   write_esr(PP_MACHINE, shire_id, REGION_OTHER, SHIRE_OTHER_DLL_AUTO_CONFIG, REG_SHIRE_DLL_AUTO_CONFIG_SHADOW1);
   // check shire_pll_wrapper.v for details on values for PLL registers
   write_esr(PP_MACHINE, shire_id, REGION_OTHER, SHIRE_OTHER_PLL_CONFIG_DATA_0, 0x0000000a000101e8);
   write_esr(PP_MACHINE, shire_id, REGION_OTHER, SHIRE_OTHER_PLL_CONFIG_DATA_1, 0x0003027e002e0003);
   write_esr(PP_MACHINE, shire_id, REGION_OTHER, SHIRE_OTHER_PLL_CONFIG_DATA_2, 0x0100000a02be0033);
   write_esr(PP_MACHINE, shire_id, REGION_OTHER, SHIRE_OTHER_PLL_CONFIG_DATA_3, 0x00010001000000d5);
   write_esr(PP_MACHINE, shire_id, REGION_OTHER, SHIRE_OTHER_PLL_CONFIG_DATA_4, 0x0000000000010001);
   write_esr(PP_MACHINE, shire_id, REGION_OTHER, SHIRE_OTHER_PLL_CONFIG_DATA_5, 0x0000000100000000);
   // check shire_pll_wrapper.v for details on values for DLL registers
   write_esr(PP_MACHINE, shire_id, REGION_OTHER, SHIRE_OTHER_DLL_CONFIG_DATA_0, 0x000a002000080001);
   // Enable state machines for PLL
   write_esr(PP_MACHINE, shire_id, REGION_OTHER, SHIRE_OTHER_PLL_AUTO_CONFIG, REG_SHIRE_PLL_AUTO_CONFIG_SHADOW2);
   // Reset pll_read reg
   write_esr(PP_MACHINE, shire_id, REGION_OTHER, SHIRE_OTHER_PLL_AUTO_CONFIG, REG_SHIRE_PLL_AUTO_CONFIG_SHADOW5);
   // Trigger configuration of PLL
   write_esr(PP_MACHINE, shire_id, REGION_OTHER, SHIRE_OTHER_PLL_AUTO_CONFIG, REG_SHIRE_PLL_AUTO_CONFIG_SHADOW3);

   // Wait until PLL configuration is finished
   while((read_esr(PP_MACHINE, shire_id, REGION_OTHER, SHIRE_PLL_READ_DATA) & 0x10000));
   // Stop configuration state machine for PLL
   write_esr(PP_MACHINE, shire_id, REGION_OTHER, SHIRE_OTHER_PLL_AUTO_CONFIG, REG_SHIRE_PLL_AUTO_CONFIG_SHADOW4);
   // Wait until PLL is locked to change clock mux
   while(!(read_esr(PP_MACHINE, shire_id, REGION_OTHER, SHIRE_PLL_READ_DATA) & 0x20000));
   // force reg_shire_ctrl_clockmux = 4'b0100;
   write_esr(PP_MACHINE, shire_id, REGION_OTHER, SHIRE_OTHER_CTRL_CLOCKMUX, 0x4);

   // Enable state machines for DLL
   write_esr(PP_MACHINE, shire_id, REGION_OTHER, SHIRE_OTHER_DLL_AUTO_CONFIG, REG_SHIRE_DLL_AUTO_CONFIG_SHADOW2);
   // Reset dll_read reg
   write_esr(PP_MACHINE, shire_id, REGION_OTHER, SHIRE_OTHER_DLL_AUTO_CONFIG, REG_SHIRE_DLL_AUTO_CONFIG_SHADOW5);
   // Trigger configuration of DLL
   write_esr(PP_MACHINE, shire_id, REGION_OTHER, SHIRE_OTHER_DLL_AUTO_CONFIG, REG_SHIRE_DLL_AUTO_CONFIG_SHADOW3);
   // Wait until DLL configuration is finished
   //while(!(read_esr(PP_MACHINE, shire_id, REGION_OTHER, SHIRE_DLL_READ_DATA) & 0x20000));
   //while(read_esr(PP_MACHINE, shire_id, REGION_OTHER, SHIRE_DLL_READ_DATA) & 0x11);
   // Stop configuration state machine for DLL

   write_esr(PP_MACHINE, shire_id, REGION_OTHER, SHIRE_OTHER_DLL_AUTO_CONFIG, REG_SHIRE_DLL_AUTO_CONFIG_SHADOW4);
   // force reg_shire_ctrl_clockmux = 4'b1100;
   write_esr(PP_MACHINE, shire_id, REGION_OTHER, SHIRE_OTHER_CTRL_CLOCKMUX, 0xc);

}

//void minion_pll_config ( uint64_t minion_shire_mask){
static void minion_pll_config(uint64_t mask) {
    uint32_t n;
    for (n = 0; n <= 33; n++) {
        if (mask & 1u) {
            pll_config(n);
        }
     mask = mask >> 1u;
   }
}

int configure_minion_plls_and_dlls(void) {
    minion_pll_config(1);
    return 0;
}
