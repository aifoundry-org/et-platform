/* vim: set ts=8 sw=4 et sta cin cino=\:0s,l1,g0,N-s,E-s,i0,+2s,(0,W2s : */

#ifndef BEMU_TRAPS_H
#define BEMU_TRAPS_H

#include <cstddef>
#include <cstdint>
#include "state.h"

//namespace bemu {


// base class for all traps
class trap_t {
  public:
    trap_t(uint64_t n) : cause(n) {}
    uint64_t get_cause() const { return cause; }

    virtual bool has_tval() const = 0;
    virtual uint64_t get_tval() const = 0;
    virtual const char* what() const = 0;

  private:
    const uint64_t cause;
};


// define a trap type without tval
#define DECLARE_TRAP_TVAL_N(CAUSE, TRAP) \
    class TRAP : public trap_t { \
      public: \
        TRAP() : trap_t(CAUSE) {} \
        virtual bool has_tval() const override { return false; } \
        virtual uint64_t get_tval() const override { return 0; } \
        virtual const char* what() const override { return #TRAP; } \
    }


// define a trap type with tval
#define DECLARE_TRAP_TVAL_Y(CAUSE, TRAP) \
    class TRAP : public trap_t { \
      public: \
        TRAP(uint64_t v) : trap_t(CAUSE), tval(v) {} \
        virtual bool has_tval() const override { return true; } \
        virtual uint64_t get_tval() const override { return tval; } \
        virtual const char* what() const override { return #TRAP; } \
      private: \
        const uint64_t tval; \
    }


// Exceptions
DECLARE_TRAP_TVAL_Y(0x00, trap_instruction_address_misaligned);
DECLARE_TRAP_TVAL_Y(0x01, trap_instruction_access_fault);
DECLARE_TRAP_TVAL_Y(0x02, trap_illegal_instruction);
DECLARE_TRAP_TVAL_Y(0x03, trap_breakpoint);
DECLARE_TRAP_TVAL_Y(0x04, trap_load_address_misaligned);
DECLARE_TRAP_TVAL_Y(0x05, trap_load_access_fault);
DECLARE_TRAP_TVAL_Y(0x06, trap_store_address_misaligned);
DECLARE_TRAP_TVAL_Y(0x07, trap_store_access_fault);
DECLARE_TRAP_TVAL_N(0x08, trap_user_ecall);
DECLARE_TRAP_TVAL_N(0x09, trap_supervisor_ecall);
DECLARE_TRAP_TVAL_N(0x0b, trap_machine_ecall);
DECLARE_TRAP_TVAL_Y(0x0c, trap_instruction_page_fault);
DECLARE_TRAP_TVAL_Y(0x0d, trap_load_page_fault);
DECLARE_TRAP_TVAL_Y(0x0f, trap_store_page_fault);
DECLARE_TRAP_TVAL_N(0x19, trap_fetch_bus_error);
DECLARE_TRAP_TVAL_N(0x1a, trap_fetch_ecc_error);
DECLARE_TRAP_TVAL_Y(0x1b, trap_load_split_page_fault);
DECLARE_TRAP_TVAL_Y(0x1c, trap_store_split_page_fault);
DECLARE_TRAP_TVAL_Y(0x1d, trap_bus_error);
DECLARE_TRAP_TVAL_Y(0x1e, trap_mcode_instruction);

// Interrupts
DECLARE_TRAP_TVAL_N(0x00 + (1ull<<(XLEN-1)), trap_user_software_interrupt);
DECLARE_TRAP_TVAL_N(0x01 + (1ull<<(XLEN-1)), trap_supervisor_software_interrupt);
DECLARE_TRAP_TVAL_N(0x03 + (1ull<<(XLEN-1)), trap_machine_software_interrupt);
DECLARE_TRAP_TVAL_N(0x04 + (1ull<<(XLEN-1)), trap_user_timer_interrupt);
DECLARE_TRAP_TVAL_N(0x05 + (1ull<<(XLEN-1)), trap_supervisor_timer_interrupt);
DECLARE_TRAP_TVAL_N(0x07 + (1ull<<(XLEN-1)), trap_machine_timer_interrupt);
DECLARE_TRAP_TVAL_N(0x08 + (1ull<<(XLEN-1)), trap_user_external_interrupt);
DECLARE_TRAP_TVAL_N(0x09 + (1ull<<(XLEN-1)), trap_supervisor_external_interrupt);
DECLARE_TRAP_TVAL_N(0x0B + (1ull<<(XLEN-1)), trap_machine_external_interrupt);
DECLARE_TRAP_TVAL_N(0x0F + (1ull<<(XLEN-1)), trap_bad_ipi_redirect_interrupt);
DECLARE_TRAP_TVAL_N(0x13 + (1ull<<(XLEN-1)), trap_icache_ecc_counter_overflow_interrupt);


//} // namespace bemu

#endif // BEMU_TRAPS_H
