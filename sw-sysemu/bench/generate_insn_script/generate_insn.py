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

Returns
instruction_list - list of strings(instructions)

This function retrieves the instruction set that belongs to the rv64i category. It then constructs a template string that we will populate with the instruction operator and its associated operands. 
It will repeat this for the number of instructions we want to generate as specified. It will return a list of instruction strings that would be written to our test file.
"""
def generate_insn(insn_count, category):
    instruction_set = instruction_table[category]
    operators = list(instruction_set.keys())
    instruction_list = []

    for count in range(insn_count):

        random_operator = random.choice(operators)
        instruction_string = 'asm volatile("' + random_operator + ' '
        operand_count = len(instruction_set[random_operator])
        op_list = []

        # Create Template String
        for count in range(operand_count):
            if count == operand_count - 1:
                instruction_string += '{}");'
            else:
                instruction_string += '{}, '
        
        # Match operands from lookup and populate with values        
        for operand in instruction_set[random_operator]:
            if operand == 'rs1' or operand == 'rs2' or operand == 'rd':
                op_list.append(random.choice(instruction_table['registers']))

            elif operand == 'imm':
                op_list.append(random.randrange(0, 1500))
        
        instruction_list.append(instruction_string.format(*op_list))
    
    instruction_list.insert(0, category.upper())
    return instruction_list


"""
Args: 
instruction_groups_list - a list storing list(s) of instruction strings

This function writes our generate instruction sequence strings to a C file named
generated_inst_seq.c. Note that it will rewrite the contents each time.
"""
def write_to_test_file_C(instruction_groups_list):
    test_file = open("generated_inst_seq.c", "w")
    test_file.write('#include "macros.h"\n\n')
    test_file.write('int main() {\n')

    for instruction_list in instruction_groups_list:
        category = '/* {} */\n'.format(instruction_list[0])
        test_file.write(category)
        for insn in instruction_list[1:]:
            test_file.write('\t')
            test_file.write(insn)
            test_file.write("\n")
    
    test_file.write('}')


if __name__ == "__main__":
    instruction_groups_list = []
    total_insn = args.count

    instruction_groups_list.append(generate_insn(int(total_insn), 'rv64i'))
    instruction_groups_list.append(generate_insn(int(total_insn), 'rv64m'))

    write_to_test_file_C(instruction_groups_list)
