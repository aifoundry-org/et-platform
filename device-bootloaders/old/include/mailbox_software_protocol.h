#ifndef __MAILBOX_SOFTWARE_PROTOCOL__
#define __MAILBOX_SOFTWARE_PROTOCOL__

#include <stdint.h>
#include <assert.h>

#define MAILBOX_INTERFACE_VERSION 0x00000001
#define LOW_PRIORITY_QUEUE_SIZE 15

typedef enum MAILBOX_MASTER_STATUS_e {
    MAILBOX_MASTER_STATUS_NOT_READY = 0,
    MAILBOX_MASTER_STATUS_READY,
    MAILBOX_MASTER_STATUS_WAITING_FOR_SLAVE,
    MAILBOX_MASTER_STATUS_FATAL_ERROR,
    MAILBOX_MASTER_STATUS_UNINITIALIZED // reserved
} MAILBOX_MASTER_STATUS_t;

typedef enum MAILBOX_SLAVE_STATUS_e {
    MAILBOX_SLAVE_STATUS_NOT_READY = 0,
    MAILBOX_SLAVE_STATUS_READY,
    MAILBOX_SLAVE_STATUS_WAITING_FOR_MB_RESET,
    MAILBOX_SLAVE_STATUS_FATAL_ERROR,
    MAILBOX_SLAVE_STATUS_UNINITIALIZED // reserved
} MAILBOX_SLAVE_STATUS_t;

typedef struct MESSAGE_PAYLOAD_s {
    uint32_t service_id;
    uint32_t command_id;
    uint32_t sender_tag_lo;
    uint32_t sender_tag_hi;
    uint32_t data[12];
} MESSAGE_PAYLOAD_t;
static_assert(64 == sizeof(MESSAGE_PAYLOAD_t), "sizeof(MESSAGE_PAYLOAD_t) is not 64!");

typedef struct MAILBOX_QUEUE_DESCRIPTOR_s {
    uint32_t head_index;
    uint32_t tail_index;
} MAILBOX_QUEUE_DESCRIPTOR_t;
static_assert(8 == sizeof(MAILBOX_QUEUE_DESCRIPTOR_t), "sizeof(MAILBOX_QUEUE_DESCRIPTOR_t) is not 8!");

typedef struct SECURE_MAILBOX_CONTROL_s {
    uint32_t status;
    uint32_t reserved[5];
    uint32_t hi_pri_req_ready;  // request to the other side
    uint32_t hi_pri_rsp_ready;  // response to requests from the other side

    uint32_t lo_pri_req_head;   // head index for the request queue to the other side
    uint32_t lo_pri_rsp_tail;   // tail index for the response queue from the other side
    uint32_t lo_pri_req_tail;   // tail index for the request queue from the other side
    uint32_t lo_pri_rsp_head;   // head index for the response queue to the other side
} SECURE_MAILBOX_CONTROL_t;

typedef struct SECURE_MAILBOX_INTERFACE_s {
    // cache line 0

    uint32_t interface_size;
    uint32_t interface_version;
    uint32_t reserved0[2];

    uint32_t master_status;
    uint32_t reserved1[5];
    uint32_t hi_pri_m2s_req_ready;
    uint32_t hi_pri_s2m_rsp_ready;

    uint32_t lo_pri_m2s_req_head;
    uint32_t lo_pri_m2s_rsp_tail;
    uint32_t lo_pri_s2m_req_tail;
    uint32_t lo_pri_s2m_rsp_head;

    // cache line 1

    uint32_t reserved2[4];

    uint32_t slave_status;
    uint32_t reserved3[5];
    uint32_t hi_pri_m2s_rsp_ready;
    uint32_t hi_pri_s2m_req_ready;

    uint32_t lo_pri_m2s_req_tail;
    uint32_t lo_pri_m2s_rsp_head;
    uint32_t lo_pri_s2m_req_head;
    uint32_t lo_pri_s2m_rsp_tail;

    // cache line 2

    MESSAGE_PAYLOAD_t hi_pri_m2s_data;

    // cache line 3

    MESSAGE_PAYLOAD_t hi_pri_s2m_data;

    // cache lines 4-18

    MESSAGE_PAYLOAD_t lo_pri_m2s_req_queue[LOW_PRIORITY_QUEUE_SIZE];

    // cache lines 19-33

    MESSAGE_PAYLOAD_t lo_pri_m2s_rsp_queue[LOW_PRIORITY_QUEUE_SIZE];

    // cache lines 34-48

    MESSAGE_PAYLOAD_t lo_pri_s2m_req_queue[LOW_PRIORITY_QUEUE_SIZE];

    // cache lines 49-63

    MESSAGE_PAYLOAD_t lo_pri_s2m_rsp_queue[LOW_PRIORITY_QUEUE_SIZE];
} SECURE_MAILBOX_INTERFACE_t;
static_assert(4096 == sizeof(SECURE_MAILBOX_INTERFACE_t), "sizeof(SECURE_MAILBOX_INTERFACE_t) is not 4096!");

#endif
