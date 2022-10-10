/*-------------------------------------------------------------------------
* Copyright (C) 2020, Esperanto Technologies Inc.
* The copyright to the computer program(s) herein is the
* property of Esperanto Technologies, Inc. All Rights Reserved.
* The program(s) may be used and/or copied only with
* the written permission of Esperanto Technologies and
* in accordance with the terms and conditions stipulated in the
* agreement/contract under which the program(s) have been supplied.
*-------------------------------------------------------------------------*/

#include "emu_gio.h"
#include "sys_emu.h"


////////////////////////////////////////////////////////////////////////////////
// Parses a file that defines the memory regions plus contents to be
// loaded in the different regions
////////////////////////////////////////////////////////////////////////////////

bool sys_emu::parse_mem_file(const char* filename)
{
    FILE * file = fopen(filename, "r");
    if (file == NULL)
    {
        LOG_AGENT(FTL, agent, "Parse Mem File Error -> Couldn't open file %s for reading!!", filename);
        return false;
    }

    // Parses the contents
    char buffer[1024];
    char * buf_ptr = (char *) buffer;
    size_t buf_size = 1024;
    while (getline(&buf_ptr, &buf_size, file) != -1)
    {
        uint64_t base_addr;
        uint64_t size;
        uint32_t value;
        char str[1024];
        if(sscanf(buffer, "New Mem Region: 40'h%" PRIX64 ", 40'h%" PRIX64 ", %s", &base_addr, &size, str) == 3)
        {
            WARN_AGENT(other, agent, "Ignore: New Mem Region found: @ 0x%" PRIx64 ", size = 0x%" PRIu64, base_addr, size);
        }
        else if(sscanf(buffer, "File Load: 40'h%" PRIX64 ", %s", &base_addr, str) == 2)
        {
            LOG_AGENT(INFO, agent, "New File Load found: %s @ 0x%" PRIx64, str, base_addr);
            try
            {
                chip.load_raw(str, base_addr);
            }
            catch (...)
            {
                fclose(file);
                LOG_AGENT(FTL, agent, "Error loading file \"%s\"", str);
                return false;
            }
        }
        else if(sscanf(buffer, "ELF Load: %s", str) == 1)
        {
            LOG_AGENT(INFO, agent, "New ELF Load found: %s", str);
            try
            {
                chip.load_elf(str);
            }
            catch (...)
            {
                fclose(file);
                LOG_AGENT(FTL, agent, "Error loading ELF \"%s\"", str);
                return false;
            }
        }
        else if(sscanf(buffer, "Mem write32: 40'h%" PRIX64 ", 32'h%" PRIX32 , &base_addr, &value) == 2)
        {
            chip.memory.write(agent, base_addr, sizeof(value),
                              reinterpret_cast<bemu::MainMemory::const_pointer>(&value));
        }
    }
    // Closes the file
    fclose(file);
    return true;
}
