#!/usr/bin/python3

import sys
from struct import *

sp_otp_size = 8192
physical_bit_index_adjustment = 16218

def save_zebu_otp_file(zebu_path_prefix, index, data, offset, count):
    # print("zebu_path_prefix arguments:")
    # print("zebu_path_prefix =", zebu_path_prefix)
    # print("index =", index)
    # print("data =", len(data), ":", data)
    # print("offset =", offset)
    # print("count =", count)

    filename = "{0}_{1}.hex".format(zebu_path_prefix, index)
    try:
        out_file = open(filename, "w")
    except:
        print("Error opening file '{0}' for writing!")
        sys.exit(-1)

    for word_index in range(offset, offset + count):
        value = unpack_from("<H", data, word_index * 2)[0]
        #print("@{0:02x} {1:04x}{2:04x}".format(word_index, value, value), file = out_file)
        print("{0:04x}{1:04x}".format(value, value), file = out_file)
    
    out_file.close()
    print("Saved '{0}'.".format(filename))

def convert_bin_to_zebu(binary_path, zebu_path_prefix):
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
    
    if len(binary_data) != (sp_otp_size // 8):
        print("Invalid binary file size!")
        sys.exit(-1)
    
    physical_pages = [ bytearray(512), bytearray(512), bytearray(512) ]

    for virtual_word_index in range(256):
        virtual_word_value = unpack_from("<I", binary_data, 4 * virtual_word_index)[0]
        for virtual_bit_in_word_offset in range(32):
            bit_value = virtual_word_value & (1 << virtual_bit_in_word_offset)
            if 0 != bit_value:
                virtual_bit_index = virtual_bit_in_word_offset + 32 * virtual_word_index
                physical_bit_index = 8192 - virtual_bit_index + physical_bit_index_adjustment
                physical_bit_index_within_word = physical_bit_index % 16
                physical_word_index = physical_bit_index // 16
                physical_bit_value = 1 << physical_bit_index_within_word
                physical_page_index = physical_word_index // 256
                if physical_page_index < 3 or physical_page_index > 5:
                    print("Error: physical page index out of 3..5 range!")
                    sys.exit(-1)
                physical_word_index_in_page = physical_word_index % 256

                # print("setting virtual bit {0} (word {1} bit {2} as physical bit {3} (page {4} word {5} bit {6})".format(virtual_bit_index, virtual_word_index, virtual_bit_in_word_offset, physical_bit_index, physical_page_index, physical_word_index_in_page, physical_bit_index_within_word))

                old_physical_word_value = unpack_from("<H", physical_pages[physical_page_index - 3], physical_word_index_in_page * 2)[0]
                new_physical_word_value = old_physical_word_value | physical_bit_value
                
                # print("Changed word from {0:04x} to {1:04x}".format(old_physical_word_value, new_physical_word_value))
                
                pack_into("<H", physical_pages[physical_page_index - 3], physical_word_index_in_page * 2, new_physical_word_value)

    #save_zebu_otp_file(zebu_path_prefix, 3, physical_pages[0], 245, 256 - 245)
    save_zebu_otp_file(zebu_path_prefix, 3, physical_pages[0], 0, 256)
    save_zebu_otp_file(zebu_path_prefix, 4, physical_pages[1], 0, 256)
    #save_zebu_otp_file(zebu_path_prefix, 5, physical_pages[2], 0, 246)
    save_zebu_otp_file(zebu_path_prefix, 5, physical_pages[2], 0, 256)

def convert_zebu_to_bin(binary_path, mem_filepaths_list):
    print("convert_zebu_to_bin not implmented!")

def usage():
    print("Usage: sp_otp_convert.py <command> [arguments...]")
    print("")
    print("       sp_otp_convert.py B2Z <binary_file> <zebu_files_prefix>")
    print("       sp_otp_convert.py Z2B <mem_3.hex> <mem_4.hex> <mem_5.hex> <binary_file>")
    sys.exit(-1)

if len(sys.argv) < 2:
    usage()

if sys.argv[1] == "B2Z":
    if len(sys.argv) != 4:
        usage()
    
    convert_bin_to_zebu(sys.argv[2], sys.argv[3])

elif sys.argv[1] == "Z2B":
    if len(sys.argv) != 6:
        usage()
    
    convert_zebu_to_bin(sys.argv[5], { sys.argv[2], sys.argv[3], sys.argv[4] })

else:
    usage()
