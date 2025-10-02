import yaml
import random
import argparse

with open('table_config.yml', 'r') as lookup_table:
    instruction_table = yaml.safe_load(lookup_table)

parser = argparse.ArgumentParser(description='Generate random RISC-V instructions.')
parser.add_argument('-c', '--count', default=1)      # number of instructions 
args = parser.parse_args()


"""
Args: 
insn_count - the number of instructions we want to generate for this category
Write to file - this is an optional argument to write the sequence to a C file

Returns
instruction_list - list of strings(instructions)

This function retrieves the instructions from the global RISC V green book/look up table. It then constructs a template string that we will populate with the instruction operator and its associated operands. It will repeat this for the number of instructions we want to generate as specified. It will return a list of instruction strings that would be written to our instruction sequence which will be used to test SW simulator (SysEMU).
"""
def generate_insn(insn_count, category, include_c_code=True, add_float_params=False):
    instruction_set, instruction_list = instruction_table[category], []
    operators = list(instruction_set.keys())

    for count in range(insn_count):
        random_operator = random.choice(operators)
        instruction_string = ''

        if include_c_code:
            instruction_string += 'asm volatile ("' + random_operator + ' '
        else:
            instruction_string += random_operator + ' '

        operand_count = len(instruction_set[random_operator])
        op_list = []

        # Create Template String
        for count in range(operand_count):
            if count == operand_count - 1:
                if include_c_code:
                    instruction_string += '{}");'
                    if add_float_params:
                        instruction_string = instruction_string[:-2] + generate_float_params_helper(operand_count) + ");"
                else:
                    instruction_string += '{}'                    
            else:
                instruction_string += '{}, '
        
        # Match operands from lookup and populate with values
        for operand in instruction_set[random_operator]:
            if operand == 'rs1' or operand == 'rs2' or operand == 'rd':
                op_list.append(random.choice(instruction_table['registers']))
            elif operand == 'imm':
                op_list.append(random.randrange(1, 100))
            elif operand == 'offset':
                op_list.append(random.choice(instruction_table['offsets']))
            elif '%' in operand:
                op_list.append(operand)

        instruction_list.append(instruction_string.format(*op_list))

    instruction_list.insert(0, category.upper())
    return instruction_list

def generate_float_params_helper(count):
    params_str = (' : "=f" (output): ')
    for op in range(count - 1):
            if op == count - 2:
                params_str += ('"f" (f{})').format(op + 1)
            else:
                params_str += ('"f" (f{}),').format(op + 1)
    return params_str

def generate_tensor_insns(total_insn):
    instruction_list = []
    tensor_instruction_dict = {
        0 : "tensor_load(%d, %d, %d, %d, %d, %s, %d, %d, %d, %d);\n" % (0,        # use_tmask
                                                                        0,        # use_coop
                                                                        0,       # dst_start
                                                                        7,        #transformation
                                                                        0,        # use_tenb
                                                                        "0x8100000000 + hart_id * 0x4000",#addr
                                                                        0,        #offset
                                                                        15,                          #num_lines
                                                                        0,        #stride
                                                                        0),     #id
        
        1 : "tensor_fma(%d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %s, %d);\n" % (0,     # use_tmask
                                                                                    random.randint(1, 14),   # b_num_col
                                                                                    random.randint(1, 14),   # a_num_rows
                                                                                    random.randint(1, 14),   # a_num_cols
                                                                                    random.randint(1, 10),    # offset
                                                                                    0,    # tenc_loc
                                                                                    0,    # tenb_unsigned
                                                                                    0,    # tena_unsigned
                                                                                    0,    # tenb_loc
                                                                                    0,    # scp_loc_b
                                                                                    0,    # scp_loc_a
                                                                                    "011",                   # opcode
                                                                                    0),   # first_pass

        2 : "tensor_reduce_uint32(%d, %d, %d, %d);\n" % (0,        # value
                                                         0,         # operation
                                                         0,        # partnerID
                                                         0),         # action

        3 : "tensor_reduce_float(%d, %d, %d, %d, %d);\n" % (0,      # freg  
                                                            0,      # operation
                                                            random.randint(0, 10),     # num_reg
                                                            0,    # partnerId
                                                            0),      # action

        4 : "tensor_reduce(%d, %d, %d, %d, %d);\n" % (0,        # start_reg
                                                      0,        # operation
                                                      random.randint(0, 10),       # num_reg
                                                      0,      # partnerId
                                                      0),        # action

        5 : "tensor_reduce_send(%d, %d, %d);\n" % (0,           # start_reg
                                                   random.randint(0, 10),          # num_reg
                                                   0),        # partnerID
        
        6: "tensor_wait(thread_id);\n"
    }
    # load tensor
    instruction_list.append("\t\ttensor_wait(thread_id);\n")
    instruction_list.append("\t\t" + tensor_instruction_dict[0])
    # instruction_list.append("\t\ttensor_wait(thread_id);\n")
    for _ in range(total_insn):
        random_instruction = random.randint(1, 5)
        # if(random_instruction == 1):
        #     instruction_list.append("\t\ttensor_wait(thread_id);\n")
        #     instruction_list.append("\t\t" + tensor_instruction_dict[0])
        instruction_list.append("\t\ttensor_wait(thread_id);\n")
        instruction_list.append("\t\t" + tensor_instruction_dict[random_instruction])
    instruction_list.append("\t\ttensor_wait(thread_id);\n")
    return instruction_list

def write_scratchpad_setup(file_write):
    evict_dcache = r"""
static inline void evict_dcache(void)
{
    register uint64_t set asm("a7");
    for(set = 0; set < L1D_NUM_SETS; set++)
    {
        // use_tmask=0, dst=1 (L2/SP_RAM), set=X, way=0, num_lines=15
        __asm__ __volatile__(
            // Wait for previous memory accesses to finish
            "fence\n"
            // Evict L1 Dcache: EvictSW for the 4 ways
            "csrw evict_sw, %0\n"
            "addi %0, %0, 64\n"
            "csrw evict_sw, %0\n"
            "addi %0, %0, 64\n"
            "csrw evict_sw, %0\n"
            "addi %0, %0, 64\n"
            "csrw evict_sw, %0\n"
            "addi %0, %0, 64\n"
            // Wait for the evicts to complete
            "csrwi tensor_wait, 6\n"
            : 
            : "r"((1ull << 58) + ((set & 0xF) << 14) + 15ull)
            : "memory");
    }
	set = 0;
}
"""
    setup_cache =  """
void setup_cache_scp(){
    // PRM-8: Cache Control Extension
    EXCL_MODE(1);
    // Evict the whole L1$
    evict_dcache();
    // Shared Mode
    MCACHE_CONTROL(0, 0, 0, 0);
    WAIT_CACHEOPS;
    // D1Split Mode
    MCACHE_CONTROL(0, 0, 0, 1);
    WAIT_CACHEOPS;
    // Scratchpad Mode
    MCACHE_CONTROL(0, 0, 1, 1);
    WAIT_CACHEOPS;
    EXCL_MODE(0);
}
"""
    file_write.write(evict_dcache)
    file_write.write(setup_cache)

"""
Args: 
instruction_groups_list - a list storing list(s) of instruction strings

This function writes our generate instruction sequence strings to a C file named
generated_inst_seq.c. Note that it will rewrite the contents each time.

Example usage:
  instruction_groups_list is the list of instruction sequences generated by generate_insn function
  Pass the generated instruction list to the write_to_test_file_C function to write to "generated_inst_seq.c" file.
  For example:
  instruction_groups_list = [generate_insn(5, 'example_category', include_c_code=True), generate_insn(5, 'another_category', include_c_code=True)]
  write_to_test_file_C(instruction_groups_list)
"""
category_files = {
    'rv64i' : "../device_kernels/src/rv64i.c",
    'rv64f' : "../device_kernels/src/rv64f.c",
    'rv64m' : "../device_kernels/src/rv64m.c",
    'tensors' : "../device_kernels/src/tensors.c",
    'rv64a' : "../device_kernels/src/rv64a.c",
    'rv64d' : "../device_kernels/src/rv64d.c",
}
def write_to_test_file_C(instruction_groups_list, category=None):

    include_str = '#include "macros.h"\n#include "etsoc/isa/tensors.h"\n#include "etsoc/isa/cacheops.h"\n#include "etsoc/isa/hart.h"\n'
    f1 = random.uniform(0.0, 100.0)
    f2 = random.uniform(0.0, 100.0)
    f3 = random.uniform(0.0, 100.0)

    with open(category_files[category], "w") as test_file:
        test_file.write(include_str)
        if category == 'tensors':
            write_scratchpad_setup(test_file)
        test_file.write('\nint main() {\n')
        test_file.write('\tfloat f1 = {}, f2 = {}, f3 = {}, output;\n'.format(f1, f2, f3))
        test_file.write('\t(void)f1;\n\t(void)f2;\n\t(void)f3;\n\t(void)output;\n')
        if category != 'tensors':
            for instruction_list in instruction_groups_list:
                category_comment = '/* {} */\n'.format(instruction_list[0])
                test_file.write(category_comment)
                
                for insn in instruction_list[1:]:
                    indented_insn = '\t' + insn + '\n'
                    test_file.write(indented_insn)

        if category == 'tensors':
            # tensor instructions
            test_file.write('/* {} */\n'.format("Tensors"))
            for _ in range(1):
                if _ != 0:
                    test_file.write("\ttensor_wait(thread_id);\n")
                test_file.write('\tevict_dcache();\n')
                test_file.write('\tsetup_cache_scp();\n')
                if _ == 0:
                    test_file.write('\tint hart_id = get_hart_id();\n')
                    test_file.write('\tint thread_id = get_thread_id();\n')
                else:
                    test_file.write('\thart_id = get_hart_id();\n')
                    test_file.write('\tthread_id = get_thread_id();\n')
                if _ != 0:
                    test_file.write("\ttensor_wait(thread_id);\n")
                test_file.write('\tif(thread_id == 0) {\n')
                for tensor_insn in generate_tensor_insns(200):
                    test_file.write(tensor_insn)
                test_file.write('\t}\n')
        test_file.write('}')


if __name__ == "__main__":
    # Create an empty list to store instruction groups
    instruction_groups_list = []

    # Get the total instruction count from command-line arguments (assuming 'args' is defined)
    total_insn = args.count

    # Generate instruction sequences for different instruction categories

    # RV64I
    # Base integer instructions
    instruction_groups_list.append(generate_insn(int(total_insn), 'rv64i'))
    write_to_test_file_C(instruction_groups_list, 'rv64i')
    instruction_groups_list = []

    # RV64M
    #Integer multiplication and division instructions.
    instruction_groups_list.append(generate_insn(int(total_insn), 'rv64m'))
    write_to_test_file_C(instruction_groups_list, 'rv64m')
    instruction_groups_list = []

    # RV64A
    #Atomic instructions for atomic memory operations.
    instruction_groups_list.append(generate_insn(int(total_insn), 'rv64a', True, True))
    write_to_test_file_C(instruction_groups_list, 'rv64a')
    instruction_groups_list = []

    #RV64Fs
    #Single-precision floating-point instructions.
    instruction_groups_list.append(generate_insn(int(total_insn), 'rv64f', True, True))
    write_to_test_file_C(instruction_groups_list, 'rv64f')
    instruction_groups_list = []

    #RV64D
    #Double-precision floating-point instructions.
    # instruction_groups_list.append(generate_insn(int(total_insn), 'rv64d', True, True))

    #RV64C
    #Compressed instructions for reduced code size.

    #RV64G
    #General extension that includes RV64I and the standard extensions (M, A, F, D, C).

    #RV64IM
    #Integer base and integer multiplication/division extension.

    #RV64GC
    #General extension with compressed instructions.

    #RV64GC_F
    #General extension with compressed and single-precision floating-point instructions.

    #RV64GC_D
    #General extension with compressed and double-precision floating-point instructions.

    #Tensors
    write_to_test_file_C(generate_tensor_insns(200), 'tensors')

    # Write the generated instruction sequences to "generated_inst_seq.c" file
    # write_to_test_file_C(instruction_groups_list)
