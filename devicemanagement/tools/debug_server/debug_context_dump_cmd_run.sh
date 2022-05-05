# Uncomment below to enable umode hang loop dump scenario,
# and remember to provide symbol file path for hang kernel in debug_data.txt:SYMBOL_TABLE_FILE_PATH
#cd ../../../firmwareTests
#ctest -R FunctionalTestDevOpsApiKernelCmds.hpSq_abortCommandHangKernelSingleDeviceSingleQueue &
#sleep 2
#cd ../deviceManagement/tools/debug_server
#./debug_context_dump.sh -s 0x0000000000000001 -t 0x0000000000000001 -c 0x0000006B -n 0


#umode idle dump scenario
./debug_context_dump.sh -s 0x0000000100000001 -t 0x8000000000000001 -c 0x0000000B -n 0
