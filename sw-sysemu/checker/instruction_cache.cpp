#include "instruction_cache.h"
#include <unistd.h>

// Constructor
instruction_cache::instruction_cache(main_memory * memory_, function_pointer_cache * func_cache_)
    : log("instruction cache", LOG_DEBUG)
{
    memory = memory_;
    func_cache = func_cache_;
}

// Destructor
instruction_cache::~instruction_cache()
{
}

// Decodes the instructions for this block
void instruction_cache::decode(uint64_t base_pc)
{
    log << LOG_DEBUG << "decode" << endm;
    uint16_t instruction_buffer[INSTRUCTION_CACHE_BLOCK_INSTR];
    instruction *instructions[INSTRUCTION_CACHE_BLOCK_INSTR];

    // Reads the raw instructions from memory
    memory->read(base_pc, INSTRUCTION_CACHE_BLOCK_SIZE, instruction_buffer);

    // Creates a file with the disasm contents
    //std::string tmp_in_file = "/tmp/dasm_checker_in_" + ::getpid();
    char tmp_in_file[128];
    char tmp_out_file[128];

    sprintf(tmp_in_file, "./dasm_checker_in");
    sprintf(tmp_out_file, "./dasm_checker_out");

    FILE * file = fopen(tmp_in_file, "w");
    if(file == NULL)
        log << LOG_FTL << "Couldn't open file " << tmp_in_file << endm;

    // Disasm them
    int inst_read = 0;
    int next_inst = 0;
    while (next_inst < (INSTRUCTION_CACHE_BLOCK_INSTR))
    {
        bool compressed = ((instruction_buffer[next_inst] & 0x03) != 0x3);
        if (compressed)
        {
            fprintf(file, "DASM(%04X)\n", instruction_buffer[next_inst]);
            instructions[next_inst] = new instruction();
            instructions[next_inst]->set_pc(base_pc + next_inst * 2);
            instructions[next_inst]->set_enc(instruction_buffer[next_inst]);
            instructions[next_inst]->set_compressed(true);
            inst_read++;
            next_inst++;
        }
        // Can't execute a 32b instruction if we only have 16b available
        else if((next_inst + 1) < (INSTRUCTION_CACHE_BLOCK_INSTR))
        {
            uint32_t bits = uint32_t(instruction_buffer[next_inst]) | uint32_t(instruction_buffer[next_inst+1]) << 16;
            fprintf(file, "DASM(%08X)\n", bits);
            instructions[next_inst] = new instruction();
            instructions[next_inst]->set_pc(base_pc + next_inst * 2);
            instructions[next_inst]->set_enc(bits);
            instructions[next_inst]->set_compressed(false);
            inst_read++;
            next_inst += 2;
        }
        else
        {
            next_inst++;
        }
    }
    fclose(file);

    // Disasm call
    std::string cmd = "$RISCV/bin/spike-dasm < " + std::string(tmp_in_file) + " > " + std::string(tmp_out_file);
    system(cmd.c_str());

    file = fopen(tmp_out_file, "r");
    int trials = 60;
    while ((file == NULL) && (trials > 0))
    {
        log << LOG_INFO << "Failed opening file " << tmp_out_file << ". Pending trials: " << trials << endm;
        sleep(1000);
        file = fopen(tmp_out_file, "r");
        trials--;
    }
    if (file == NULL)
    {
        log << LOG_FTL << "Couldn't open file " << tmp_out_file << endm;
    }
    
    int inst_disasm = 0;
    next_inst = 0;
    char str[1024];
    while(fgets(str, 1024, file) == str)
    {
        if(inst_disasm == inst_read)
            log << LOG_FTL << "Error: read more disasm than expected!!" << endm;
        instructions[next_inst]->set_mnemonic(str, func_cache, &log);
     
        cache[base_pc + next_inst * 2] = instructions[next_inst];

        if (instructions[next_inst]->get_is_compressed())
            next_inst++;
        else
            next_inst += 2;

        inst_disasm++;
    }


    if(inst_disasm != inst_read)
        log << LOG_FTL << "Error: read less disasm than expected!!" << endm;

    // Removes the files
    cmd = std::string("rm -rf ") + tmp_in_file;
    system(cmd.c_str());
    cmd = std::string("rm -rf ") + tmp_out_file;
    system(cmd.c_str());
}

// Returns a decoded instruction from a specific PC
instruction * instruction_cache::get_instruction(uint64_t pc)
{
    inst_cache_hash_it_t inst = cache.find(pc);

    // If instruction is present, return the instruction
    if(inst != cache.end())
        return inst->second;
    else
    {
        decode(pc);
        return cache[pc];
    }
}

