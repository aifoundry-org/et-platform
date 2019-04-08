// compile with
// gcc -g -O0 -o slave slave.c shared_memory.c -lrt

#include <sys/types.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <unistd.h>

#include "shared_memory.h"
#include "mailbox_protocol.h"
#include "producer_consumer_tests.h"

static int run_slave_tests(volatile SECURE_MAILBOX_INTERFACE_t * mailbox, int master_pid) {
    int rv;
    printf("Master PID: %u\n", master_pid);
    printf("Slave tests started.\n");
    printf("mailbox size: 0x%x\n", mailbox->interface_size);
    printf("mailbox version: 0x%x\n", mailbox->interface_version);

    rv = run_tests(mailbox, false);

    printf("Slave tests completed.\n");
    return rv;
}

int main(int argc, char ** argv) {
    int rv;
    const size_t shared_memory_size = sizeof(SECURE_MAILBOX_INTERFACE_t) + 2 * sizeof(int);
    int * shared_ptr;
    int master_pid;
    int slave_pid;

    if (argc < 2) {
        printf("Usage: slave <shared_memory_file>\n");
        return -1;
    }

    slave_pid = getpid();
    printf("Slave started (PID: %d).\n", slave_pid);

    if (0 != open_shared_memory(argv[1], shared_memory_size, (void**)&shared_ptr)) {
        printf("create_shared_memory() failed!\n");
        return -1;
    }

    master_pid = shared_ptr[0];
    shared_ptr[1] = slave_pid;

    rv = run_slave_tests((SECURE_MAILBOX_INTERFACE_t*)(shared_ptr + 2), master_pid);

    shared_ptr[1] = 0;

    if (0 != close_shared_memory(shared_ptr, shared_memory_size)) {
        printf("close_shared_memory() failed!\n");
        return -1;
    }

    return rv;
}
