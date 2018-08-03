#!/usr/bin/bash

g++ -mf16c -O0 -I../ -I $SGPU/include -I ./ ../emu.cpp ../ttrans.cpp ../rbox.cpp ../cvt.cpp ../log.cpp ../ipc.cpp ../emu_gio.cpp rbox_test.c triangle_setup.c tile_rast.c -o rbox_test

