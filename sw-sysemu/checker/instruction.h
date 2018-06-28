#ifndef _INSTRUCTION_
#define _INSTRUCTION_

// Local
#include "emu.h"
#include "function_pointer_cache.h"
#include "testLog.h"

// STD
#include <string>

// Class that holds information for a decoded instruction
class instruction
{
    public:
        // Constructor and destructor
        instruction();
        ~instruction();

        // Access
        void set_pc(uint64 pc_);
        uint64 get_pc();
        void set_enc(uint32 enc_bits_);
        uint32 get_enc();
        void set_mnemonic(std::string mnemonic_, function_pointer_cache * func_cache, testLog * log_);
        std::string get_mnemonic();
        void set_compressed(bool v);
        bool get_is_load();
        bool get_is_fpload();
        bool get_is_wfi();
        bool get_is_reduce();
        bool get_is_tensor_load();
        bool get_is_tensor_fma();
        bool get_is_texrcv();
        bool get_is_texsndh();
        bool get_is_1ulp();
        bool get_is_amo();
        bool get_is_flb();
        bool get_is_compressed();
        int get_param(int param);

        // Execution
        void exec();
    private:
        uint64                       pc;             // PC of the instruction
        uint32                       enc_bits;       // Encoded bits
        std::string                  mnemonic;       // Mnemonic of the instruction
        bool                         is_load;        // If the instruction is an integer load
        bool                         is_fpload;      // If the instruction is a floating point load
        bool                         is_wfi;         // If the instruction is WFI
        bool                         is_reduce;      // If the instruction is a reduce
        bool                         is_tensor_load; // If the instruction is a tensor load
        bool                         is_tensor_fma;  // If the instruction is a tensor FMA
        bool                         is_texsndh;     // If the instruction is a texsndh
        bool                         is_texrcv;      // If the instruction is a texrcv
        bool                         is_1ulp;        // The instruction must be checked with 1ULP precision
        bool                         is_amo;         // If the instruction is an atomic operation
        bool                         is_flb;         // If the instruction is a fast local barrier
        bool                         is_compressed;  // If the instruction is a compressed encoding
        int                          params[4];      // Params to call the function
        int                          num_params;     // Number of params for the call
        func_ptr                     emu_func;       // Pointer to the emulation function
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
};

#endif // _INSTRUCTION_

