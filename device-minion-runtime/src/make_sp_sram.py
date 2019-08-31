#!/usr/bin/python3

import sys, os

def get_parity(byte_val):
    ones_count = 0
    for bit_index in range(8):
        mask = 1 << bit_index
        if 0 != (byte_val & mask):
            ones_count += 1
    if 0 == (ones_count & 1):
        return 0
    else:
        return 1

def write_bit(temp_bytes, bit_value, bit_offset):
    byte_index = bit_offset // 8
    in_byte_offset = bit_offset % 8
    if 0 != bit_value:
        # set the bit
        temp_bytes[byte_index] = temp_bytes[byte_index] | (1 << in_byte_offset)
    else:
        # clear the bit
        temp_bytes[byte_index] = temp_bytes[byte_index] & ~(1 << in_byte_offset)
    
def write_byte(temp_bytes, byte_value, bit_offset):
    for bit_index in range(8):
        bit_value = byte_value & 1
        byte_value = byte_value >> 1
        write_bit(temp_bytes, bit_value, bit_offset + bit_index)

def encode_line(hex, data_bytes):
    if 36 != len(hex):
        print("Invalid string length! ({})".format(hex))
        sys.exit(-1)
    if(16 != len(data_bytes)):
        print("data_bytes bytearray length is not 16! ({})")
        sys.exit(-1)

    temp_bytes = bytearray(18)
    parity_bits = 0

    for n in range(0, 16):
        bit_offset = n * 9
        byte_value = data_bytes[n]
        parity_bit = get_parity(byte_value)
        write_byte(temp_bytes, byte_value, bit_offset)
        write_bit(temp_bytes, parity_bit, bit_offset + 8)
        parity_bits = parity_bits | (parity_bit << n)

    for n in range(0, 18):
        hex_str = "{0:02X}".format(temp_bytes[17-n])
        hex[2*n] = ord(hex_str[0])
        hex[2*n+1] = ord(hex_str[1])

    return parity_bits

def make_region(hex_files, bin_file, address):
    eol = bytearray(1)
    eol[0] = ord('\n')
    line = bytearray(36)
    for line_no in range(0, 4096):
        for n in range(0,4):
            try:
                data_bytes = bin_file.read(16)
            except:
                print(ex)
                print("Error reading bin file!")
                sys.exit(-1)
                    
            parity_bits = encode_line(line, data_bytes)

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
                hex_files[n].write(line)
                hex_files[n].write(eol)
            except:
                print(ex)
                print("Error writing to hex file!")
                sys.exit(-1)

def make(hex_files, bin_files):
    make_region(hex_files[0:4],   bin_files[0], 0x40400000)
    # make_region(hex_files[4:8],   bin_files[1], 0x40440000)
    # make_region(hex_files[8:12],  bin_files[2], 0x40480000)
    # make_region(hex_files[12:16], bin_files[3], 0x404C0000)

def usage():
    print("Usage:", sys.argv[0], "<output_hex_file_prefix> <input_bin_file_prefix>")
    sys.exit(-1)

try:
    from bitstring import BitArray
except ImportError as error:
        print("You don't have bitstring installed. pip3 install bitstring")

if len(sys.argv) != 3:
    usage()
    sys.exit(-1)


bin_files = []
for n in range(0, 4):
    try:
        filename = "{0}_{1:1X}.bin".format(sys.argv[2], n)
        bin_files.append(open(filename, 'rb'))
    except Exception as ex:
        print(ex)
        print("Could not open input file {}".format(filename))
        sys.exit(-1)
    print("Opened {0} for reading".format(filename))

print()

hex_files = []
for n in range(0, 16):
    try:
        filename = "{0}_{1:1X}.hex".format(sys.argv[1], n)
        hex_files.append(open(filename, 'wb'))
    except Exception as ex:
        print(ex)
        print("Could not open output file {}".format(filename))
        sys.exit(-1)
    print("Opened {0} for writing".format(filename))

print()

make(hex_files, bin_files)
