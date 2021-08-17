# This file is the Test Framework command
# and response specification. This file is
# currently manually kept in sync with the
# device side specification.
import time, sys, os
import json
from io import BytesIO

sys.path.append(".")

class TfSpecification:
    def __init__(self, tf_spec_file):
        try:
            with open(tf_spec_file, "r") as f:
                self.data = json.load(f)
                f.close()
                # inverse json command and response id definition to obatin
                # a dictionary that uses command and response indces as key
                #commands
                tf_cmd_ids = self.data["tf_sp_command_ids"]
                self.tf_cmd_id_list = {}
                cmd_id_count=tf_cmd_ids["TF_CMD_OFFSET"]
                for key in tf_cmd_ids:
                    self.tf_cmd_id_list[tf_cmd_ids[key]] = key
                    cmd_id_count += 1
                self.tf_cmd_id_count = cmd_id_count
                #responses
                tf_rsp_ids = self.data["tf_sp_response_ids"]
                self.tf_rsp_id_list = {}
                rsp_id_count=tf_rsp_ids["TF_RSP_OFFSET"]
                for key in tf_rsp_ids:
                    self.tf_rsp_id_list[tf_rsp_ids[key]] = key
                    rsp_id_count += 1
                self.tf_rsp_id_count = rsp_id_count
                #device-ops api responses
                tf_ops_rsp_ids = self.data["tf_mm_device_api_rsp_ids"]
                self.tf_ops_rsp_id_list = {}
                for key in tf_ops_rsp_ids:
                    self.tf_ops_rsp_id_list[tf_ops_rsp_ids[key]] = key
        except Exception:
            print("Error opening test specification JSON file")

    def view_json(self):
        print(json.dumps(self.data, indent=4))

    def prettyprint(self, d, indent=0):
        for key, value in d.items():
            print('\t' * indent + str(key))
            if isinstance(value, dict):
                self.prettyprint(value, indent+1)
            else:
                print('\t' * (indent+1) + str(value))

    def response_process_shell_payload(self, raw_rsp, start_idx, payload_size):
        new_processed_rsp = {}
        ctypes = self.data["cdata_types"]
        rsp_hdr = self.data["tf_mm_device_api_rsp_hdr"]
        #obtain size
        end_idx = start_idx + ctypes[rsp_hdr["size"]]
        new_processed_rsp["size"] = int.from_bytes(raw_rsp[start_idx:end_idx],"little")
        start_idx = end_idx
        #obtain tag id
        end_idx = start_idx + ctypes[rsp_hdr["tag_id"]]
        new_processed_rsp["tag_id"] = int.from_bytes(raw_rsp[start_idx:end_idx],"little")
        start_idx = end_idx
        #obtain msg id
        end_idx = start_idx + ctypes[rsp_hdr["msg_id"]]
        rsp_id = int.from_bytes(raw_rsp[start_idx:end_idx],"little")
        new_processed_rsp["msg_id"] = rsp_id
        start_idx = end_idx
        #obtain flags
        end_idx = start_idx + ctypes[rsp_hdr["flags"]]
        new_processed_rsp["flags"] = int.from_bytes(raw_rsp[start_idx:end_idx],"little")
        start_idx = end_idx
        #obtain rsp name and spec
        rsp_name = self.tf_ops_rsp_id_list[rsp_id]
        rsp_spec = self.data["tf_mm_device_api_rsp_spec"][rsp_name]
        rsp_hdr_size = ctypes[rsp_hdr["size"]] + ctypes[rsp_hdr["tag_id"]] + ctypes[rsp_hdr["msg_id"]] + ctypes[rsp_hdr["flags"]]
        #populate payload args
        if new_processed_rsp["size"] > rsp_hdr_size:
            payload_args = rsp_spec["payload_args"]
            idx = start_idx
            for arg in payload_args:
                start_idx = idx
                end_idx = start_idx + ctypes[payload_args[arg]]
                new_processed_rsp[arg] = int.from_bytes(raw_rsp[start_idx:end_idx],"little")
                idx = idx + (end_idx-start_idx)

        return new_processed_rsp

    def response_get_payload_args(self, processed_rsp, raw_rsp, rsp_spec, end_idx, mm_rsp_shell):
        ctypes = self.data["cdata_types"]
        if processed_rsp["payload_size"] != 0:
            payload_args = rsp_spec["payload_args"]
            #print(payload_args)
            #set idx to start of payload
            idx = end_idx
            for arg in payload_args:
                start_idx = idx
                arg_val = payload_args[arg]
                if arg_val == "unknown_bytes" and mm_rsp_shell == True:
                    processed_rsp = self.response_process_shell_payload(raw_rsp, start_idx, processed_rsp["mm_rsp_size"])
                elif arg_val == "flexible_bytes" and "bytes_read" in payload_args:
                    end_idx = start_idx + processed_rsp["bytes_read"]
                    processed_rsp[arg] = int.from_bytes(raw_rsp[start_idx:end_idx], "little")
                    idx = idx + (end_idx - start_idx)
                elif arg_val == "flexible_bytes" or arg_val == "unknown_bytes":
                    processed_rsp[arg] = "This is variable sized data."
                else:
                    end_idx = start_idx + ctypes[payload_args[arg]]
                    processed_rsp[arg] = int.from_bytes(raw_rsp[start_idx:end_idx],"little")
                    idx = idx + (end_idx-start_idx)
        return processed_rsp

    def command(self, key, target, *user_args):
        protocol =self.data["tf_protocol"]
        ctypes = self.data["cdata_types"]
        data = BytesIO()
        #Obtain command specification based on user key
        if(target == "SP"):
            cmd_id = self.data["tf_sp_command_ids"][key]
            cmds_spec = self.data["tf_sp_cmd_spec"][key]
            #Add protocol start
            data.write(protocol["body_start"].to_bytes(1,byteorder='little'))
            #Add header
            data.write(cmd_id.to_bytes(protocol["hdr_id_bytes"],byteorder='little')) #cmd_id
            data.write(cmds_spec["flags"].to_bytes(protocol["hdr_flags_bytes"],byteorder='little')) #flags
            data.write(cmds_spec["payload_size"].to_bytes(protocol["hdr_payloadSize_bytes"],byteorder='little')) #payload size
        if(target == "MM"):
            cmd_id = self.data["tf_mm_device_api_cmd_ids"][key]
            cmds_spec = self.data["tf_mm_device_api_cmd_spec"][key]
            mm_cmd_hdr = self.data["tf_mm_device_api_cmd_hdr"]
            mm_tag_id = 1 #TODO; hardcoding for now
            mm_flags = 0 #TODO; hardcoding for now
            #Add header -- ARRDEBUG - read this Arvind, properly create MM command header dynamically here
            mm_payload_size = cmds_spec["payload_size"] + ctypes[mm_cmd_hdr["size"]] + ctypes[mm_cmd_hdr["tag_id"]] + ctypes[mm_cmd_hdr["msg_id"]] + ctypes[mm_cmd_hdr["flags"]]
            data.write(mm_payload_size.to_bytes(ctypes["uint16_t"], byteorder='little')) #size = mm header size + payload size
            data.write(mm_tag_id.to_bytes(ctypes["uint16_t"], byteorder='little')) #tag_id
            data.write(cmd_id.to_bytes(ctypes["uint16_t"], byteorder='little')) #msg_id
            data.write(mm_flags.to_bytes(ctypes["uint16_t"], byteorder='little')) #flags
        #Add payload
        payload_args = cmds_spec["payload_args"]
        #Ensure num user args and json spec for command match
        assert (len(payload_args) == len(user_args)), "User Error: Improper arg count for test command"
        #Write user privoded payload args to test payload
        if(cmd_id == self.data["tf_sp_command_ids"]["TF_CMD_MOVE_DATA_TO_DEVICE"]):
            data.write(user_args[0].to_bytes(ctypes[payload_args["dst_addr"]],byteorder='little'))
            data.write(user_args[1].to_bytes(ctypes[payload_args["size"]],byteorder='little'))
            data.write(user_args[2])
        elif(cmd_id == self.data["tf_sp_command_ids"]["TF_CMD_MM_CMD_SHELL"]):
            data.write(user_args[0].to_bytes(ctypes[payload_args["mm_cmd_size"]],byteorder='little'))
            data.write(user_args[1])
        else:
            args_count=0
            for key in payload_args:
                data.write(user_args[args_count].to_bytes(ctypes[payload_args[key]],byteorder='little'))
                args_count += 1
        if(target == "SP"):
            #Add protocol byte - <body end #>
            data.write(protocol["body_end"].to_bytes(1,byteorder='little'))
            #Add crc32
            data.write((0xA5A5A5A5).to_bytes(4,byteorder='little'))
            #Add protocol byte - <crc end %>
            data.write(protocol["crc_end"].to_bytes(1,byteorder='little'))
        #Reset stream pointer
        data.seek(0)
        return data.read()

    def response(self, raw_rsp):
        processed_rsp = {}
        protocol = self.data["tf_protocol"]
        ctypes = self.data["cdata_types"]
        byte_idx = 0
        for byte in raw_rsp:
            if byte == protocol["body_start"]:
                #obtain response id
                start_idx = byte_idx + protocol["id_idx"]
                end_idx = start_idx + ctypes["uint16_t"]
                rsp_id = int.from_bytes(raw_rsp[start_idx:end_idx],"little")
                processed_rsp["id"] = rsp_id
                #check for shell command response
                mm_rsp_shell = rsp_id == self.data["tf_sp_response_ids"]["TF_RSP_MM_CMD_SHELL"]
                #obtain response name
                rsp_name = self.tf_rsp_id_list[rsp_id]
                #obtain response spec
                rsp_hdr = self.data["tf_cmd_rsp_hdr"]
                rsp_spec = self.data["tf_sp_rsp_spec"][rsp_name]
                #obtain flags
                start_idx = byte_idx + protocol["flags_idx"]
                end_idx = start_idx + ctypes[rsp_hdr["flags"]]
                processed_rsp["flags"] = int.from_bytes(raw_rsp[start_idx:end_idx],"little")
                #obtain payload size
                start_idx = byte_idx + protocol["payload_size_idx"]
                end_idx = start_idx + ctypes[rsp_hdr["payload_size"]]
                processed_rsp["payload_size"] = int.from_bytes(raw_rsp[start_idx:end_idx],"little")
                #obtain payload args
                processed_rsp = self.response_get_payload_args(processed_rsp, raw_rsp, rsp_spec, end_idx, mm_rsp_shell)
                break
            byte_idx += 1
        return processed_rsp