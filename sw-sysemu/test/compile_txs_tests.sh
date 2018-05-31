#!/usr/bin/bash

g++ -mf16c -O3 -I../ -I$SGPU/1.iassembly/ -I./ ../emu.c ../txs.c ../tbox_emu.c ../cvt.c ../log.c ../ipc.c $SGPU/1.iassembly/gio.cpp txs_test.c -o txs_test

