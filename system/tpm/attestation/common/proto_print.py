#!/usr/bin/python2

#
# Copyright (C) 2015 The Android Open Source Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#

"""A C++ code generator for printing protobufs which use the LITE_RUNTIME.

Normally printing a protobuf would be done with Message::DebugString(). However,
this is not available when using only MessageLite. This script generates code to
emulate Message::DebugString() without using reflection. The input must be a
valid .proto file.

Usage: proto_print.py [--subdir=foo] <bar.proto>

Files named print_bar_proto.h and print_bar_proto.cc will be created in the
current working directory.
"""

from __future__ import print_function

import argparse
import collections
from datetime import date
import os
import re
import subprocess


# Holds information about a protobuf message field.
#
# Attributes:
#   repeated: Whether the field is a repeated field.
#   type_: The type of the field. E.g. int32.
#   name: The name of the field.
Field = collections.namedtuple('Field', 'repeated type_ name')


class Message(object):
  """Holds information about a protobuf message.

  Attributes:
    name: The name of the message.
    fields: A list of Field tuples.
  """

  def __init__(self, name):
    """Initializes a Message instance.

    Args:
      name: The protobuf message name.
    """
    self.name = name
    self.fields = []

  def AddField(self, attribute, field_type, field_name):
    """Adds a new field to the message.

    Args:
      attribute: This should be 'optional', 'required', or 'repeated'.
      field_type: The type of the field. E.g. int32.
      field_name: The name of the field.
    """
    self.fields.append(Field(repeated=attribute == 'repeated',
                             type_=field_type, name=field_name))


class Enum(object):
  """Holds information about a protobuf enum.

  Attributes:
    name: The name of the enum.
    values: A list of enum value names.
  """

  def __init__(self, name):
    """Initializes a Enum instance.

    Args:
      name: The protobuf enum name.
    """
    self.name = name
    self.values = []

  def AddValue(self, value_name):
    """Adds a new value to the enum.

    Args:
      value_name: The name of the value.
    """
    self.values.append(value_name)


def ParseProto(input_file):
  """Parses a proto file and returns a tuple of parsed information.

  Args:
    input_file: The proto file to parse.

  Returns:
    A tuple in the form (package, imports, messages, enums) where
      package: A string holding the proto package.
      imports: A list of strings holding proto imports.
      messages: A list of Message objects; one for each message in the proto.
      enums: A list of Enum objects; one for each enum in the proto.
  """
  package = ''
  imports = []
  messages = []
  enums = []
  current_message_stack = []
  current_enum = None
  package_re = re.compile(r'package\s+(\w+);')
  import_re = re.compile(r'import\s+"(\w+).proto";')
  message_re = re.compile(r'message\s+(\w+)\s*{')
  field_re = re.compile(r'(optional|required|repeated)\s+(\w+)\s+(\w+)\s*=')
  enum_re = re.compile(r'enum\s+(\w+)\s*{')
  enum_value_re = re.compile(r'(\w+)\s*=')
  for line in input_file:
    line = line.strip()
    if not line or line.startswith('//'):
      continue
    # Close off the current scope. Enums first because they can't be nested.
    if line == '}':
      if current_enum:
        enums.append(current_enum)
        current_enum = None
      if current_message_stack:
        messages.append(current_message_stack.pop())
      continue
    # Look for a message definition.
    match = message_re.search(line)
    if match:
      prefix = ''
      if current_message_stack:
        prefix = '::'.join([m.name for m in current_message_stack]) + '::'
      current_message_stack.append(Message(prefix + match.group(1)))
      continue
    # Look for a message field definition.
    if current_message_stack:
      match = field_re.search(line)
      if match:
        current_message_stack[-1].AddField(match.group(1),
                                           match.group(2),
                                           match.group(3))
        continue
    # Look for an enum definition.
    match = enum_re.search(line)
    if match:
      current_enum = Enum(match.group(1))
      continue
    # Look for an enum value.
    if current_enum:
      match = enum_value_re.search(line)
      if match:
        current_enum.AddValue(match.group(1))
        continue
    # Look for a package statement.
    match = package_re.search(line)
    if match:
      package = match.group(1)
    # Look for an import statement.
    match = import_re.search(line)
    if match:
      imports.append(match.group(1))
  return package, imports, messages, enums


def GenerateFileHeaders(proto_name, package, imports, subdir, header_file_name,
                        header_file, impl_file):
  """Generates and prints file headers.

  Args:
    proto_name: The name of the proto file.
    package: The protobuf package.
    imports: A list of imported protos.
    subdir: The --subdir arg.
    header_file_name: The header file name.
    header_file: The header file handle, open for writing.
    impl_file: The implementation file handle, open for writing.
  """
  if subdir:
    guard_name = '%s_%s_PRINT_%s_PROTO_H_' % (package.upper(),
                                              subdir.upper(),
                                              proto_name.upper())
    package_with_subdir = '%s/%s' % (package, subdir)
  else:
    guard_name = '%s_PRINT_%s_PROTO_H_' % (package.upper(), proto_name.upper())
    package_with_subdir = package
  includes = '\n'.join(
      ['#include "%(package_with_subdir)s/print_%(import)s_proto.h"' % {
          'package_with_subdir': package_with_subdir,
          'import': current_import} for current_import in imports])
  header = """\
// Copyright %(year)s The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// THIS CODE IS GENERATED.

#ifndef %(guard_name)s
#define %(guard_name)s

#include <string>

#include "%(package_with_subdir)s/%(proto)s.pb.h"

namespace %(package)s {
""" % {'year': date.today().year,
       'guard_name': guard_name,
       'package': package,
       'proto': proto_name,
       'package_with_subdir': package_with_subdir}
  impl = """\
// Copyright %(year)s The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// THIS CODE IS GENERATED.

#include "%(package_with_subdir)s/%(header_file_name)s"

#include <string>

#include <base/strings/string_number_conversions.h>
#include <base/strings/stringprintf.h>

%(includes)s

namespace %(package)s {
""" % {'year': date.today().year,
       'package': package,
       'package_with_subdir': package_with_subdir,
       'header_file_name': header_file_name,
       'includes': includes}

  header_file.write(header)
  impl_file.write(impl)


def GenerateFileFooters(proto_name, package, subdir, header_file, impl_file):
  """Generates and prints file footers.

  Args:
    proto_name: The name of the proto file.
    package: The protobuf package.
    subdir: The --subdir arg.
    header_file: The header file handle, open for writing.
    impl_file: The implementation file handle, open for writing.
  """
  if subdir:
    guard_name = '%s_%s_PRINT_%s_PROTO_H_' % (package.upper(),
                                              subdir.upper(),
                                              proto_name.upper())
  else:
    guard_name = '%s_PRINT_%s_PROTO_H_' % (package.upper(), proto_name.upper())
  header = """

}  // namespace %(package)s

#endif  // %(guard_name)s
""" % {'guard_name': guard_name, 'package': package}
  impl = """
}  // namespace %(package)s
""" % {'package': package}

  header_file.write(header)
  impl_file.write(impl)


def GenerateEnumPrinter(enum, header_file, impl_file):
  """Generates and prints a function to print an enum value.

  Args:
    enum: An Enum instance.
    header_file: The header file handle, open for writing.
    impl_file: The implementation file handle, open for writing.
  """
  declare = """
std::string GetProtoDebugStringWithIndent(%(name)s value, int indent_size);
std::string GetProtoDebugString(%(name)s value);""" % {'name': enum.name}
  define_begin = """
std::string GetProtoDebugString(%(name)s value) {
  return GetProtoDebugStringWithIndent(value, 0);
}

std::string GetProtoDebugStringWithIndent(%(name)s value, int indent_size) {
""" % {'name': enum.name}
  define_end = """
  return "<unknown>";
}
"""
  condition = """
  if (value == %(value_name)s) {
    return "%(value_name)s";
  }"""

  header_file.write(declare)
  impl_file.write(define_begin)
  for value_name in enum.values:
    impl_file.write(condition % {'value_name': value_name})
  impl_file.write(define_end)


def GenerateMessagePrinter(message, header_file, impl_file):
  """Generates and prints a function to print a message.

  Args:
    message: A Message instance.
    header_file: The header file handle, open for writing.
    impl_file: The implementation file handle, open for writing.
  """
  declare = """
std::string GetProtoDebugStringWithIndent(const %(name)s& value,
                                          int indent_size);
std::string GetProtoDebugString(const %(name)s& value);""" % {'name':
                                                              message.name}
  define_begin = """
std::string GetProtoDebugString(const %(name)s& value) {
  return GetProtoDebugStringWithIndent(value, 0);
}

std::string GetProtoDebugStringWithIndent(const %(name)s& value,
                                          int indent_size) {
  std::string indent(indent_size, ' ');
  std::string output = base::StringPrintf("[%%s] {\\n",
                                          value.GetTypeName().c_str());
""" % {'name': message.name}
  define_end = """
  output += indent + "}\\n";
  return output;
}
"""
  singular_field = """
  if (value.has_%(name)s()) {
    output += indent + "  %(name)s: ";
    base::StringAppendF(&output, %(format)s);
    output += "\\n";
  }"""
  repeated_field = """
  output += indent + "  %(name)s: {";
  for (int i = 0; i < value.%(name)s_size(); ++i) {
    base::StringAppendF(&output, %(format)s);
  }
  output += "}\\n";"""
  singular_field_get = 'value.%(name)s()'
  repeated_field_get = 'value.%(name)s(i)'
  formats = {'bool': '"%%s", %(value)s ? "true" : "false"',
             'int32': '"%%d", %(value)s',
             'int64': '"%%ld", %(value)s',
             'uint32': '"%%u", %(value)s',
             'uint64': '"%%lu", %(value)s',
             'string': '"%%s", %(value)s.c_str()',
             'bytes': """"%%s", base::HexEncode(%(value)s.data(),
                                                %(value)s.size()).c_str()"""}
  subtype_format = ('"%%s", GetProtoDebugStringWithIndent(%(value)s, '
                    'indent_size + 2).c_str()')

  header_file.write(declare)
  impl_file.write(define_begin)
  for field in message.fields:
    if field.repeated:
      value_get = repeated_field_get % {'name': field.name}
      field_code = repeated_field
    else:
      value_get = singular_field_get % {'name': field.name}
      field_code = singular_field
    if field.type_ in formats:
      value_format = formats[field.type_] % {'value': value_get}
    else:
      value_format = subtype_format % {'value': value_get}
    impl_file.write(field_code % {'name': field.name,
                                  'format': value_format})
  impl_file.write(define_end)


def FormatFile(filename):
  subprocess.call(['clang-format', '-i', '-style=Chromium', filename])


def main():
  parser = argparse.ArgumentParser(description='print proto code generator')
  parser.add_argument('input_file')
  parser.add_argument('--subdir', default='')
  args = parser.parse_args()
  with open(args.input_file) as input_file:
    package, imports, messages, enums = ParseProto(input_file)
  proto_name = os.path.basename(args.input_file).rsplit('.', 1)[0]
  header_file_name = 'print_%s_proto.h' % proto_name
  impl_file_name = 'print_%s_proto.cc' % proto_name
  with open(header_file_name, 'w') as header_file:
    with open(impl_file_name, 'w') as impl_file:
      GenerateFileHeaders(proto_name, package, imports, args.subdir,
                          header_file_name, header_file, impl_file)
      for enum in enums:
        GenerateEnumPrinter(enum, header_file, impl_file)
      for message in messages:
        GenerateMessagePrinter(message, header_file, impl_file)
      GenerateFileFooters(proto_name, package, args.subdir, header_file,
                          impl_file)
  FormatFile(header_file_name)
  FormatFile(impl_file_name)

if __name__ == '__main__':
  main()
