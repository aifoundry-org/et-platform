#include <pthread.h>
#include <time.h>
#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "mailbox_protocol.h"
#include "producer_consumer_tests.h"
#include "basic_queue.h"

static int gs_counter = 0;
static bool terminate = false;
static volatile SECURE_MAILBOX_INTERFACE_t * gs_mailbox = NULL;
BASIC_QUEUE_t gs_requests_queue;

static pthread_t gs_threads[MAX_PRODUCERS];

static void * producer_thread(void * arg) {
    MESSAGE_PAYLOAD_t new_request;
    uint32_t thread_id = (uint32_t)(size_t)arg;
    
    printf("thread %u started...\n", thread_id);
    
    while (!terminate) {
        // prepare a request
        new_request.service_id = (uint32_t)(rand() % MAX_SERVICE_ID);
        new_request.command_id = (uint32_t)(rand() % MAX_COMMAND_ID);
        new_request.sender_tag_lo = (uint32_t)__sync_add_and_fetch(&gs_counter, 1);
        new_request.sender_tag_hi = 0;

        // send the request
        if (0 != basic_queue_enqueue(&gs_requests_queue, &new_request)) {
            printf("producer_thread(%u): basic_queue_enqueue() failed!\n", thread_id);
            return NULL;
        }

        printf("producer_thread(%u): queued request (T: %u, S:%u, C:%u)\n", thread_id, new_request.sender_tag_lo, new_request.service_id, new_request.command_id);
    }

    return NULL;
}

static int driver_reset_state(volatile SECURE_MAILBOX_INTERFACE_t * mailbox) {
    if (NULL == mailbox) {
        return -1;
    }
    return 0;
}

static int driver_get_ready(volatile SECURE_MAILBOX_INTERFACE_t * mailbox) {
    if (NULL == mailbox) {
        return -1;
    }
    return 0;
}

static int driver_dispatch_request(volatile SECURE_MAILBOX_INTERFACE_t * mailbox, const MESSAGE_PAYLOAD_t * request, MESSAGE_PAYLOAD_t * response) {
    if (NULL == mailbox) {
        return -1;
    }

    *response = *request;
    printf("driver_dispatch_request: received request (T: %u, S:%u, C:%u) and dispatched response.\n", request->sender_tag_lo, request->service_id, request->command_id);

    return 0;
}

static int driver_dispatch_response(volatile SECURE_MAILBOX_INTERFACE_t * mailbox, const MESSAGE_PAYLOAD_t * response) {
    if (NULL == mailbox) {
        return -1;
    }

    printf("driver_dispatch_response: received response (T: %u, S:%u, C:%u) and dispatched response.\n", response->sender_tag_lo, response->service_id, response->command_id);

    return 0;
}

static int driver_get_next_request(MESSAGE_PAYLOAD_t * request) {
    int rv;

    rv = basic_queue_dequeue_no_block(&gs_requests_queue, request);
    if (0 == rv) {
        printf("driver_get_next_request: picked-up request (T: %u, S:%u, C:%u).\n", request->sender_tag_lo, request->service_id, request->command_id);
    }

    return rv;
}

static int pseduo_driver(volatile SECURE_MAILBOX_INTERFACE_t * mailbox, bool master) {
    int rv;

    printf("Pseduo-driver started.\n");

    while (!terminate) {
        if (master) {
            rv = mailbox_master_handler(mailbox, &driver_reset_state, &driver_get_ready, &driver_dispatch_request, &driver_dispatch_response, &driver_get_next_request);
        } else {
            rv = mailbox_slave_handler(mailbox, &driver_reset_state, &driver_get_ready, &driver_dispatch_request, &driver_dispatch_response, &driver_get_next_request);
        }
        if (0 != rv) {
            break;
        }
    }

    printf("Pseduo-driver terminated.\n");

    return rv;
}

int run_tests(volatile SECURE_MAILBOX_INTERFACE_t * mailbox, bool master) {
    int rv, r;
    uint32_t n;
    MESSAGE_PAYLOAD_t dummy;

    gs_mailbox = mailbox;

    if (0 != basic_queue_create(&gs_requests_queue, sizeof(MESSAGE_PAYLOAD_t), REQUESTS_QUEUE_SIZE)) {
        printf("run_tests: basic_queue_create() failed!\n");
        return -1;
    }

    for (n = 0; n < MAX_PRODUCERS; n++) {
        if (0 != pthread_create(&gs_threads[n], NULL, &producer_thread, (void*)(size_t)n)) {
            printf("run_tests: pthread_create() failed!\n");
            return -1;
        }
    }

    rv = pseduo_driver(mailbox, master);

    terminate = true;

    // empty the queue to unblock the threads
    do {
        r = basic_queue_dequeue_no_block(&gs_requests_queue, &dummy);
    } while (0 == r);
    if (2 != r) {
        printf("run_tests: basic_queue_dequeue_no_block() failed!\n");
        rv = -1;
    }
    // wait for the threads to terminate
    for (n = 0; n < MAX_PRODUCERS; n++) {
        if (0 != pthread_join(gs_threads[n], NULL)) {
            printf("run_tests: pthread_join() failed!\n");
            rv = -1;
            break;
        }
    }

    // delete the queue
    if (0 != basic_queue_delete(&gs_requests_queue)) {
        printf("run_tests: basic_queue_delete() failed!\n");
        return -1;
    }

    return rv;
}
