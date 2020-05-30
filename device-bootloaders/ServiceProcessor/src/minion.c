#include "minion.h"

const uint64_t ESR_MEMORY_REGION = 0x0100000000UL;     // [32]=1

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

static inline volatile uint64_t* __attribute__((always_inline)) esr_address(esr_protection_t pp, uint8_t shire_id, esr_region_t region, uint32_t address)
{
    volatile uint64_t *p = (uint64_t *) (  ESR_MEMORY_REGION
                         | ((uint64_t)(pp       & 0x03    ) << 30)
                         | ((uint64_t)(shire_id & 0xff    ) << 22)
                         | ((uint64_t)(region   & 0x03    ) << 20)
                         | ((uint64_t)(address  & 0x01ffff) <<  3));
    return p;
}

static inline void __attribute__((always_inline)) write_esr(esr_protection_t pp, uint8_t shire_id, esr_region_t region, uint32_t address, uint64_t value)
{
    volatile uint64_t *p = esr_address(pp, shire_id, region, address);
    *p = value;
}

static int enable_minion_neighborhoods(uint8_t shire_id) {
    uint64_t neigh_en = ((uint64_t)0x1f00 | shire_id);
    
    write_esr(PP_MACHINE, shire_id, REGION_OTHER, SHIRE_OTHER_CONFIG, neigh_en);

    return 0;
}

static int enable_minion_threads(uint8_t shire_id) {
    write_esr(PP_MACHINE, shire_id, REGION_OTHER, SHIRE_OTHER_THREAD0_DISABLE, 0x0);
    write_esr(PP_MACHINE, shire_id, REGION_OTHER, SHIRE_OTHER_THREAD1_DISABLE, 0x0);
    return 0;
}

int enable_master_shire(void) {
    enable_minion_neighborhoods(MASTER_SHIRE_ID); 
    enable_minion_threads(MASTER_SHIRE_ID); 	
   return 0;
}

int enable_compute_shire(void) {
   for(uint8_t shire_id=0; shire_id < MAX_NUM_COMPUTE_SHIRE; shire_id++)  {
      enable_minion_neighborhoods(shire_id); 
      enable_minion_threads(shire_id); 	
   }
   return 0;
}



