#!/usr/bin/python3

import sys

if len(sys.argv) != 3:
    print("Usage: fill_file <hex_file> <binary_file>")
    sys.exit(-1)

file_name = sys.argv[1]

try:
    binary_file = open(sys.argv[2], "rb")
except:
    print("Unable to open binary file '{:s}' for reading!".format(sys.argv[2]))
    sys.exit(-1)

try:
    binary_data = binary_file.read()
except:
    print("Error reading binary file!")
    sys.exit(-1)

try:
    hex_file = open(sys.argv[1], "w")
except:
    print("Unable to open hex file '{:s}' for writing!".format(sys.argv[1]))
    sys.exit(-1)

offset = 0
for b in binary_data:
    print("@{:08x} {:02x}".format(offset, b), file = hex_file)
    offset += 1

hex_file.close()

print("Converted binary file '{:s}' to hex file '{:s}".format(sys.argv[2], sys.argv[1]))

sys.exit(0)
