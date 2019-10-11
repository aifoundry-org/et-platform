#!/usr/bin/python3

import sys
from struct import *

sp_rom_max_size = 0x20000
sp_rom_bank_width = 8
sp_rom_banks_x_count = 8
sp_rom_banks_y_count = 2

def get_bank_file_name(prefix, x, y):
    return "{0}_{1}_{2}.hex".format(prefix, y, x)

def get_bank_index(x, y):
    return y * sp_rom_banks_x_count + x

def convert_rom_bin_to_hex(binary_path, hex_path_prefix):
    try:
        binary_file = open(binary_path, "rb")
    except:
        print("Unable to open binary file '{:s}' for reading!".format(binary_path))
        sys.exit(-1)

    try:
        binary_data = binary_file.read()
    except:
        print("Error reading binary file!")
        sys.exit(-1)
    
    binary_data_len = len(binary_data)
    print("ROM file size: {0} (0x{1:x})".format(binary_data_len, binary_data_len))
    if binary_data_len > sp_rom_max_size:
        print("Invalid binary file size!")
        sys.exit(-1)

    hex_files = [None] * (sp_rom_banks_x_count * sp_rom_banks_y_count)
    for y in range(sp_rom_banks_y_count):
        for x in range(sp_rom_banks_x_count):
            file_index = get_bank_index(x, y)
            file_path = get_bank_file_name(hex_path_prefix, x, y)
            try:
                hex_files[file_index] = open(file_path, "w")
            except:
                print("Unable to open ROM HEX file {0} for writing!".format(file_path))
                sys.exit(-1)
            print("Created file '{0}'.".format(file_path))

    physical_rom = bytearray(sp_rom_max_size)
    physical_rom[0:binary_data_len] = binary_data

    offset = 0
    for y in range(sp_rom_banks_y_count):
        for row in range(1024):
            for x in range(sp_rom_banks_x_count):
                file_index = get_bank_index(x, y)
                (value_u64,) = unpack_from("<Q", physical_rom, offset)
                #print("value_u64:", value_u64)
                try:
                    print("{0:016X}".format(value_u64), file=hex_files[file_index])
                except:
                    print("Error writing to file!")
                    sys.exit(-1)
                offset += sp_rom_bank_width
    
    for file in hex_files:
        try:
            file.close()
        except:
            print("Error closing file!")
            sys.exit(-1)

    print("ROM HEX files saved.")


def convert_zebu_to_bin(binary_path, mem_filepaths_list):
    print("convert_zebu_to_bin not implmented!")

def usage():
    print("Usage: maske_sp_rom_for_tapeout.py <ROM_BIN_FILE> <ROM_HEX_FILE_PREFIX>")
    sys.exit(-1)

if len(sys.argv) != 3:
    usage()

convert_rom_bin_to_hex(sys.argv[1], sys.argv[2])
