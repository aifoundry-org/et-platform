// Global
#include <string.h>

// Local
#include "main_memory_region_rbox.h"
#include "emu.h"
#include "rbox.h"
#include "emu_gio.h"

using namespace std;

// Creator
main_memory_region_rbox::main_memory_region_rbox(uint64_t base, uint64_t size, testLog & l, func_ptr_get_thread& get_thr)
    : main_memory_region(base, size, l, get_thr)
{
}
 
// Destructor: free allocated mem
main_memory_region_rbox::~main_memory_region_rbox()
{
}

// Write to memory region
void main_memory_region_rbox::write(uint64_t ad, int size, const void * data)
{
    unsigned rbox_id;
    uint64_t reg_id;

    LOG(DEBUG, "Writing to RBOX ESR Region with address %016" PRIx64, ad);

    if (size != 8)
        LOG(WARN, "Write to RBOX ESR Region with address %016" PRIx64 " is not 64-bit", ad);

    decode_esr(ad, rbox_id, reg_id);

    uint64_t reg_wr_data = *((uint64_t *) data);

    switch(reg_id)
    {
        case ESR_RBOX_CONFIG      :
        case ESR_RBOX_IN_BUF_PG   :
        case ESR_RBOX_IN_BUF_CFG  :
        case ESR_RBOX_OUT_BUF_PG  :
        case ESR_RBOX_OUT_BUF_CFG :
        case ESR_RBOX_START       :
        case ESR_RBOX_CONSUME     :
            GET_RBOX(rbox_id, 0).write_esr((reg_id >> 3) & 0x3FFF, reg_wr_data);
            break;
    }

    switch(reg_id)
    {
        case ESR_RBOX_CONFIG      :
            LOG(DEBUG, "Write to RBOX %02u ESR CONFIG the value %016" PRIx64, rbox_id, reg_wr_data);
            break;
        case ESR_RBOX_IN_BUF_PG   :
            LOG(DEBUG, "Write to RBOX %02u ESR INPUT BUFFER PAGES the value %016" PRIx64, rbox_id, reg_wr_data);
            break;
        case ESR_RBOX_IN_BUF_CFG  :
            LOG(DEBUG, "Write to RBOX %02u ESR INPUT BUFFER CONFIG the value %016" PRIx64, rbox_id, reg_wr_data);
            break;
        case ESR_RBOX_OUT_BUF_PG  :
            LOG(DEBUG, "Write to RBOX %02u ESR OUTPUT BUFFER PAGE the value %016" PRIx64, rbox_id, reg_wr_data);
            break;
        case ESR_RBOX_OUT_BUF_CFG :
            LOG(DEBUG, "Write to RBOX %02u ESR OUTPUT BUFFER CONFIG the value %016" PRIx64, rbox_id, reg_wr_data);
            break;
        case ESR_RBOX_START       :
            LOG(DEBUG, "Write to RBOX %02u ESR START the value %016" PRIx64, rbox_id, reg_wr_data);
            break;
        case ESR_RBOX_CONSUME     :
            LOG(DEBUG, "Write to RBOX %02u ESR CONSUME the value %016" PRIx64, rbox_id, reg_wr_data);
            break;
        case ESR_RBOX_STATUS      :
            // Read only
            LOG(WARN, "Write to RBOX %02u READ ONLY ESR STATUS", rbox_id);
            break;
        default :
            LOG(WARN, "Write to RBOX %02u UNDEFINED ESR %" PRId64, rbox_id, reg_id);
            break;
    }
}


// Read from memory region
void main_memory_region_rbox::read(uint64_t ad, int size, void * data)
{
    unsigned rbox_id;
    uint64_t reg_id;

    LOG(DEBUG, "Reading from RBOX ESR Region with address %016" PRIx64, ad);

    if (size != 8)
        LOG(WARN, "Read RBOX ESR Region with address %016" PRIx64 " is not 64-bit", ad);

    decode_esr(ad, rbox_id, reg_id);

    uint64_t reg_rd_data = 0;

    switch(reg_id)
    {
        case ESR_RBOX_CONFIG      :
        case ESR_RBOX_IN_BUF_PG   :
        case ESR_RBOX_IN_BUF_CFG  :
        case ESR_RBOX_OUT_BUF_PG  :
        case ESR_RBOX_OUT_BUF_CFG :
        case ESR_RBOX_START       :
        case ESR_RBOX_CONSUME     :
        case ESR_RBOX_STATUS      :
            reg_rd_data = GET_RBOX(rbox_id, 0).read_esr((reg_id >> 3) & 0x3FFF);
            break;
    }

    switch(reg_id)
    {
        case ESR_RBOX_CONFIG      :
            LOG(DEBUG, "Read from RBOX %02u ESR CONFIG the value 0x%016" PRIx64, rbox_id, reg_rd_data);
            break;
        case ESR_RBOX_IN_BUF_PG   :
            LOG(DEBUG, "Read from RBOX %02u ESR INPUT BUFFER PAGES the value 0x%016" PRIx64, rbox_id, reg_rd_data);
            break;
        case ESR_RBOX_IN_BUF_CFG  :
            LOG(DEBUG, "Read from RBOX %02u ESR INPUT BUFFER CONFIG the value 0x%016" PRIx64, rbox_id, reg_rd_data);
            break;
        case ESR_RBOX_OUT_BUF_PG  :
            LOG(DEBUG, "Read from RBOX %02u ESR OUTPUT BUFFER PAGES the value 0x%016" PRIx64, rbox_id, reg_rd_data);
            break;
        case ESR_RBOX_OUT_BUF_CFG :
            LOG(DEBUG, "Read from RBOX %02u ESR OUTPUT BUFFER CONFIG the value 0x%016" PRIx64, rbox_id, reg_rd_data);
            break;
        case ESR_RBOX_START       :
            LOG(DEBUG, "Read from RBOX %02u ESR START the value 0x%016" PRIx64, rbox_id, reg_rd_data);
            break;
        case ESR_RBOX_CONSUME     :
            LOG(DEBUG, "Read from RBOX %02u ESR CONSUME the value 0x%016" PRIx64, rbox_id, reg_rd_data);
            break;
        case ESR_RBOX_STATUS      :
            LOG(DEBUG, "Read from RBOX %02u ESR STATUS the value 0x%016" PRIx64, rbox_id, reg_rd_data);
            break;
        default :
            LOG(WARN, "Read from RBOX %02u UNDEFINED ESR 0x%016" PRIx64, rbox_id, reg_id);
            break;
    }

    *((uint64_t *) data) = reg_rd_data;
}

void main_memory_region_rbox::decode_esr(uint64_t ad, unsigned &rbox_id, uint64_t &reg_id)
{
    uint32_t shire_id = (ad & ESR_REGION_SHIRE_MASK);

    if (shire_id == ESR_REGION_LOCAL_SHIRE)
    {
        rbox_id = (current_thread / EMU_THREADS_PER_SHIRE);
    }
    else
    {
        rbox_id = (shire_id >> ESR_REGION_SHIRE_SHIFT);
    }

    reg_id = ad & ESR_RBOX_ESR_MASK;
}

