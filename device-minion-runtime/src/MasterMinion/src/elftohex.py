#!/usr/bin/python
# Utility to convert an elf file to ZeBu hex files to preload SP RAM (inc. parity) or ROM
#
# The 1MB of SP RAM is organized into 4 contiguous 256KB blocks
# Each 256KB block is organized into 4 panels that store 18 bytes each
# (16 byte of data + 2 bytes of parity)
# A 64-byte cache line is spread across 4 panels: bytes 0-15 in 0, 16-31 in 1, etc.
# addr[3:0] byte in panel
# addr[5:4] panel
# addr[17:6] line
# addr[19:18] block
#
# The 64BK of ROM is organized into one contiguous block
# A 64-byte cache line is spread across 8 panels: bytes 0-7 in panel 0, 8-15 in 1, etc. 

import sys, os
from elftools.elf.elffile import ELFFile

if len(sys.argv) < 2:
    print "Usage:", sys.argv[0], "infile [outfile]"
    sys.exit(-1)

try:
    inFile = open(sys.argv[1], 'rb')
    elfFile = ELFFile(inFile)
except:
    print "Could not open", sys.argv[1]
    sys.exit(-1)

outFiles = []

def open_output_files(files):
    if len(sys.argv) >= 3:
        (name,ext) = os.path.splitext(sys.argv[2]) # Use outfile arg for output filename
    else:
        (name,ext) = os.path.splitext(sys.argv[1]) # Use infile arg for output filename

    try:
        for i in range(files):
            outFiles.append(open("%s%d.hex" % (name, i), 'w'))
    except:
        print "Could not open output files"
        sys.exit(-1)

    return

# graphics.stanford.edu/~seander/bithacks.html#ParityParallel
def even_parity(x):
    x ^= x >> 4
    x &= 0xf
    return (0x6996 >> x) & 1

def calc_parity_byte(x):
    parityByte = 0
    for i in range(8):
        if even_parity(x[i]) : parityByte |= (1 << i)
    return parityByte

def mem_write_seg(baseAddress, addr, size, inputBytesPerPanel, outputBytesPerPanel, panelsPerLine, parity):    
    inputBytesPerLine = inputBytesPerPanel * panelsPerLine

    # For each output row, set all 4 panels including parity even if input data isn't available
    for offset in range(0, size, inputBytesPerLine):
        for panel in range(panelsPerLine):
            # ZeBu style address. Address 0 is at base of memory, address 1 is the next line, etc. 
            wl = "@%010x " % ((addr + offset - baseAddress) / inputBytesPerLine)

            # For SP RAM, bytes 0-15 for panel 0, 16-31 for panel 1, etc.
            # pyelftools stores the raw bytes as str type, so ord() back to int for parity calculations
            # If we've run out of input range, substitute 0.
            outputBytes = [ord(seg.data()[offset + x]) if (offset + x) < size else 0 for x in range(panel*inputBytesPerPanel, (panel+1)*inputBytesPerPanel)]

            if (parity):
                outputBytes.append(calc_parity_byte(outputBytes[0:8])) # parity for lower 8 bytes
                outputBytes.append(calc_parity_byte(outputBytes[8:16])) # parity for upper 8 bytes

            # Append outputBytes in reverse order, parity first then MSByte to LSByte
            for byte in reversed(outputBytes):
                wl += ("%02x" % byte)

            outFiles[panel].write(wl)
            outFiles[panel].write("\n")
    return

def sp_rom_write_seg(addr, size):
    open_output_files(8)
    mem_write_seg(0x40000000, addr, size, 8, 8, 8, False)
    return

def sp_ram_write_seg(addr, size):
    open_output_files(4)
    mem_write_seg(0x40400000, addr, size, 16, 18, 4, True)
    return

for seg in elfFile.iter_segments():
    addr = seg['p_paddr']
    size = seg['p_filesz']
    print("Segment start: %x size: %x" % (addr, size))
    sp_ram_write_seg(addr, size)

