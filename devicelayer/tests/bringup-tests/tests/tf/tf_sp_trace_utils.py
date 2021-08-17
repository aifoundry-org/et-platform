# SP trace standard defines
TRACE_CONTROL_TRACE_ENABLE = 1
TRACE_CONTROL_TRACE_DISABLE = 0
TRACE_CONTROL_TRACE_UART_ENABLE = 3
TRACE_CONTROL_TRACE_UART_DISABLE = 1
TRACE_CONFIGURE_EVENT_STRING = 1
TRACE_CONFIGURE_EVENT_PMC = 2
TRACE_CONFIGURE_EVENT_MARKER = 3
TRACE_CONFIGURE_TRACE_EVENT_ENABLE_ALL = 65535
TRACE_EVENT_STRING_CRITICAL = 0
TRACE_EVENT_STRING_ERROR = 1
TRACE_EVENT_STRING_WARNING = 2
TRACE_EVENT_STRING_INFO = 3
TRACE_EVENT_STRING_DEBUG = 4
TRACE_BUFFER_HDR_SIZE = 16
TRACE_STRING_HDR_SIZE = 16
TRACE_STRING_DATA_SIZE = 64
TRACE_STRING_EVENT_SIZE = 80


class TfSpTraceUtils:
    def __init__(self, tf_spec_):
        print("Initializing TF SP trace helper object...")
        self.tf_spec = tf_spec_

    def get_control_cmd(self):
        # Redirect tarce to tarce buffer
        command = self.tf_spec.command("TF_CMD_SP_TRACE_RUN_CONTROL", "SP", TRACE_CONTROL_TRACE_UART_DISABLE)
        return command

    def get_config_cmd(self):
        # Configure string event with debug level
        command = self.tf_spec.command("TF_CMD_SP_TRACE_RUN_CONFIG", "SP", TRACE_CONFIGURE_EVENT_STRING
                                       , TRACE_EVENT_STRING_DEBUG)
        return command

    def get_trace_info_cmd(self):
        command = self.tf_spec.command("TF_CMD_SP_TRACE_GET_INFO", "SP")
        return command

    # get trace buffer for given base address upto offset address
    def get_trace_buffer_for_offset_cmd(self, base, offset):
        command = self.tf_spec.command("TF_CMD_MOVE_DATA_TO_HOST", "SP", base, offset)
        return command

    def get_trace_buffer_cmd(self):
        command = self.tf_spec.command("TF_CMD_SP_TRACE_GET_BUFFER", "SP")
        return command

    def parse_trace_buffer(self, raw_trace):
        bytes_data = raw_trace["data_ptr"].to_bytes(raw_trace["bytes_read"], 'little')
        trace_buffer = []
        # Remove trace header
        string_events = bytes_data[TRACE_BUFFER_HDR_SIZE:]
        total_size = len(string_events)
        processed_size = 0
        end_idx = TRACE_STRING_EVENT_SIZE
        if total_size > 0:
            while processed_size != total_size:
                string_event = string_events[processed_size:end_idx]
                hdr = string_event[0:TRACE_STRING_HDR_SIZE]
                hdr_type = int.from_bytes(hdr[8:10], "little")
                data = string_event[TRACE_STRING_HDR_SIZE:].split(b'\00')[0].decode('utf-8')

                if hdr_type == 0 and len(data) != 0:
                    trace_buffer.append(str(data))
                processed_size += TRACE_STRING_EVENT_SIZE
                end_idx += TRACE_STRING_EVENT_SIZE

        return trace_buffer
