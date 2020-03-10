#!/bin/tcsh

# Script to generate code + data images for kernel based on mem_desc.txt
# Generates images and sends them to Zebu run directories.

# Still things to do:
# Point to the proper master/worker/machine in mem_desc.txt (for sysemu) === DONE
# == put correct memdesc2hex in ../../../host_sw/scripts/zebu/elf2hex == TODO
# Run and call script that generates random data files based on mem_desc.txt (to avoid copying hex files) == DONE
#  Probably we can avoid memdesc2hex if we generate hex files (for zebu) and raw binaries (for sysemu) == NO NEED
# Do not use relative directories, or have an option to pass sw-platform top dir == DONE

# Test this flow -- this script has not been tested at all.
# The rest of the changes hgave been tested.
# Rebase or start from a clean branch and apply all changes.

# if ($#argv==3) then
#   set TOP_DIR = $3
#else
   set TOP_DIR = /projects/esperanto/$user/sw-platform
#endif

set ELF2HEX_FILE=$TOP_DIR/host_sw/scripts/zebu/elf2hex
set MEMDESC2HEX_FILE=$PWD/memdesc2hex
set GENZEBUMEM_FILE=$TOP_DIR/host_sw/scripts/zebu/genZebuMem.pl

if (! -f $ELF2HEX_FILE) then
   echo "File $ELF2HEX_FILE not found"
   exit
endif

if (! -f $MEMDESC2HEX_FILE) then
   echo "File $MEMDESC2HEX_FILE not found"
   exit
endif

if (! -f $GENZEBUMEM_FILE) then
   echo "File $GENZEBUMEM_FILE not found"
   exit
endif

set CUR_DIR=$PWD
set TARGET_DIR=$TOP_DIR/build/device-software/test-compute-kernels/$1/$1_$2

cp $ELF2HEX_FILE $TARGET_DIR/elf2hex
cp $MEMDESC2HEX_FILE $TARGET_DIR/memdesc2hex
cp $GENZEBUMEM_FILE $TARGET_DIR/genZebuMem.pl

python3 ./gen_random_params.py --data 1 --data_size 5120 --random_data_file data.raw
python3 ./gen_random_params.py --data 1 --data_size 1024 --random_data_file out.raw
python3 ./gen_random_params.py --data 1 --data_size 2048 --random_data_file in.raw

cp data.raw $TARGET_DIR/data.raw
cp out.raw $TARGET_DIR/out.raw 
cp in.raw $TARGET_DIR/in.raw

set ELF2HEX=./elf2hex
set MEMDESC2HEX=./memdesc2hex
set GENZEBUMEM=./genZebuMem.pl

cd $TARGET_DIR

cp memImage_derow0_even.hex memImage_derow0_even.hex.orig
cp memImage_derow0_odd.hex memImage_derow0_odd.hex.orig
cp memImage_derow1_even.hex memImage_derow1_even.hex.orig
cp memImage_derow1_odd.hex memImage_derow1_odd.hex.orig
cp memImage_derow2_even.hex memImage_derow2_even.hex.orig
cp memImage_derow2_odd.hex memImage_derow2_odd.hex.orig
cp memImage_derow3_even.hex memImage_derow3_even.hex.orig
cp memImage_derow3_odd.hex memImage_derow3_odd.hex.orig
cp memImage_dwrow0_even.hex memImage_dwrow0_even.hex.orig
cp memImage_dwrow0_odd.hex memImage_dwrow0_odd.hex.orig
cp memImage_dwrow1_even.hex memImage_dwrow1_even.hex.orig
cp memImage_dwrow1_odd.hex memImage_dwrow1_odd.hex.orig
cp memImage_dwrow2_even.hex memImage_dwrow2_even.hex.orig
cp memImage_dwrow2_odd.hex memImage_dwrow2_odd.hex.orig
cp memImage_dwrow3_even.hex memImage_dwrow3_even.hex.orig
cp memImage_dwrow3_odd.hex memImage_dwrow3_odd.hex.orig

$ELF2HEX $1_$2.elf $1_$2.hex
$MEMDESC2HEX > data.hex
cat $1_$2.hex data.hex > memImage.hex
$GENZEBUMEM -synopsys memImage.hex

# Now copy everything to the Zebu directories
# This is assumed to be /projects/esperanto/$user/zebu/ -- but it may change...
set DEVICE_FW_BUILD_DIR=$TOP_DIR/build/device-software/device-minion-runtime/src/
cp $DEVICE_FW_BUILD_DIR/MasterMinion/memImage_*hex /projects/esperanto/$user/zebu/run/master/.
cp $DEVICE_FW_BUILD_DIR/WorkerMinion/memImage_*hex /projects/esperanto/$user/zebu/run/worker/.
cp $DEVICE_FW_BUILD_DIR/MachineMinion/memImage_*hex /projects/esperanto/$user/zebu/run/machine/.
cp memImage_*hex /projects/esperanto/$user/zebu/run/kernel/.

cd -

