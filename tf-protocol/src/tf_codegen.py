#!/usr/bin/env python3

#------------------------------------------------------------------------------
# Copyright (C) 2021, Esperanto Technologies Inc.
# The copyright to the computer program(s) herein is the
# property of Esperanto Technologies, Inc. All Rights Reserved.
# The program(s) may be used and/or copied only with
# the written permission of Esperanto Technologies and
# in accordance with the terms and conditions stipulated in the
# agreement/contract under which the program(s) have been supplied.
#------------------------------------------------------------------------------

import argparse
import hashlib
import jinja2
import json
import jsonschema
import os
import sys

sys.dont_write_bytecode = True

class TFProtocolCodeGeneratorHelper(object):
    """Helper class that provides a set of static methods to help us with code-generation

    The purpose of this class is to move here any code that requires additonal logic
    that we do not want to have in the template in an effort to keep the Jinja template code
    simple

    Attributes:
       spec_data (dict) : Reference to the parsed and validated JSON specification data
    """
    def __init__(self, spec_data):
        self.spec_data = spec_data

    @property
    def api_name(self):
        return self.spec_data['name']

    @property
    def c_api_name(self):
        return self.api_name.replace("-", "_").lower()

    def spec_hash(self):
        """Return string that corresponds to a 64bit number of the LSB bigs of the md5 sub of the schema"""
        m = hashlib.md5()
        m.update(f"{str(self.spec_data)}".encode())
        res = m.hexdigest()
        return res[-17:-1]

    def tf_protocol_parameters(self):
        """Return the list of defined TF protocol parameters

        Returns:
           list[dict]: List of defined TF protocol parameters
        """
        return self.spec_data['tf_protocol']

    def tf_protocol_parameters_id(self, parameter):
        """Return the ID of the TF protocol parameters

        Returns:
           id: ID of the TF protocol parameters
        """
        return self.tf_protocol_parameters()[parameter]

    def interception_points(self):
        """Return the list of defined interception points for TF

        Returns:
           list[dict]: List of defined inteception points
        """
        return self.spec_data['sp_tf_interception_points']

    def interception_points_id(self, point):
        """Return the ID of the inteception point

        Returns:
           id: ID of the inteception point
        """
        return self.interception_points()[point]

    def cmd_rsp_header(self):
        """Return the list of cmd/rsp header fields

        Returns:
           list[dict]: List of arguments
        """
        return self.spec_data['tf_cmd_rsp_hdr']

    def cmd_rsp_header_arg_type(self, arg):
        """Return the type of the required argument

        Returns:
           string: Type
        """
        return self.cmd_rsp_header()[arg]

    def cmd_structs(self):
        """Return the list of defined command types

        Returns:
           list[dict]: List of command structs
        """
        return self.spec_data['tf_sp_cmd_spec']

    def cmd_struct_args(self, cmd):
        """Return the list of arguments in the given cmd

        Returns:
           list[dict]: List of arguments
        """
        return self.cmd_structs()[cmd]["payload_args"]

    def cmd_struct_args_type(self, cmd, arg):
        """Return the type of the required argument in the command

        Returns:
           string: Type
        """
        return self.cmd_struct_args(cmd)[arg]

    def rsp_structs(self):
        """Return the list of defined response types

        Returns:
           list[dict]: List of response structs
        """
        return self.spec_data['tf_sp_rsp_spec']

    def rsp_struct_args(self, rsp):
        """Return the list of arguments in the given rsp

        Returns:
           list[dict]: List of argumetns
        """
        return self.rsp_structs()[rsp]["payload_args"]

    def rsp_struct_args_type(self, rsp, arg):
        """Return the type of the required argument in the response

        Returns:
           string: Type
        """
        return self.rsp_struct_args(rsp)[arg]

    def commands_description(self):
        """Return the description of the command IDs

        Returns:
           string: description of TF commands
        """
        return self.spec_data['tf_sp_command_ids.description']

    def commands(self, get_first = False, get_last = False):
        """Return the list of defined command types

        Returns:
           list[dict]: List of defined commands to be exchanged between host and device-fw
        """
        if get_first == True:
            return list(self.spec_data['tf_sp_command_ids'])[0]
        elif get_last == True:
            return list(self.spec_data['tf_sp_command_ids'])[-1]
        else:
            return self.spec_data['tf_sp_command_ids']

    def command_id(self, command):
        """Return the ID of the command

        Returns:
           id: ID of the command
        """
        return self.commands()[command]

    def responses_description(self):
        """Return the description of the response IDs

        Returns:
           string: description of TF responses
        """
        return self.spec_data['tf_sp_response_ids.description']

    def responses(self, get_first = False, get_last = False):
        """Return the list of defined response types

        Returns:
           list[dict]: List of defined responses to be exchanged between host and device-fw
        """
        if get_first == True:
            return list(self.spec_data['tf_sp_response_ids'])[0]
        elif get_last == True:
            return list(self.spec_data['tf_sp_response_ids'])[-1]
        else:
            return self.spec_data['tf_sp_response_ids']

    def response_id(self, response):
        """Return the ID of the response

        Returns:
           id: ID of the response
        """
        return self.responses()[response]

    def mm_dev_api_commands(self):
        """Return the list of defined device api command types

        Returns:
           list[dict]: List of defined commands to be exchanged between SP and MM
        """
        return self.spec_data['tf_mm_device_api_cmd_ids']

    def mm_dev_api_command_id(self, command):
        """Return the ID of the device api command

        Returns:
           id: ID of the command
        """
        return self.mm_dev_api_commands()[command]

    def mm_dev_api_responses(self):
        """Return the list of defined device api response types

        Returns:
           list[dict]: List of defined responses to be exchanged between SP and MM
        """
        return self.spec_data['tf_mm_device_api_rsp_ids']

    def mm_dev_api_response_id(self, response):
        """Return the ID of the device api response

        Returns:
           id: ID of the response
        """
        return self.mm_dev_api_responses()[response]

def gen_code(args):
    """Generate code form the input Jinja template

    Args:
      Args (Namespace): Namespace with the values of the parsed command line arguments
    """

    with open(args.spec, 'r') as isspec:
        spec_data = json.load(isspec)
        isspec.close()
    # Check if the specification is valid
    v = jsonschema.Draft7Validator(spec_data)

    # Now open the Jinja template and generate code
    dir_name, tmpl_name = os.path.split(os.path.abspath(args.template))
    file_loader = jinja2.FileSystemLoader(dir_name)
    ### Warning
    # Change the jinja environment delimiters to start with "<", ">" instead of "{", "}"
    # This is in an effort to reduce confusion with the curly braches used in C++ code
    env = jinja2.Environment('<%', '%>', '<<', '>>', '<%doc>', '</%doc>', '%', '##',
                             loader=file_loader)
    tmpl = env.get_template(tmpl_name)
    cgh = TFProtocolCodeGeneratorHelper(spec_data)
    output = tmpl.render(spec=spec_data,
                         cgh=cgh)
    with open(args.output, "w") as ofile:
        ofile.write(output)

if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("--spec",
                        required=True,
                        help="Path to the TF specification JSON file")
    parser.add_argument("--template",
                        required=True,
                        help="Path to the Jinja template")
    parser.add_argument("--output",
                        required=True,
                        help="Path to the output file")
    args = parser.parse_args()
    gen_code(args)
