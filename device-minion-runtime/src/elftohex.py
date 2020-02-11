#!/usr/bin/python3
# Utility to convert an elf file to ZeBu hex files to preload SP RAM (inc. parity), SP ROM, AXI DRAM or DDR DRAM
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
# The 128KB of ROM is organized into two contiguous 64KB blocks
# A 64-byte cache line is spread across 8 panels: bytes 0-7 in panel 0, 8-15 in 1, etc.
# addr[2:0] byte in panel
# addr[5:3] panel
# addr[15:6] line
# addr[16] block
#
# DRAM is spread across 16 memory controllers in 8 memory shires
# Each memory controller deals with 64-byte cache line sized transactions
# addr[5:0] byte in panel
# addr[8:6] memshire
# addr[9] controller (0=even, 1=odd)
#
# If using old DDR DRAM models, additional address bit swizzling is implemented:
# Mesh  Synopsys DDR    Comment
# addr  dword addr
# bit   bit
# 0     -               Byte address bit 0
# 1     -               Byte address bit 1
# 2     -               Byte address bit 2
# 3     0               Byte address bit 3 (double-word address bit 0)
# 4     1               Byte address bit 4 (double-word address bit 1)
# 5     24              DDR bank address bit 0
# 6     -               Memshire number (bit 0) 000 = dwrow[0], 100 = derow[0]
# 7     -               Memshire number (bit 1) 001 = dwrow[1], 101 = derow[1]
# 8     -               Memshire number (bit 2) 010 = dwrow[2], 110 = derow[2]
# 9     even/odd        Memory controller
# 10    25              DDR bank address bit 1
# 11    2
# 12    3
# 13    4
# 14    26              DDR bank address bit 2
# 15    8
# 16    9
# 17    10
# 18    11
# 19    12
# 20    13
# 21    14
# 22    15
# 23    16
# 24    17
# 25    18
# 26    19
# 27    20
# 28    21
# 29    5
# 30    6
# 31    7
# 32    22
# 33    23

# If using new DDR DRAM models, additional address bit swizzling is implemented:
# Mesh  Synopsys DDR    Comment
# addr  dword addr
# bit   bit
# 0     -               Byte address bit 0
# 1     -               Byte address bit 1
# 2     -               Byte address bit 2
# 3     0               Byte address bit 3 (double-word address bit 0)
# 4     1               Byte address bit 4 (double-word address bit 1)
# 5     2               Byte address bit 5 (double-word address bit 2)
# 6     -               Memshire number (bit 0) 000 = dwrow[0], 100 = derow[0]
# 7     -               Memshire number (bit 1) 001 = dwrow[1], 101 = derow[1]
# 8     -               Memshire number (bit 2) 010 = dwrow[2], 110 = derow[2]
# 9     even/odd        Memory controller
# 10    24              DDR bank address bit 0
# 11    25              DDR bank address bit 1
# 12    26              DDR bank address bit 2
# 13    3
# 14    4
# 15    5
# 16    6
# 17    7
# 18    8
# 19    9
# 20    10
# 21    11
# 22    12
# 23    13
# 24    14
# 25    15
# 26    16
# 27    17
# 28    18
# 29    19
# 30    20
# 31    21
# 32    22
# 33    23
import argparse
import sys, os

try:
    from elftools.elf.elffile import ELFFile
except ImportError as error:
        print("You don't have pyelftools installed. pip3 install pyelftools")
        raise error

try:
    from bitstring import BitArray
except ImportError as error:
        print("You don't have bitstring installed. pip3 install bitstring")
        raise error

inFile = None
elfFile = None

outFileNames = []
outFiles = []

def open_output_files(prefix, suffix, files, cmd_line_args):
    if cmd_line_args.output_file:
        (name,ext) = os.path.splitext(args.output_file) # Use outfile arg for output filename
    else:
        (name,ext) = os.path.splitext(args.infile) # Use infile arg for output filename

    try:
        for i in range(files):
            filename = "%s%s%d%s.hex" % (name, prefix, i, suffix)
            outFileNames.append(filename)
            if not cmd_line_args.print_output_files:
                outFiles.append(open(filename, 'w'))
    except Exception as e:
        print(f"Could not open output files: {e}")
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

# basic operation on a bitfield
# bitfield - an array of 18 bytes
# offset - which bit (0-143) in the bitfield should be set to 1
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

# basic operation on a bitfield
# bitfield - an array of 18 bytes
# offset - bit offset (0-137) where the 8-bit byte value should be written to the bitfield
# val - value to be written
# the function assumes that the bitfield was initially set to ALL ZEROS!  It does NOT clear any bits, only sets bits to 1.
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

# adds parity bits to an array of bytes
# x - array of 16 bytes
# return value - array of 18 bytes where each original 8 bits has a 1 parity bit added
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

def write_hex(baseAddress, bytesPerZebuRow, inputBytesPerPanel, outputBytesPerPanel, panelsPerLine, maxLinesPerFile, parity, ddr):
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
            #print("1: write_lines(base_address=0x{0:x} line_address=0x{1:x} bytesPerZebuRow={2:d} inputBytesPerPanel={3:d} panelsPerLine={4:d}".format(baseAddress, lineAddress, bytesPerZebuRow, inputBytesPerPanel, panelsPerLine))
            write_lines(bytes, baseAddress, lineAddress, bytesPerZebuRow, inputBytesPerPanel, panelsPerLine, maxLinesPerFile, parity, ddr)
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
    #print("2: write_lines(base_address=0x{0:x} line_address=0x{1:x} bytesPerZebuRow={2:d} inputBytesPerPanel={3:d} panelsPerLine={4:d}".format(baseAddress, lineAddress, bytesPerZebuRow, inputBytesPerPanel, panelsPerLine))
    write_lines(bytes, baseAddress, lineAddress, bytesPerZebuRow, inputBytesPerPanel, panelsPerLine, maxLinesPerFile, parity, ddr)

# requires address is aligned to the beginning of a line - will not zero pad before
# will zero pad the end of a line
def write_lines(bytes, baseAddress, address, bytesPerZebuRow, inputBytesPerPanel, panelsPerLine, maxLinesPerFile, parity, ddr):
    inputBytesPerLine = inputBytesPerPanel * panelsPerLine
    #print("Writing lines from %08x to %08x" % (address, address + len(bytes)))

    # For each line, set all panel bytes including parity even if input data isn't available
    for lineIndex in range(0, len(bytes), inputBytesPerLine):

        # For each panel in the line, generate the .hex file output lines
        for panel in range(0, panelsPerLine):
            panelIndex = panel * inputBytesPerPanel

            # Some panels require multiple ZeBu output rows per line, e.g. when using DDR DRAM each memory
            # controller is a 64 byte panel but the memory init hex file requires 8 bytes per row.
            for rowIndex in range(0, inputBytesPerPanel, bytesPerZebuRow):
                # ZeBu style address. Address 0 is at base of memory
                if (ddr == "DDR"):
                    # DDR DRAM models require swizzled address bits
                    writeAddress = mesh_addr_to_synopsys_ddr_addr(address + lineIndex + rowIndex - baseAddress)
                elif (ddr == "DDR_NEW"):
                    # Newer DDR DRAM models require different swizzled address bits
                    writeAddress = mesh_addr_to_new_synopsys_ddr_addr(address + lineIndex + rowIndex - baseAddress)
                else:
                    writeAddress = (address + lineIndex + rowIndex - baseAddress) // inputBytesPerLine

                if maxLinesPerFile > 0:
                    fileIndex = writeAddress // maxLinesPerFile
                    writeAddress = writeAddress % maxLinesPerFile
                    fileIndex = fileIndex * panelsPerLine
                else:
                    fileIndex = 0

                wl = "@%010x " % (writeAddress)

                startIndex = lineIndex + panelIndex + rowIndex
                stopIndex = startIndex + bytesPerZebuRow

                # For SP RAM, bytes 0-15 for panel 0, 16-31 for panel 1, etc.
                # For SP ROM, bytes 0-7 for panel 0, 8-15 for panel 1, etc.
                # If we've run out of input data, substitute 0.
                outputBytes = [bytes[x] if (x) < len(bytes) else 0 for x in range(startIndex, stopIndex)]

                if parity:
                    outputBytes = add_parity_bytes(outputBytes)

                # Append outputBytes in reverse order MSByte to LSByte
                for byte in reversed(outputBytes):
                    wl += ("%02x" % byte)

                wl += "\n"
                outFiles[fileIndex + panel].write(wl)
    return

def mesh_addr_to_synopsys_ddr_addr(addr):
    mesh_addr = BitArray(uint=addr, length=36)
    ddr_addr = BitArray(uint=0, length=36)

    # BitArray insists on indexing the MSB as 0, so reverse for natural indexing
    mesh_addr.reverse()

    # python slice indices are [m:n+1] for bits [m:n]
    ddr_addr[0:2]   = mesh_addr[3:5]
    ddr_addr[2:5]   = mesh_addr[11:14]
    ddr_addr[5:8]   = mesh_addr[29:32]
    ddr_addr[8:10]  = mesh_addr[15:17]
    ddr_addr[10:22] = mesh_addr[17:29]
    ddr_addr[22:24] = mesh_addr[32:34]
    ddr_addr[24]    = mesh_addr[5]
    ddr_addr[25]    = mesh_addr[10]
    ddr_addr[26]    = mesh_addr[14]

    # Undo previous reverse before converting back to int
    ddr_addr.reverse()

    return ddr_addr.int

def mesh_addr_to_new_synopsys_ddr_addr(addr):
    mesh_addr = BitArray(uint=addr, length=36)
    ddr_addr = BitArray(uint=0, length=36)

    # BitArray insists on indexing the MSB as 0, so reverse for natural indexing
    mesh_addr.reverse()

    # python slice indices are [m:n+1] for bits [m:n]
    ddr_addr[0:3]   = mesh_addr[3:6]
    ddr_addr[3:24]   = mesh_addr[13:34]
    ddr_addr[24:27]   = mesh_addr[10:13]

    # Undo previous reverse before converting back to int
    ddr_addr.reverse()

    return ddr_addr.int

def sp_rom_write_hex(cmd_line_args):
    open_output_files("_lo_", "", 8, cmd_line_args)
    open_output_files("_hi_", "", 8, cmd_line_args)
    if cmd_line_args.print_output_files:
        print(" ".join(outFileNames))
        return
    #write_hex(baseAddress, bytesPerZebuRow, inputBytesPerPanel, outputBytesPerPanel, panelsPerLine, maxLinesPerFile, parity, ddr)
    write_hex(0x40000000, 8, 8, 8, 8, 1024, False, False)
    return

def sp_ram_write_hex(cmd_line_args):
    open_output_files("SP_RAM", "", 4, cmd_line_args)
    if cmd_line_args.print_output_files:
        print(" ".join(outFileNames))
        return
    write_hex(0x40400000, 16, 16, 18, 4, 0, True, False)
    return

def dram_write_hex(ddr, cmd_line_args):
    open_output_files("_dwrow", "_even", 4, cmd_line_args) # memshire 0-3 "west 0-3" addr[9] = 0 "even"
    open_output_files("_derow", "_even", 4, cmd_line_args) # memshire 4-7 "east 0-3" addr[9] = 0 "even"
    open_output_files("_dwrow", "_odd", 4, cmd_line_args)  # memshire 0-3 "west 0-3" addr[9] = 1 "odd"
    open_output_files("_derow", "_odd", 4, cmd_line_args)  # memshire 4-7 "east 0-3" addr[9] = 1 "odd"
    if cmd_line_args.print_output_files:
        print(" ".join(outFileNames))
        return
    if (ddr == "DDR") or (ddr == "DDR_NEW"):
        # DDR models with address swizzling and 8 bytes per ZeBu hex line
        write_hex(0x8000000000,  8, 64, 64, 16, 0, False, ddr)
    else:
        # AXI models with 64 bytes per ZeBu hex line
        write_hex(0x8000000000, 64, 64, 64, 16, 0, False, ddr)
    return


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("ram_type",
                        choices=["ROM", "SP_RAM","PU_RAM", "DRAM","DDR", "DDR_NEW"],
                        help="Type of memory to generate the hex file for")
    parser.add_argument("infile",
                        help="Path to the input file")
    parser.add_argument("--output-file",
                        required=False,
                        default=None,
                        help="Path to output file")
    parser.add_argument("--print-output-files",
                        action='store_true',
                        required=False,
                        help="If set then only print the names of output that would be generated")
    args = parser.parse_args()

    if not args.print_output_files:
        try:
            inFile = open(sys.argv[2], 'rb')
            elfFile = ELFFile(inFile)
        except:
            print("Could not open", sys.argv[2])
            sys.exit(-1)

    if args.ram_type == "ROM":
        sp_rom_write_hex(args)
    elif args.ram_type == "SP_RAM":
        sp_ram_write_hex(args)
    elif args.ram_type == "PU_RAM":
        print("PU_RAM support not yet implemented")
        sys.exit(-1)
    elif args.ram_type == "DRAM":
        dram_write_hex(args.ram_type, args)
    elif args.ram_type == "DDR":
        dram_write_hex(args.ram_type, args)
    elif args.ram_type == "DDR_NEW":
        dram_write_hex(args.ram_type, args)
    else:
        print("Unsupported argument {}".format(args.ram_type))
        sys.exit(-1)

    if not args.print_output_files:
        inFile.close()
        for i in outFiles:
            i.close()
