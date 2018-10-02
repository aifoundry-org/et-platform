#include <cassert>
#include <unordered_map>
#include <unistd.h>
#include "emu_defines.h"
#include "instruction.h"

// The instruction cache data structure is a hash table indexed by the
// physical PC of the instruction being fetched.

static std::unordered_map<uint64_t,instruction*> insn_cache;

// Logging
static testLog log("instruction cache", LOG_INFO);

// memory emulation
extern uint64_t (*vmemtranslate) (uint64_t, mem_access_type);
extern void (*pmemfetch512) (uint64_t, uint16_t*);

// program state
extern uint64_t current_pc;
extern uint32_t current_inst;

// Instruction cache block size (in bytes)
#define ICACHE_BLOCK_SIZE       64
#define INSNS_PER_ICACHE_BLOCK  (ICACHE_BLOCK_SIZE / 2)

#define ICACHE_ALIGNED(x)       ((x) & (~uint64_t(ICACHE_BLOCK_SIZE-1)))
#define ICACHE_OFFSET(x)        ((x) % ICACHE_BLOCK_SIZE)

// Paranoia
#if (ICACHE_BLOCK_SIZE & 63)
#error "ICACHE_BLOCK_SIZE must be a multiple of 64 bytes"
#endif

static inline bool insn_is_compressed(uint16_t bits)
{
    return (bits & 0x0003) != 0x0003;
}

void flush_insn_cache()
{
    insn_cache.clear();
}

// Returns a decoded instruction from a specific PC
instruction * get_inst()
{
    log << LOG_DEBUG << "instruction_cache::get_instruction("<<current_pc<<")"<<endm;

    // Physical address corresponding to virtual PC
    uint64_t paddr = vmemtranslate(current_pc, Mem_Access_Fetch);

    // If instruction is present, return the instruction
    auto it = insn_cache.find(paddr);
    if(it != insn_cache.end())
    {
        instruction * insn = it->second;
        current_inst = insn->get_enc();
        return insn;
    }

    // We haven't decoded the instruction before, so we need to fetch it into
    // the "cache" first. We fetch blocks of 64B, naturally aligned (to avoid
    // misalignment and PMA faults).

    // Cache-block aligned address and offset inside the block
    uint64_t paddr_aligned = ICACHE_ALIGNED(paddr);
    int paddr_offset  = ICACHE_OFFSET(paddr);

    // Instruction buffer; it holds 2 cache blocks
    uint16_t ibuf[2*INSNS_PER_ICACHE_BLOCK];
    int first_index = paddr_offset/2;

    // Fetch a cache block
    pmemfetch512(paddr_aligned, &ibuf[0]);
    int last_index = INSNS_PER_ICACHE_BLOCK;

    // Check if an instruction crosses a block or page boundary. This case can
    // only happen if the instruction is 4B and it has only 2B in the first
    // block.  to fetch a 2nd block so we can decode a full instruction. If
    // the 2nd block is in a different page we need to perform V2P translation
    // as well, which may generate faults! Because we fetch speculatively, we
    // must do all this only if the first instruction crosses a block.
    if ((paddr_offset == ICACHE_BLOCK_SIZE-2) && !insn_is_compressed(ibuf[first_index]))
    {
        assert(ICACHE_ALIGNED(current_pc+2) == (current_pc+2));
        uint64_t paddr_aligned2 = vmemtranslate(current_pc+2, Mem_Access_Fetch);
        pmemfetch512(paddr_aligned2, &ibuf[1]);
        last_index = 2*INSNS_PER_ICACHE_BLOCK;
    }

    instruction * insns[INSNS_PER_ICACHE_BLOCK+1];
    int insn_index = first_index;
    int insn_count = 0;

    // Creates a file with the disasm contents
    FILE * file = fopen("./dasm_checker_in", "w");
    if (file == NULL)
    {
        log << LOG_FTL << "Failed opening file 'dasm_checker_in'." << endm;
    }
    uint64_t pc = paddr;
    while ((insn_index < last_index) && (insn_count <= INSNS_PER_ICACHE_BLOCK))
    {
        instruction * insn = new instruction();
        insn->set_pc(pc);
        if (insn_is_compressed(ibuf[insn_index]))
        {
            uint16_t bits = ibuf[insn_index];
            insn->set_enc(bits);
            insn->set_compressed(true);
            pc += 2;
            ++insn_index;
            fprintf(file,"DASM(%04X)\n", bits);
        }
        else
        {
            uint32_t bits = uint32_t(ibuf[insn_index]) | (uint32_t(ibuf[insn_index+1]) << 16);
            insn->set_enc(bits);
            insn->set_compressed(false);
            pc += 4;
            insn_index += 2;
            fprintf(file,"DASM(%08X)\n", bits);
        }
        insns[insn_count++] = insn;
    }
    fclose(file);

    // Call "spike-dasm" to disassemble the file contents
    system("$RISCV/bin/spike-dasm < ./dasm_checker_in > ./dasm_checker_out");
    int retries = 60;
    while (true)
    {
        file = fopen("./dasm_checker_out", "r");
        if (file != nullptr)
            break;
        if (--retries <= 0)
        {
            log << LOG_FTL << "Failed opening file 'dasm_checker_out'." << endm;
        }
        log << LOG_INFO << "Failed opening file 'dasm_checker_out'. Pending retries: " << retries << endm;
        sleep(1000);
    }

    // Read "spike-dasm" output
    int insn_disasm = 0;
    char str[1024];
    while (fgets(str, 1024, file) == str)
    {
        if (insn_disasm == insn_count)
        {
            log << LOG_FTL << "Error: read more disasm than expected!!" << endm;
        }
        instruction * insn = insns[insn_disasm++];
        insn->set_mnemonic(str, &log);
        insn_cache[insn->get_pc()] = insn;
    }
    fclose(file);
    if (insn_disasm != insn_count)
    {
        log << LOG_FTL << "Error: read less disasm than expected!!" << endm;
    }

    // Remove the files
    system("/bin/rm -f ./dasm_checker_in ./dasm_checker_out");

    auto jt = insn_cache.find(paddr);
    if(jt == insn_cache.end())
    {
        log << LOG_FTL << "Error: internal error while updating the instruction cache!!" << endm;
    }
    instruction * insn = jt->second;
    current_inst = insn->get_enc();
    return insn;
}
