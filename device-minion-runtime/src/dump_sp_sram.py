#!/usr/bin/python3

import sys, os

def read_hex_line(file):    
    line = file.readline().strip()
    while line.startswith("//"):
        line = file.readline().strip()
    return line

def decode_line(hex, data_bytes):
    if 36 != len(hex):
        print("Invalid string length! ({})".format(hex))
        sys.exit(-1)
    if(16 != len(data_bytes)):
        print("data_bytes bytearray length is not 16! ({})")
        sys.exit(-1)

    parity_bits = 0
    for n in range(0, 16):
        bit_offset = n * 9
        start_byte = (int)(bit_offset / 8)
        start_bit = bit_offset % 8
        start_hex_offset = start_byte * 2

        # print("start_byte:", start_byte)
        # print("start_hex_offset:", start_hex_offset)
        hex_0 = hex[34  - start_hex_offset: 36 - start_hex_offset]
        byte_0 = int(hex_0, 16)
        hex_1 = hex[32  - start_hex_offset: 34 - start_hex_offset]
        byte_1 = int(hex_1, 16)
        if start_hex_offset <= 30:
            hex_2 = hex[30  - start_hex_offset: 32 - start_hex_offset]
            byte_2 = int(hex_2, 16)
        else:
            hex_2 = "XX"
            byte_2 = 0

        # print(hex_0, hex_1, hex_2)

        value_24 = byte_0 | (byte_1 << 8) | (byte_2 << 16)
        value_9 = (value_24 >> start_bit) & 0x1FF

        value_8 = value_9 & 0xFF
        parity = value_9 >> 8

        # print("{0:02x}, P{1}".format(value_8, parity))

        parity_bits = parity_bits | (parity << n)
        data_bytes[n] = value_8

    return parity_bits

def dump_region(bin_file, hex_files, address):
    eof = False
    data_bytes = bytearray(16)
    while False == eof:
        for n in range(0,4 ):
            line = read_hex_line(hex_files[n])
            if "" == line:
                eof = True
                break
            
            parity_bits = decode_line(line, data_bytes)
            hex_bytes = ""
            str_bytes = ""
            for b in data_bytes:
                hex_bytes = hex_bytes + " {0:02x}".format(b)
                if 32 <= b and b < 127:
                    str_bytes = str_bytes + chr(b)
                else:
                    str_bytes = str_bytes + "."

            print("{0:08x}.{1}: {2} {3} {4:04x}  {5}".format(address, n, line, hex_bytes, parity_bits, str_bytes))
            address += 16

            try:
                bin_file.write(data_bytes)
            except:
                print(ex)
                print("Error writing to bin file!")
                sys.exit(-1)

def dump(bin_files, hex_files):
    dump_region(bin_files[0], hex_files[0:4],   0x40400000)
    # dump_region(bin_files[1], hex_files[4:8],   0x40440000)
    # dump_region(bin_files[2], hex_files[8:12],  0x40480000)
    # dump_region(bin_files[3], hex_files[12:16], 0x404C0000)

def usage():
    print("Usage:", sys.argv[0], "<output_bin_file_prefix> <input_hex_file_prefix>")
    sys.exit(-1)

try:
    from bitstring import BitArray
except ImportError as error:
        print("You don't have bitstring installed. pip3 install bitstring")

if len(sys.argv) != 3:
    usage()
    sys.exit(-1)

hex_files = []
for n in range(0, 16):
    try:
        filename = "{0}_{1:1X}.hex".format(sys.argv[2], n)
        hex_files.append(open(filename, 'r'))
    except Exception as ex:
        print(ex)
        print("Could not open input file {}".format(filename))
        sys.exit(-1)
    print("Opened {0} for reading".format(filename))

print()

bin_files = []
for n in range(0, 4):
    try:
        filename = "{0}_{1:1X}.bin".format(sys.argv[1], n)
        bin_files.append(open(filename, 'wb'))
    except Exception as ex:
        print(ex)
        print("Could not open output file {}".format(filename))
        sys.exit(-1)
    print("Opened {0} for writing".format(filename))

print()

dump(bin_files, hex_files)
