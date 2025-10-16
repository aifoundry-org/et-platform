/*-------------------------------------------------------------------------
* Copyright (c) 2025 Ainekko, Co.
* SPDX-License-Identifier: Apache-2.0
*-------------------------------------------------------------------------*/

#ifndef BEMU_TRAPS_H
#define BEMU_TRAPS_H

#include <cstddef>
#include <cstdint>

namespace bemu {


// Base class for all traps
struct Trap {
    virtual uint64_t cause() const = 0;
    virtual uint64_t tval() const = 0;
    virtual const char* what() const = 0;
};


// Base class for synchronous traps
struct Exception : virtual public Trap { };


// Base class for asynchronous traps
struct Interrupt : virtual public Trap { };


// Define a trap type without tval
#define DEF_TRAP_N(CAUSE, TYPE, TRAP) \
    struct TRAP : public TYPE { \
        virtual uint64_t cause() const override { return CAUSE; } \
        virtual uint64_t tval() const override { return 0; } \
        virtual const char* what() const override { return #TRAP; } \
    }


// Define a trap type with tval
#define DEF_TRAP_Y(CAUSE, TYPE, TRAP) \
    struct TRAP : public TYPE { \
        TRAP(uint64_t v) : val(v) {} \
        virtual uint64_t cause() const override { return CAUSE; } \
        virtual uint64_t tval() const override { return val; } \
        virtual const char* what() const override { return #TRAP; } \
      private: \
        const uint64_t val; \
    }


// Exceptions
DEF_TRAP_Y( 0, Exception, trap_instruction_address_misaligned);
DEF_TRAP_Y( 1, Exception, trap_instruction_access_fault);
DEF_TRAP_Y( 2, Exception, trap_illegal_instruction);
DEF_TRAP_Y( 3, Exception, trap_breakpoint);
DEF_TRAP_Y( 4, Exception, trap_load_address_misaligned);
DEF_TRAP_Y( 5, Exception, trap_load_access_fault);
DEF_TRAP_Y( 6, Exception, trap_store_address_misaligned);
DEF_TRAP_Y( 7, Exception, trap_store_access_fault);
DEF_TRAP_N( 8, Exception, trap_user_ecall);
DEF_TRAP_N( 9, Exception, trap_supervisor_ecall);
DEF_TRAP_N(11, Exception, trap_machine_ecall);
DEF_TRAP_Y(12, Exception, trap_instruction_page_fault);
DEF_TRAP_Y(13, Exception, trap_load_page_fault);
DEF_TRAP_Y(15, Exception, trap_store_page_fault);
DEF_TRAP_N(25, Exception, trap_instruction_bus_error);
DEF_TRAP_N(26, Exception, trap_instruction_ecc_error);
DEF_TRAP_Y(27, Exception, trap_load_split_page_fault);
DEF_TRAP_Y(28, Exception, trap_store_split_page_fault);
DEF_TRAP_Y(30, Exception, trap_mcode_instruction);

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

DEF_TRAP_N(USER_SOFTWARE_INTERRUPT               + (1ull<<63), Interrupt, trap_user_software_interrupt);
DEF_TRAP_N(SUPERVISOR_SOFTWARE_INTERRUPT         + (1ull<<63), Interrupt, trap_supervisor_software_interrupt);
DEF_TRAP_N(MACHINE_SOFTWARE_INTERRUPT            + (1ull<<63), Interrupt, trap_machine_software_interrupt);
DEF_TRAP_N(USER_TIMER_INTERRUPT                  + (1ull<<63), Interrupt, trap_user_timer_interrupt);
DEF_TRAP_N(SUPERVISOR_TIMER_INTERRUPT            + (1ull<<63), Interrupt, trap_supervisor_timer_interrupt);
DEF_TRAP_N(MACHINE_TIMER_INTERRUPT               + (1ull<<63), Interrupt, trap_machine_timer_interrupt);
DEF_TRAP_N(USER_EXTERNAL_INTERRUPT               + (1ull<<63), Interrupt, trap_user_external_interrupt);
DEF_TRAP_N(SUPERVISOR_EXTERNAL_INTERRUPT         + (1ull<<63), Interrupt, trap_supervisor_external_interrupt);
DEF_TRAP_N(MACHINE_EXTERNAL_INTERRUPT            + (1ull<<63), Interrupt, trap_machine_external_interrupt);
DEF_TRAP_N(BAD_IPI_REDIRECT_INTERRUPT            + (1ull<<63), Interrupt, trap_bad_ipi_redirect_interrupt);
DEF_TRAP_N(ICACHE_ECC_COUNTER_OVERFLOW_INTERRUPT + (1ull<<63), Interrupt, trap_icache_ecc_counter_overflow_interrupt);
DEF_TRAP_N(BUS_ERROR_INTERRUPT                   + (1ull<<63), Interrupt, trap_bus_error_interrupt);


// For ebreak, trigger, and single step, we rely on Exceptions
// to abort the execution of an instruction and enter debug mode.
struct Debug_entry : public std::exception {
    enum class Cause {
        ebreak  = 1, // An ebreak instruction was executed
        trigger = 2, // The Trigger Module caused a breakpoint exception
        haltreq = 3, // The debugger requested entry to Debug Mode
        step    = 4, // The hart single stepped because step was set
    };

    Debug_entry(Cause cause) : cause(cause) { }

    const char* what() const noexcept { return "Debug entry"; }

    Cause cause;
};


} // namespace bemu

#endif // BEMU_TRAPS_H
