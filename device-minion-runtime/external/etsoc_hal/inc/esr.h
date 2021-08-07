/*-------------------------------------------------------------------------
* Copyright (C) 2021, Esperanto Technologies Inc.
* The copyright to the computer program(s) herein is the
* property of Esperanto Technologies, Inc. All Rights Reserved.
* The program(s) may be used and/or copied only with
* the written permission of Esperanto Technologies and
* in accordance with the terms and conditions stipulated in the
* agreement/contract under which the program(s) have been supplied.
*-------------------------------------------------------------------------
*/

// see PRM-19: Memory Map, PMA and PMP (Ch. 2.4 ESR Region, 2.4.1 Minion Shire ESR Map)
// see $RTLROOT/shire/esr/scripts/Minion\ ESR\ Registers.xlsx


#ifndef __ESR_H
#define __ESR_H

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
} esr_reg_t;

#ifndef ESR_MEMORY_REGION    // [32]=1
#define ESR_MEMORY_REGION 0x0100000000UL
#endif

inline volatile uint64_t* __attribute__((always_inline)) esr_address(esr_protection_t pp, uint8_t shire_id, esr_reg_t region, uint32_t address)
{
    // this is essential to use the new etsoc_hal 2.0 header files.  All address passed-in should be the real offset with 8 byte alignment.
    address >>= 3;

    volatile uint64_t *p = (uint64_t *) (  ESR_MEMORY_REGION
                         | ((uint64_t)(pp       & 0x03    ) << 30)
                         | ((uint64_t)(shire_id & 0xff    ) << 22)
                         | ((uint64_t)(region   & 0x03    ) << 20)
                         | ((uint64_t)(address  & 0x01ffff) <<  3));
    return p;
}


inline uint64_t __attribute__((always_inline)) read_esr(esr_protection_t pp, uint8_t shire_id, esr_reg_t region, uint32_t address)
{
    volatile uint64_t *p = esr_address(pp, shire_id, region, address);
    return *p;
}

inline void __attribute__((always_inline)) write_esr(esr_protection_t pp, uint8_t shire_id, esr_reg_t region, uint32_t address, uint64_t value)
{
    volatile uint64_t *p = esr_address(pp, shire_id, region, address);
    *p = value;
}


// new functions
inline volatile uint64_t* __attribute__((always_inline)) esr_address_new(esr_protection_t pp, uint8_t shire_id, esr_reg_t region, uint8_t subregion, uint32_t address, uint8_t bnk_or_thrd /*=0x0*/)
{

    uint64_t final_addr = (  ESR_MEMORY_REGION
                         | ((uint64_t)(pp       & 0x03    ) << 30)     // 2-bit [31:30]
                         | ((uint64_t)(shire_id & 0xff    ) << 22)     // 8-bit [29:22]
                         | ((uint64_t)(region   & 0x03    ) << 20)     // 2-bit [21:20]
                          );

    // this is essential to use the new etsoc_hal 2.0 header files.  All address passed-in should be the real offset with 8 byte alignment.
    address >>= 3;

    if(region == REGION_MINION) {
        // subregion = Minion# (7-bit);
        // bnk_or_thrd = Thread_id (1-bit)
        final_addr = ( final_addr
                     | ((uint64_t)(subregion   & 0x7f    ) << 13)     // 7-bit [19:13]
                     | ((uint64_t)(bnk_or_thrd & 0x01    ) << 12)     // 1-bit [12]
                     | ((uint64_t)(address     & 0x01ff  ) <<  3));   // 9-bit [11:3]

    } else if(region == REGION_NEIGHBOURHOOD) {
        // subregion = Neighbor# (4-bit) [19:16];
        final_addr = ( final_addr
                     | ((uint64_t)(subregion   & 0x0f    ) << 16)     // 4-bit [19:16]
                     | ((uint64_t)(address     & 0x01fff ) <<  3));   // 13-bit [15:3]

    } else if(region == REGION_OTHER) {     //
        // subregion: 3-bit
        if(subregion == 0x00) {   // Shire Cache ESR or Debug
            // shire_cache_bank#: 4-bit [16:13]
            final_addr = ( final_addr
                         | ((uint64_t)(subregion   & 0x07    ) << 17)     // 3-bit [19:17]
                         | ((uint64_t)(bnk_or_thrd & 0x0f    ) << 13)     // 3-bit [16:13]
                         | ((uint64_t)(address     & 0x03ff  ) <<  3));   // 10-bit [12:3]
        } else {
            final_addr = ( final_addr
                         | ((uint64_t)(subregion   & 0x07    ) << 17)     // 3-bit [19:17]
                         | ((uint64_t)(address     & 0x03fff ) <<  3));   // 14-bit [16:3]
        }
    } else {   // REGION_TBOX
       ;  // Unsuppported
    }

    volatile uint64_t *p = (uint64_t *)(final_addr);
    return p;
}

inline uint64_t __attribute__((always_inline)) read_esr_new(esr_protection_t pp, uint8_t shire_id, esr_reg_t region, uint8_t subregion, uint32_t address, uint8_t bnk_or_thrd /*=0x0*/)
{
    volatile uint64_t *p = esr_address_new(pp, shire_id, region, subregion, address, bnk_or_thrd);
    return *p;
}

inline void __attribute__((always_inline)) write_esr_new(esr_protection_t pp, uint8_t shire_id, esr_reg_t region, uint8_t subregion, uint32_t address, uint64_t value, uint8_t bnk_or_thrd /*=0x0*/)
{
    volatile uint64_t *p = esr_address_new(pp, shire_id, region, subregion, address, bnk_or_thrd);
    *p = value;
}


inline uint64_t __attribute__((always_inline)) read_rbox_esr(esr_protection_t pp, uint8_t shire_id, uint32_t address)
{
    return read_esr(pp, shire_id, REGION_OTHER, (address & 0x03fff) | 0x04000);
    //return read_esr_new(pp, shire_id, REGION_OTHER, 0x01, (address & 0x03fff), 0x0);    // subregion=3'b001
}

inline void __attribute__((always_inline)) write_rbox_esr(esr_protection_t pp, uint8_t shire_id, uint32_t address, uint64_t value)
{
    write_esr(pp, shire_id, REGION_OTHER, (address & 0x03fff) | 0x04000, value);
    //write_esr_new(pp, shire_id, REGION_OTHER, 0x01, (address & 0x03fff), value, 0x0);   // subregion=3'b001
}



#endif // ! __ESR_H
