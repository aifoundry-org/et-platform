// SYSEMU -shires 0x400000001 -minions 0x1 -single_thread

#include "diag.h"

/***** Service processor *****/
#ifdef SPIO

static void test_no_harts()
{
    Select_Harts(0, 0x0);

    uint64_t hactrl    = read_hactrl(0, 0);
    uint64_t hastatus0 = read_hastatus0(0, 0);
    uint64_t tree0     = read_andortreel0(0, 0);
    uint64_t tree1     = read_andortreel1(0);
    uint64_t tree2     = read_andortreel2();

    EXPECTX(hactrl, ==, 0x0);
    EXPECTX(hastatus0, ==, 0xffff000000010000);
    EXPECTX(tree0, ==, 0x0);
    EXPECTX(tree1, ==, 0x0);
    EXPECTX(tree2, ==, 0x0);

    Unselect_Harts(0, 0);
}


static void test_running_hart()
{
    Select_Harts(0, 0x1);

    uint64_t hactrl    = read_hactrl(0, 0);
    uint64_t hastatus0 = read_hastatus0(0, 0);
    uint64_t tree0     = read_andortreel0(0, 0);
    uint64_t tree1     = read_andortreel1(0);
    uint64_t tree2     = read_andortreel2();

    EXPECTX(hactrl, ==, 0x00010001);
    EXPECTX(hastatus0, ==, 0xffff000000010000);
    EXPECTX(tree0, ==, 0x2cc);
    EXPECTX(tree1, ==, 0x598);
    EXPECTX(tree2, ==, 0x330);

    Unselect_Harts(0, 0x1);
}


static void test_unavailable_hart()
{
    Select_Harts(0, 0x2);

    uint64_t hactrl    = read_hactrl(0, 0);
    uint64_t hastatus0 = read_hastatus0(0, 0);
    uint64_t tree0     = read_andortreel0(0, 0);
    uint64_t tree1     = read_andortreel1(0);
    uint64_t tree2     = read_andortreel2();

    EXPECTX(hactrl, ==, 0x00020002);
    EXPECTX(hastatus0, ==, 0xffff000000010000);
    EXPECTX(tree0, ==, 0x3c0);
    EXPECTX(tree1, ==, 0x780);
    EXPECTX(tree2, ==, 0x700);

    Unselect_Harts(0, 0x2);
}


static void test_all_harts()
{
    Select_Harts(0, 0xffff);

    uint64_t hactrl    = read_hactrl(0, 0);
    uint64_t hastatus0 = read_hastatus0(0, 0);
    uint64_t tree0     = read_andortreel0(0, 0);
    uint64_t tree1     = read_andortreel1(0);
    uint64_t tree2     = read_andortreel2();

    EXPECTX(hactrl, ==, 0xFFFFFFFF);
    EXPECTX(hastatus0, ==, 0xFFFF000000010000);
    EXPECTX(tree0, ==, 0x3C4);
    EXPECTX(tree1, ==, 0x788);
    EXPECTX(tree2, ==, 0x710);

    Unselect_Harts(0, 0xffff);
}


int main()
{
    test_no_harts();
    test_running_hart();
    test_unavailable_hart();
    test_all_harts();
    return 0;
}


/***** Minions *****/
#else


int main()
{
    for (int i = 0; i < 1000; ++i)
        asm volatile("addi x0,x0,0");

    return 0;
}


#endif
