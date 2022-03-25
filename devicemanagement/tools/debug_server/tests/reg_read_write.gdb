# Setting width, height and verbose for batch mode
set width 0
set height 0
set verbose off

# Enabling logging
set logging file gdb_log.out
set logging on

# Disable debugging traces
set debug remote 0

# Attaching to the server
target remote :51000

# Reading the registers value
info register

# Setting register a0 to 0x1
set $a0 = 0x1

# Print value of register a0
print $a0

# Check if the value of the register is set to expected value
if($a0 == 0x1)
  info register
else
  # Generate error and test failure
  quit 1