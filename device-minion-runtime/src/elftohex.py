#!/usr/bin/python3
# Utility to convert an elf file to ZeBu hex files to preload SP RAM (inc. parity), SP ROM or DRAM
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
# addr[2:0] byte in panel
# addr[5:3] panel
#
# DRAM is spread across 16 memory controllers in 8 memory shires
# Each memory controller deals with 64-byte cache line sized transactions
# addr[5:0] byte in panel
# addr[8:6] memshire
# addr[9] controller (0=even, 1= odd)

import sys, os
from elftools.elf.elffile import ELFFile

if len(sys.argv) < 3:
    print("Usage:", sys.argv[0], "ROM|SP_RAM|PU_RAM|DRAM infile [outfile]")
    sys.exit(-1)

try:
    inFile = open(sys.argv[2], 'rb')
    elfFile = ELFFile(inFile)
except:
    print("Could not open", sys.argv[2])
    sys.exit(-1)

outFiles = []

def open_output_files(prefix, suffix, files):
    if len(sys.argv) >= 3:
        (name,ext) = os.path.splitext(sys.argv[3]) # Use outfile arg for output filename
    else:
        (name,ext) = os.path.splitext(sys.argv[2]) # Use infile arg for output filename

    try:
        for i in range(files):
            outFiles.append(open("%s_%s%d%s.hex" % (name, prefix, i, suffix), 'w'))
    except:
        print("Could not open output files")
        sys.exit(-1)

    return

# graphics.stanford.edu/~seander/bithacks.html#ParityParallel
def even_parity(x):
    if not 0 <= x <= 255:
        print("illegal byte value", x)
        sys.exit(-1)
    x ^= x >> 4
    x &= 0xf
    return (0x6996 >> x) & 1

def set_bit_in_bitfield(bfield, offset):
    if not len(bfield) == 18:
        print("set_bit_in_bitfield: illegal list length", x)
        sys.exit(-1)
    if offset < 0 or 144 <= offset:
        print("set_bit_in_bitfield: illegal offset", offset)
        sys.exit(-1)
    
    byte_index = int(offset / 8)
    bit_index = offset % 8

    bfield[byte_index] |= (1 << bit_index)

def set_byte_in_bitfield(bfield, offset, val):
    if not len(bfield) == 18:
        print("set_byte_in_bitfield: illegal list length", x)
        sys.exit(-1)
    if offset < 0 or (144-7) <= offset:
        print("set_byte_in_bitfield: illegal offset", offset)
        sys.exit(-1)

    for bit_index in range(8):
        bit_val = val & (1 << bit_index)
        if 0 != bit_val:
            set_bit_in_bitfield(bfield, offset + bit_index)

def add_parity_bytes(x):
    if not len(x) == 16:
        print("add_parity: illegal list length", x)
        sys.exit(-1)
    
    result = [0] * 18
    offset = 0
    for i in x:
        set_byte_in_bitfield(result, offset, i)
        if even_parity(i):
            set_bit_in_bitfield(result, offset + 8)
        offset += 9

    return result

def write_hex(baseAddress, inputBytesPerPanel, outputBytesPerPanel, panelsPerLine, parity):
    inputBytesPerLine = inputBytesPerPanel * panelsPerLine
    bytes = []
    firstSegment = True

    # Accumulate segments on contiguous lines. When a gap is detected, write out lines for accumulated segments.
    for seg in elfFile.iter_segments():
        segAddr = seg['p_paddr']
        segSize = seg['p_filesz']
        segEndAddress = segAddr + segSize
        print("Segment start: %x size: %x" % (segAddr, segSize))

        # If this segment starts on a later line than the previous segment ended on,
        # write out the lines for the previous segment(s) and start a new group of lines
        if (not firstSegment and ((segAddr // inputBytesPerLine) > (prevSegEndAddress // inputBytesPerLine))):
            write_lines(bytes, baseAddress, lineAddress, inputBytesPerPanel, panelsPerLine, parity)
            bytes = []
            firstSegment = True

        # The first segment may start unaligned: zero-pad the lineAddress to segAddr gap, if any.
        if (firstSegment):
            lineAddress = (segAddr // inputBytesPerLine) * inputBytesPerLine
            bytes.extend([0] * (segAddr - lineAddress))
            firstSegment = False

        # Subsequent segments that start on the same line aren't necessarily contiguous: zero-pad the gap, if any.
        elif (segAddr > prevSegEndAddress):
            bytes.extend([0] * (segAddr - prevSegEndAddress))

        bytes.extend(seg.data())
        prevSegEndAddress = segEndAddress

    # write out lines after last segment
    write_lines(bytes, baseAddress, lineAddress, inputBytesPerPanel, panelsPerLine, parity)

# requires address is aligned to the beginning of a line - will not zero pad before
# will zero pad the end of a line
def write_lines(bytes, baseAddress, address, inputBytesPerPanel, panelsPerLine, parity):
    inputBytesPerLine = inputBytesPerPanel * panelsPerLine

    print("Writing lines from %08x to %08x" % (address, address + len(bytes)))

    # For each output row, set all panels including parity even if input data isn't available
    for offset in range(0, len(bytes), inputBytesPerLine):
        for panel in range(panelsPerLine):
            # ZeBu style address. Address 0 is at base of memory, address 1 is the next line, etc.
            wl = "@%010x " % ((address + offset - baseAddress) // inputBytesPerLine)

            # For SP RAM, bytes 0-15 for panel 0, 16-31 for panel 1, etc.
            # For SP ROM, bytes 0-7 for panel 0, 8-15 for panel 1, etc.
            # If we've run out of input data, substitute 0.
            outputBytes = [bytes[offset + x] if (offset + x) < len(bytes) else 0 for x in range(panel*inputBytesPerPanel, (panel+1)*inputBytesPerPanel)]

            if parity:
                outputBytes = add_parity_bytes(outputBytes)

            # Append outputBytes in reverse order, parity first then MSByte to LSByte
            for byte in reversed(outputBytes):
                wl += ("%02x" % byte)

            wl += "\n"
            outFiles[panel].write(wl)
    return

def sp_rom_write_hex():
    open_output_files("ROM", "", 8)
    write_hex(0x40000000, 8, 8, 8, False)
    return

def sp_ram_write_hex():
    open_output_files("SP_RAM", "", 4)
    write_hex(0x40400000, 16, 18, 4, True)
    return

def dram_write_hex():
    open_output_files("DRAM_W_", "_even", 4) # memshire 0-3 "west 0-3" addr[9] = 0 "even"
    open_output_files("DRAM_E_", "_even", 4) # memshire 4-7 "east 0-3" addr[9] = 0 "even"
    open_output_files("DRAM_W_", "_odd", 4)  # memshire 0-3 "west 0-3" addr[9] = 1 "odd"
    open_output_files("DRAM_E_", "_odd", 4)  # memshire 4-7 "east 0-3" addr[9] = 1 "odd"
    write_hex(0x8000000000, 64, 64, 16, False)
    return

if (sys.argv[1]) == "ROM":
    sp_rom_write_hex()
elif (sys.argv[1]) == "SP_RAM":
    sp_ram_write_hex()
elif (sys.argv[1]) == "PU_RAM":
    print("PU_RAM support not yet implemented")
    sys.exit(-1)
elif (sys.argv[1]) == "DRAM":
    dram_write_hex()
else:
    print("Unsupported argument", sys.argv[1])
    sys.exit(-1)
