/*-------------------------------------------------------------------------
* Copyright (C) 2023, Esperanto Technologies Inc.
* The copyright to the computer program(s) herein is the
* property of Esperanto Technologies, Inc. All Rights Reserved.
* The program(s) may be used and/or copied only with
* the written permission of Esperanto Technologies and
* in accordance with the terms and conditions stipulated in the
* agreement/contract under which the program(s) have been supplied.
*-------------------------------------------------------------------------
*/
#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include "log.h"
#include "bl_error_code.h"

#include "bl2_emmc_controller.h"

#include "gptTables.h"

// Macro to return sector-aligned size
#define SECTOR_ALIGN_SIZE(size) \
    ((size_t)((size_t)(size) + (EMMC_BLOCK_SIZE - 1)) & ~(size_t)(EMMC_BLOCK_SIZE - 1))

/************************************************************************
*
*   FUNCTION
*
*       readGptHeader
*
*   DESCRIPTION
*
*       This function reads the GPT header from EMMC disk
*
*   INPUTS
*
*       N/A 
*
*   OUTPUTS
*
*       error status
*
***********************************************************************/
struct GptHeader readGptHeader(bool verbose)
{
    struct GptHeader header;

    // Create a buffer to hold the data
    uint8_t gptHeaderBuffer[EMMC_BLOCK_SIZE / sizeof(uint32_t)];

// GPT header is located at the second sector,
// Size of the GPT header is 512 bytes
#define GPT_SECTOR_NUM 1
    int status = Emmc_read_to_buffer(gptHeaderBuffer, sizeof(gptHeaderBuffer), GPT_SECTOR_NUM);
    if (status != SUCCESS)
    {
        Log_Write(LOG_LEVEL_ERROR,
                  "EMMC::readGptHeader: failed to read GPT header from EMMC device\r\n");
        // Handle the error, maybe return an error code or take appropriate action
        // For now, I'll return an empty header
        return header;
    }

    // Copy data from the buffer to the GptHeader struct
    memcpy(&header, gptHeaderBuffer, sizeof(header));

    if (verbose)
    {
        // Display GPT header information
        // EFI Partition : 0x5452415020494645ULL
        Log_Write(LOG_LEVEL_INFO, "GPT Signature: %lx\n", header.signature);
        Log_Write(LOG_LEVEL_INFO, "Number of Partition Entries: %u\n",
                  header.num_partition_entries);
        Log_Write(LOG_LEVEL_INFO, "Size of Partition Entry: %u\n", header.sizeof_partition_entry);
    }

    return header;
}
/************************************************************************
*
*   FUNCTION
*
*       readGptPartitionEntry
*
*   DESCRIPTION
*
*       This function reads a LBA entry from a GPT table
*
*   INPUTS
*
*       sector     Starting sector address
*
*   OUTPUTS
*
*       error status
*
***********************************************************************/
struct GptPartitionEntry *readGptPartitionEntry(uint64_t sector)
{
    // Define a static GptPartitionEntry variable
    static struct GptPartitionEntry entryDesc;

    // Read data from EMMC into the static variable
    int status =
        Emmc_read_to_buffer((uint8_t *)&entryDesc, sizeof(struct GptPartitionEntry), sector);

    if (status != SUCCESS)
    {
        Log_Write(LOG_LEVEL_ERROR,
                  "EMMC::readGptPartitionEntry: failed to read sector %ld from EMMC device\r\n",
                  sector);
    }

    // Return a pointer to the static variable
    return &entryDesc;
}
/************************************************************************
*
*   FUNCTION
*
*       printGptPartitionEntry
*
*   DESCRIPTION
*
*       This function prints the details of a given LBA entry from a GPT table
*
*   INPUTS
*
*       entry  Pointer to the entry to print 
* 
*   OUTPUTS
*
*       error status
*
***********************************************************************/
void printGptPartitionEntry(struct GptPartitionEntry *entry)
{
    Log_Write(LOG_LEVEL_INFO, "Partition Type GUID: ");
    for (uint32_t i = 0; i < 16; i++)
    {
        Log_Write(LOG_LEVEL_INFO, "%02X ", entry->partition_type_guid[i]);
    }

    Log_Write(LOG_LEVEL_INFO, "\nUnique Partition GUID: ");

    for (int i = 0; i < 16; i++)
    {
        Log_Write(LOG_LEVEL_INFO, "%02X ", entry->unique_partition_guid[i]);
    }

    Log_Write(LOG_LEVEL_INFO, "Starting LBA: %lu\n", entry->starting_lba);
    Log_Write(LOG_LEVEL_INFO, "Ending LBA: %lu\n", entry->ending_lba);
    Log_Write(LOG_LEVEL_INFO, "Attributes: %lu\n", entry->attributes);
    Log_Write(LOG_LEVEL_INFO, "Partition Name: %s\n", entry->partition_name);
}

/************************************************************************
*
*   FUNCTION
*
*       printGptPartitionEntries
*
*   DESCRIPTION
*
*       This function prints the details of all LBA entry from a GPT table
*
*   INPUTS
*
*      N/A 
*  
*   OUTPUTS
*
*     N/A
*
***********************************************************************/
void printGptPartitionEntries(void)
{
    struct GptHeader header = readGptHeader(true);
    struct GptPartitionEntry *entryDesc;

    // Read and display GPT partition entries
    for (uint32_t idx = 0; idx < header.num_partition_entries; idx++)
    {
        entryDesc = readGptPartitionEntry(header.partition_entry_lba);
        Log_Write(LOG_LEVEL_INFO, "Partition Entry %d:\n", idx + 1);
        printGptPartitionEntry(entryDesc);
    }
}
