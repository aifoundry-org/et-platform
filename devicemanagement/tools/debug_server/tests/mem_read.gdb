# Setting width, height and verbose for batch mode
set width 0
set height 0
set verbose off

# Enabling logging
set logging file gdb_log.out
set logging on

# Disable debugging traces
set debug remote 0

# Attaching to the GDB server
target remote :51000

# Currently we are only reading memory address not writing and then validating after read back becasue
# currently memory write is not supported. Test need to be updated when memory write is implimented.
# TODO: Write to memory and validate after reading

printf "Reading memory at address 0x8005802064\n"
x /x 0x8005802064
