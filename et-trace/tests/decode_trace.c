/*
 * Test: create_trace
 * Randomly generates a synthetic device trace.
 * This trace is then read and decoded.
 */

#include <stdlib.h>

int main(int argc, const char** argv)
{
    int seed = argc > 1 ? atoi(argv[1]) : 1453;
}
