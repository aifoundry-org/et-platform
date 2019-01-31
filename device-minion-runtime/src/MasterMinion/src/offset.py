#!/usr/bin/python

# Utility to add an address offset to a ZeBu format hex dump
import sys

if len(sys.argv) < 4:
    print "Usage:", sys.argv[0], "offset <infile> <outfile>"
    sys.exit(-1)

# Base 0 to guess base from prefix - needs 0x prefix for hex
offset = int(sys.argv[1], 0)

try:
    inFile = open(sys.argv[2], 'r')
except IOError:
    print "Could not open", sys.argv[2]
    sys.exit(-1)

try:
    outFile = open(sys.argv[3], 'w')
except IOError:
    print "Could not open", sys.argv[3]
    sys.exit(-1)

print "Adding offset 0x{:010x} to".format(offset), sys.argv[2]

for line in inFile:
    address, data = [int(x,16) for x in line.split()]
    print >> outFile, "@{:010x} {:08x}".format(address + offset, data) # Add leading @ for ZeBu

print "Wrote", sys.argv[3]