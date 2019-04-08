#include <stdbool.h>
#include <stdio.h>

#include "mailbox_protocol.h"

typedef enum HI_PRI_RX_CHANNEL_STATE_e {
    HI_PRI_RX_CHANNEL_STATE_INVALID,
    HI_PRI_RX_CHANNEL_STATE_WAITING_FOR_REQUEST,
    HI_PRI_RX_CHANNEL_STATE_SENT_RESPONSE
} HI_PRI_RX_CHANNEL_STATE_t;

typedef enum HI_PRI_TX_CHANNEL_STATE_e {
    HI_PRI_TX_CHANNEL_STATE_INVALID,
    HI_PRI_TX_CHANNEL_STATE_IDLE,
    HI_PRI_TX_CHANNEL_STATE_SENT_REQUEST,
    HI_PRI_TX_CHANNEL_STATE_WAITING_FOR_RSP_CLEAR
} HI_PRI_TX_CHANNEL_STATE_t;

static MAILBOX_SLAVE_STATUS_t sg_slave_status = MAILBOX_SLAVE_STATUS_NOT_READY;
static MAILBOX_MASTER_STATUS_t sg_old_master_status = MAILBOX_MASTER_STATUS_UNINITIALIZED;
static HI_PRI_TX_CHANNEL_STATE_t sg_tx_channel_status = HI_PRI_TX_CHANNEL_STATE_IDLE;
static HI_PRI_RX_CHANNEL_STATE_t sg_rx_channel_status = HI_PRI_RX_CHANNEL_STATE_WAITING_FOR_REQUEST;

static int mailbox_slave_check_for_messages(volatile SECURE_MAILBOX_INTERFACE_t * mailbox, DISPATCH_REQUEST_PFN req_dispatch_function, DISPATCH_RESPONSE_PFN rsp_dispatch_function, GET_NEXT_REQUEST_PFN get_next_req_function) {
    MESSAGE_PAYLOAD_t response, request;
    switch (sg_tx_channel_status) {
    case HI_PRI_TX_CHANNEL_STATE_IDLE:
        if (0 != get_next_req_function(&request)) {
            mailbox->hi_pri_s2m_data = request;
            mailbox->hi_pri_s2m_req_ready = 1;
            sg_tx_channel_status = HI_PRI_TX_CHANNEL_STATE_SENT_REQUEST;
        }
        break;
    
    // handle TX channel state transitions
    case HI_PRI_TX_CHANNEL_STATE_SENT_REQUEST:
        if (0 != mailbox->hi_pri_s2m_rsp_ready) {
            response = mailbox->hi_pri_s2m_data;
            rsp_dispatch_function(mailbox, &response);
            mailbox->hi_pri_s2m_req_ready = 0;
            sg_tx_channel_status = HI_PRI_TX_CHANNEL_STATE_WAITING_FOR_RSP_CLEAR;
        }
        break;
    
    case HI_PRI_TX_CHANNEL_STATE_WAITING_FOR_RSP_CLEAR:
        if (0 == mailbox->hi_pri_s2m_rsp_ready) {
            mailbox->hi_pri_s2m_req_ready = 0;
            sg_tx_channel_status = HI_PRI_TX_CHANNEL_STATE_IDLE;
        }
        break;

    default:
        printf("Invalid TX Channel state!\n");
        return -1;
    }

    // handle RX channel state transitions
    switch (sg_rx_channel_status) {
    case HI_PRI_RX_CHANNEL_STATE_WAITING_FOR_REQUEST:
        if (mailbox->hi_pri_m2s_req_ready) {
            request = mailbox->hi_pri_m2s_data;
            req_dispatch_function(mailbox, &request, &response);
            mailbox->hi_pri_m2s_data = response;
            mailbox->hi_pri_m2s_rsp_ready = 1;
            sg_rx_channel_status = HI_PRI_RX_CHANNEL_STATE_SENT_RESPONSE;
        }
        break;
    
    case HI_PRI_RX_CHANNEL_STATE_SENT_RESPONSE:
        if (0 == mailbox->hi_pri_m2s_req_ready) {
            mailbox->hi_pri_m2s_rsp_ready = 0;
            sg_rx_channel_status = HI_PRI_RX_CHANNEL_STATE_WAITING_FOR_REQUEST;
        }
        break;

    default:
        printf("Invalid RX Channel state!\n");
        return -1;
    }

    return 0;
}

int mailbox_slave_handler(volatile SECURE_MAILBOX_INTERFACE_t * mailbox, DRIVER_RESET_STATE_PFN driver_reset_state_pfn, DRIVER_GET_READY_PFN driver_get_ready_pfn, DISPATCH_REQUEST_PFN dispatch_req_function, DISPATCH_RESPONSE_PFN dispatch_rsp_function, GET_NEXT_REQUEST_PFN get_next_req_function) {
    MAILBOX_MASTER_STATUS_t new_master_status = mailbox->master_status;
    if (sg_old_master_status != new_master_status) {
        switch (sg_slave_status) {
        case MAILBOX_SLAVE_STATUS_NOT_READY:
            switch (new_master_status) {
            case MAILBOX_MASTER_STATUS_NOT_READY:
                break;

            case MAILBOX_MASTER_STATUS_READY:
                printf("Slave: master unexpectedly reported status READY!\n");
                return -1;

            case MAILBOX_MASTER_STATUS_WAITING_FOR_SLAVE:
                if (0 != driver_get_ready_pfn(mailbox)) {
                    printf("Slave: driver_get_ready_pfn() failed!\n");
                    return -1;
                }
                sg_slave_status = MAILBOX_SLAVE_STATUS_READY;
                mailbox->slave_status = sg_slave_status;
                break;

            case MAILBOX_MASTER_STATUS_FATAL_ERROR:
                printf("Slave: master reported status FATAL_ERROR!\n");
                return -1;
            
            default:
                printf("Slave: master reported invalid status!\n");
                return -1;
            }
            break;

        case MAILBOX_SLAVE_STATUS_READY:
            switch (new_master_status) {
            case MAILBOX_MASTER_STATUS_NOT_READY:
                printf("Slave: master reported status NOT_READY!\n");
                sg_slave_status = MAILBOX_SLAVE_STATUS_NOT_READY;
                if (0 != driver_reset_state_pfn(mailbox)) {
                    printf("Slave: driver_reset_state_pfn() failed!\n");
                    return -1;
                }
                mailbox->slave_status = sg_slave_status;
                break;

            case MAILBOX_MASTER_STATUS_READY:
                break;

            case MAILBOX_MASTER_STATUS_WAITING_FOR_SLAVE:
                break;

            case MAILBOX_MASTER_STATUS_FATAL_ERROR:
                printf("Slave: master reported fatal error!\n");
                return -1;
            
            default:
                printf("Slave: master reported INVALID status value!\n");
                return -1;
            }
            break;

        case MAILBOX_SLAVE_STATUS_WAITING_FOR_MB_RESET:
            switch (new_master_status) {
            case MAILBOX_MASTER_STATUS_NOT_READY:
                sg_slave_status = MAILBOX_SLAVE_STATUS_NOT_READY;
                if (0 != driver_reset_state_pfn(mailbox)) {
                    printf("Slave: driver_reset_state_pfn() failed!\n");
                    return -1;
                }
                mailbox->slave_status = sg_slave_status;
                break;

            case MAILBOX_MASTER_STATUS_READY:
                break;

            case MAILBOX_MASTER_STATUS_WAITING_FOR_SLAVE:
                sg_slave_status = MAILBOX_SLAVE_STATUS_READY;
                mailbox->slave_status = sg_slave_status;
                break;

            case MAILBOX_MASTER_STATUS_FATAL_ERROR:
                printf("Master: slave reported fatal error!\n");
                return -1;

            default:
                printf("Master: slave reported INVALID status value!\n");
                return -1;
            }
            break;
            
        case MAILBOX_SLAVE_STATUS_FATAL_ERROR:
            printf("Slave: slave is in FATAL_ERROR state!\n");
            return -1;

        default:
            printf("INVALID master status value!\n");
            return -1;
        }

        sg_old_master_status = new_master_status;
    }

    if (MAILBOX_SLAVE_STATUS_READY == sg_slave_status && MAILBOX_MASTER_STATUS_READY == new_master_status) {
        return mailbox_slave_check_for_messages(mailbox, dispatch_req_function, dispatch_rsp_function, get_next_req_function);
    }

    return 0;
}

