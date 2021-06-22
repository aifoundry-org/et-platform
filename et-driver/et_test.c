/* SPDX-License-Identifier: GPL-2.0 */

/***********************************************************************
 *
 * Copyright (C) 2020 Esperanto Technologies Inc.
 * The copyright to the computer program(s) herein is the
 * property of Esperanto Technologies, Inc. All Rights Reserved.
 * The program(s) may be used and/or copied only with
 * the written permission of Esperanto Technologies and
 * in accordance with the terms and conditions stipulated in the
 * agreement/contract under which the program(s) have been supplied.
 *
 **********************************************************************/

#include "et_test.h"
#include "et_vqueue.h"

static ssize_t et_cqueue_copy_to_kbuf(struct et_pci_dev *et_dev,
				      bool is_mgmt,
				      u16 cq_index,
				      char *kbuf,
				      size_t count)
{
	struct et_cqueue *cq;
	struct et_msg_node *msg = NULL;
	ssize_t rv;

	if (is_mgmt)
		cq = et_dev->mgmt.cq_pptr[cq_index];
	else
		cq = et_dev->ops.cq_pptr[cq_index];

	if (cq->vq_common->aborting)
		return -EINTR;

	if (!kbuf || !count)
		return -EINVAL;

	msg = et_dequeue_msg_node(cq);
	if (!msg || !(msg->msg)) {
		// Empty; no message to POP, returning EAGAIN
		rv = -EAGAIN;
		goto update_cq_bitmap;
	}

	if (count < msg->msg_size) {
		pr_err("Kernel buffer not large enough\n");
		rv = -ENOMEM;
		goto update_cq_bitmap;
	}

	memcpy(kbuf, msg->msg, msg->msg_size);

	rv = msg->msg_size;

update_cq_bitmap:
	// Update cq_bitmap
	if (et_cqueue_msg_available(cq))
		set_bit(cq->index, cq->vq_common->cq_bitmap);
	else
		clear_bit(cq->index, cq->vq_common->cq_bitmap);

	return rv;
}

long test_virtqueue(struct et_pci_dev *et_dev, u16 cmd_count)
{
	struct device_ops_echo_cmd_t echo_cmd;
	struct device_ops_echo_rsp_t echo_rsp;
	u16 cmd_sent, rsp_received;
	u16 sq_idx = 0, cq_idx = 0;
	u16 sq_threshold_old;
	long rv;

	// Fill in the command struct
	echo_cmd.command_info.cmd_hdr.size = sizeof(echo_cmd);
	echo_cmd.command_info.cmd_hdr.msg_id = DEV_OPS_API_MID_ECHO_CMD;
	echo_cmd.command_info.cmd_hdr.flags = 0;

	cmd_sent = 0;
	rsp_received = 0;

	pr_info("TEST_VQ: cmd_count: %d", cmd_count);

	// Saving actual threshold value
	sq_threshold_old =
		atomic_read(&et_dev->ops.sq_pptr[sq_idx]->sq_threshold);

	atomic_set(&et_dev->ops.sq_pptr[sq_idx]->sq_threshold,
		   sizeof(echo_cmd));

	while (cmd_sent < cmd_count || rsp_received < cmd_count) {
		// If no event occurred for 10secs, exit the test
		rv = wait_event_interruptible_timeout(
			et_dev->ops.vq_common.waitqueue,
			et_squeue_event_available(
				et_dev->ops.sq_pptr[sq_idx]) ||
				et_cqueue_event_available(
					et_dev->ops.cq_pptr[cq_idx]),
			msecs_to_jiffies(10000));
		if (rv == -ERESTARTSYS) {
			goto error_destroy_msg_list;
		} else if (!rv) {
			rv = -ETIMEDOUT;
			goto error_destroy_msg_list;
		}

		rv = sizeof(echo_cmd);

		for (; cmd_sent < cmd_count; cmd_sent++) {
			echo_cmd.command_info.cmd_hdr.tag_id = cmd_sent;

			rv = et_squeue_push(et_dev->ops.sq_pptr[sq_idx],
					    &echo_cmd,
					    sizeof(echo_cmd));
			if (rv <= 0)
				break;
		}

		if (rv <= 0 && rv != -EAGAIN) {
			pr_err("ioctl: ETSOC1_IOCTL_TEST_VQ: SQ_PUSH failed!\n");
			goto error_destroy_msg_list;
		}

		for (; rsp_received < cmd_sent; rsp_received++) {
			rv = et_cqueue_copy_to_kbuf(et_dev,
						    false /* ops_dev */,
						    cq_idx,
						    (char *)&echo_rsp,
						    sizeof(echo_rsp));
			if (rv <= 0)
				break;

			if (echo_rsp.device_cmd_start_ts == 0) {
				rv = -EINVAL;
				break;
			}
		}

		if (rv <= 0 && rv != -EAGAIN) {
			pr_err("ioctl: ETSOC1_IOCTL_TEST_VQ: CQ_POP failed!\n");
			goto error_destroy_msg_list;
		}
	}

	return cmd_sent;

error_destroy_msg_list:
	pr_info("Commands completed (%d/%d)", rsp_received, cmd_count);
	et_destroy_msg_list(et_dev->ops.cq_pptr[cq_idx]);

	// Restoring actual threshold value
	atomic_set(&et_dev->ops.sq_pptr[sq_idx]->sq_threshold,
		   sq_threshold_old);

	return rv;
}
