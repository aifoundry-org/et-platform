#!/usr/bin/bash

g++ -mf16c -O0 -I../ -I $SGPU/1.iassembly/ -I ./ ../emu.c ../rbox.c ../cvt.c ../log.c ../ipc.c $SGPU/1.iassembly/gio.cpp rbox_test.c triangle_setup.c tile_rast.c -o rbox_test

