#!/usr/bin/python3

import sys
import csv

def print_file_prolog():
    print("#ifndef __MOVEELLUS_PLL_MODES_CONFIG_H__")
    print("#define __MOVEELLUS_PLL_MODES_CONFIG_H__")
    print("")
    print("#include <stdint.h>")
    print("")

def print_file_epilog():
    print("")
    print("#endif")
    print("")

def print_struct():
    print("typedef struct PLL_MODE_CFG_s {")
    print("    uint8_t mode;")
    print("    uint8_t count;")
    print("    const uint8_t * offsets;")
    print("    const uint16_t * values;")
    print("} PLL_SETTING_t;")
    print("")

def print_array_prolog():
    print("static const PLL_SETTING_t gs_pll_settings[] = {")

def print_array_epilog():
    print("};")

def is_digit(character):
    character_code = ord(character)
    if character_code < ord("0") or character_code > ord("9"):
        return False
    else:
        return True

def get_mode_from_filename(filename):
    # assume that the filename uses the following pattern
    # xxxxxxxmodeNNN.csv

    filename_length = len(filename)
    if filename_length < 5:
        print("filename '{0}' too short!".format(filename))
        sys.exit(-1)
    
    start_index = filename_length - 4
    if filename[start_index:filename_length] != ".csv":
        print("filename '{0}' does not end with '.csv'!".format(filename))
        sys.exit(-1)

    end_index = start_index    
    start_index -= 1
    character = filename[start_index:start_index+1]
    if not is_digit(character):
        print("filename '{0} does not contain a valid number before the '.csv' postfix!".format(filename))
        sys.exit(-1)
    
    found_start_index = False
    while start_index >= 0:
        start_index -= 1
        character = filename[start_index:start_index+1]
        character_code = ord(character)
        if character_code < ord("0") or character_code > ord("9"):
            start_index += 1
            found_start_index = True
            break

    number_str = filename[start_index:end_index]
    number = int(number_str)
    return number

def print_array_entry(filename, final):
    mode = get_mode_from_filename(filename)
    csv_file = open(filename)
    csv_reader = csv.reader(csv_file, delimiter=',')
    line_count = 0
    offsets = []
    values = []

    for row in csv_reader:
        if line_count == 0:
            # print(f'Column names are {", ".join(row)}')
            line_count += 1
        else:
            offsets.append(row[0])
            values.append(row[1])
            line_count += 1

    print("    {{ // {0}".format(filename))
    print("      .mode = {0},".format(mode))
    print("      .count = {0},".format(len(values)))
    print("      .offsets = {", end = '')
    index = 0
    for offset in offsets:
        print(" 0x{0:02x}".format(int(offset, 0)), end = '')
        index += 1
        if index < len(offsets):
            print(",", end = '')
            if 0 == (index % 9):
                print("")
                print("                  ", end='')
    print("")
    print("      },")

    print("      .values = {", end = '')
    index = 0
    for value in values:
        print(" 0x{0:04x}".format(int(value, 0)), end = '')
        index += 1
        if index < len(values):
            print(",", end = '')
            if 0 == (index % 9):
                print("")
                print("                 ", end='')
    print("")
    print("      }")

    print("    }", end = '')
    if final:
        print("")
    else:
        print(",")

if len(sys.argv) < 2:
    print("Usage: {0} [<source_csv_file> [...]]".format(sys.argv[0]))
    sys.exit(-1)

print_file_prolog()
print_struct()
print_array_prolog()

for file_index in range(1, len(sys.argv)):
    filename = sys.argv[file_index]
    if file_index == (len(sys.argv) - 1):
        final = True
    else:
        final = False

    print_array_entry(filename, final)

print_array_epilog()
print_file_epilog()
