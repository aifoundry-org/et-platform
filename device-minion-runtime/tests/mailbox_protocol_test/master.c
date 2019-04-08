// compile with
// gcc -g -O0 -o master master.c shared_memory.c -lrt

#include <sys/types.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>

#include "shared_memory.h"
#include "mailbox_protocol.h"
#include "producer_consumer_tests.h"

static int run_master_tests(volatile SECURE_MAILBOX_INTERFACE_t * mailbox, int slave_pid) {
    int rv;

    printf("Slave PID: %u\n", slave_pid);


    printf("Master tests started.\n");
    mailbox->interface_size = sizeof(SECURE_MAILBOX_INTERFACE_t);
    mailbox->interface_version = 0x12345678;

    rv = run_tests(mailbox, true);

    printf("Master tests completed.\n");

    return rv;
}

int main(int argc, char ** argv) {
    int rv;
    const size_t shared_memory_size = sizeof(SECURE_MAILBOX_INTERFACE_t) + 2 * sizeof(int);
    int * shared_ptr;
    int master_pid;
    int slave_pid;

    if (argc < 2) {
        printf("Usage: master <shared_memory_file>\n");
        return -1;
    }

    master_pid = getpid();
    printf("Master started (PID: %d).\n", master_pid);

    if (0 != create_shared_memory(argv[1], shared_memory_size, (void**)&shared_ptr)) {
        printf("create_shared_memory() failed!\n");
        return -1;
    }
    memset(shared_ptr, 0, shared_memory_size);
    shared_ptr[0] = master_pid;

    printf("Waiting for slave to start...\n");
    do {
        slave_pid = shared_ptr[1];
    } while (0 == slave_pid);

    rv = run_master_tests((SECURE_MAILBOX_INTERFACE_t*)(shared_ptr + 2), slave_pid);
    
    if (0 != delete_shared_memory(argv[1], shared_ptr, shared_memory_size)) {
        printf("delete_shared_memory() failed!\n");
        return -1;
    }

    return rv;
}
