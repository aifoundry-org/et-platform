#ifndef _MAIN_MEMORY_REGION_ESR_
#define _MAIN_MEMORY_REGION_ESR_

#include "main_memory_region.h"


// Memory region used to implement the access to all ESRs defined in the memory map

class main_memory_region_esr : main_memory_region
{
public:
    // Constructors and destructors
    main_memory_region_esr(uint64_t base, uint64_t size, testLog & l, func_ptr_get_thread& get_th);
    ~main_memory_region_esr();
    
    // read and write
    void write(uint64_t ad, int size, const void* data);
    void read(uint64_t ad, int size, void* data);

private:

    typedef enum
    {
        ESR_Prot_User       = 0,
        ESR_Prot_Supervisor = 1,
        ESR_Prot_Machine    = 2,
        ESR_Prot_Debug      = 3
    } esr_protection_t;

    typedef enum
    {
        ESR_Region_HART         = 0,
        ESR_Region_Neighborhood = 1,
        ESR_Region_Reserved     = 2,
        ESR_Region_Extended     = 3,
        ESR_Region_Shire_Cache  = 0x18,
        ESR_Region_Shire_RBOX   = 0x19,
        ESR_Region_Shire        = 0x1A
    } esr_region_t;
    
    typedef struct {
        esr_protection_t protection;
        esr_region_t     region;
        uint64_t         shire;
        uint64_t         neighborhood;
        uint64_t         hart;
        uint64_t         bank;
        uint64_t         address;
        bool             valid;
    } esr_info_t;

    void decode_ESR_address(uint64_t address, esr_info_t *info);
};

#endif // _MAIN_MEMORY_REGION_ESR_

