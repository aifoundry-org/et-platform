// Utility to convert addresses in DRAM memory space to ZeBu DDR model address

#include <inttypes.h>
#include <stdio.h>

//#define STANDALONE_TESTING

int get_mem_address(const uint64_t address, uint64_t* const memShire, uint64_t* const memController, uint64_t* memAddress);
static void get_ddr_address(const uint64_t address, uint64_t* const memShire, uint64_t* const memController, uint64_t* memAddress);

#ifdef STANDALONE_TESTING
int main(void)
{
    uint64_t input = 1;
    uint64_t memShire = 0, memController = 0, memAddress = 0;

    for (int i = 0; i <= 33; i++)
    {
        get_mem_address(input, &memShire, &memController, &memAddress);
        printf("bit %2i input = %010lx memShire = %010lx memController = %010lx memAddress = %010lx\r\n", i, input, memShire, memController, memAddress);
        input <<= 1ULL;
    }
}
#endif

// input address = linear address in DRAM space
// output memShire = memShire index 0-15
// output memController = memController index 0-1
// output memAddress = memAddress for ZeBu DDR model with bits swizzled
int get_mem_address(const uint64_t address, uint64_t* const memShire, uint64_t* const memController, uint64_t* memAddress)
{
    int result;

    if ((memShire != NULL) && (memController != NULL) && (memAddress != NULL))
    {
        switch (address)
        {
            // R_L3_Mcode, R_L3_Linux, R_L3_DRAM
            case 0x8000000000ULL ... (0x8800000000ULL - 1U):
                get_ddr_address(address, memShire, memController, memAddress);
                result = 0;
            break;

            // R_DRCT_Mcode, R_DRCT_Linux, R_DRCT_DRAM
            case 0xC000000000ULL ... (0xC800000000ULL - 1U):
                get_ddr_address(address - 0x4000000000ULL, memShire, memController, memAddress);
                result = 0;
            break;

            default:
#ifdef STANDALONE_TESTING
                get_ddr_address(address, memShire, memController, memAddress);
                result = 0;
#else
                result = -1;
#endif
            break;
        }
    }
    else
    {
        result = -1;
    }

    return result;
}

// DRAM is spread across 16 memory controllers in 8 memory shires
// Each memory controller deals with 64-byte cache line sized transactions
// addr[5:0] byte in panel
// addr[8:6] memshire
// addr[9] controller (0=even, 1=odd)
//
// If using DDR DRAM models, additional address bit swizzling is implemented:
// Mesh  Synopsys DDR    Comment
// addr  dword addr
// bit   bit
// 0     -               Byte address bit 0
// 1     -               Byte address bit 1
// 2     -               Byte address bit 2
// 3     0               Byte address bit 3 (double-word address bit 0)
// 4     1               Byte address bit 4 (double-word address bit 1)
// 5     24              DDR bank address bit 0
// 6     -               Memshire number (bit 0) 000 = dwrow[0], 100 = derow[0]
// 7     -               Memshire number (bit 1) 001 = dwrow[1], 101 = derow[1]
// 8     -               Memshire number (bit 2) 010 = dwrow[2], 110 = derow[2]
// 9     even/odd        Memory controller
// 10    25              DDR bank address bit 1
// 11    2
// 12    3
// 13    4
// 14    26              DDR bank address bit 2
// 15    8
// 16    9
// 17    10
// 18    11
// 19    12
// 20    13
// 21    14
// 22    15
// 23    16
// 24    17
// 25    18
// 26    19
// 27    20
// 28    21
// 29    5
// 30    6
// 31    7
// 32    22
// 33    23

//moves a bit from address[input] to tempAddress[output]
#define MOVE_BIT(input, output) tempAddress |= (((address >> input) & 0x1U) << output)

static void get_ddr_address(const uint64_t address, uint64_t* const memShire, uint64_t* const memController, uint64_t* memAddress)
{
    uint64_t tempAddress = 0;

    *memShire = (address >> 6U) & 0x7U;
    *memController = (address >> 9U) & 0x1U;

    MOVE_BIT( 3U,  0U);
    MOVE_BIT( 4U,  1U);
    MOVE_BIT( 5U, 24U);

    MOVE_BIT(10U, 25U);
    MOVE_BIT(11U,  2U);
    MOVE_BIT(12U,  3U);
    MOVE_BIT(13U,  4U);
    MOVE_BIT(14U,  6U);

    // Map source bits 15:28 to dest bits 8:21
    tempAddress |= ((address >> 7U) & 0x3FFF00U);

    MOVE_BIT(29U,  5U);
    MOVE_BIT(30U,  6U);
    MOVE_BIT(31U,  7U);
    MOVE_BIT(32U, 22U);
    MOVE_BIT(33U, 23U);

    *memAddress = tempAddress;
}
