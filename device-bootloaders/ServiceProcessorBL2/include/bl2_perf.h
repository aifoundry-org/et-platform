#ifndef BL2_PERF_SERVICE_H
#define BL2_PERF_SERVICE_H

#include <stdint.h>
#include "dm.h"
#include "dm_service.h"


// Function prototypes 
void process_performance_request(tag_id_t tag_id, msg_id_t msg_id);

#endif
