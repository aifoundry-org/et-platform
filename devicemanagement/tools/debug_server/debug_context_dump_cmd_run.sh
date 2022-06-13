#*************************************************************************************************************
#  Additional configuration required for Usecase #1.
#  1. Provide symbol file path for hang kernel in debug_data.txt:SYMBOL_TABLE_FILE_PATH 

#  Additional configuration required when DUMP_DATA_STRUCTS bit is enabled in context mask(-c) of input argument
#  1. Please see file debug_dump_predef_data_structs_mem.txt. The file is in the format below
#    <StartAddress> <Size> <Access_Initiator> <Access_Type>
#      
#     StartAddress: Address of the FW data struct to be dumped.
#     Size : Size of the FW data struct
#     Access_Initiator: Hart ID should be specified 
#                       For CMs, Valid range is 0-2047 and 2080-2111.
#                       For MMs, Valid range is 2048-2079.
#                       For SP, Specify access initiator as SP.
#     Access_Type: GLOBAL_ATOMIC, LOCAL_ATOMIC, NORMAL
# 
#    Update the file with required information for the data structs to be dumped.   
# 
#  2. Add the aditional argument (-x <filepath>) to list of arguments below  
#    ex: ./debug_context_dump.sh -s 0x0000000000000001 -t 0x0000000000000001 -c 0x00000001B -n 0 
#        -x /mnt/esperanto/<user>/sw-platform/host-software/deviceManagement/tools/debug_server/debug_dump_predef_data_structs_mem.txt
#*************************************************************************************************************

#*************************************************************************************************************
# Usecase #1 
# Umode Hang Kernel loop dump 
#**************************************************************************************************************
#cd ../../../firmwareTests
#ctest -R FunctionalTestDevOpsApiKernelCmds.hpSq_abortCommandHangKernelSingleDeviceSingleQueue &
#sleep 2
#cd ../deviceManagement/tools/debug_server
#./debug_context_dump.sh -s 0x0000000000000001 -t 0x0000000000000001 -c 0x0000006B -n 0

#****************************************************************************************************************
# Usecase #2 
# Umode Idle dump
#  This usecase dumps the required data defined by the context mask (-c) option.

#*****************************************************************************************************************
./debug_context_dump.sh -s 0x0000000100000001 -t 0x8000000000000001 -c 0x0000001B -n 0
