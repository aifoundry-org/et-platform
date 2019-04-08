#ifndef __MAILBOX_PROTOCOL_H__
#define __MAILBOX_PROTOCOL_H__

#include "../../src/ServiceProcessor/include/mailbox_software_protocol.h"

int mailbox_master_init(volatile SECURE_MAILBOX_INTERFACE_t * mailbox);
int mailbox_slave_init(volatile SECURE_MAILBOX_INTERFACE_t * mailbox);

typedef int (*DRIVER_RESET_STATE_PFN)(volatile SECURE_MAILBOX_INTERFACE_t * mailbox);
typedef int (*DRIVER_GET_READY_PFN)(volatile SECURE_MAILBOX_INTERFACE_t * mailbox);
typedef int (*DISPATCH_REQUEST_PFN)(volatile SECURE_MAILBOX_INTERFACE_t * mailbox, const MESSAGE_PAYLOAD_t * hi_pri_request, MESSAGE_PAYLOAD_t * hi_pri_response);
typedef int (*DISPATCH_RESPONSE_PFN)(volatile SECURE_MAILBOX_INTERFACE_t * mailbox, const MESSAGE_PAYLOAD_t * hi_pri_response);
typedef int (*GET_NEXT_REQUEST_PFN)(MESSAGE_PAYLOAD_t * hi_pri_request);

int mailbox_master_handler(volatile SECURE_MAILBOX_INTERFACE_t * mailbox, DRIVER_RESET_STATE_PFN driver_reset_state_pfn, DRIVER_GET_READY_PFN driver_get_ready_pfn, DISPATCH_REQUEST_PFN dispatch_req_function, DISPATCH_RESPONSE_PFN dispatch_rsp_function, GET_NEXT_REQUEST_PFN get_next_req_function);
int mailbox_slave_handler(volatile SECURE_MAILBOX_INTERFACE_t * mailbox, DRIVER_RESET_STATE_PFN driver_reset_state_pfn, DRIVER_GET_READY_PFN driver_get_ready_pfn, DISPATCH_REQUEST_PFN dispatch_req_function, DISPATCH_RESPONSE_PFN dispatch_rsp_function, GET_NEXT_REQUEST_PFN get_next_req_function);

int mailbox_master_send_hi_pri_req_to_slave(volatile SECURE_MAILBOX_INTERFACE_t * mailbox, const MESSAGE_PAYLOAD_t * request, MESSAGE_PAYLOAD_t * response);
int mailbox_master_send_hi_pri_rsp_to_slave(volatile SECURE_MAILBOX_INTERFACE_t * mailbox, const MESSAGE_PAYLOAD_t * response);

int mailbox_is_master_to_slave_req_queue_full(volatile SECURE_MAILBOX_INTERFACE_t * mailbox);
int mailbox_is_master_to_slave_rsp_queue_empty(volatile SECURE_MAILBOX_INTERFACE_t * mailbox);
int mailbox_is_slave_to_master_req_queue_empty(volatile SECURE_MAILBOX_INTERFACE_t * mailbox);
int mailbox_is_slave_to_master_rsp_queue_full(volatile SECURE_MAILBOX_INTERFACE_t * mailbox);

int mailbox_master_enqueue_lo_pri_req_to_slave(volatile SECURE_MAILBOX_INTERFACE_t * mailbox, const MESSAGE_PAYLOAD_t * request);
int mailbox_master_dequeue_lo_pri_rsp_from_slave(volatile SECURE_MAILBOX_INTERFACE_t * mailbox, MESSAGE_PAYLOAD_t * response);
int mailbox_master_dequeue_lo_pri_req_from_slave(volatile SECURE_MAILBOX_INTERFACE_t * mailbox, MESSAGE_PAYLOAD_t * request);
int mailbox_master_enqueue_lo_pri_rsp_to_slave(volatile SECURE_MAILBOX_INTERFACE_t * mailbox, const MESSAGE_PAYLOAD_t * response);

int mailbox_slave_send_hi_pri_rsp_to_master(volatile SECURE_MAILBOX_INTERFACE_t * mailbox, const MESSAGE_PAYLOAD_t * response);
int mailbox_slave_send_hi_pri_req_to_master(volatile SECURE_MAILBOX_INTERFACE_t * mailbox, const MESSAGE_PAYLOAD_t * request, MESSAGE_PAYLOAD_t * response);

int mailbox_is_master_to_slave_req_queue_empty(volatile SECURE_MAILBOX_INTERFACE_t * mailbox);
int mailbox_is_master_to_slave_rsp_queue_full(volatile SECURE_MAILBOX_INTERFACE_t * mailbox);
int mailbox_is_slave_to_master_req_queue_full(volatile SECURE_MAILBOX_INTERFACE_t * mailbox);
int mailbox_is_slave_to_master_rsp_queue_empty(volatile SECURE_MAILBOX_INTERFACE_t * mailbox);

int mailbox_master_enqueue_lo_pri_req_to_slave(volatile SECURE_MAILBOX_INTERFACE_t * mailbox, const MESSAGE_PAYLOAD_t * request);
int mailbox_master_dequeue_lo_pri_rsp_from_slave(volatile SECURE_MAILBOX_INTERFACE_t * mailbox, MESSAGE_PAYLOAD_t * response);
int mailbox_master_dequeue_lo_pri_req_from_slave(volatile SECURE_MAILBOX_INTERFACE_t * mailbox, MESSAGE_PAYLOAD_t * request);
int mailbox_master_enqueue_lo_pri_rsp_to_slave(volatile SECURE_MAILBOX_INTERFACE_t * mailbox, const MESSAGE_PAYLOAD_t * response);

#endif
