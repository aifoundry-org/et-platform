#include "etsoc/isa/hart.h"

#define INPUT_DATA 1024

typedef struct {
    char input[INPUT_DATA];
    char* output;
} EchoParams;

inline int min(int a, int b)
{
    return a < b ? a : b;
}

/* Echo's the data back sent as input in kernel arguments */
int main(const EchoParams* const params)
{
    if (get_thread_id() == 0)
    {
        return 0;
    }
    int mid = (int)get_minion_id();
    int numWorkers = SOC_MINIONS_PER_SHIRE;
    int elemsPerWorker = (INPUT_DATA + numWorkers - 1) / numWorkers;
    if (elemsPerWorker % 16)
    {
        elemsPerWorker += 16 - (elemsPerWorker % 16);
    }
    int init = elemsPerWorker * mid;
    int end = min(elemsPerWorker * (mid + 1), INPUT_DATA);

    for (int i = init; i < end; ++i)
    {
        params->output[i] = params->input[i];
    }
    return 0;
}
