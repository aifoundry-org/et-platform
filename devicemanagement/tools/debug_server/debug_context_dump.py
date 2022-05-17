import os
import errno
import sys
from datetime import datetime

minion_defs = {
    "MM_SHIRE_IDX":32,
    "CM_SHIRE_IDX":0,
    "HART_IDX_START":0,
    "HART_IDX_END":1
}

context_defs = {
    "DUMP_GPRS":0x01,
    "DUMP_CSRS":0x02,
    "DUMP_ESRS":0x04,
    "DUMP_STACK_FRAME":0x08,
    "DUMP_DATA_STRUCTS":0x10,
    "DUMP_MM_SMODE_TRACE_DATA":0x20,
    "DUMP_CM_SMODE_TRACE_DATA":0x40
}

def setup():

    print('Running GDB from: %s\n'%(gdb.PYTHONDIR))
    print ("%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%")
    print ("Setting up GDB Environment...")
    gdb.execute("set width 0")
    gdb.execute("set height 0")
    gdb.execute("set verbose off")
    # Do not ask for any confirmations - doing this to work around pesty confirmation on quit
    gdb.execute("set confirm off")

    gdb.execute("show architecture")
    gdb.execute("show configuration")

    gdb.execute("set debug remote 0")

    gdb.execute("set trace-commands on")
    gdb.execute("set print pretty")

    gdb.execute("set remotetimeout 6")

    gdb.execute("set tdesc filename target.xml")
    gdb.execute("show tdesc filename")

    print ("Input args to python GDB script...")
    shire_mask = arg0
    thread_mask = arg1
    context_mask = arg2
    shire_index = arg3
    thread_index = arg4

    print ("Connecting to ETSoC Target...")
    gdb.execute("target remote :51000")

    print('\nSetup complete !!\n')

    return shire_mask, thread_mask, context_mask, shire_index, thread_index


def debug_session(shire_mask, thread_mask, context_mask, shire_index, thread_index):
    print("Debug session:start...")

    #Obtain debug data
    d = {}
    file = open("debug_data.txt")
    for x in file:
        f = x.split(":")
        d.update({f[0].strip(): f[1].strip()})

    #for key, value in d.items():
    #    print(key, ' : ', value)

    #Log file managmeent
    shire_name = "shire" + str(shire_index)
    hart_name = "hart" + str(thread_index)
    dir_root = "logs" + "/" + shire_name
    if not os.path.exists(dir_root):
        os.makedirs(dir_root)
    log_file_name = dir_root + '/' + hart_name + ".log"
    gdb.execute('set logging file %s'%(log_file_name))
    gdb.execute('set logging on')

    #Load the kernel elf file to load for now, will be improved.
    symbol_file_path = "file" + " " + d["SYMBOL_TABLE_FILE_PATH"]
    if d["SYMBOL_TABLE_FILE_PATH"]:
        gdb.execute(symbol_file_path)

    #Dump context based on user selection
    if (context_mask & context_defs["DUMP_GPRS"]) != 0:
        if(((context_mask & context_defs["DUMP_CSRS"]) !=0) == False):
            gdb.execute("info registers")

    if (context_mask & context_defs["DUMP_CSRS"]) != 0:
        gdb.execute("info all-registers")

    if (context_mask & context_defs["DUMP_STACK_FRAME"]) != 0:
        gdb.execute("bt")
        gdb.execute("info frame")

    if (context_mask & context_defs["DUMP_MM_SMODE_TRACE_DATA"]) != 0:
        if(shire_index == minion_defs["MM_SHIRE_IDX"]) and (thread_index == minion_defs["HART_IDX_START"]):
            mm_smode_trace_file = "logs" + '/' + "mm_smode_trace" + ".bin"
            dump_mm_trace_data_command = "dump binary memory" + " " + mm_smode_trace_file + " " + d["MM_SMODE_TRACE_BUFF_START"] + " " + d["MM_SMODE_TRACE_BUFF_END"]
            print dump_mm_trace_data_command
            gdb.execute(dump_mm_trace_data_command)

    if (context_mask & context_defs["DUMP_CM_SMODE_TRACE_DATA"]) != 0:
        if(shire_index == minion_defs["CM_SHIRE_IDX"]) and (thread_index == minion_defs["HART_IDX_START"]):
            cm_smode_trace_file = "logs" + '/' + "cm_smode_trace" + ".bin"
            dump_cm_trace_data_command = "dump binary memory" + " " + cm_smode_trace_file + " " + d["CM_SMODE_TRACE_BUFF_START"] + " " + d["CM_SMODE_TRACE_BUFF_END"]
            print dump_cm_trace_data_command
            gdb.execute(dump_cm_trace_data_command)

    #Terminate debug session
    gdb.execute("quit")
    print("Debug session:end...")

def main():
    shire_mask, thread_mask, context_mask, shire_index, thread_index = setup()
    debug_session(shire_mask, thread_mask, context_mask, shire_index, thread_index)

main()