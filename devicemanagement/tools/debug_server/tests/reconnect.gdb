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

# Reading the registers value
info register

# Setting value of register a0 to 0x1
set $a0 = 0x1

# Disconnectiong from the debug server
disconnect

# Connecting to GDB server
target remote :51000

# Printing the value of a0 register
print $a0

# Check if the value of the a0 register is set to expected value after reconnecting to the GDB server
if($a0 == 0x1)
  info register
else
  # Generate error and test failure
  quit 1