Generate RISC-V Instruction Sequence
========

### Installation
Make sure you have Python3 installed.
We also use the following libraries:

    yaml
    random
    argparse

### Basic Usage
We can specify the number of instructions we want to generate.

    python3 generate_insn.py -c <count>

If no count is provided, it will default to 1.

Note that you can check the options available:

    python3 generate_insn.py --help

### Extending the Functionality

We define our RISC-V lookup table or green card in the table_config.yml YAML file.
At the top we have a list of the general purpose registers in RISC-V.

Next, we define each of the different categories, followed by the specific instructions that fall under these categories, followed by a list of the associated operands in the correct order.

We currently support this mapping:
    rd/rs1/rs2: registers
    imm : immediate

NOTE: Currently, we only have portions of the rv64i and rv64i instruction categories. In order to extend to include more types of instructions and/or specific instructions, add an entry for the particular instruction under its respective category along with a list of its operands. If the category or operand mapping you want to include is not present, please add them following the format. 