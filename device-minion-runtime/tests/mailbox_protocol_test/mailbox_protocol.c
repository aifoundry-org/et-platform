#include "mailbox_protocol.h"

int mailbox_master_init(volatile SECURE_MAILBOX_INTERFACE_t * mailbox) {

}

int mailbox_slave_init(volatile SECURE_MAILBOX_INTERFACE_t * mailbox) {

}

int mailbox_master_send_hi_pri_req_to_slave(volatile SECURE_MAILBOX_INTERFACE_t * mailbox, const MESSAGE_PAYLOAD_t * request, MESSAGE_PAYLOAD_t * response) {

}

int mailbox_master_send_hi_pri_rsp_to_slave(volatile SECURE_MAILBOX_INTERFACE_t * mailbox, const MESSAGE_PAYLOAD_t * response) {

}

int mailbox_is_master_to_slave_req_queue_full(volatile SECURE_MAILBOX_INTERFACE_t * mailbox) {

}

int mailbox_is_master_to_slave_rsp_queue_empty(volatile SECURE_MAILBOX_INTERFACE_t * mailbox) {

}

int mailbox_is_slave_to_master_req_queue_empty(volatile SECURE_MAILBOX_INTERFACE_t * mailbox) {

}

int mailbox_is_slave_to_master_rsp_queue_full(volatile SECURE_MAILBOX_INTERFACE_t * mailbox) {

}

int mailbox_master_enqueue_lo_pri_req_to_slave(volatile SECURE_MAILBOX_INTERFACE_t * mailbox, const MESSAGE_PAYLOAD_t * request) {

}

int mailbox_master_dequeue_lo_pri_rsp_from_slave(volatile SECURE_MAILBOX_INTERFACE_t * mailbox, MESSAGE_PAYLOAD_t * response) {

}

int mailbox_master_dequeue_lo_pri_req_from_slave(volatile SECURE_MAILBOX_INTERFACE_t * mailbox, MESSAGE_PAYLOAD_t * request) {

}

int mailbox_master_enqueue_lo_pri_rsp_to_slave(volatile SECURE_MAILBOX_INTERFACE_t * mailbox, const MESSAGE_PAYLOAD_t * response) {

}

int mailbox_slave_send_hi_pri_rsp_to_master(volatile SECURE_MAILBOX_INTERFACE_t * mailbox, const MESSAGE_PAYLOAD_t * response) {

}

int mailbox_slave_send_hi_pri_req_to_master(volatile SECURE_MAILBOX_INTERFACE_t * mailbox, const MESSAGE_PAYLOAD_t * request, MESSAGE_PAYLOAD_t * response) {

}

int mailbox_is_master_to_slave_req_queue_empty(volatile SECURE_MAILBOX_INTERFACE_t * mailbox) {

}

int mailbox_is_master_to_slave_rsp_queue_full(volatile SECURE_MAILBOX_INTERFACE_t * mailbox) {

}

int mailbox_is_slave_to_master_req_queue_full(volatile SECURE_MAILBOX_INTERFACE_t * mailbox) {

}

int mailbox_is_slave_to_master_rsp_queue_empty(volatile SECURE_MAILBOX_INTERFACE_t * mailbox) {

}

int mailbox_master_enqueue_lo_pri_req_to_slave(volatile SECURE_MAILBOX_INTERFACE_t * mailbox, const MESSAGE_PAYLOAD_t * request) {

}

int mailbox_master_dequeue_lo_pri_rsp_from_slave(volatile SECURE_MAILBOX_INTERFACE_t * mailbox, MESSAGE_PAYLOAD_t * response) {

}

int mailbox_master_dequeue_lo_pri_req_from_slave(volatile SECURE_MAILBOX_INTERFACE_t * mailbox, MESSAGE_PAYLOAD_t * request) {

}

int mailbox_master_enqueue_lo_pri_rsp_to_slave(volatile SECURE_MAILBOX_INTERFACE_t * mailbox, const MESSAGE_PAYLOAD_t * response) {

}
