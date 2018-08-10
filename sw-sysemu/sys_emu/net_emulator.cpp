// Global
#include <stdlib.h>
#include <inttypes.h>

// Local
#include "net_emulator.h"

// Constructor
net_emulator::net_emulator(main_memory * mem_)
{
    log = NULL;
    mem = mem_;
    enabled = false;
}

// Destructor
net_emulator::~net_emulator()
{
}

// Set
void net_emulator::set_file(char * net_desc_file)
{
    FILE * f = fopen(net_desc_file, "r");
    if(f == NULL)
    {
        * log << LOG_FTL << "Parse Network File Error -> Couldn't open file " << net_desc_file << " for reading!!" << endm;
    }


    while(!feof(f))
    {
        net_emulator_layer layer;
        int res = fscanf(f, "Compute: %" PRIx64 ", Helper: %" PRIx64 ", ShireMask: %" PRIx64 ", MinionMask: %" PRIx64 ", ID: %i, Name: %s\n", &layer.compute_pc, &layer.helper_pc, &layer.shire_mask, &layer.minion_mask, &layer.id, layer.name);
        if(res == 6)
        {
            res = fscanf(f, "TensorA: %" PRIx64 ", TensorB: %" PRIx64 ", TensorC: %" PRIx64 ", TensorD: %" PRIx64 ", TensorE: %" PRIx64 ", ComputeSize: %x, HelperSize: %x, InfoPointer: %" PRIx64 "\n", &layer.tensor_a, &layer.tensor_b, &layer.tensor_c, &layer.tensor_d, &layer.tensor_e, &layer.compute_size, &layer.helper_size, &layer.info_pointer);
            if(res == 8)
                layers.push_back(layer);
            else
                break;
        }
        else
        {
            break;
        }
    }

    helper_done = false;
    enabled     = true;
}

// Set
void net_emulator::set_log(testLog * log_)
{
    log = log_;
}

// Access
bool net_emulator::done()
{
    return (layers.size() == 0);
}

// Enabed
bool net_emulator::is_enabled(){
    return enabled;
}

// Execution
void net_emulator::get_new_ipi(std::list<int> * enabled_threads, std::list<int> * ipi_threads, uint64_t * new_pc)
{
    // Wait until all threads are in WFI before sending IPI for next layer
    if(!helper_done && enabled_threads->size()) return;
    // No layers pending
    if(layers.size() == 0) return;

    // Send IPI for helper first (if not done yet)
    net_emulator_layer layer;
    int thread;

    layer = layers.front();
    if(!helper_done)
    {
        // Update the contents of the layer dynamic info
        uint64_t ptr = layer.info_pointer;
        mem->write(ptr,      8, &layer.tensor_a);
        mem->write(ptr + 8,  8, &layer.tensor_b);
        mem->write(ptr + 16, 8, &layer.tensor_c);
        mem->write(ptr + 24, 8, &layer.tensor_d);
        mem->write(ptr + 32, 8, &layer.tensor_e);
        mem->write(ptr + 40, 8, &layer.compute_pc);
        mem->write(ptr + 48, 8, &layer.helper_pc);
        mem->write(ptr + 56, 4, &layer.compute_size);
        mem->write(ptr + 60, 4, &layer.helper_size);

        // IPI
        helper_done = true;
        thread = 1; // Going to thread 1
        * new_pc = layer.helper_pc;
    }
    else
    {
        helper_done = false;
        thread = 0; // Going to thread 0
        * new_pc = layer.compute_pc;
        layers.pop_front();
    }

    // Generate list of IPI based on masks
    for(int s = 0; s < 64; s++)
    {
        // If shire enabled
        if((layer.shire_mask >> s) & 1)
        {
            for(int m = 0; m < 64; m++)
            {
                // If minion enabled
                if((layer.minion_mask >> m) & 1)
                {
                    int minion_id = s * 128 + m * 2 + thread;
                    ipi_threads->push_back(minion_id);
                }
            }
        }
    }
}

