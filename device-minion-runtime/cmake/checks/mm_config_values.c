#include "config/mm_config.h"

/* definition to expand macro then apply to pragma message */
#define VALUE_TO_STRING(x) #x
#define VALUE(x) VALUE_TO_STRING(x)

#pragma message("DISPATCHER_BASE_HART_ID="VALUE(DISPATCHER_BASE_HART_ID))
#pragma message("SQW_BASE_HART_ID="VALUE(SQW_BASE_HART_ID))
#pragma message("KW_BASE_HART_ID="VALUE(KW_BASE_HART_ID))
#pragma message("DMAW_BASE_HART_ID="VALUE(DMAW_BASE_HART_ID))
#pragma message("DMAW_NUM="VALUE(DMAW_NUM+DMAW_NUM))

int main(void)
{
    return 0;
}