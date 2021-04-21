# This file is the Test Framework command
# and response specification. This file is
# currently manually kept in sync with the
# device side specification. 
import time, sys, os
import json
from io import BytesIO

sys.path.append(".")

class tf_specification:
    def __init__(self, tf_spec_file):
        try:
            with open(tf_spec_file, "r") as f:
                self.data = json.load(f)
                f.close()
                # inverse json command and response id definition to obatin
                # a dictionary that uses command and response indces as key
                #commands
                tf_cmd_ids = self.data["command_ids"] 
                self.tf_cmd_id_list = {}
                cmd_id_count=tf_cmd_ids["TF_CMD_OFFSET"]
                for key in tf_cmd_ids:
                    self.tf_cmd_id_list[cmd_id_count] = key 
                    cmd_id_count += 1
                self.tf_cmd_id_count = cmd_id_count
                #responses
                tf_rsp_ids = self.data["response_ids"] 
                self.tf_rsp_id_list = {}
                rsp_id_count=tf_rsp_ids["TF_RSP_OFFSET"]
                for key in tf_rsp_ids:
                    self.tf_rsp_id_list[rsp_id_count] = key 
                    rsp_id_count += 1
                self.tf_rsp_id_count = rsp_id_count
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

    def command(self, key, *user_args):
        #Obtain command specification based on user key
        cmd_id = self.data["command_ids"][key]
        cmds_spec = self.data["cmds"][key]
        protocol =self.data["protocol"]
        ctypes = self.data["cdata_types"]
        data = BytesIO()
        data.write(protocol["body_start"].to_bytes(1,byteorder='little'))
        #Add header
        data.write(cmd_id.to_bytes(protocol["hdr_id_bytes"],byteorder='little'))
        data.write(cmds_spec["flags"].to_bytes(protocol["hdr_flags_bytes"],byteorder='little'))
        data.write(cmds_spec["payload_size"].to_bytes(protocol["hdr_payloadSize_bytes"],byteorder='little'))
        #Add payload
        payload_args = cmds_spec["payload_args"]
        #Ensure num user args and json spec for command match
        assert (len(payload_args) == len(user_args)), "User Error: Improper arg count for test command"
        #Write user privoded payload args to test payload
        args_count=0
        for key in payload_args:
            data.write(user_args[args_count].to_bytes(ctypes[payload_args[key]],byteorder='little'))
            args_count += 1
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
        protocol = self.data["protocol"]
        ctypes = self.data["cdata_types"]
        #rsps = self.data["rsps"]
        byte_idx = 0
        for byte in raw_rsp:
            if byte == protocol["body_start"]:
                #obtain response id
                start_idx = byte_idx + protocol["id_idx"]
                end_idx = start_idx + ctypes["uint16_t"]
                rsp_id = int.from_bytes(raw_rsp[start_idx:end_idx],"little")
                processed_rsp["id"] = rsp_id
                #obtain response name
                rsp_name = self.tf_rsp_id_list[rsp_id]
                #obtain response spec
                rsp_hdr = self.data["rsp_hdr"] 
                rsp_spec = self.data["rsps"][rsp_name] 
                #obtain flags
                start_idx = byte_idx + protocol["flags_idx"]
                end_idx = start_idx + ctypes[rsp_hdr["flags"]]
                processed_rsp["flags"] = int.from_bytes(raw_rsp[start_idx:end_idx],"little")
                #obtain payload size
                start_idx = byte_idx + protocol["payload_size_idx"]
                end_idx = start_idx + ctypes[rsp_hdr["payload_size"]]
                processed_rsp["payload_size"] = int.from_bytes(raw_rsp[start_idx:end_idx],"little")
                #obtain payload args
                if processed_rsp["payload_size"] != 0:
                    payload_args = rsp_spec["payload_args"] 
                    #print(payload_args)
                    #set idx to start of payload
                    idx = end_idx
                    for arg in payload_args:
                        start_idx = idx
                        end_idx = start_idx + ctypes[payload_args[arg]]
                        processed_rsp[arg] = int.from_bytes(raw_rsp[start_idx:end_idx],"little")
                        idx = idx + (end_idx-start_idx)
                break
            byte_idx += 1
        return processed_rsp