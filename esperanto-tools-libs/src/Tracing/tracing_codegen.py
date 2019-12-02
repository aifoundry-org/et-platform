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
import copy
import jinja2
import json
import jsonschema
import logging
import os
import sys
import yaml

sys.dont_write_bytecode = True

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

class CodeGeneratorHelper(object):
    """Helper class that provides a set of static methods to help us with code-generation

    The purpose of this class is to move here any code that requires additonal logic
    that we do not want to have in the tempalte in an effort to keep the Jinja template code
    simple

    Attributes:
       spec_data (dict) : Reference to the parsed and validated YAML trace specification data
    """
    def __init__(self, spec_data):
        self.spec_data = spec_data
        self.convert_device_api_to_traceinfo()


    def convert_device_api_to_traceinfo(self):
        """Convert the schema described in the DeviceAPI into a module to be recorded in the trace"""
        DevAPIMod = {
            'Name': 'DeviceAPI',
            'Enums': [],
            'Structs': [],
            'EnableTextLogging': True,
            'EnablePBLogging': True,
            'Functions': []
        }
        # Copy any enum definition
        for mod in self.spec_data['DeviceAPI']['Modules']:
            for enum in mod.get('Enums', []):
                menum = copy.deepcopy(enum)
                menum['Name'] = f"{mod['Name']}_{menum['Name']}"
                DevAPIMod['Enums'].append(menum)
        # Copy any struct definition
        for mod in self.spec_data['DeviceAPI']['Modules']:
            for struct in mod.get('Structs', []):
                DevAPIMod['Structs'].append(
                    {
                        'Name': f'{mod["Name"]}_{struct["Name"]}',
                        'Fields': [
                            {
                                'Name': f['Name'],
                                'Type' : 'struct' if f['Type'] == 'struct' else 'enum' if f['Type'] == 'enum' else self.cxx_to_proto_type(mod, f),
                                'Struct': self.cxx_to_proto_type(mod, f) if f['Type'] == 'struct' else "",
                                'Enum': self.cxx_to_proto_type(mod, f) if f['Type'] == 'enum' else "",
                            }
                            for f in struct['Fields']
                        ]
                    })
        # Convert the messages to tracing function calls
        for mod in self.spec_data['DeviceAPI']['Modules']:
            entries = []
            if 'Messages' in mod:
                entries = mod.get('Messages', [])
            elif 'TraceEvents' in mod:
                entries = mod.get('TraceEvents', [])
            for msg in entries:
                DevAPIMod['Functions'].append(
                    {
                        'Name': f'{mod["Name"]}_{msg["Name"]}',
                        'EnableTextLogging': True,
                        'EnablePBLogging': True,
                        'Arguments': [
                            {
                                'Name': f['Name'],
                                'Type' : 'struct' if f['Type'] == 'struct' else 'enum' if f['Type'] == 'enum' else self.cxx_to_proto_type(mod, f),
                                'Struct': self.cxx_to_proto_type(mod, f) if f['Type'] == 'struct' else "",
                                'Enum': self.cxx_to_proto_type(mod, f) if f['Type'] == 'enum' else "",
                            }
                            for f in msg['Fields']
                        ]
                    })
        self.spec_data['Modules'].append(DevAPIMod)

    def get_struct(self, struct_name):
        """Return dict with the struct definition if it exists

        Args:
          struct_name (str): Name of the structure

        Returns:
          dict : Struct definition
        """
        slist = [ ]
        for mod in self.spec_data['Modules']:
            for i in mod.get('Structs', []):
                if f"{mod['Name']}_{i['Name']}" == struct_name:
                    slist.append(i)
        if not slist:
            return {}
        assert(len(slist) == 1)
        return slist[0]

    @staticmethod
    def module_enable_pb_logging(module):
        """Return true if we should enable protobuf logging for the module by default

        Args:
           module (dict) : Specification for a module
        """
        return "true" if module.get("EnablePBLogging", False) else "false"

    @staticmethod
    def module_enable_text_logging(module):
        """Return true if we should enable text logging for the module

        Args:
           module (dict) : Specification for a module
        """
        return "true" if module.get("EnableTextLogging", False) else "false"

    @staticmethod
    def function_enable_pb_logging(function):
        """Return true if we should enable protobuf logging for the function by default

        Args:
           function (dict) : Specification for a function
        """
        return "true" if function.get("EnablePBLogging", False) else "false"

    @staticmethod
    def function_enable_text_logging(function):
        """Return true if we should enable text logging for the function

        Args:
           function (dict) : Specification for a function
        """
        return "true" if function.get("EnableTextLogging", False) else "false"

    @staticmethod
    def function_get_argname_list(function):
        """Return string with all function argument names comma separated

        Args:
          function (dict): Function specification
        """
        args = function.get("Arguments", [])
        return ", ".join([arg['Name'] for arg in args])

    @staticmethod
    def proto_to_cxx_type(module, arg):
        """Convert the protobuf type to a C++ type

        Args:
          type_name (str): Name of type
        """
        type_name = None
        if CodeGeneratorHelper.is_proto_type(arg['Type']) or arg['Type'][-2:] == "_e":
            type_name = arg['Type']
        elif arg['Type'] == 'enum':
            type_name = arg['Enum']
        elif arg['Type'][-2:] == "_t":
            type_name = arg['Type']
        elif arg['Type'] == 'struct':
            type_name = arg['Struct']
        elif arg['Type'] == 'union':
            type_name = arg['Union']
        else:
            raise RuntimeError(f"Unhandled argument type: '{arg['Type']}")
        types = {
            "double" : "double",
            "float": "float",
            "int32": "int32_t",
            "int64": "int64_t",
            "uint32": "uint32_t",
            "uint64": "uint64_t",
            "sint32": "int32_t",
            "sint64": "int64_t",
            "fixed32": "int32_t",
            "fixed64": "int64_t",
            "sfixed32": "int32_t",
            "sfixed64": "int64_t",
            "bool": "bool",
            "string": "std::string",
            "bytes": "std::vector<uint8_t>",
        }

        if type_name in types:
            return types[type_name]
        return f"{module['Name']}_{type_name}"

    @staticmethod
    def is_proto_type(type_name):
        """Return true of it is a protobuf primitive type
        """
        types = [
            "double", "float", "int32", "int64", "uint32",
            "uint64", "sint32", "sint64", "fixed32",
            "fixed64", "sfixed32", "sfixed64",
            "bool", "string",  "bytes",
        ]
        return type_name in types


    @staticmethod
    def proto_type(module, field):
        """Return the protobuf name of a type"""
        if CodeGeneratorHelper.is_proto_type(field['Type']):
            return field['Type']
        elif field['Type'] == 'struct':
            return f"{module['Name']}_{field['Struct']}"
        elif field['Type'] == 'enum':
            return f"{module['Name']}_{field['Enum']}"
        elif field['Type'] == 'union':
            return f"{module['Name']}_{field['Union']}"
        raise RuntimeError(f"Unknown field type {field['Type']}")

    @staticmethod
    def cxx_to_proto_type(module, field):
        """Convert the C/C++ type to proto

        Args:
          type_name (str): Name of type
        """
        if 'struct' == field['Type']:
            return f'{module["Name"]}_{field["Struct"]}'
        elif 'enum' == field['Type']:
            return f'{module["Name"]}_{field["Enum"]}'
        types = {
            "double" : "double",
            "float": "float",
            # Upcast small C/C++ types
            'int8_t' : 'int32',
            'uint8_t': 'uint32',
            'int16_t': 'int32',
            'uint16_t': 'uint32',
            "int32_t": "int32",
            "uint32_t": "uint32",
            "int64_t": "int64",
            "uint64_t": "uint64",
            "bool": "bool",
            "bytes": "bytes",
        }
        return types[field['Type']]


    @staticmethod
    def get_cxx_function_arg_list(module, function):
        """Return string with all the arguments of the C++ function

        Args:
          function (dict): Function specification
        """
        args = function.get("Arguments", [])
        params = []
        for arg in args:
            pass_by_ref = True
            type = CodeGeneratorHelper.proto_to_cxx_type(module, arg)
            if arg['Type'][-2:] == "_t" or arg['Type'] == 'struct':
                pass_by_ref = False
            if arg.get('Repeated', False):
                params += [f"const std::vector<{type}>& {arg['Name']}"]
            elif pass_by_ref:
                params += [f"const {type}& {arg['Name']}"]
            else:
                params += [f"{type}* {arg['Name']}"]
        return ", ".join(params)


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
    cgh = CodeGeneratorHelper(spec_data)
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
