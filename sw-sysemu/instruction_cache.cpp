/* -*- Mode:C++; c-basic-offset: 4; -*- */

#include <cassert>
#include <unordered_map>
#include <stdexcept>
#include <unistd.h>
#include <cstdlib>

#include "emu_gio.h"
#include "instruction.h"


// The instruction cache data structure is a hash table indexed by the
// physical PC of the instruction being fetched.

static std::unordered_map<uint64_t,instruction*> insn_cache;

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
    // Physical address corresponding to virtual PC
    uint64_t paddr = vmemtranslate(current_pc, Mem_Access_Fetch);

    // Cache-block aligned address and offset inside the block
    uint64_t paddr_aligned = ICACHE_ALIGNED(paddr);
    int paddr_offset  = ICACHE_OFFSET(paddr);

    // Instruction buffer; it holds 2 cache blocks
    uint16_t ibuf[2*INSNS_PER_ICACHE_BLOCK];
    int first_index = paddr_offset/2;

    // If instruction is present, return the instruction unless it crosses a block boundary
    // in which case it may not have been decoded properly before and we need to fetch the next block.
    auto it = insn_cache.find(paddr);
    if (it != insn_cache.end()) {
        instruction * insn = it->second;
        current_inst = insn->get_enc();
        if ((paddr_offset != ICACHE_BLOCK_SIZE-2) || (insn->get_is_compressed())) {
           LOG(DEBUG, "Fetched %sinstruction from PC 0x%016llx (PA: 0x%010llx): 0x%08x (%s)", insn->get_is_compressed() ? "compressed " : "", current_pc, paddr, current_inst, insn->get_mnemonic().c_str());
           return insn;
        } else {
           insn_cache.erase(it);
        }
    }

    // We haven't decoded the instruction before, so we need to fetch it into
    // the "cache" first. We fetch blocks of 64B, naturally aligned (to avoid
    // misalignment and PMA faults).

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
        LOG(DEBUG, "Instruction at PA 0x%010llx is crossing a block. Fetching next one", paddr);
        assert(ICACHE_ALIGNED(current_pc+2) == (current_pc+2));
        uint64_t paddr_aligned2 = vmemtranslate(current_pc+2, Mem_Access_Fetch);
        pmemfetch512(paddr_aligned2, &ibuf[32]);
        last_index = 2*INSNS_PER_ICACHE_BLOCK;
    }

    instruction * insns[INSNS_PER_ICACHE_BLOCK+1];
    int insn_index = first_index;
    int insn_count = 0;

    char dasm_str[1024] = "";
    uint64_t pc = paddr;
    while ((insn_index < last_index) && (insn_count <= INSNS_PER_ICACHE_BLOCK))
    {
        instruction * insn;
        auto it = insn_cache.find(pc);
        if (it != insn_cache.end()) {
           insn = it->second;
           delete insn;
           insn_cache.erase(it);
        }
        insn = new instruction();
        insn->set_pc(pc);
        if (insn_is_compressed(ibuf[insn_index]))
        {
            uint16_t bits = ibuf[insn_index];
            insn->set_enc(bits);
            insn->set_compressed(true);
            pc += 2;
            ++insn_index;
            sprintf(dasm_str,"%sDASM(%04X)\n", dasm_str, bits);
        }
        else
        {
            uint32_t bits = uint32_t(ibuf[insn_index]) | (uint32_t(ibuf[insn_index+1]) << 16);
            insn->set_enc(bits);
            insn->set_compressed(false);
            pc += 4;
            insn_index += 2;
            sprintf(dasm_str,"%sDASM(%08X)\n", dasm_str, bits);
        }
        insns[insn_count++] = insn;
    }

    // Call "spike-dasm" to disassemble the stream contents
    FILE * stream;
    static char cmd[1024];
    snprintf(cmd, 1024, "echo -n '%s' | $RISCV/bin/spike-dasm", dasm_str);

    stream = popen(cmd, "r");
    if (stream == nullptr) throw std::runtime_error("Failed while running spike-dasm");

    // Read "spike-dasm" output
    int insn_disasm = 0;
    char str[1024];
    while (fgets(str, 1024, stream) == str)
    {
        if (insn_disasm == insn_count) throw std::runtime_error("Read more disasm than expected.");

        instruction * insn = insns[insn_disasm++];
        insn->set_mnemonic(str);
        insn_cache[insn->get_pc()] = insn;
    }
    pclose(stream);
    if (insn_disasm != insn_count) throw std::runtime_error("Read less disasm than expected.");

    auto jt = insn_cache.find(paddr);
    if(jt == insn_cache.end()) throw std::runtime_error("Internal error while updating the instruction cache.");

    instruction * insn = jt->second;
    current_inst = insn->get_enc();
    LOG(DEBUG, "Fetched %sinstruction from PC 0x%016llx (PA: 0x%010llx): 0x%08x (%s)", insn->get_is_compressed() ? "compressed " : "", current_pc, paddr, current_inst, insn->get_mnemonic().c_str());
    return insn;
}
