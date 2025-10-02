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
/************************************************************************/
/*! \file gptTables.h
    \brief GUID Partition Table (GPT) table definitions
*/
/***********************************************************************/

#include <stdint.h>

struct GptPartitionEntry
{
    uint8_t partition_type_guid[16];
    uint8_t unique_partition_guid[16];
    uint64_t starting_lba;
    uint64_t ending_lba;
    uint64_t attributes;
    char partition_name[72];
};

struct GptHeader
{
    uint64_t signature;
    uint32_t revision;
    uint32_t header_size;
    uint32_t header_crc32;
    uint32_t reserved1;
    uint64_t current_lba;
    uint64_t backup_lba;
    uint64_t first_usable_lba;
    uint64_t last_usable_lba;
    uint8_t disk_guid[16];
    uint64_t partition_entry_lba;
    uint32_t num_partition_entries;
    uint32_t sizeof_partition_entry;
    uint32_t partition_entry_array_crc32;
    // Additional fields are present in the actual GPT header, but we'll only include the essential ones for our use case
};

struct GptHeader readGptHeader(bool verbose);
struct GptPartitionEntry *readGptPartitionEntry(uint64_t sector);
void printGptPartitionEntry(struct GptPartitionEntry *entry);
void printGptPartitionEntries(void);
