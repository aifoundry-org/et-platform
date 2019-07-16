/* vim: set ts=8 sw=4 et sta cin cino=\:0s,l1,g0,N-s,E-s,i0,+2s,(0,W2s : */

#ifndef BEMU_TRAPS_H
#define BEMU_TRAPS_H

#include <cstddef>
#include <cstdint>

//namespace bemu {


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
DEF_TRAP_N( 0 + (1ull<<63), async, trap_user_software_interrupt);
DEF_TRAP_N( 1 + (1ull<<63), async, trap_supervisor_software_interrupt);
DEF_TRAP_N( 3 + (1ull<<63), async, trap_machine_software_interrupt);
DEF_TRAP_N( 4 + (1ull<<63), async, trap_user_timer_interrupt);
DEF_TRAP_N( 5 + (1ull<<63), async, trap_supervisor_timer_interrupt);
DEF_TRAP_N( 7 + (1ull<<63), async, trap_machine_timer_interrupt);
DEF_TRAP_N( 8 + (1ull<<63), async, trap_user_external_interrupt);
DEF_TRAP_N( 9 + (1ull<<63), async, trap_supervisor_external_interrupt);
DEF_TRAP_N(11 + (1ull<<63), async, trap_machine_external_interrupt);
DEF_TRAP_N(16 + (1ull<<63), async, trap_bad_ipi_redirect_interrupt);
DEF_TRAP_N(19 + (1ull<<63), async, trap_icache_ecc_counter_overflow_interrupt);
DEF_TRAP_N(23 + (1ull<<63), async, trap_bus_error_interrupt);


//} // namespace bemu

#endif // BEMU_TRAPS_H
