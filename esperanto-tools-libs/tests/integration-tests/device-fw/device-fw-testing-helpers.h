/*------------------------------------------------------------------------------
 * Copyright (C) 2020, Esperanto Technologies Inc.
 * The copyright to the computer program(s) herein is the
 * property of Esperanto Technologies, Inc. All Rights Reserved.
 * The program(s) may be used and/or copied only with
 * the written permission of Esperanto Technologies and
 * in accordance with the terms and conditions stipulated in the
 * agreement/contract under which the program(s) have been supplied.
 ------------------------------------------------------------------------------ */

#include "device-fw-testing-helpers-common.h"

bool Test_Trace_verify_ring_buffer_data(uint8_t* trace_buffers, size_t buffer_size, uint32_t num_of_buffers,
                                        int test_case_nm) {
  size_t tail;
  size_t head;
  uint8_t* trace_buffer;
  ::device_api::non_privileged::buffer_header_t* buffer_hdr;
  ::device_api::non_privileged::message_header_t* msg_hdr;

  const size_t test_string_size = 128;         // Test case specific
  const uint32_t max_str_events_per_buff = 30; // MAX  string messages per buffer when test_string_size is 128
  const uint32_t extra_events = 4;             // Number of packets more than the full buffer size

  char test_buff[test_string_size]; // Data to be used for testing
  bool test_status = true;
  uint32_t counter;                     // Messages counter per buffer by one minion
  uint32_t all_minions_msg_counter = 0; // Sum of all the trace messages logged by all minions
  char test_char = 'A';                 // Alphabet to be used to create test data
  uint32_t i = 0;                       // Trace buffer index
  uint32_t trace_buffer_counter = 0;    // Trace buffer counter

  // Clear out the string
  memset(test_buff, 0, test_string_size);

  // Loop through all the ring buffers
  for (; i < num_of_buffers; i++) {
    // Fetch next ring buffer from memory
    trace_buffer = trace_buffers + buffer_size * i;
    buffer_hdr = (::device_api::non_privileged::buffer_header_t*)trace_buffer;
    tail = buffer_hdr->tail;
    head = buffer_hdr->head;

    // Reset the message counter per ring buffer
    counter = 0;

    // Loop until the tail approaches the head
    while (tail != head) {
      // Fetch the current message
      msg_hdr = (::device_api::non_privileged::message_header_t*)(&(buffer_hdr->buffer[tail]));

      switch (msg_hdr->event_id) {

      case ::device_api::non_privileged::TRACE_EVENT_ID_OVERFLOW: {
        RTDEBUG << "Overflow event:" << msg_hdr->event_id << " Hart ID:" << buffer_hdr->hart_id
                << " Buffer is wrapped up. "
                << "\n";
        // Buffer overflow condition, reset tail
        tail = 0;
        break;
      }

      case ::device_api::non_privileged::TRACE_EVENT_ID_TEXT_STRING: {
        // Count the valueable ring buffers i.e. ones that contain our desired data
        trace_buffer_counter = i + 1;

        ::device_api::non_privileged::trace_string_t* trace_string =
          (::device_api::non_privileged::trace_string_t*)msg_hdr;

        // A string message was logged, increase the message counter/buffer
        counter++;

        // Check the ring buffer test case. It can be 1, 2 or 3.
        if (test_case_nm == 3) {
          /* This stands for the Case # 3 which checks the wrapping functionality of the buffer.
             On the device side, 4 messages more than the full buffer size were logged to wrapp it up.
             So, as an overflow of four messages were introduced, add four to create a
             letter/data for first iteration and keep increasing it accordingly. 65 is ascii A. */
          test_char = (char)(65 + counter - 1 + extra_events);
        }

        else {
          /* This stands for case # 1 (Normal read) and Case # 2 (Full buffer). Increment the character 'A'
             to create new data on every iteration. For first iteration it should be 'A'.  65 is ascii A. */
          test_char = (char)(65 + counter - 1);
        }

        // Prepare the test data
        memset(test_buff, test_char, test_string_size - 1);

        // Compare the received message with expected message
        if (strncmp(trace_string->msg, test_buff, trace_string->size) != 0) {
          RTDEBUG << "Ivalid Data: Hart ID: " << i << " Test No. " << test_case_nm << " Message No: " << counter
                  << " Size: " << trace_string->size << " Message: " << trace_string->msg << "\r\n";
          test_status = false;
        }

        std::vector<uint8_t> msg(trace_string->msg, trace_string->msg + trace_string->size);

        tail += ALIGN(offsetof(::device_api::non_privileged::trace_string_t, msg) + trace_string->size, 8);
        break;
      }

      default: {
        RTDEBUG << "Invalid Event Id: " << msg_hdr->event_id << "\n";
        return false;
      }
      }
    }

    all_minions_msg_counter += counter;
  }

  if ((test_status == true) && ((all_minions_msg_counter == trace_buffer_counter) ||
                                (all_minions_msg_counter == (max_str_events_per_buff * trace_buffer_counter)))) {
    return true;
  } else {
    RTDEBUG << "Failed: test_status: " << test_status << " All minion messages counter: " << all_minions_msg_counter
            << "trace_buffer_counter: " << trace_buffer_counter;
    return false;
  }
}
