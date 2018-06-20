#!/usr/bin/bash

g++ -mf16c -O0 -I../ -I $SGPU/include -I ./ ../emu.c ../ttrans.c ../rbox.c ../cvt.c ../log.c ../ipc.c $SGPU/src/gio.cpp rbox_test.c triangle_setup.c tile_rast.c -o rbox_test

