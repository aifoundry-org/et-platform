#ifndef MARKERS_H
#define MARKERS_H

#define START_WAVES_MARKER  __asm__ __volatile("slti x0,x0,0xfb");
#define START_PHASE_MARKER  __asm__ __volatile("slti x0,x0,0xaa");
#define END_PHASE_MARKER  __asm__ __volatile("slti x0,x0,0xab");

#endif
