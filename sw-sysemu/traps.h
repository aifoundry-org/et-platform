/*-------------------------------------------------------------------------
* Copyright (C) 2020, Esperanto Technologies Inc.
* The copyright to the computer program(s) herein is the
* property of Esperanto Technologies, Inc. All Rights Reserved.
* The program(s) may be used and/or copied only with
* the written permission of Esperanto Technologies and
* in accordance with the terms and conditions stipulated in the
* agreement/contract under which the program(s) have been supplied.
*-------------------------------------------------------------------------*/

#ifndef BEMU_TRAPS_H
#define BEMU_TRAPS_H

#include <cstddef>
#include <cstdint>

namespace bemu {


// Forward declaration
struct Hart;


// Base class for all traps
struct trap_t {
    virtual uint64_t cause() const = 0;
    virtual uint64_t tval() const = 0;
    virtual const char* what() const = 0;
};


// Base class for synchronous traps
struct sync_trap_t : virtual public trap_t { };


// Base class for asynchronous traps
struct async_trap_t : virtual public trap_t { };


// Define a trap type without tval
#define DEF_TRAP_N(CAUSE, TYPE, TRAP) \
    struct TRAP : public TYPE##_trap_t { \
        virtual uint64_t cause() const override { return CAUSE; } \
        virtual uint64_t tval() const override { return 0; } \
        virtual const char* what() const override { return #TRAP; } \
    }


// Define a trap type with tval
#define DEF_TRAP_Y(CAUSE, TYPE, TRAP) \
    struct TRAP : public TYPE##_trap_t { \
        TRAP(uint64_t v) : val(v) {} \
        virtual uint64_t cause() const override { return CAUSE; } \
        virtual uint64_t tval() const override { return val; } \
        virtual const char* what() const override { return #TRAP; } \
      private: \
        const uint64_t val; \
    }


// Exceptions
DEF_TRAP_Y( 0, sync,  trap_instruction_address_misaligned);
DEF_TRAP_Y( 1, sync,  trap_instruction_access_fault);
DEF_TRAP_Y( 2, sync,  trap_illegal_instruction);
DEF_TRAP_Y( 3, sync,  trap_breakpoint);
DEF_TRAP_Y( 4, sync,  trap_load_address_misaligned);
DEF_TRAP_Y( 5, sync,  trap_load_access_fault);
DEF_TRAP_Y( 6, sync,  trap_store_address_misaligned);
DEF_TRAP_Y( 7, sync,  trap_store_access_fault);
DEF_TRAP_N( 8, sync,  trap_user_ecall);
DEF_TRAP_N( 9, sync,  trap_supervisor_ecall);
DEF_TRAP_N(11, sync,  trap_machine_ecall);
DEF_TRAP_Y(12, sync,  trap_instruction_page_fault);
DEF_TRAP_Y(13, sync,  trap_load_page_fault);
DEF_TRAP_Y(15, sync,  trap_store_page_fault);
DEF_TRAP_N(25, sync,  trap_instruction_bus_error);
DEF_TRAP_N(26, sync,  trap_instruction_ecc_error);
DEF_TRAP_Y(27, sync,  trap_load_split_page_fault);
DEF_TRAP_Y(28, sync,  trap_store_split_page_fault);
DEF_TRAP_Y(30, sync,  trap_mcode_instruction);

// Interrupts
#define USER_SOFTWARE_INTERRUPT               0
#define SUPERVISOR_SOFTWARE_INTERRUPT         1
#define MACHINE_SOFTWARE_INTERRUPT            3
#define USER_TIMER_INTERRUPT                  4
#define SUPERVISOR_TIMER_INTERRUPT            5
#define MACHINE_TIMER_INTERRUPT               7
#define USER_EXTERNAL_INTERRUPT               8
#define SUPERVISOR_EXTERNAL_INTERRUPT         9
#define MACHINE_EXTERNAL_INTERRUPT            11
#define BAD_IPI_REDIRECT_INTERRUPT            16
#define ICACHE_ECC_COUNTER_OVERFLOW_INTERRUPT 19
#define BUS_ERROR_INTERRUPT                   23

DEF_TRAP_N(USER_SOFTWARE_INTERRUPT               + (1ull<<63), async, trap_user_software_interrupt);
DEF_TRAP_N(SUPERVISOR_SOFTWARE_INTERRUPT         + (1ull<<63), async, trap_supervisor_software_interrupt);
DEF_TRAP_N(MACHINE_SOFTWARE_INTERRUPT            + (1ull<<63), async, trap_machine_software_interrupt);
DEF_TRAP_N(USER_TIMER_INTERRUPT                  + (1ull<<63), async, trap_user_timer_interrupt);
DEF_TRAP_N(SUPERVISOR_TIMER_INTERRUPT            + (1ull<<63), async, trap_supervisor_timer_interrupt);
DEF_TRAP_N(MACHINE_TIMER_INTERRUPT               + (1ull<<63), async, trap_machine_timer_interrupt);
DEF_TRAP_N(USER_EXTERNAL_INTERRUPT               + (1ull<<63), async, trap_user_external_interrupt);
DEF_TRAP_N(SUPERVISOR_EXTERNAL_INTERRUPT         + (1ull<<63), async, trap_supervisor_external_interrupt);
DEF_TRAP_N(MACHINE_EXTERNAL_INTERRUPT            + (1ull<<63), async, trap_machine_external_interrupt);
DEF_TRAP_N(BAD_IPI_REDIRECT_INTERRUPT            + (1ull<<63), async, trap_bad_ipi_redirect_interrupt);
DEF_TRAP_N(ICACHE_ECC_COUNTER_OVERFLOW_INTERRUPT + (1ull<<63), async, trap_icache_ecc_counter_overflow_interrupt);
DEF_TRAP_N(BUS_ERROR_INTERRUPT                   + (1ull<<63), async, trap_bus_error_interrupt);


// Traps
void take_trap(Hart&, const trap_t&);

// Interrupts
void raise_interrupt(Hart& cpu, int cause, uint64_t mip, uint64_t mbusaddr=0);
void raise_software_interrupt(Hart& cpu);
void clear_software_interrupt(Hart& cpu);
void raise_timer_interrupt(Hart& cpu);
void clear_timer_interrupt(Hart& cpu);
void raise_external_machine_interrupt(Hart& cpu);
void clear_external_machine_interrupt(Hart& cpu);
void raise_external_supervisor_interrupt(Hart& cpu);
void clear_external_supervisor_interrupt(Hart& cpu);
void raise_bus_error_interrupt(Hart& cpu, uint64_t busaddr);
void clear_bus_error_interrupt(Hart& cpu);


} // namespace bemu

#endif // BEMU_TRAPS_H
