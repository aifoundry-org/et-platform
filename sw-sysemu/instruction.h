#ifndef _INSTRUCTION_
#define _INSTRUCTION_

// Local
#define CHECKER
#include "testLog.h"

// STD
#include <string>

// Defines the function pointer typedef
typedef void (*func_ptr)();
typedef void (*func_ptr_0)(const char* comm);
typedef void (*func_ptr_1)(int arg0, const char* comm);
typedef void (*func_ptr_2)(int arg0, int arg1, const char* comm);
typedef void (*func_ptr_3)(int arg0, int arg1, int arg2, const char* comm);
typedef void (*func_ptr_4)(int arg0, int arg1, int arg2, int arg3, const char* comm);
typedef void (*func_ptr_5)(int arg0, int arg1, int arg2, int arg3, int arg4, const char* comm);

// Class that holds information for a decoded instruction
class instruction
{
    public:
        // Constructor and destructor
        instruction();
        ~instruction();

        // Access
        void set_pc(uint64_t pc_)         { pc = pc_; }
        uint64_t get_pc() const           { return pc; }
        void set_enc(uint32_t enc_bits_)  { enc_bits = enc_bits_; }
        uint32_t get_enc() const          { return enc_bits; }
        void set_mnemonic(std::string mnemonic_, testLog * log_);
        std::string get_mnemonic()        { return mnemonic; }
        void set_compressed(bool v)       { is_compressed = v; }
        bool get_is_compressed() const    { return is_compressed; }
        bool get_is_load() const          { return is_load; }
        bool get_is_fpload() const        { return is_fpload; }
        bool get_is_wfi() const           { return is_wfi; }
        bool get_is_reduce() const        { return is_reduce; }
        bool get_is_tensor_load() const   { return is_tensor_load; }
        bool get_is_tensor_fma() const    { return is_tensor_fma; }
        bool get_is_texrcv() const        { return is_texrcv; }
        bool get_is_texsndh() const       { return is_texsndh; }
        bool get_is_1ulp() const          { return is_1ulp; }
        bool get_is_amo() const           { return is_amo; }
        bool get_is_flb() const           { return is_flb; }
        bool get_is_fcc() const           { return is_fcc; }
        bool get_is_csr_read() const      { return is_csr_read; }
        unsigned get_size() const         { return is_compressed ? 2 : 4; }
        int get_param(int param) const    { return params[param]; }
        func_ptr get_emu_func() const     { return emu_func; }

        // Execution
        void exec();
    private:
        uint64_t                     pc;                     // PC of the instruction
        uint32_t                     enc_bits;               // Encoded bits
        std::string                  mnemonic;               // Mnemonic of the instruction
        bool                         is_load;                // If the instruction is an integer load
        bool                         is_fpload;              // If the instruction is a floating point load
        bool                         is_wfi;                 // If the instruction is WFI
        bool                         is_reduce;              // If the instruction is a reduce
        bool                         is_tensor_load;         // If the instruction is a tensor load
        bool                         is_tensor_fma;          // If the instruction is a tensor FMA
        bool                         is_texsndh;             // If the instruction is a texsndh
        bool                         is_texrcv;              // If the instruction is a texrcv
        bool                         is_1ulp;                // The instruction must be checked with 1ULP precision
        bool                         is_amo;                 // If the instruction is an atomic operation
        bool                         is_flb;                 // If the instruction is a fast local barrier
        bool                         is_fcc;                 // If the instruction is a fast credit counter
        bool                         is_compressed = false;  // If the instruction is a compressed encoding
        bool                         is_csr_read;            // If the instruction is a CSR access
        int                          params[5];              // Params to call the function
        int                          num_params;             // Number of params for the call
        func_ptr                     emu_func;               // Pointer to the emulation function
        func_ptr_0                   emu_func0;
        func_ptr_1                   emu_func1;
        func_ptr_2                   emu_func2;
        func_ptr_3                   emu_func3;
        func_ptr_4                   emu_func4;
        func_ptr_5                   emu_func5;
        testLog *                    log;
        bool                         has_error;  // The instruction decode had an error
        std::string                  str_error;  // Error of the instruction decoding

        void add_parameter(std::string param);

        func_ptr get_function_ptr(std::string func, bool * error = NULL, std::string * error_msg = NULL);
};

#endif // _INSTRUCTION_

