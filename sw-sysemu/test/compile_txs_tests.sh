#!/usr/bin/bash

g++ -mf16c -O3 -I../ -I ../../../swr/gfx/include/ -I./ ../emu.cpp ../ttrans.cpp ../txs.cpp ../tbox_emu.cpp ../cvt.cpp ../log.cpp ../ipc.cpp ../emu_gio.cpp txs_test.c -o txs_test

