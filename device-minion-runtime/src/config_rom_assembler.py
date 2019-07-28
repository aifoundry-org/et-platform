import argparse
from array import *
import ctypes
import yaml

#keep this in sync with boot_config.c
memSpaces = [
    {
        'name': 'R_SP_CRU_BASEADDR',
        'baseAddr': 0x0052028000,
        'size': 0x1000,
        'memSpace': 1
    },
    {
        'name': 'R_PCIE_ESR_BASEADDR',
        'baseAddr': 0x0058200000,
        'size': 0x1000,
        'memSpace': 2
    },
    {
        'name': 'R_PCIE_APB_SUBSYS_BASEADDR',
        'baseAddr': 0x0058400000,
        'size': 0x0000200000,
        'memSpace': 3
    },
    {
        'name': 'R_PCIE0_DBI_SLV_BASEADDR',
        'baseAddr': 0x7e80000000,
        'size': 0x0080000000,
        'memSpace': 4
    }
]

def parse_address(address, verbose):
    for memSpace in memSpaces:
        minAddr = memSpace['baseAddr']
        maxAddr = minAddr + memSpace['size']

        if address >= minAddr and address < maxAddr:
            offset = address - minAddr

            if verbose: print("Addr 0x%x is %s + offset %d" % (address, memSpace['name'], offset))

            return memSpace['memSpace'], offset

    #searched all memSpaces without finding a match
    raise AttributeError("Addr 0x%x is not valid to use from bootloader config ROM" % address)

    return 0, 0

def parse_file(input_data, outfile, verbose):
    out_array = bytearray()

    for op in input_data['ops']:
        if verbose: print(op)

        if op['op'] == 'w':
            opCode = 1
            memSpace, offset = parse_address(op['address'], verbose)
            
            dw0 = opCode << 28 | memSpace << 24 | offset
            dw1 = op['writeVal']
            dw2 = op['mask'] if 'mask' in op else 0xFFFFFFFF
        elif op['op'] == 'rmw':
            opCode = 2
            memSpace, offset = parse_address(op['address'], verbose)

            dw0 = opCode << 28 | memSpace << 24 | offset
            dw1 = op['writeVal']
            dw2 = op['mask']
        elif op['op'] == 'poll':
            opCode = 3
            memSpace, offset = parse_address(op['address'], verbose)

            dw0 = opCode << 28 | memSpace << 24 | offset
            dw1 = op['desiredVal']
            dw2 = op['mask']
        elif op['op'] == 'wait':
            opCode = 4
            dw0 = opCode << 28
            dw1 = op['tickCount']
            dw2 = 0
        else:
            raise AttributeError("Invalid op", op['op'])

        if verbose: print("Out: 0x%08x 0x%08x 0x%08x" % (dw0, dw1, dw2))
        
        out_array += dw0.to_bytes(4, byteorder='little', signed=False)
        out_array += dw1.to_bytes(4, byteorder='little', signed=False)
        out_array += dw2.to_bytes(4, byteorder='little', signed=False)

    #Insert terminator op
    out_array += (0xFFFFFFFF).to_bytes(4, byteorder='little', signed=False)
    out_array += (0xFFFFFFFF).to_bytes(4, byteorder='little', signed=False)
    out_array += (0xFFFFFFFF).to_bytes(4, byteorder='little', signed=False)

    with open(outfile, 'wb') as f:
        f.write(out_array)
    return

if __name__ == "__main__":
    parser = argparse.ArgumentParser()

    parser.add_argument("--infile",
                        required=True,
                        help='Path to input file')
    parser.add_argument("--outfile",
                        required=True,
                        help='Output path of instructions after phy is loaded')
    parser.add_argument("--verbose",
                        required=False,
                        default=False,
                        help='Trace operation')
    args = parser.parse_args()

    # Read YAML file
    with open(args.infile, 'r') as stream:
        input_data = yaml.safe_load(stream)

    parse_file(input_data, args.outfile, args.verbose)