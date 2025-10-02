#ifndef MARKERS_H
#define MARKERS_H

// This was used originally for binary mode.
#define ORIGINAL_START_WAVES_MARKER  __asm__ __volatile("slti x0,x0,0xfb");

// Currently when the waves start it is automatically the beginning of the 1st phase
#define START_WAVES_MARKER  __asm__ __volatile("slti x0,x0,0xaa");
#define START_PHASE_MARKER  __asm__ __volatile("slti x0,x0,0xaa");
#define END_PHASE_MARKER  __asm__ __volatile("slti x0,x0,0xab");

#endif
