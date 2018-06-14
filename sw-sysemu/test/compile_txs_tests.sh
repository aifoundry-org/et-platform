#!/usr/bin/bash

g++ -mf16c -O3 -I../ -I$SGPU/include/ -I./ ../emu.c ../txs.c ../tbox_emu.c ../cvt.c ../log.c ../ipc.c $SGPU/src/gio.cpp txs_test.c -o txs_test

