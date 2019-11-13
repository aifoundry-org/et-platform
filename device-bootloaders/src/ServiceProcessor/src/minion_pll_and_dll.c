#include <stdio.h>

#include "minion_pll_and_dll.h"
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

/*==================== Function Separator =============================*/
#define TIMEOUT_PLL_CONFIG 100000
#define TIMEOUT_PLL_LOCK 100000

/*==================== Function Separator =============================*/
static void pll_config_multiple_write(uint32_t shire_id, uint32_t reg_first, uint32_t reg_num)
{
  uint64_t reg_value;

  // Enable PLL auto config
  reg_value =  SHIRE_OTHER_PLL_AUTO_CONFIG_PCLK_SEL
            | (reg_num << SHIRE_OTHER_PLL_AUTO_CONFIG_REG_NUM_OFF)
            | (reg_first << SHIRE_OTHER_PLL_AUTO_CONFIG_REG_FIRST_OFF)
            | (0x1 << SHIRE_OTHER_PLL_AUTO_CONFIG_ENABLE_OFF);
  write_esr(PP_MACHINE, (uint8_t)shire_id, REGION_OTHER, SHIRE_OTHER_PLL_AUTO_CONFIG, reg_value);
  
  // Start PLL auto config
  reg_value =  SHIRE_OTHER_PLL_AUTO_CONFIG_PCLK_SEL
            | (reg_num << SHIRE_OTHER_PLL_AUTO_CONFIG_REG_NUM_OFF)
            | (reg_first << SHIRE_OTHER_PLL_AUTO_CONFIG_REG_FIRST_OFF)
            | (0x1 << SHIRE_OTHER_PLL_AUTO_CONFIG_WRITE_OFF)
            | (0x1 << SHIRE_OTHER_PLL_AUTO_CONFIG_RUN_OFF)
            | (0x1 << SHIRE_OTHER_PLL_AUTO_CONFIG_ENABLE_OFF);
  write_esr(PP_MACHINE, (uint8_t)shire_id, REGION_OTHER, SHIRE_OTHER_PLL_AUTO_CONFIG, reg_value);
  
  // Wait for the PLL configuration to finish
  while((read_esr(PP_MACHINE, (uint8_t)shire_id, REGION_OTHER, SHIRE_PLL_READ_DATA) & 0x10000));
  
  // Stop the PLL auto config
  reg_value =  SHIRE_OTHER_PLL_AUTO_CONFIG_PCLK_SEL
            | (reg_num << SHIRE_OTHER_PLL_AUTO_CONFIG_REG_NUM_OFF)
            | (reg_first << SHIRE_OTHER_PLL_AUTO_CONFIG_REG_FIRST_OFF);
  write_esr(PP_MACHINE, (uint8_t)shire_id, REGION_OTHER, SHIRE_OTHER_PLL_AUTO_CONFIG, reg_value);
}
/*
static void dll_config_multiple_write(uint32_t shire_id, uint32_t reg_first, uint32_t reg_num)
{
  uint64_t reg_value;

  // Enable DLL auto config
  reg_value =  SHIRE_OTHER_DLL_AUTO_CONFIG_PCLK_SEL
            | (0x1 << SHIRE_OTHER_DLL_AUTO_CONFIG_DLL_EN_OFF)
            | (reg_num << SHIRE_OTHER_DLL_AUTO_CONFIG_REG_NUM_OFF)
            | (reg_first << SHIRE_OTHER_DLL_AUTO_CONFIG_REG_FIRST_OFF)
            | (0x1 << SHIRE_OTHER_DLL_AUTO_CONFIG_ENABLE_OFF);
  write_esr(PP_MACHINE, (uint8_t)shire_id, REGION_OTHER, SHIRE_OTHER_DLL_AUTO_CONFIG, reg_value);
  
  // Start DLL auto config
  reg_value =  SHIRE_OTHER_DLL_AUTO_CONFIG_PCLK_SEL
            | (0x1 << SHIRE_OTHER_DLL_AUTO_CONFIG_DLL_EN_OFF)
            | (reg_num << SHIRE_OTHER_DLL_AUTO_CONFIG_REG_NUM_OFF)
            | (reg_first << SHIRE_OTHER_DLL_AUTO_CONFIG_REG_FIRST_OFF)
            | (0x1 << SHIRE_OTHER_DLL_AUTO_CONFIG_WRITE_OFF)
            | (0x1 << SHIRE_OTHER_DLL_AUTO_CONFIG_RUN_OFF)
            | (0x1 << SHIRE_OTHER_DLL_AUTO_CONFIG_ENABLE_OFF);
  write_esr(PP_MACHINE, (uint8_t)shire_id, REGION_OTHER, SHIRE_OTHER_DLL_AUTO_CONFIG, reg_value);
  
  // Wait for the DLL configuration to finish
  while((read_esr(PP_MACHINE, (uint8_t)shire_id, REGION_OTHER, SHIRE_DLL_READ_DATA) & 0x10000));
  
  // Stop the DLL auto config
  reg_value =  SHIRE_OTHER_DLL_AUTO_CONFIG_PCLK_SEL
            | (0x1 << SHIRE_OTHER_DLL_AUTO_CONFIG_DLL_EN_OFF)
            | (reg_num << SHIRE_OTHER_DLL_AUTO_CONFIG_REG_NUM_OFF)
            | (reg_first << SHIRE_OTHER_DLL_AUTO_CONFIG_REG_FIRST_OFF);
  write_esr(PP_MACHINE, (uint8_t)shire_id, REGION_OTHER, SHIRE_OTHER_DLL_AUTO_CONFIG, reg_value);
}
*/
static void pll_config(uint32_t shire_id )
{
   uint64_t reg_value;
   
   // Select 1 GHz from step_clock, Bits[2:0] = 3'b011. Bit 3 to '1' to go with DLL output
   write_esr(PP_MACHINE, (uint8_t)shire_id, REGION_OTHER, SHIRE_OTHER_CTRL_CLOCKMUX, 0xb);
   
   // PLL and DLL initialization
   /////////////////////////////////////////////////////////////////////////////
   // Initialize auto-config register and reset PLL
   reg_value =  SHIRE_OTHER_PLL_AUTO_CONFIG_PCLK_SEL
             | (0x1 << SHIRE_OTHER_PLL_AUTO_CONFIG_RESET_OFF);
   write_esr(PP_MACHINE, (uint8_t)shire_id, REGION_OTHER, SHIRE_OTHER_PLL_AUTO_CONFIG, reg_value);
   
   // Initialize auto-config register and reset DLL
   reg_value =  SHIRE_OTHER_DLL_AUTO_CONFIG_PCLK_SEL
             | (0x1 << SHIRE_OTHER_DLL_AUTO_CONFIG_RESET_OFF);
   write_esr(PP_MACHINE, (uint8_t)shire_id, REGION_OTHER, SHIRE_OTHER_DLL_AUTO_CONFIG, reg_value);
   
   // PLL configuration (register values taken from Movellus testbench for LVDPLL v1.2.1a)
   /////////////////////////////////////////////////////////////////////////////
   
   // Reset PLL strobe register 0x38
   write_esr(PP_MACHINE, (uint8_t)shire_id, REGION_OTHER, SHIRE_OTHER_PLL_CONFIG_DATA_0, 0x0000000000000000);
   pll_config_multiple_write(shire_id,0x38,0x0);

   // Write PLL registers 0x00 to 0x0f. PLL registers can be downloaded in chunks of 16 registers
 //write_esr(PP_MACHINE, shire_id, REGION_OTHER, SHIRE_OTHER_PLL_CONFIG_DATA_0, 0x00000014000101b8); // 1GHz
 //write_esr(PP_MACHINE, shire_id, REGION_OTHER, SHIRE_OTHER_PLL_CONFIG_DATA_1, 0x02bb1bf40aeb02bb);
 //write_esr(PP_MACHINE, shire_id, REGION_OTHER, SHIRE_OTHER_PLL_CONFIG_DATA_2, 0x003d01f01bf40aeb);
 //write_esr(PP_MACHINE, shire_id, REGION_OTHER, SHIRE_OTHER_PLL_CONFIG_DATA_3, 0x000200020000000c);
   write_esr(PP_MACHINE, (uint8_t)shire_id, REGION_OTHER, SHIRE_OTHER_PLL_CONFIG_DATA_0, 0x0000000f000101b8); // 750 MHz
   write_esr(PP_MACHINE, (uint8_t)shire_id, REGION_OTHER, SHIRE_OTHER_PLL_CONFIG_DATA_1, 0x02bb1bf40aeb02bb); 
   write_esr(PP_MACHINE, (uint8_t)shire_id, REGION_OTHER, SHIRE_OTHER_PLL_CONFIG_DATA_2, 0x001e01f01bf40aeb); 
   write_esr(PP_MACHINE, (uint8_t)shire_id, REGION_OTHER, SHIRE_OTHER_PLL_CONFIG_DATA_3, 0x000200020000000c); 
   pll_config_multiple_write(shire_id,0x00,0xf);
   
   // Write PLL registers 0x10 to 0x1a.
 //write_esr(PP_MACHINE, shire_id, REGION_OTHER, SHIRE_OTHER_PLL_CONFIG_DATA_0, 0x0000000000020002); // 1GHz    
 //write_esr(PP_MACHINE, shire_id, REGION_OTHER, SHIRE_OTHER_PLL_CONFIG_DATA_1, 0x0000000000000000);            
 //write_esr(PP_MACHINE, shire_id, REGION_OTHER, SHIRE_OTHER_PLL_CONFIG_DATA_2, 0x0000000000000000);            
 //write_esr(PP_MACHINE, shire_id, REGION_OTHER, SHIRE_OTHER_PLL_CONFIG_DATA_3, 0x0000000000000000);            
   write_esr(PP_MACHINE, (uint8_t)shire_id, REGION_OTHER, SHIRE_OTHER_PLL_CONFIG_DATA_0, 0x0000000000020002); // 750 MHz 
   write_esr(PP_MACHINE, (uint8_t)shire_id, REGION_OTHER, SHIRE_OTHER_PLL_CONFIG_DATA_1, 0x0000000000000000);
   write_esr(PP_MACHINE, (uint8_t)shire_id, REGION_OTHER, SHIRE_OTHER_PLL_CONFIG_DATA_2, 0x0000000000000001);
   write_esr(PP_MACHINE, (uint8_t)shire_id, REGION_OTHER, SHIRE_OTHER_PLL_CONFIG_DATA_3, 0x0000000000000000);
   pll_config_multiple_write(shire_id,0x10,0xa);

   // Write PLL registers 0x20 to 0x24.
 //write_esr(PP_MACHINE, shire_id, REGION_OTHER, SHIRE_OTHER_PLL_CONFIG_DATA_0, 0x0000000000000001); // 1GHz    
 //write_esr(PP_MACHINE, shire_id, REGION_OTHER, SHIRE_OTHER_PLL_CONFIG_DATA_1, 0x0000000000000000);            
   write_esr(PP_MACHINE, (uint8_t)shire_id, REGION_OTHER, SHIRE_OTHER_PLL_CONFIG_DATA_0, 0x0000000000000003); // 750 MHz 
   write_esr(PP_MACHINE, (uint8_t)shire_id, REGION_OTHER, SHIRE_OTHER_PLL_CONFIG_DATA_1, 0x0000000000000000);
   pll_config_multiple_write(shire_id,0x20,0x4);

   // Write PLL registers 0x27 to 0x28.(DATA_1 and DATA_2 and DATA_3 not changed)
   write_esr(PP_MACHINE, (uint8_t)shire_id, REGION_OTHER, SHIRE_OTHER_PLL_CONFIG_DATA_0, 0x000000000000000c);
   pll_config_multiple_write(shire_id,0x27,0x1);
   
   // Write PLL strobe register 0x38 to load previous configuration
   write_esr(PP_MACHINE, (uint8_t)shire_id, REGION_OTHER, SHIRE_OTHER_PLL_CONFIG_DATA_0, 0x0000000000000000);
   pll_config_multiple_write(shire_id,0x38,0x0);
   write_esr(PP_MACHINE, (uint8_t)shire_id, REGION_OTHER, SHIRE_OTHER_PLL_CONFIG_DATA_0, 0x0000000000000001);
   pll_config_multiple_write(shire_id,0x38,0x0);
   write_esr(PP_MACHINE, (uint8_t)shire_id, REGION_OTHER, SHIRE_OTHER_PLL_CONFIG_DATA_0, 0x0000000000000000);
   pll_config_multiple_write(shire_id,0x38,0x0);
   
   // Update PLL register 0 to enable PLL
   write_esr(PP_MACHINE, (uint8_t)shire_id, REGION_OTHER, SHIRE_OTHER_PLL_CONFIG_DATA_0, 0x0000001400010138);
   pll_config_multiple_write(shire_id,0x00,0x0);
   
   // Write PLL strobe register 0x38 to load previous configuration
   write_esr(PP_MACHINE, (uint8_t)shire_id, REGION_OTHER, SHIRE_OTHER_PLL_CONFIG_DATA_0, 0x0000000000000001);
   pll_config_multiple_write(shire_id,0x38,0x0);
   write_esr(PP_MACHINE, (uint8_t)shire_id, REGION_OTHER, SHIRE_OTHER_PLL_CONFIG_DATA_0, 0x0000000000000000);
   pll_config_multiple_write(shire_id,0x38,0x0);
   
   // Update PLL register 0 to enable PLL
   write_esr(PP_MACHINE, (uint8_t)shire_id, REGION_OTHER, SHIRE_OTHER_PLL_CONFIG_DATA_0, 0x00000014000101b8);
   pll_config_multiple_write(shire_id,0x00,0x0);
   
   // Write PLL strobe register 0x38 to load previous configuration
   write_esr(PP_MACHINE, (uint8_t)shire_id, REGION_OTHER, SHIRE_OTHER_PLL_CONFIG_DATA_0, 0x0000000000000001);
   pll_config_multiple_write(shire_id,0x38,0x0);
   write_esr(PP_MACHINE, (uint8_t)shire_id, REGION_OTHER, SHIRE_OTHER_PLL_CONFIG_DATA_0, 0x0000000000000000);
   pll_config_multiple_write(shire_id,0x38,0x0);
   
   
   // Wait until PLL is locked to change clock mux
   while(!(read_esr(PP_MACHINE, (uint8_t)shire_id, REGION_OTHER, SHIRE_PLL_READ_DATA) & 0x20000));
   
   // Select PLL[0] output. Bits[2:0]=3'b100. Bit 3 to '1' to go with DLL output
   write_esr(PP_MACHINE, (uint8_t)shire_id, REGION_OTHER, SHIRE_OTHER_CTRL_CLOCKMUX, 0xc);

   // DLL configuration (register values taken from Movellus testbench v 1.1.0b)
   /////////////////////////////////////////////////////////////////////////////

   // Auto-config register set dll_enable and get reset deasserted of the DLL
   reg_value =  SHIRE_OTHER_DLL_AUTO_CONFIG_PCLK_SEL
             | (0x1 << SHIRE_OTHER_DLL_AUTO_CONFIG_DLL_EN_OFF);
   write_esr(PP_MACHINE, (uint8_t)shire_id, REGION_OTHER, SHIRE_OTHER_DLL_AUTO_CONFIG, reg_value);

   // Write registers from 0x0 to 0x4
   //write_esr(PP_MACHINE, shire_id, REGION_OTHER, SHIRE_OTHER_DLL_CONFIG_DATA_0, 0x00102200400000c0);
   //write_esr(PP_MACHINE, shire_id, REGION_OTHER, SHIRE_OTHER_DLL_CONFIG_DATA_1, 0x0000000000000101);
   //dll_config_multiple_write(shire_id,0x0,0x4);

   // Wait until DLL is locked to change clock mux
   while(!(read_esr(PP_MACHINE, (uint8_t)shire_id, REGION_OTHER, SHIRE_DLL_READ_DATA) & 0x20000));

}

static void minion_pll_config ( uint64_t minion_shire_mask){

   for ( uint32_t i = 0; i <= 33; i ++){
     if (minion_shire_mask & 1)
       pll_config(i);
     minion_shire_mask >>=1;
   }
}

int configure_minion_plls_and_dlls(void) {
    minion_pll_config(1);
    return 0;
}

int enable_minion_neighborhoods(void) {
    //enable shire 0 neigh
    write_esr(PP_MACHINE, 0, REGION_OTHER, SHIRE_OTHER_CONFIG, ESR_SHIRE_CONFIG_0_EN);
    //enable shire 32 neigh
    write_esr(PP_MACHINE, 32, REGION_OTHER, SHIRE_OTHER_CONFIG, ESR_SHIRE_CONFIG_32_EN);

    return 0;
}

int enable_minion_threads(void) {
    //enable shire 0 thread 0
    write_esr(PP_MACHINE, 0, REGION_OTHER, SHIRE_OTHER_THREAD0_DISABLE, 0xfffffffe);
    //enable shire 32 thread 0
    write_esr(PP_MACHINE, 32, REGION_OTHER, SHIRE_OTHER_THREAD0_DISABLE, 0xfffffffe);

    return 0;
}
