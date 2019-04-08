#ifndef __PRODUCER_CONSUMER_TESTS_H_
#define __PRODUCER_CONSUMER_TESTS_H_

#include "mailbox_protocol.h"

#define MAX_PRODUCERS  1
#define MAX_SERVICE_ID 10
#define MAX_COMMAND_ID 10
#define REQUESTS_QUEUE_SIZE (MAX_PRODUCERS+1)

int run_tests(volatile SECURE_MAILBOX_INTERFACE_t * mailbox, bool master);

#endif
