#!/bin/bash
NUM_SHIRES=33
NUM_THREADS=64

script_name=$(basename $0)

function usage()
{
   cat << HEREDOC

   Usage: $script_name [-s SHIRE_MASK -t THREAD_MASK -c CONTEXT_MASK -n INDEX -x FILEPATH]

   optional arguments:
     -h, --help                                  show this help message and exit
     -s, --shire_mask   SHIRE_MASK               pass in 64 bit shire_mask in hex  (default: 0x0000000000000001)
     -t, --thread_mask  THREAD_MASK              pass in 64 bit thread mask in hex (default: 0x0000000000000001)
     -c, --context_mask CONTEXT_MASK             pass in 32 bit debug context mask in hex (default: 0x00000001)
                                                 Bit 0: Registers - GPRs
                                                 Bit 1: Registers - CSRs
                                                 Bit 2: Registers - ESRs
                                                 Bit 3: Stack Frame
                                                 Bit 4: Data Structures
                                                 Bit 5: MM S Mode Trace Data
                                                 Bit 6: CM S Mode Trace Data
     -n, --device_index INDEX                    ETSoC1 device index on which the debug server is to be started (default: 0)
     -x, --file_path FILEPATH                    Metadata file path consisting of details of FW data to be dumped

HEREDOC
}

while true; do
  case "$1" in
    -h | --help ) usage; exit; ;;
    -s | --shire_mask ) let "shire_mask = $2"; shift 2 ;;
    -t | --thread_mask ) let "thread_mask = $2"; shift 2 ;;
    -c | --context_mask ) let "context_mask = $2"; shift 2 ;;
    -n | --device_index ) let "device_index = $2"; shift 2 ;;
    -x | --file_path ) file_path=$2; shift 2 ;;
    -- ) shift; break ;;
    * ) break ;;
  esac
done

#let "shire_mask = 0x0000000000000001"; let "thread_mask = 0x0000000000000001"; let "context_mask = 0x00000001"; let "device_index = 0";

#let "s_mask = $1"
#let "t_mask = $2"

echo "Input args:"
printf 'Shire mask = 0x%x\n' $shire_mask
printf 'Thread mask = 0x%x\n' $thread_mask
printf 'Context mask = 0x%x\n' $context_mask
echo "Device Index = "$device_index

#: <<'END'
for ((i=0; i < $NUM_SHIRES; i++))
do
    let "curr_bit = $((1 << $i))"
    let "shire_curr_bit = curr_bit"
    #echo $shire_curr_bit
    #printf '%x\n' $shire_mask
    #printf '%x\n' $curr_bit
    if [[ $(($shire_mask & $curr_bit)) -ne 0 ]]; then
        #echo "Shire Passed"
        echo "##############################"
        for ((j=0; j < $NUM_THREADS; j++))
        do
            let "curr_bit = $((1 << $j))"
            let "thread_curr_bit = curr_bit"
            #echo $thread_curr_bit
            #printf '%x\n' $thread_mask
            #printf '%x\n' $curr_bit
            if [[ $(($thread_mask & $curr_bit)) -ne 0 ]]; then
                echo "******************************"
                echo "Launching Debug session with;"
                let "shire_index = $i"
                let "thread_index = $j"
                echo "Shire index = " $shire_index
                echo "Thread index = " $thread_index

                # Launch Debug Server
                ./debug-server -s $shire_index -m $thread_curr_bit -n $device_index -x $file_path &

                # Start client with context_dump.gdb script
                #sleep 1

                /esperanto/minion/bin/riscv64-unknown-elf-gdb -ex 'py arg0='$shire_mask'; arg1='$thread_mask'; arg2='$context_mask'; arg3='$shire_index'; arg4='$thread_index'' -x debug_context_dump.py
            #else
                #echo "Thread failed"
            fi
        done

    #else
        #echo "Shire failed"
    fi

    #echo "**********************************"
done

date_time_stamp="$(date +"%m-%d-%Y-%H-%M-%S")"

if [[ -d "logs" ]];then
    mv logs $date_time_stamp
    if [[ -d "latest_log" ]];then
        rm latest_log
    fi
    ln -s $date_time_stamp latest_log
fi
#END
