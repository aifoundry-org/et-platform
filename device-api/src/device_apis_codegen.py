#!/usr/bin/env python3

#------------------------------------------------------------------------------
# Copyright (C) 2019, Esperanto Technologies Inc.
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
import logging
import os
import sys
import yaml

sys.dont_write_bytecode = True

# Minimum alignment requirement for all messages
MSG_BYTE_ALIGN_REQ = 8

_LOG_LEVEL_STRINGS = ['CRITICAL', 'ERROR', 'WARNING', 'INFO', 'DEBUG']

# Inspired by https://stackoverflow.com/questions/528281/how-can-i-include-a-yaml-file-inside-another
# will be used to support the !include statement in YAML files
class Loader(yaml.SafeLoader):
    """Inherit from the SafeLoaded and extend it to handle the "include" statement"""

    def __init__(self, stream):
        self._root = os.path.split(stream.name)[0]
        super(Loader, self).__init__(stream)

    def include(self, node):
        filename = os.path.join(self._root, self.construct_scalar(node))
        with open(filename, 'r') as f:
            return yaml.load(f, Loader)

Loader.add_constructor('!include', Loader.include)

class Enum(object):
    """Class holding the information of a enum"""
    def __init__(self, schema):
        for k, v in schema.items():
            setattr(self, k ,v)

    @property
    def c_type_name(self):
        """Return string with the C type name"""
        return f"{self.Name}_e"


class DevAPICodeGeneratorHelper(object):
    """Helper class that provides a set of static methods to help us with code-generation

    The purpose of this class is to move here any code that requires additonal logic
    that we do not want to have in the tempalte in an effort to keep the Jinja template code
    simple

    Attributes:
       spec_data (dict) : Reference to the parsed and validated YAML trace specification data
    """
    def __init__(self, spec_data):
        self.spec_data = spec_data

    @property
    def api_name(self):
        return self.spec_data['Name']

    @property
    def c_api_name(self):
        return self.api_name.replace("-", "_").lower()

    def spec_hash(self):
        """Return string that corresponds to a 64bit number of the LSB bigs of the md5 sub of the schema"""
        m = hashlib.md5()
        m.update(f"{str(self.spec_data)}".encode())
        res = m.hexdigest()
        return res[-17:-1]

    def rpc_calls(self):
        """Return the schema of the RPC_calls module

        Returns:
           dict: Schema of the RPC_Calls module
        """
        modules = self.spec_data.get("Modules")
        for mod in modules:
            if mod['Name'] == "Ops_API_Calls":
                return mod
            if mod['Name'] == "Mgmt_API_Calls":
                return mod
        return None

    def rpc_commands(self):
        """Return list of all the rpc Commands"""
        return [ i for i in self.rpc_calls()['Messages'] if i['Type'] == 'Command']

    def rpc_responses(self):
        """Return list of all the rpc Responses"""
        return [ i for i in self.rpc_calls()['Messages'] if i['Type'] == 'Response']

    def rpc_events(self):
        """Return list of all the rpc Events"""
        return [ i for i in self.rpc_calls()['Messages'] if i['Type'] == 'Event']

    def trace_module(self):
        """Return the schema of the DeviceFwTracing module

        Returns:
           dict: Schema of the DeviceFwTracing module
        """
        modules = self.spec_data.get("Modules")
        for mod in modules:
            if mod['Name'] == "DeviceFWTracing":
                return mod
        return None


    def enums(self):
        """ Return the list of defined Enums

        Returns:
          list[dict]: Return the list of defined enums
        """
        rpc_calls = self.rpc_calls()
        if not rpc_calls:
            return []
        return [Enum(i) for i in rpc_calls.get("Enums", [])]

    def structs(self):
        """Return the list of defined structs

        Returns:
          list[dict]: List of defined structs to be used in our messages
        """
        rpc_calls = self.rpc_calls()
        if not rpc_calls:
            return []
        return rpc_calls.get("Structs", [])

    @property
    def message_start_id(self):
        """Return the stating ID of the messages, used when building C/C++ enumerations"""
        id = self.spec_data.get("MessageStartID", 0)
        if id < 0:
            raise RuntimeError("Expecting a positive MessageStartID value")
        return id

    @property
    def message_max_id(self):
        """Return the last ID that the messages can have or the length of messages we have """
        res = self.spec_data.get("MessageMaxID", 0)
        if res < 0:
            raise RuntimeError("Expecting positive value for MessageMaxID")
        if res == 0:
            return self.message_start_id + len(self.messages()) + 1
        last_message_id = self.message_start_id + len(self.messages())
        if res < last_message_id:
            raise RuntimeError(f"MessageMaxID: {res} should be larger than the last "
                               "message ID: {last_message_id}")
        return res

    def messages(self):
        """Return the list of defined message types

        Returns:
           list[dict]: List of defined Messages to be exchanged between host and device-fw
        """
        rpc_calls = self.rpc_calls()
        if not rpc_calls:
            return []
        return rpc_calls.get("Messages", [])

    def trace_groups(self):
        """Return the list of defined trace events

        Returns:
          list[dict] " List of defined TraceEvents
        """
        trace_mod = self.trace_module()
        if not trace_mod:
            return []
        return trace_mod.get("TraceGroups", [])

    def custom_type(self, field):
        """Return true if this is a custom device-api type
        """
        return field['Type'] in ["enum", "struct"]


    def message_field_type(self, field):
        """Return the correct type for a given message field

        Args:
          field (dict) : Dictionary with the field related information

        Returns
          str: String with the type name
        """
        type = field["Type"]
        if type  == "bool":
            return "uint8_t"
        elif type == "enum":
            enum_name = field["Enum"]
            enums = list(filter(lambda x : x.Name == enum_name, self.enums()))
            if not len(enums) == 1:
                raise RuntimeError(f"None or multiple enums found: {enums}, {enum_name}")
            return enums[0].c_type_name
        elif type == "struct":
            struct_name = field["Struct"]
            structs = list(filter(lambda x : x["Name"] == struct_name, self.structs()))
            if not len(structs) == 1:
                raise RuntimeError(f"None or multiple structs found: {structs}, {struct_name}")
            return f'{struct_name}'
        return type

    def message_field_storage_type(self, field):
        """Return the underlying storage type for a given message field

        Args:
          field (dict) : Dictionary with the field related information

        Returns
          str: String with the type name
        """
        type = field["Type"]
        if type == "enum":
            enum_name = field["Enum"]
            enums = list(filter(lambda x : x.Name == enum_name, self.enums()))
            if not len(enums) == 1:
                raise RuntimeError(f"None or multiple enums found: {enums}, {enum_name}")
            return enums[0].Type
        else:
            return self.message_field_type(field)

    @staticmethod
    def name_to_camelcase(name):
        """Convert a underscore separate name to camel case"""
        words = name.split("_")
        name = ""
        for w in words:
            name += f'{w[0].upper()}{w[1:]}'
        return name

    @staticmethod
    def validate_trace(trace_event):
        """Validate that in trace event a bytes type appears only once and at the end"""
        fields = trace_event.get("Fields", [])
        for i in range(len(fields)):
            if fields[i]["Type"] ==  "bytes" and i != (len(fields) - 1):
                raise RuntimeError(f"Found bytes field not at the end of record, {trace_event}")

    @staticmethod
    def get_type_size(type_name):
        """Return the size in bytes of a C/C++ type, input string with the size name"""
        types = {
            "double": 8,
            "float": 4,
            "bool": 1,
            "int8_t": 1,
            "uint8_t": 1,
            "int16_t": 2,
            "uint16_t": 2,
            "int32_t": 4,
            "uint32_t": 4,
            "int64_t": 8,
            "uint64_t": 8,
            "char": 1,
            }
        return types[type_name]

    @staticmethod
    def get_type_format_specifier(type_name):
        """Return the format specifier for of a C type"""
        types = {
            "double":   "\"%lf\"",
            "float":    "\"%f\"",
            "int8_t":   "\"%\" PRId8 ",
            "uint8_t":  "\"%\" PRIu8 ",
            "int16_t":  "\"%\" PRId16 ",
            "uint16_t": "\"%\" PRIu16 ",
            "int32_t":  "\"%\" PRId32 ",
            "uint32_t": "\"%\" PRIu32 ",
            "int64_t":  "\"%\" PRId64 ",
            "uint64_t": "\"%\" PRIu64 ",
            "bytes":    "\"%s\"",
            }
        return types[type_name]

    @staticmethod
    def fields_to_arg_list(obj):
        """Return string with all the arguments present in Fields list

        Args:
          obj (dict): obj specification
        """
        fields = obj.get("Fields", [])
        params = []
        for field in fields:
            if field['Type'] == "bytes":
                params += [f"char* const {field['Name']}"]
            else:
                params += [f"const {field['Type']} {field['Name']}"]
        return ", ".join(params)

    @staticmethod
    def fields_to_printf_format(obj):
        """Return printf stype string with all the arguments present in the Fields list

        Args:
          obj (dict): obj specification
        """
        fields = obj.get("Fields", [])
        params = []
        for field in fields:
            specifier = DevAPICodeGeneratorHelper.get_type_format_specifier(field['Type'])
            params += [f"\"{field['Name']}: \"" + specifier]
        return "\", \"".join(params)

    @staticmethod
    def get_field_from_type(obj, typ):
        """Return the very first field Name of type typ if present in Fields list

        Args:
          obj (dict): obj specification
          typ (string): type to look for
        """

        fields = obj.get("Fields", [])
        for field in fields:
            if field["Type"] == typ:
                return field["Name"]
        return []

    @staticmethod
    def get_array_size(string_name):
        """Return the size specified array field; if the string is not array, returns -1

        Args:
          string_name (string): string to search for array size
        """

        str_len = len(string_name)
        index = string_name.find('[')
        if index != -1:
            val = ''
            j = index + 1
            # Dynamic/flexible array, hence the size value won't be available
            if (string_name[j] == ']'):
                return 0
            while (j < str_len) and (string_name[j] != ']'):
                val += string_name[j]
                j += 1
            return int(val)
        else:
            return -1

    def validate_api(self):
        """Run a couple of checks on the schema to avoid common mistakes"""
        # Check that unsigned enums hold positive values
        for module in self.spec_data.get("Modules", []):
            for enum in module.get("Enums", []):
                etype = enum['Type']
                unsigned = "uint" in etype
                for val in enum["Values"]:
                    if unsigned and val["Value"] < 0:
                        raise RuntimeError(f"Unsigned enum cannot hold negative value: {val}")
        # Check that member fields of structs or messages are ordered in decreasing size
        # Also check that member fields of structs or messages are meeting minimum allignment requirement
        for module in self.spec_data.get("Modules", []):
            for struct in module.get("Structs", []):
                last_size = 32 # some big value
                struct_size = 0
                for field in struct["Fields"]:
                    ftype = field["Type"]
                    ftype_size = DevAPICodeGeneratorHelper.get_type_size(ftype)
                    if ftype_size > last_size:
                        raise RuntimeError(f"{struct['Name']}: Member types are "
                                           f"expected to be placed in creasing size, {field} is not")
                    else:
                        last_size = ftype_size
                    # Check if array and calculate the size
                    array_size = DevAPICodeGeneratorHelper.get_array_size(field["Name"])
                    if array_size != -1:
                        ftype_size *= array_size
                    # Increment size of structure
                    struct_size += ftype_size
                if not (struct_size % MSG_BYTE_ALIGN_REQ == 0):
                    raise RuntimeError(f"{struct['Name']}: Must be {MSG_BYTE_ALIGN_REQ}-byte aligned")

            # Do the same for messages
            for message in module.get("Messages", []):
                last_size = 1000 # some big value
                for field in message["Fields"]:
                    ftype = self.message_field_storage_type(field)
                    # Require that structs are declared first before any other member fields
                    # fake this by assigning structs a "big" size
                    if field["Type"] == "struct":
                        ftype_size = 1000
                    else:
                        ftype_size = DevAPICodeGeneratorHelper.get_type_size(ftype)
                    if ftype_size > last_size:
                        raise RuntimeError(f"{message['Name']}: Field types are expected "
                                           f"to be placed in increasing size, {field} is not")
                    else:
                        last_size = ftype_size

            # Verify that the structs of messages meet minimum alignment requirement
            for message in module.get("Messages", []):
                msg_size = 0
                for field in message["Fields"]:
                    # If the type is struct, fake the field size.
                    # Struct sizes are verified previously.
                    if field["Type"] == "struct":
                        msg_size += MSG_BYTE_ALIGN_REQ
                    else:
                        ftype = DevAPICodeGeneratorHelper.message_field_storage_type(self, field)
                        ftype_size = DevAPICodeGeneratorHelper.get_type_size(ftype)
                        # Check if array and calculate the size
                        array_size = DevAPICodeGeneratorHelper.get_array_size(field["Name"])
                        if array_size != -1:
                            ftype_size *= array_size
                        msg_size += ftype_size
                if not (msg_size % MSG_BYTE_ALIGN_REQ == 0):
                    raise RuntimeError(f"{message['Name']}: Must be {MSG_BYTE_ALIGN_REQ}-byte aligned")
        # Check that the values of PairedMessage are valid message names
        for module in self.spec_data.get("Modules", []):
            messages = module.get("Messages", [])
            for msg in messages:
                paired_message = msg['PairedMessage']
                if not paired_message == "None":
                    pmsg_exists = [i for i in messages if i['Name'] == paired_message]
                    if not pmsg_exists:
                        raise RuntimeError(f"In message: {msg['Name']}, paired message:"
                                           f" {paired_message} does not exist")



def gen_code(args):
    """Generate code form the input Jinja template

    Args:
      Args (Namespace): Namespace with the values of the parsed command line arguments
    """
    # Open the YAML tracing specification
    with open(args.spec, 'r') as ifile:
        spec_data = yaml.load(ifile, Loader)
    #print(spec_data)
    # Read the json schema that describes the expected structure, and types/values of the
    # tracing specification
    with open(args.schema, 'r') as ischema:
        schema = json.load(ischema)
    # Check if the specification is valid
    v = jsonschema.Draft7Validator(schema)
    errors = sorted(v.iter_errors(spec_data), key=lambda e: e.path)
    for error in errors:
        print(error.message, "  ", error.path, " ", error.instance)
        for suberror in sorted(error.context, key=lambda e: e.schema_path):
            print(list(suberror.schema_path), suberror.message, sep=", ")
    if not len(errors) == 0:
        raise RuntimeError(f"Failed to parse Esperanto Trace Specification: {args.spec}")
    # Now open the Jinja template and generate code
    dir_name, tmpl_name = os.path.split(os.path.abspath(args.template))
    file_loader = jinja2.FileSystemLoader(dir_name)
    ### Warning
    # Change the jinja environment delimiters to start with "<", ">" instead of "{", "}"
    # This is in an effort to reduce confusion with the curly braches used in C++ code
    env = jinja2.Environment('<%', '%>', '<<', '>>', '<%doc>', '</%doc>', '%', '##',
                             loader=file_loader)
    tmpl = env.get_template(tmpl_name)
    cgh = DevAPICodeGeneratorHelper(spec_data)
    cgh.validate_api()
    output = tmpl.render(spec=spec_data,
                         cgh=cgh)
    with open(args.output, "w") as ofile:
        ofile.write(output)


def _log_level_string_to_int(log_level_string):
    if not log_level_string in _LOG_LEVEL_STRINGS:
        message = 'invalid choice: {0} (choose from {1})'.format(log_level_string, _LOG_LEVEL_STRINGS)
        raise argparse.ArgumentTypeError(message)
    log_level_int = getattr(logging, log_level_string, logging.INFO)
    # check the logging log_level_choices have not changed from our expected values
    assert isinstance(log_level_int, int)
    return log_level_int

if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument('--log-level',
                        default='INFO',
                        dest='log_level',
                        type=_log_level_string_to_int,
                        nargs='?',
                        help='Set the logging output level. {0}'.format(_LOG_LEVEL_STRINGS))
    parser.add_argument("--spec",
                        required=True,
                        help="Path to the Tracing specification YAML file")
    parser.add_argument("--schema",
                        required=True,
                        help="Path to the jsonschema file")
    parser.add_argument("--template",
                        required=True,
                        help="Path to the Jinja template")
    parser.add_argument("--output",
                        required=True,
                        help="Path to the output file")
    args = parser.parse_args()
    gen_code(args)
