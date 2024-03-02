/*-------------------------------------------------------------------------
* Copyright (C) 2020, Esperanto Technologies Inc.
* The copyright to the computer program(s) herein is the
* property of Esperanto Technologies, Inc. All Rights Reserved.
* The program(s) may be used and/or copied only with
* the written permission of Esperanto Technologies and
* in accordance with the terms and conditions stipulated in the
* agreement/contract under which the program(s) have been supplied.
*-------------------------------------------------------------------------*/

#include "tstore_checker.h"
#include "emu_gio.h"
#include "memmap.h"

#define TS_CHECKER_LOG(addr, thread, cmd) \
    { if((addr == 0x0) || (log_addr == 0x0) || (addr == log_addr)) \
    { \
        if((thread == 0xFFFFFFFF) || (log_thread == 0xFFFFFFFF) || ((thread >> 3) == (log_thread >> 3))) \
        { \
            cmd; \
        } \
    } }

// Constructor
tstore_checker::tstore_checker(bemu::System* chip) : bemu::Agent(chip)
{
}

// This functions captures the execution of a tensor store
void tstore_checker::execute(uint32_t thread_id, uint64_t address, uint64_t stride, uint32_t coop, uint32_t lines, uint32_t cols)
{
    coop_tstore store;

    store.address = address;
    store.stride  = stride;
    store.coop    = coop;
    store.lines   = lines;
    store.cols    = cols;
    pending_list[thread_id].push_back(store);
    TS_CHECKER_LOG(address, thread_id, LOG_AGENT(DEBUG, *this, "tstore_checker::execute => adding entry! addr: %016llX, stride: %llX, coop: %i, lines: %i, cols: %i, thread_id: %i",
        (long long unsigned int) address, (long long unsigned int) stride, coop, lines, cols, thread_id));
}

// This function checks if the cooperative tensor stores of a thread_id have a match and check they are doing the righ
// thing
void tstore_checker::check_and_drain(uint32_t thread_id)
{
    // Checks and drains as far as there is a matching tensor store
    while(check_and_drain_head(thread_id))
    {
    }
}

// This function returns if all the cooperative tensor stores had been resolved
void tstore_checker::is_empty()
{
    for (uint32_t thread_id = 0; thread_id < EMU_NUM_THREADS; thread_id++) {
        const auto& pending_list_thread = pending_list[thread_id];

        // Store the size in a variable
        const std::size_t list_size = pending_list_thread.size();

        // Check if the list is not empty
        if (list_size != 0) {
            const auto& head_coop = pending_list_thread.front();

            LOG_AGENT(FTL, *this, "tstore_checker::is_empty => found non-resolved cooperative store! addr: %016llX, stride: %llX, coop: %i, lines: %i, cols: %i, thread_id: %i",
                      (long long unsigned int)head_coop.address, (long long unsigned int)head_coop.stride, head_coop.coop, head_coop.lines, head_coop.cols, thread_id);

        }
    }
}

// This function checks if the cooperative tensor stores of a thread_id have a match and check they are doing the righ
// thing
bool tstore_checker::check_and_drain_head(uint32_t thread_id)
{
    // Nothing to check if the list is empty
    if(pending_list[thread_id].size() == 0)
    {
        return false;
    }

    auto &head_coop = pending_list[thread_id].front();

    TS_CHECKER_LOG(0x0, thread_id, LOG_AGENT(DEBUG, *this, "tstore_checker::check_and_drain => checking head! addr: %016llX, stride: %llX, coop: %i, lines: %i, cols: %i, thread_id: %i",
        (long long unsigned int) head_coop.address, (long long unsigned int) head_coop.stride, head_coop.coop, head_coop.lines, head_coop.cols, thread_id));

    uint32_t first_thread_id = thread_id;
    if(head_coop.coop == 2)
    {
        first_thread_id = thread_id & ~3;
    }
    else if(head_coop.coop == 4)
    {
        first_thread_id = thread_id & ~7;
    }
    else
    {
        LOG_AGENT(FTL, *this, "tstore_checker::check_and_drain => found unexpected coop count! addr: %016llX, stride: %llX, coop: %i, lines: %i, cols: %i, thread_id: %i",
            (long long unsigned int) head_coop.address, (long long unsigned int) head_coop.stride, head_coop.coop, head_coop.lines, head_coop.cols, thread_id);
    }

    // Checks correctness for all elements
    bool first = true;
    bool drain = true;
    coop_tstore first_store = head_coop;

    for(uint32_t i = 0; i < head_coop.coop; i++)
    {
        uint32_t current_thread_id = first_thread_id + i * 2;

        // If a thread hasn't executed yet the tensor store, skip the check and drain
        if(pending_list[current_thread_id].size() == 0)
        {
            TS_CHECKER_LOG(0x0, thread_id, LOG_AGENT(DEBUG, *this, "tstore_checker::check_and_drain => checking step %i! no tensor store pending thread_id %i", i, current_thread_id));
            drain = false;
            break;
        }
        auto &current_store = pending_list[current_thread_id].front();
        TS_CHECKER_LOG(0x0, thread_id, LOG_AGENT(DEBUG, *this, "tstore_checker::check_and_drain => checking step %i! addr: %016llX, stride: %llX, coop: %i, lines: %i, cols: %i, thread_id: %i",
              i, (long long unsigned int) current_store.address, (long long unsigned int) current_store.stride, current_store.coop, current_store.lines, current_store.cols, current_thread_id));

        // For first thread, copy the store data
        if(first)
        {
            first = false;
            first_store = current_store;
            // Checks 64B alignment
            if((first_store.coop == 4) || ((first_store.coop == 2) && (first_store.cols == 2)))
            {
                if((first_store.address % 64) || (first_store.stride % 64))
                {
                    LOG_AGENT(FTL, *this, "tstore_checker::check_and_drain => 64B alignment requirement not met! addr: %016llX, stride: %llX, coop: %i, lines: %i, cols: %i, thread_id: %i",
                        (long long unsigned int) current_store.address, (long long unsigned int) current_store.stride, current_store.coop, current_store.lines, current_store.cols, current_thread_id);
                }
            }
            else if((first_store.coop == 2) && (first_store.cols == 1))
            {
                if((first_store.address % 32) || (first_store.stride % 32))
                {
                    LOG_AGENT(FTL, *this, "tstore_checker::check_and_drain => 32B alignment requirement not met! addr: %016llX, stride: %llX, coop: %i, lines: %i, cols: %i, thread_id: %i",
                            (long long unsigned int) current_store.address, (long long unsigned int) current_store.stride, current_store.coop, current_store.lines, current_store.cols, current_thread_id);
                }
            }
        }
        // For the other check correctness
        else
        {
            // Stride must be the same
            if(first_store.stride != current_store.stride)
            {
                LOG_AGENT(FTL, *this, "tstore_checker::check_and_drain => stride mismatch! addr: %016llX, stride: %llX, coop: %i, lines: %i, cols: %i, thread_id: %i",
                    (long long unsigned int) current_store.address, (long long unsigned int) current_store.stride, current_store.coop, current_store.lines, current_store.cols, current_thread_id);
            }
            // Coop count must be the same
            if(first_store.coop != current_store.coop)
            {
                LOG_AGENT(FTL, *this, "tstore_checker::check_and_drain => coop count mismatch! addr: %016llX, stride: %llX, coop: %i, lines: %i, cols: %i, thread_id: %i",
                    (long long unsigned int) current_store.address, (long long unsigned int) current_store.stride, current_store.coop, current_store.lines, current_store.cols, current_thread_id);
            }
            // Lines must be the same
            if(first_store.lines != current_store.lines)
            {
                LOG_AGENT(FTL, *this, "tstore_checker::check_and_drain => line count mismatch! addr: %016llX, stride: %llX, coop: %i, lines: %i, cols: %i, thread_id: %i",
                    (long long unsigned int) current_store.address, (long long unsigned int) current_store.stride, current_store.coop, current_store.lines, current_store.cols, current_thread_id);
            }
            // Cols must be the same
            if(first_store.cols != current_store.cols)
            {
                LOG_AGENT(FTL, *this, "tstore_checker::check_and_drain => line count mismatch! addr: %016llX, stride: %llX, coop: %i, lines: %i, cols: %i, thread_id: %i",
                    (long long unsigned int) current_store.address, (long long unsigned int) current_store.stride, current_store.coop, current_store.lines, current_store.cols, current_thread_id);
            }
            // Address offset must be correct between threads
            uint64_t offset_bytes = 16 * current_store.cols * i;
            if(first_store.address != (current_store.address - offset_bytes))
            {
                LOG_AGENT(FTL, *this, "tstore_checker::check_and_drain => address offset mismatch! addr: %016llX, stride: %llX, coop: %i, lines: %i, cols: %i, thread_id: %i",
                    (long long unsigned int) current_store.address, (long long unsigned int) current_store.stride, current_store.coop, current_store.lines, current_store.cols, current_thread_id);
            }
        }
    }

    // If all threads are present, we can drain the head
    if(drain)
    {
        uint32_t coop_count = head_coop.coop; // WARNING: keep the variable local, as the reference in the list will be deleted during the for
        for(uint32_t i = 0; i < coop_count; i++)
        {
            uint32_t current_thread_id = first_thread_id + i * 2;
            pending_list[current_thread_id].pop_front();
            TS_CHECKER_LOG(0x0, thread_id, LOG_AGENT(DEBUG, *this, "tstore_checker::check_and_drain => draining head for thread_id %i", current_thread_id));
        }
    }
    return drain;
}

