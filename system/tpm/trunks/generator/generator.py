#!/usr/bin/python2

#
# Copyright (C) 2014 The Android Open Source Project
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

"""A code generator for TPM 2.0 structures and commands.

The generator takes as input a structures file as emitted by the
extract_structures.sh script and a commands file as emitted by the
extract_commands.sh script.  It outputs valid C++ into tpm_generated.{h,cc}.

The input grammar is documented in the extract_* scripts. Sample input for
structures looks like this:
_BEGIN_TYPES
_OLD_TYPE UINT32
_NEW_TYPE TPM_HANDLE
_END
_BEGIN_CONSTANTS
_CONSTANTS (UINT32) TPM_SPEC
_TYPE UINT32
_NAME TPM_SPEC_FAMILY
_VALUE 0x322E3000
_NAME TPM_SPEC_LEVEL
_VALUE 00
_END
_BEGIN_STRUCTURES
_STRUCTURE TPMS_TIME_INFO
_TYPE UINT64
_NAME time
_TYPE TPMS_CLOCK_INFO
_NAME clockInfo
_END

Sample input for commands looks like this:
_BEGIN
_INPUT_START TPM2_Startup
_TYPE TPMI_ST_COMMAND_TAG
_NAME tag
_COMMENT TPM_ST_NO_SESSIONS
_TYPE UINT32
_NAME commandSize
_TYPE TPM_CC
_NAME commandCode
_COMMENT TPM_CC_Startup {NV}
_TYPE TPM_SU
_NAME startupType
_COMMENT TPM_SU_CLEAR or TPM_SU_STATE
_OUTPUT_START TPM2_Startup
_TYPE TPM_ST
_NAME tag
_COMMENT see clause 8
_TYPE UINT32
_NAME responseSize
_TYPE TPM_RC
_NAME responseCode
_END
"""

from __future__ import print_function

import argparse
import re

import union_selectors

_BASIC_TYPES = ['uint8_t', 'int8_t', 'int', 'uint16_t', 'int16_t',
                'uint32_t', 'int32_t', 'uint64_t', 'int64_t']
_OUTPUT_FILE_H = 'tpm_generated.h'
_OUTPUT_FILE_CC = 'tpm_generated.cc'
_COPYRIGHT_HEADER = (
    '//\n'
    '// Copyright (C) 2015 The Android Open Source Project\n'
    '//\n'
    '// Licensed under the Apache License, Version 2.0 (the "License");\n'
    '// you may not use this file except in compliance with the License.\n'
    '// You may obtain a copy of the License at\n'
    '//\n'
    '//      http://www.apache.org/licenses/LICENSE-2.0\n'
    '//\n'
    '// Unless required by applicable law or agreed to in writing, software\n'
    '// distributed under the License is distributed on an "AS IS" BASIS,\n'
    '// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or '
    'implied.\n'
    '// See the License for the specific language governing permissions and\n'
    '// limitations under the License.\n'
    '//\n\n'
    '// THIS CODE IS GENERATED - DO NOT MODIFY!\n')
_HEADER_FILE_GUARD_HEADER = """
#ifndef %(name)s
#define %(name)s
"""
_HEADER_FILE_GUARD_FOOTER = """
#endif  // %(name)s
"""
_HEADER_FILE_INCLUDES = """
#include <string>

#include <base/callback_forward.h>
#include <base/macros.h>

#include "trunks/trunks_export.h"
"""
_IMPLEMENTATION_FILE_INCLUDES = """
#include <string>

#include <base/bind.h>
#include <base/callback.h>
#include <base/logging.h>
#include <base/macros.h>
#include <base/stl_util.h>
#include <base/strings/string_number_conversions.h>
#include <base/sys_byteorder.h>
#include <crypto/secure_hash.h>

#include "trunks/authorization_delegate.h"
#include "trunks/command_transceiver.h"
#include "trunks/error_codes.h"

"""
_LOCAL_INCLUDE = """
#include "trunks/%(filename)s"
"""
_NAMESPACE_BEGIN = """
namespace trunks {
"""
_NAMESPACE_END = """
}  // namespace trunks
"""
_FORWARD_DECLARATIONS = """
class AuthorizationDelegate;
class CommandTransceiver;
"""
_FUNCTION_DECLARATIONS = """
TRUNKS_EXPORT size_t GetNumberOfRequestHandles(TPM_CC command_code);
TRUNKS_EXPORT size_t GetNumberOfResponseHandles(TPM_CC command_code);
"""
_CLASS_BEGIN = """
class TRUNKS_EXPORT Tpm {
 public:
  // Does not take ownership of |transceiver|.
  explicit Tpm(CommandTransceiver* transceiver) : transceiver_(transceiver) {}
  virtual ~Tpm() {}

"""
_CLASS_END = """
 private:
  CommandTransceiver* transceiver_;

  DISALLOW_COPY_AND_ASSIGN(Tpm);
};
"""
_SERIALIZE_BASIC_TYPE = """
TPM_RC Serialize_%(type)s(const %(type)s& value, std::string* buffer) {
  VLOG(3) << __func__;
  %(type)s value_net = value;
  switch (sizeof(%(type)s)) {
    case 2:
      value_net = base::HostToNet16(value);
      break;
    case 4:
      value_net = base::HostToNet32(value);
      break;
    case 8:
      value_net = base::HostToNet64(value);
      break;
    default:
      break;
  }
  const char* value_bytes = reinterpret_cast<const char*>(&value_net);
  buffer->append(value_bytes, sizeof(%(type)s));
  return TPM_RC_SUCCESS;
}

TPM_RC Parse_%(type)s(
    std::string* buffer,
    %(type)s* value,
    std::string* value_bytes) {
  VLOG(3) << __func__;
  if (buffer->size() < sizeof(%(type)s))
    return TPM_RC_INSUFFICIENT;
  %(type)s value_net = 0;
  memcpy(&value_net, buffer->data(), sizeof(%(type)s));
  switch (sizeof(%(type)s)) {
    case 2:
      *value = base::NetToHost16(value_net);
      break;
    case 4:
      *value = base::NetToHost32(value_net);
      break;
    case 8:
      *value = base::NetToHost64(value_net);
      break;
    default:
      *value = value_net;
  }
  if (value_bytes) {
    value_bytes->append(buffer->substr(0, sizeof(%(type)s)));
  }
  buffer->erase(0, sizeof(%(type)s));
  return TPM_RC_SUCCESS;
}
"""
_SERIALIZE_DECLARATION = """
TRUNKS_EXPORT TPM_RC Serialize_%(type)s(
    const %(type)s& value,
    std::string* buffer);

TRUNKS_EXPORT TPM_RC Parse_%(type)s(
    std::string* buffer,
    %(type)s* value,
    std::string* value_bytes);
"""

_SIMPLE_TPM2B_HELPERS_DECLARATION = """
TRUNKS_EXPORT %(type)s Make_%(type)s(
    const std::string& bytes);
TRUNKS_EXPORT std::string StringFrom_%(type)s(
    const %(type)s& tpm2b);
"""
_COMPLEX_TPM2B_HELPERS_DECLARATION = """
TRUNKS_EXPORT %(type)s Make_%(type)s(
    const %(inner_type)s& inner);
"""

_HANDLE_COUNT_FUNCTION_START = """
size_t GetNumberOf%(handle_type)sHandles(TPM_CC command_code) {
  switch (command_code) {"""
_HANDLE_COUNT_FUNCTION_CASE = """
    case %(command_code)s: return %(handle_count)s;"""
_HANDLE_COUNT_FUNCTION_END = """
    default: LOG(WARNING) << "Unknown command code: " << command_code;
  }
  return 0;
}
"""

def FixName(name):
  """Fixes names to conform to Chromium style."""
  # Handle names with array notation. E.g. 'myVar[10]' is grouped as 'myVar' and
  # '[10]'.
  match = re.search(r'([^\[]*)(\[.*\])*', name)
  # Transform the name to Chromium style. E.g. 'myVarAgain' becomes
  # 'my_var_again'.
  fixed_name = re.sub(r'([a-z0-9])([A-Z])', r'\1_\2', match.group(1)).lower()
  return fixed_name + match.group(2) if match.group(2) else fixed_name


def IsTPM2B(name):
  return name.startswith('TPM2B_')


def GetCppBool(condition):
  if condition:
    return 'true'
  return 'false'


class Typedef(object):
  """Represents a TPM typedef.

  Attributes:
    old_type: The existing type in a typedef statement.
    new_type: The new type in a typedef statement.
  """

  _TYPEDEF = 'typedef %(old_type)s %(new_type)s;\n'
  _SERIALIZE_FUNCTION = """
TPM_RC Serialize_%(new)s(
    const %(new)s& value,
    std::string* buffer) {
  VLOG(3) << __func__;
  return Serialize_%(old)s(value, buffer);
}
"""
  _PARSE_FUNCTION = """
TPM_RC Parse_%(new)s(
    std::string* buffer,
    %(new)s* value,
    std::string* value_bytes) {
  VLOG(3) << __func__;
  return Parse_%(old)s(buffer, value, value_bytes);
}
"""

  def __init__(self, old_type, new_type):
    """Initializes a Typedef instance.

    Args:
      old_type: The existing type in a typedef statement.
      new_type: The new type in a typedef statement.
    """
    self.old_type = old_type
    self.new_type = new_type

  def OutputForward(self, out_file, defined_types, typemap):
    """Writes a typedef definition to |out_file|.

    Any outstanding dependencies will be forward declared. This method is the
    same as Output() because forward declarations do not apply for typedefs.

    Args:
      out_file: The output file.
      defined_types: A set of types for which definitions have already been
          generated.
      typemap: A dict mapping type names to the corresponding object.
    """
    self.Output(out_file, defined_types, typemap)

  def Output(self, out_file, defined_types, typemap):
    """Writes a typedef definition to |out_file|.

    Any outstanding dependencies will be forward declared.

    Args:
      out_file: The output file.
      defined_types: A set of types for which definitions have already been
          generated.
      typemap: A dict mapping type names to the corresponding object.
    """
    if self.new_type in defined_types:
      return
    # Make sure the dependency is already defined.
    if self.old_type not in defined_types:
      typemap[self.old_type].OutputForward(out_file, defined_types, typemap)
    out_file.write(self._TYPEDEF % {'old_type': self.old_type,
                                    'new_type': self.new_type})
    defined_types.add(self.new_type)

  def OutputSerialize(self, out_file, serialized_types, typemap):
    """Writes a serialize and parse function for the typedef to |out_file|.

    Args:
      out_file: The output file.
      serialized_types: A set of types for which serialize and parse functions
        have already been generated.
      typemap: A dict mapping type names to the corresponding object.
    """
    if self.new_type in serialized_types:
      return
    if self.old_type not in serialized_types:
      typemap[self.old_type].OutputSerialize(out_file, serialized_types,
                                             typemap)
    out_file.write(self._SERIALIZE_FUNCTION % {'old': self.old_type,
                                               'new': self.new_type})
    out_file.write(self._PARSE_FUNCTION % {'old': self.old_type,
                                           'new': self.new_type})
    serialized_types.add(self.new_type)


class Constant(object):
  """Represents a TPM constant.

  Attributes:
    const_type: The type of the constant (e.g. 'int').
    name: The name of the constant (e.g. 'kMyConstant').
    value: The value of the constant (e.g. '7').
  """

  _CONSTANT = 'const %(type)s %(name)s = %(value)s;\n'

  def __init__(self, const_type, name, value):
    """Initializes a Constant instance.

    Args:
      const_type: The type of the constant (e.g. 'int').
      name: The name of the constant (e.g. 'kMyConstant').
      value: The value of the constant (e.g. '7').
    """
    self.const_type = const_type
    self.name = name
    self.value = value

  def Output(self, out_file, defined_types, typemap):
    """Writes a constant definition to |out_file|.

    Any outstanding dependencies will be forward declared.

    Args:
      out_file: The output file.
      defined_types: A set of types for which definitions have already been
          generated.
      typemap: A dict mapping type names to the corresponding object.
    """
    # Make sure the dependency is already defined.
    if self.const_type not in defined_types:
      typemap[self.const_type].OutputForward(out_file, defined_types, typemap)
    out_file.write(self._CONSTANT % {'type': self.const_type,
                                     'name': self.name,
                                     'value': self.value})


class Structure(object):
  """Represents a TPM structure or union.

  Attributes:
    name: The name of the structure.
    is_union: A boolean indicating whether this is a union.
    fields: A list of (type, name) tuples representing the struct fields.
    depends_on: A list of strings for types this struct depends on other than
        field types. See AddDependency() for more details.
  """

  _STRUCTURE = 'struct %(name)s {\n'
  _STRUCTURE_FORWARD = 'struct %(name)s;\n'
  _UNION = 'union %(name)s {\n'
  _UNION_FORWARD = 'union %(name)s;\n'
  _STRUCTURE_END = '};\n\n'
  _STRUCTURE_FIELD = '  %(type)s %(name)s;\n'
  _SERIALIZE_FUNCTION_START = """
TPM_RC Serialize_%(type)s(
    const %(type)s& value,
    std::string* buffer) {
  TPM_RC result = TPM_RC_SUCCESS;
  VLOG(3) << __func__;
"""
  _SERIALIZE_FIELD = """
  result = Serialize_%(type)s(value.%(name)s, buffer);
  if (result) {
    return result;
  }
"""
  _SERIALIZE_FIELD_ARRAY = """
  if (arraysize(value.%(name)s) < value.%(count)s) {
    return TPM_RC_INSUFFICIENT;
  }
  for (uint32_t i = 0; i < value.%(count)s; ++i) {
    result = Serialize_%(type)s(value.%(name)s[i], buffer);
    if (result) {
      return result;
    }
  }
"""
  _SERIALIZE_FIELD_WITH_SELECTOR = """
  result = Serialize_%(type)s(
      value.%(name)s,
      value.%(selector_name)s,
      buffer);
  if (result) {
    return result;
  }
"""
  _SERIALIZE_COMPLEX_TPM2B = """
  std::string field_bytes;
  result = Serialize_%(type)s(value.%(name)s, &field_bytes);
  if (result) {
    return result;
  }
  std::string size_bytes;
  result = Serialize_UINT16(field_bytes.size(), &size_bytes);
  if (result) {
    return result;
  }
  buffer->append(size_bytes + field_bytes);
"""
  _PARSE_FUNCTION_START = """
TPM_RC Parse_%(type)s(
    std::string* buffer,
    %(type)s* value,
    std::string* value_bytes) {
  TPM_RC result = TPM_RC_SUCCESS;
  VLOG(3) << __func__;
"""
  _PARSE_FIELD = """
  result = Parse_%(type)s(
      buffer,
      &value->%(name)s,
      value_bytes);
  if (result) {
    return result;
  }
"""
  _PARSE_FIELD_ARRAY = """
  if (arraysize(value->%(name)s) < value->%(count)s) {
    return TPM_RC_INSUFFICIENT;
  }
  for (uint32_t i = 0; i < value->%(count)s; ++i) {
    result = Parse_%(type)s(
        buffer,
        &value->%(name)s[i],
        value_bytes);
    if (result) {
      return result;
    }
  }
"""
  _PARSE_FIELD_WITH_SELECTOR = """
  result = Parse_%(type)s(
      buffer,
      value->%(selector_name)s,
      &value->%(name)s,
      value_bytes);
  if (result) {
    return result;
  }
"""
  _SERIALIZE_FUNCTION_END = '  return result;\n}\n'
  _ARRAY_FIELD_RE = re.compile(r'(.*)\[(.*)\]')
  _ARRAY_FIELD_SIZE_RE = re.compile(r'^(count|size)')
  _UNION_TYPE_RE = re.compile(r'^TPMU_.*')
  _SERIALIZE_UNION_FUNCTION_START = """
TPM_RC Serialize_%(union_type)s(
    const %(union_type)s& value,
    %(selector_type)s selector,
    std::string* buffer) {
  TPM_RC result = TPM_RC_SUCCESS;
  VLOG(3) << __func__;
"""
  _SERIALIZE_UNION_FIELD = """
  if (selector == %(selector_value)s) {
    result = Serialize_%(field_type)s(value.%(field_name)s, buffer);
    if (result) {
      return result;
    }
  }
"""
  _SERIALIZE_UNION_FIELD_ARRAY = """
  if (selector == %(selector_value)s) {
    if (arraysize(value.%(field_name)s) < %(count)s) {
      return TPM_RC_INSUFFICIENT;
    }
    for (uint32_t i = 0; i < %(count)s; ++i) {
      result = Serialize_%(field_type)s(value.%(field_name)s[i], buffer);
      if (result) {
        return result;
      }
    }
  }
"""
  _PARSE_UNION_FUNCTION_START = """
TPM_RC Parse_%(union_type)s(
    std::string* buffer,
    %(selector_type)s selector,
    %(union_type)s* value,
    std::string* value_bytes) {
  TPM_RC result = TPM_RC_SUCCESS;
  VLOG(3) << __func__;
"""
  _PARSE_UNION_FIELD = """
  if (selector == %(selector_value)s) {
    result = Parse_%(field_type)s(
        buffer,
        &value->%(field_name)s,
        value_bytes);
    if (result) {
      return result;
    }
  }
"""
  _PARSE_UNION_FIELD_ARRAY = """
  if (selector == %(selector_value)s) {
    if (arraysize(value->%(field_name)s) < %(count)s) {
      return TPM_RC_INSUFFICIENT;
    }
    for (uint32_t i = 0; i < %(count)s; ++i) {
      result = Parse_%(field_type)s(
          buffer,
          &value->%(field_name)s[i],
          value_bytes);
      if (result) {
        return result;
      }
    }
  }
"""
  _EMPTY_UNION_CASE = """
  if (selector == %(selector_value)s) {
    // Do nothing.
  }
"""
  _SIMPLE_TPM2B_HELPERS = """
%(type)s Make_%(type)s(
    const std::string& bytes) {
  %(type)s tpm2b;
  CHECK(bytes.size() <= sizeof(tpm2b.%(buffer_name)s));
  memset(&tpm2b, 0, sizeof(%(type)s));
  tpm2b.size = bytes.size();
  memcpy(tpm2b.%(buffer_name)s, bytes.data(), bytes.size());
  return tpm2b;
}

std::string StringFrom_%(type)s(
    const %(type)s& tpm2b) {
  const char* char_buffer = reinterpret_cast<const char*>(
      tpm2b.%(buffer_name)s);
  return std::string(char_buffer, tpm2b.size);
}
"""
  _COMPLEX_TPM2B_HELPERS = """
%(type)s Make_%(type)s(
    const %(inner_type)s& inner) {
  %(type)s tpm2b;
  tpm2b.size = sizeof(%(inner_type)s);
  tpm2b.%(inner_name)s = inner;
  return tpm2b;
}
"""

  def __init__(self, name, is_union):
    """Initializes a Structure instance.

    Initially the instance will have no fields and no dependencies. Those can be
    added with the AddField() and AddDependency() methods.

    Args:
      name: The name of the structure.
      is_union: A boolean indicating whether this is a union.
    """
    self.name = name
    self.is_union = is_union
    self.fields = []
    self.depends_on = []
    self._forwarded = False

  def AddField(self, field_type, field_name):
    """Adds a field for this struct.

    Args:
      field_type: The type of the field.
      field_name: The name of the field.
    """
    self.fields.append((field_type, FixName(field_name)))

  def AddDependency(self, required_type):
    """Adds an explicit dependency on another type.

    This is used in cases where there is an additional dependency other than the
    field types, which are implicit dependencies.  For example, a field like
    FIELD_TYPE value[sizeof(OTHER_TYPE)] would need OTHER_TYPE to be already
    declared.

    Args:
      required_type: The type this structure depends on.
    """
    self.depends_on.append(required_type)

  def IsSimpleTPM2B(self):
    """Returns whether this struct is a TPM2B structure with raw bytes."""
    return self.name.startswith('TPM2B_') and self.fields[1][0] == 'BYTE'

  def IsComplexTPM2B(self):
    """Returns whether this struct is a TPM2B structure with an inner struct."""
    return self.name.startswith('TPM2B_') and self.fields[1][0] != 'BYTE'

  def _GetFieldTypes(self):
    """Creates a set which holds all current field types.

    Returns:
      A set of field types.
    """
    return set([field[0] for field in self.fields])

  def OutputForward(self, out_file, unused_defined_types, unused_typemap):
    """Writes a structure forward declaration to |out_file|.

    This method needs to match the OutputForward method in other type classes
    (e.g. Typedef) which is why the unused_* args exist.

    Args:
      out_file: The output file.
      unused_defined_types: Not used.
      unused_typemap: Not used.
    """
    if self._forwarded:
      return
    if self.is_union:
      out_file.write(self._UNION_FORWARD % {'name': self.name})
    else:
      out_file.write(self._STRUCTURE_FORWARD % {'name': self.name})
    self._forwarded = True

  def Output(self, out_file, defined_types, typemap):
    """Writes a structure definition to |out_file|.

    Any outstanding dependencies will be defined.

    Args:
      out_file: The output file.
      defined_types: A set of types for which definitions have already been
        generated.
      typemap: A dict mapping type names to the corresponding object.
    """
    if self.name in defined_types:
      return
    # Make sure any dependencies are already defined.
    for field_type in self._GetFieldTypes():
      if field_type not in defined_types:
        typemap[field_type].Output(out_file, defined_types, typemap)
    for required_type in self.depends_on:
      if required_type not in defined_types:
        typemap[required_type].Output(out_file, defined_types, typemap)
    if self.is_union:
      out_file.write(self._UNION % {'name': self.name})
    else:
      out_file.write(self._STRUCTURE % {'name': self.name})
    for field in self.fields:
      out_file.write(self._STRUCTURE_FIELD % {'type': field[0],
                                              'name': field[1]})
    out_file.write(self._STRUCTURE_END)
    defined_types.add(self.name)

  def OutputSerialize(self, out_file, serialized_types, typemap):
    """Writes serialize and parse functions for a structure to |out_file|.

    Args:
      out_file: The output file.
      serialized_types: A set of types for which serialize and parse functions
        have already been generated.  This type name of this structure will be
        added on success.
      typemap: A dict mapping type names to the corresponding object.
    """
    if (self.name in serialized_types or
        self.name == 'TPMU_NAME' or
        self.name == 'TPMU_ENCRYPTED_SECRET'):
      return
    # Make sure any dependencies already have serialize functions defined.
    for field_type in self._GetFieldTypes():
      if field_type not in serialized_types:
        typemap[field_type].OutputSerialize(out_file, serialized_types, typemap)
    if self.is_union:
      self._OutputUnionSerialize(out_file)
      serialized_types.add(self.name)
      return
    out_file.write(self._SERIALIZE_FUNCTION_START % {'type': self.name})
    if self.IsComplexTPM2B():
      field_type = self.fields[1][0]
      field_name = self.fields[1][1]
      out_file.write(self._SERIALIZE_COMPLEX_TPM2B % {'type': field_type,
                                                      'name': field_name})
    else:
      for field in self.fields:
        if self._ARRAY_FIELD_RE.search(field[1]):
          self._OutputArrayField(out_file, field, self._SERIALIZE_FIELD_ARRAY)
        elif self._UNION_TYPE_RE.search(field[0]):
          self._OutputUnionField(out_file, field,
                                 self._SERIALIZE_FIELD_WITH_SELECTOR)
        else:
          out_file.write(self._SERIALIZE_FIELD % {'type': field[0],
                                                  'name': field[1]})
    out_file.write(self._SERIALIZE_FUNCTION_END)
    out_file.write(self._PARSE_FUNCTION_START % {'type': self.name})
    for field in self.fields:
      if self._ARRAY_FIELD_RE.search(field[1]):
        self._OutputArrayField(out_file, field, self._PARSE_FIELD_ARRAY)
      elif self._UNION_TYPE_RE.search(field[0]):
        self._OutputUnionField(out_file, field, self._PARSE_FIELD_WITH_SELECTOR)
      else:
        out_file.write(self._PARSE_FIELD % {'type': field[0],
                                            'name': field[1]})
    out_file.write(self._SERIALIZE_FUNCTION_END)
    # If this is a TPM2B structure throw in a few convenience functions.
    if self.IsSimpleTPM2B():
      field_name = self._ARRAY_FIELD_RE.search(self.fields[1][1]).group(1)
      out_file.write(self._SIMPLE_TPM2B_HELPERS % {'type': self.name,
                                                   'buffer_name': field_name})
    elif self.IsComplexTPM2B():
      field_type = self.fields[1][0]
      field_name = self.fields[1][1]
      out_file.write(self._COMPLEX_TPM2B_HELPERS % {'type': self.name,
                                                    'inner_type': field_type,
                                                    'inner_name': field_name})
    serialized_types.add(self.name)

  def _OutputUnionSerialize(self, out_file):
    """Writes serialize and parse functions for a union to |out_file|.

    This is more complex than the struct case because only one field of the
    union is serialized / parsed based on the value of a selector.  Arrays are
    also handled differently: the full size of the array is serialized instead
    of looking for a field which specifies the count.

    Args:
      out_file: The output file
    """
    selector_type = union_selectors.GetUnionSelectorType(self.name)
    selector_values = union_selectors.GetUnionSelectorValues(self.name)
    field_types = {f[1]: f[0] for f in self.fields}
    out_file.write(self._SERIALIZE_UNION_FUNCTION_START %
                   {'union_type': self.name, 'selector_type': selector_type})
    for selector in selector_values:
      field_name = FixName(union_selectors.GetUnionSelectorField(self.name,
                                                                 selector))
      if not field_name:
        out_file.write(self._EMPTY_UNION_CASE % {'selector_value': selector})
        continue
      field_type = field_types[field_name]
      array_match = self._ARRAY_FIELD_RE.search(field_name)
      if array_match:
        field_name = array_match.group(1)
        count = array_match.group(2)
        out_file.write(self._SERIALIZE_UNION_FIELD_ARRAY %
                       {'selector_value': selector,
                        'count': count,
                        'field_type': field_type,
                        'field_name': field_name})
      else:
        out_file.write(self._SERIALIZE_UNION_FIELD %
                       {'selector_value': selector,
                        'field_type': field_type,
                        'field_name': field_name})
    out_file.write(self._SERIALIZE_FUNCTION_END)
    out_file.write(self._PARSE_UNION_FUNCTION_START %
                   {'union_type': self.name, 'selector_type': selector_type})
    for selector in selector_values:
      field_name = FixName(union_selectors.GetUnionSelectorField(self.name,
                                                                 selector))
      if not field_name:
        out_file.write(self._EMPTY_UNION_CASE % {'selector_value': selector})
        continue
      field_type = field_types[field_name]
      array_match = self._ARRAY_FIELD_RE.search(field_name)
      if array_match:
        field_name = array_match.group(1)
        count = array_match.group(2)
        out_file.write(self._PARSE_UNION_FIELD_ARRAY %
                       {'selector_value': selector,
                        'count': count,
                        'field_type': field_type,
                        'field_name': field_name})
      else:
        out_file.write(self._PARSE_UNION_FIELD %
                       {'selector_value': selector,
                        'field_type': field_type,
                        'field_name': field_name})
    out_file.write(self._SERIALIZE_FUNCTION_END)

  def _OutputUnionField(self, out_file, field, code_format):
    """Writes serialize / parse code for a union field.

    In this case |self| may not necessarily represent a union but |field| does.
    This requires that a field of an acceptable selector type appear somewhere
    in the struct.  The value of this field is used as the selector value when
    calling the serialize / parse function for the union.

    Args:
      out_file: The output file.
      field: The union field to be processed as a (type, name) tuple.
      code_format: Must be (_SERIALIZE|_PARSE)_FIELD_WITH_SELECTOR
    """
    selector_types = union_selectors.GetUnionSelectorTypes(field[0])
    selector_name = ''
    for tmp in self.fields:
      if tmp[0] in selector_types:
        selector_name = tmp[1]
        break
    assert selector_name, 'Missing selector for %s in %s!' % (field[1],
                                                              self.name)
    out_file.write(code_format % {'type': field[0],
                                  'selector_name': selector_name,
                                  'name': field[1]})

  def _OutputArrayField(self, out_file, field, code_format):
    """Writes serialize / parse code for an array field.

    The allocated size of the array is ignored and a field which holds the
    actual count of items in the array must exist.  Only the number of items
    represented by the value of that count field are serialized / parsed.

    Args:
      out_file: The output file.
      field: The array field to be processed as a (type, name) tuple.
      code_format: Must be (_SERIALIZE|_PARSE)_FIELD_ARRAY
    """
    field_name = self._ARRAY_FIELD_RE.search(field[1]).group(1)
    for count_field in self.fields:
      assert count_field != field, ('Missing count field for %s in %s!' %
                                    (field[1], self.name))
      if self._ARRAY_FIELD_SIZE_RE.search(count_field[1]):
        out_file.write(code_format % {'count': count_field[1],
                                      'type': field[0],
                                      'name': field_name})
        break


class Define(object):
  """Represents a preprocessor define.

  Attributes:
    name: The name being defined.
    value: The value being assigned to the name.
  """

  _DEFINE = '#if !defined(%(name)s)\n#define %(name)s %(value)s\n#endif\n'

  def __init__(self, name, value):
    """Initializes a Define instance.

    Args:
      name: The name being defined.
      value: The value being assigned to the name.
    """
    self.name = name
    self.value = value

  def Output(self, out_file):
    """Writes a preprocessor define to |out_file|.

    Args:
      out_file: The output file.
    """
    out_file.write(self._DEFINE % {'name': self.name, 'value': self.value})


class StructureParser(object):
  """Structure definition parser.

  The input text file is extracted from the PDF file containing the TPM
  structures specification from the Trusted Computing Group. The syntax
  of the text file is defined by extract_structures.sh.

  - Parses typedefs to a list of Typedef objects.
  - Parses constants to a list of Constant objects.
  - Parses structs and unions to a list of Structure objects.
  - Parses defines to a list of Define objects.

  The parser also creates 'typemap' dict which maps every type to its generator
  object.  This typemap helps manage type dependencies.

  Example usage:
  parser = StructureParser(open('myfile'))
  types, constants, structs, defines, typemap = parser.Parse()
  """

  # Compile regular expressions.
  _BEGIN_TYPES_TOKEN = '_BEGIN_TYPES'
  _BEGIN_CONSTANTS_TOKEN = '_BEGIN_CONSTANTS'
  _BEGIN_STRUCTURES_TOKEN = '_BEGIN_STRUCTURES'
  _BEGIN_UNIONS_TOKEN = '_BEGIN_UNIONS'
  _BEGIN_DEFINES_TOKEN = '_BEGIN_DEFINES'
  _END_TOKEN = '_END'
  _OLD_TYPE_RE = re.compile(r'^_OLD_TYPE\s+(\w+)$')
  _NEW_TYPE_RE = re.compile(r'^_NEW_TYPE\s+(\w+)$')
  _CONSTANTS_SECTION_RE = re.compile(r'^_CONSTANTS.* (\w+)$')
  _STRUCTURE_SECTION_RE = re.compile(r'^_STRUCTURE\s+(\w+)$')
  _UNION_SECTION_RE = re.compile(r'^_UNION\s+(\w+)$')
  _TYPE_RE = re.compile(r'^_TYPE\s+(\w+)$')
  _NAME_RE = re.compile(r'^_NAME\s+([a-zA-Z0-9_()\[\]/\*\+\-]+)$')
  _VALUE_RE = re.compile(r'^_VALUE\s+(.+)$')
  _SIZEOF_RE = re.compile(r'^.*sizeof\(([a-zA-Z0-9_]*)\).*$')

  def __init__(self, in_file):
    """Initializes a StructureParser instance.

    Args:
      in_file: A file as returned by open() which has been opened for reading.
    """
    self._line = None
    self._in_file = in_file

  def _NextLine(self):
    """Gets the next input line.

    Returns:
      The next input line if another line is available, None otherwise.
    """
    try:
      self._line = self._in_file.next()
    except StopIteration:
      self._line = None

  def Parse(self):
    """Parse everything in a structures file.

    Returns:
      Lists of objects and a type-map as described in the class documentation.
      Returns these in the following order: types, constants, structs, defines,
      typemap.
    """
    self._NextLine()
    types = []
    constants = []
    structs = []
    defines = []
    typemap = {}
    while self._line:
      if self._BEGIN_TYPES_TOKEN == self._line.rstrip():
        types += self._ParseTypes(typemap)
      elif self._BEGIN_CONSTANTS_TOKEN == self._line.rstrip():
        constants += self._ParseConstants(types, typemap)
      elif self._BEGIN_STRUCTURES_TOKEN == self._line.rstrip():
        structs += self._ParseStructures(self._STRUCTURE_SECTION_RE, typemap)
      elif self._BEGIN_UNIONS_TOKEN == self._line.rstrip():
        structs += self._ParseStructures(self._UNION_SECTION_RE, typemap)
      elif self._BEGIN_DEFINES_TOKEN == self._line.rstrip():
        defines += self._ParseDefines()
      else:
        print('Invalid file format: %s' % self._line)
        break
      self._NextLine()
    # Empty structs not handled by the extractor.
    self._AddEmptyStruct('TPMU_SYM_DETAILS', True, structs, typemap)
    # Defines which are used in TPM 2.0 Part 2 but not defined there.
    defines.append(Define(
        'MAX_CAP_DATA', '(MAX_CAP_BUFFER-sizeof(TPM_CAP)-sizeof(UINT32))'))
    defines.append(Define(
        'MAX_CAP_ALGS', '(TPM_ALG_LAST - TPM_ALG_FIRST + 1)'))
    defines.append(Define(
        'MAX_CAP_HANDLES', '(MAX_CAP_DATA/sizeof(TPM_HANDLE))'))
    defines.append(Define(
        'MAX_CAP_CC', '((TPM_CC_LAST - TPM_CC_FIRST) + 1)'))
    defines.append(Define(
        'MAX_TPM_PROPERTIES', '(MAX_CAP_DATA/sizeof(TPMS_TAGGED_PROPERTY))'))
    defines.append(Define(
        'MAX_PCR_PROPERTIES', '(MAX_CAP_DATA/sizeof(TPMS_TAGGED_PCR_SELECT))'))
    defines.append(Define(
        'MAX_ECC_CURVES', '(MAX_CAP_DATA/sizeof(TPM_ECC_CURVE))'))
    defines.append(Define('HASH_COUNT', '3'))
    return types, constants, structs, defines, typemap

  def _AddEmptyStruct(self, name, is_union, structs, typemap):
    """Adds an empty Structure object to |structs| and |typemap|.

    Args:
      name: The name to assign the new structure.
      is_union: A boolean indicating whether the new structure is a union.
      structs: A list of structures to which the new object is appended.
      typemap: A map of type names to objects to which the new name and object
          are added.
    """
    s = Structure(name, is_union)
    structs.append(s)
    typemap[name] = s
    return

  def _ParseTypes(self, typemap):
    """Parses a typedefs section.

    The current line should be _BEGIN_TYPES and the method will stop parsing
    when an _END line is found.

    Args:
      typemap: A dictionary to which parsed types are added.

    Returns:
      A list of Typedef objects.
    """
    types = []
    self._NextLine()
    while self._END_TOKEN != self._line.rstrip():
      match = self._OLD_TYPE_RE.search(self._line)
      if not match:
        print('Invalid old type: %s' % self._line)
        return types
      old_type = match.group(1)
      self._NextLine()
      match = self._NEW_TYPE_RE.search(self._line)
      if not match:
        print('Invalid new type: %s' % self._line)
        return types
      new_type = match.group(1)
      t = Typedef(old_type, new_type)
      types.append(t)
      typemap[new_type] = t
      self._NextLine()
    return types

  def _ParseConstants(self, types, typemap):
    """Parses a constants section.

    The current line should be _BEGIN_CONSTANTS and the method will stop parsing
    when an _END line is found. Each group of constants has an associated type
    alias. A Typedef object is created for each of these aliases and added to
    both |types| and |typemap|.

    Args:
      types: A list of Typedef objects.
      typemap: A dictionary to which parsed types are added.

    Returns:
      A list of Constant objects.
    """
    constants = []
    self._NextLine()
    while self._END_TOKEN != self._line.rstrip():
      match = self._CONSTANTS_SECTION_RE.search(self._line)
      if not match:
        print('Invalid constants section: %s' % self._line)
        return constants
      constant_typename = match.group(1)
      self._NextLine()
      match = self._TYPE_RE.search(self._line)
      if not match:
        print('Invalid constants type: %s' % self._line)
        return constants
      constant_type = match.group(1)
      # Create a typedef for the constant group name (e.g. TPM_RC).
      typedef = Typedef(constant_type, constant_typename)
      typemap[constant_typename] = typedef
      types.append(typedef)
      self._NextLine()
      match = self._NAME_RE.search(self._line)
      if not match:
        print('Invalid constant name: %s' % self._line)
        return constants
      while match:
        name = match.group(1)
        self._NextLine()
        match = self._VALUE_RE.search(self._line)
        if not match:
          print('Invalid constant value: %s' % self._line)
          return constants
        value = match.group(1)
        constants.append(Constant(constant_typename, name, value))
        self._NextLine()
        match = self._NAME_RE.search(self._line)
    return constants

  def _ParseStructures(self, section_re, typemap):
    """Parses structures and unions.

    The current line should be _BEGIN_STRUCTURES or _BEGIN_UNIONS and the method
    will stop parsing when an _END line is found.

    Args:
      section_re: The regular expression to use for matching section tokens.
      typemap: A dictionary to which parsed types are added.

    Returns:
      A list of Structure objects.
    """
    structures = []
    is_union = section_re == self._UNION_SECTION_RE
    self._NextLine()
    while self._END_TOKEN != self._line.rstrip():
      match = section_re.search(self._line)
      if not match:
        print('Invalid structure section: %s' % self._line)
        return structures
      current_structure_name = match.group(1)
      current_structure = Structure(current_structure_name, is_union)
      self._NextLine()
      match = self._TYPE_RE.search(self._line)
      if not match:
        print('Invalid field type: %s' % self._line)
        return structures
      while match:
        field_type = match.group(1)
        self._NextLine()
        match = self._NAME_RE.search(self._line)
        if not match:
          print('Invalid field name: %s' % self._line)
          return structures
        field_name = match.group(1)
        # If the field name includes 'sizeof(SOME_TYPE)', record the dependency
        # on SOME_TYPE.
        match = self._SIZEOF_RE.search(field_name)
        if match:
          current_structure.AddDependency(match.group(1))
        # Manually change unfortunate names.
        if field_name == 'xor':
          field_name = 'xor_'
        current_structure.AddField(field_type, field_name)
        self._NextLine()
        match = self._TYPE_RE.search(self._line)
      structures.append(current_structure)
      typemap[current_structure_name] = current_structure
    return structures

  def _ParseDefines(self):
    """Parses preprocessor defines.

    The current line should be _BEGIN_DEFINES and the method will stop parsing
    when an _END line is found.

    Returns:
      A list of Define objects.
    """
    defines = []
    self._NextLine()
    while self._END_TOKEN != self._line.rstrip():
      match = self._NAME_RE.search(self._line)
      if not match:
        print('Invalid name: %s' % self._line)
        return defines
      name = match.group(1)
      self._NextLine()
      match = self._VALUE_RE.search(self._line)
      if not match:
        print('Invalid value: %s' % self._line)
        return defines
      value = match.group(1)
      defines.append(Define(name, value))
      self._NextLine()
    return defines


class Command(object):
  """Represents a TPM command.

  Attributes:
    name: The command name (e.g. 'TPM2_Startup').
    command_code: The name of the command code constant (e.g. TPM2_CC_Startup).
    request_args: A list to hold command input arguments. Each element is a dict
        and has these keys:
            'type': The argument type.
            'name': The argument name.
            'command_code': The optional value of the command code constant.
            'description': Optional descriptive text for the argument.
    response_args: A list identical in form to request_args but to hold command
        output arguments.
  """

  _HANDLE_RE = re.compile(r'TPMI_.H_.*')
  _CALLBACK_ARG = """
      const %(method_name)sResponse& callback"""
  _DELEGATE_ARG = """
      AuthorizationDelegate* authorization_delegate"""
  _SERIALIZE_ARG = """
      std::string* serialized_command"""
  _PARSE_ARG = """
      const std::string& response"""
  _SERIALIZE_FUNCTION_START = """
TPM_RC Tpm::SerializeCommand_%(method_name)s(%(method_args)s) {
  VLOG(3) << __func__;
  TPM_RC rc = TPM_RC_SUCCESS;
  TPMI_ST_COMMAND_TAG tag = TPM_ST_NO_SESSIONS;
  UINT32 command_size = 10;  // Header size.
  std::string handle_section_bytes;
  std::string parameter_section_bytes;"""
  _DECLARE_COMMAND_CODE = """
  TPM_CC command_code = %(command_code)s;"""
  _DECLARE_BOOLEAN = """
  bool %(var_name)s = %(value)s;"""
  _SERIALIZE_LOCAL_VAR = """
  std::string %(var_name)s_bytes;
  rc = Serialize_%(var_type)s(
      %(var_name)s,
      &%(var_name)s_bytes);
  if (rc != TPM_RC_SUCCESS) {
    return rc;
  }"""
  _ENCRYPT_PARAMETER = """
  if (authorization_delegate) {
    // Encrypt just the parameter data, not the size.
    std::string tmp = %(var_name)s_bytes.substr(2);
    if (!authorization_delegate->EncryptCommandParameter(&tmp)) {
      return TRUNKS_RC_ENCRYPTION_FAILED;
    }
    %(var_name)s_bytes.replace(2, std::string::npos, tmp);
  }"""
  _HASH_START = """
  scoped_ptr<crypto::SecureHash> hash(crypto::SecureHash::Create(
      crypto::SecureHash::SHA256));"""
  _HASH_UPDATE = """
  hash->Update(%(var_name)s.data(),
               %(var_name)s.size());"""
  _APPEND_COMMAND_HANDLE = """
  handle_section_bytes += %(var_name)s_bytes;
  command_size += %(var_name)s_bytes.size();"""
  _APPEND_COMMAND_PARAMETER = """
  parameter_section_bytes += %(var_name)s_bytes;
  command_size += %(var_name)s_bytes.size();"""
  _AUTHORIZE_COMMAND = """
  std::string command_hash(32, 0);
  hash->Finish(string_as_array(&command_hash), command_hash.size());
  std::string authorization_section_bytes;
  std::string authorization_size_bytes;
  if (authorization_delegate) {
    if (!authorization_delegate->GetCommandAuthorization(
        command_hash,
        is_command_parameter_encryption_possible,
        is_response_parameter_encryption_possible,
        &authorization_section_bytes)) {
      return TRUNKS_RC_AUTHORIZATION_FAILED;
    }
    if (!authorization_section_bytes.empty()) {
      tag = TPM_ST_SESSIONS;
      std::string tmp;
      rc = Serialize_UINT32(authorization_section_bytes.size(),
                            &authorization_size_bytes);
      if (rc != TPM_RC_SUCCESS) {
        return rc;
      }
      command_size += authorization_size_bytes.size() +
                      authorization_section_bytes.size();
    }
  }"""
  _SERIALIZE_FUNCTION_END = """
  *serialized_command = tag_bytes +
                        command_size_bytes +
                        command_code_bytes +
                        handle_section_bytes +
                        authorization_size_bytes +
                        authorization_section_bytes +
                        parameter_section_bytes;
  CHECK(serialized_command->size() == command_size) << "Command size mismatch!";
  VLOG(2) << "Command: " << base::HexEncode(serialized_command->data(),
                                            serialized_command->size());
  return TPM_RC_SUCCESS;
}
"""
  _RESPONSE_PARSER_START = """
TPM_RC Tpm::ParseResponse_%(method_name)s(%(method_args)s) {
  VLOG(3) << __func__;
  VLOG(2) << "Response: " << base::HexEncode(response.data(), response.size());
  TPM_RC rc = TPM_RC_SUCCESS;
  std::string buffer(response);"""
  _PARSE_LOCAL_VAR = """
  %(var_type)s %(var_name)s;
  std::string %(var_name)s_bytes;
  rc = Parse_%(var_type)s(
      &buffer,
      &%(var_name)s,
      &%(var_name)s_bytes);
  if (rc != TPM_RC_SUCCESS) {
    return rc;
  }"""
  _PARSE_ARG_VAR = """
  std::string %(var_name)s_bytes;
  rc = Parse_%(var_type)s(
      &buffer,
      %(var_name)s,
      &%(var_name)s_bytes);
  if (rc != TPM_RC_SUCCESS) {
    return rc;
  }"""
  _RESPONSE_ERROR_CHECK = """
  if (response_size != response.size()) {
    return TPM_RC_SIZE;
  }
  if (response_code != TPM_RC_SUCCESS) {
    return response_code;
  }"""
  _RESPONSE_SECTION_SPLIT = """
  std::string authorization_section_bytes;
  if (tag == TPM_ST_SESSIONS) {
    UINT32 parameter_section_size = buffer.size();
    rc = Parse_UINT32(&buffer, &parameter_section_size, nullptr);
    if (rc != TPM_RC_SUCCESS) {
      return rc;
    }
    if (parameter_section_size > buffer.size()) {
      return TPM_RC_INSUFFICIENT;
    }
    authorization_section_bytes = buffer.substr(parameter_section_size);
    // Keep the parameter section in |buffer|.
    buffer.erase(parameter_section_size);
  }"""
  _AUTHORIZE_RESPONSE = """
  std::string response_hash(32, 0);
  hash->Finish(string_as_array(&response_hash), response_hash.size());
  if (tag == TPM_ST_SESSIONS) {
    CHECK(authorization_delegate) << "Authorization delegate missing!";
    if (!authorization_delegate->CheckResponseAuthorization(
        response_hash,
        authorization_section_bytes)) {
      return TRUNKS_RC_AUTHORIZATION_FAILED;
    }
  }"""
  _DECRYPT_PARAMETER = """
  if (tag == TPM_ST_SESSIONS) {
    CHECK(authorization_delegate) << "Authorization delegate missing!";
    // Decrypt just the parameter data, not the size.
    std::string tmp = %(var_name)s_bytes.substr(2);
    if (!authorization_delegate->DecryptResponseParameter(&tmp)) {
      return TRUNKS_RC_ENCRYPTION_FAILED;
    }
    %(var_name)s_bytes.replace(2, std::string::npos, tmp);
    rc = Parse_%(var_type)s(
        &%(var_name)s_bytes,
        %(var_name)s,
        nullptr);
    if (rc != TPM_RC_SUCCESS) {
      return rc;
    }
  }"""
  _RESPONSE_PARSER_END = """
  return TPM_RC_SUCCESS;
}
"""
  _ERROR_CALLBACK_START = """
void %(method_name)sErrorCallback(
    const Tpm::%(method_name)sResponse& callback,
    TPM_RC response_code) {
  VLOG(1) << __func__;
  callback.Run(response_code"""
  _ERROR_CALLBACK_ARG = """,
               %(arg_type)s()"""
  _ERROR_CALLBACK_END = """);
}
"""
  _RESPONSE_CALLBACK_START = """
void %(method_name)sResponseParser(
    const Tpm::%(method_name)sResponse& callback,
    AuthorizationDelegate* authorization_delegate,
    const std::string& response) {
  VLOG(1) << __func__;
  base::Callback<void(TPM_RC)> error_reporter =
      base::Bind(%(method_name)sErrorCallback, callback);"""
  _DECLARE_ARG_VAR = """
  %(var_type)s %(var_name)s;"""
  _RESPONSE_CALLBACK_END = """
  TPM_RC rc = Tpm::ParseResponse_%(method_name)s(
      response,%(method_arg_names_out)s
      authorization_delegate);
  if (rc != TPM_RC_SUCCESS) {
    error_reporter.Run(rc);
    return;
  }
  callback.Run(
      rc%(method_arg_names_in)s);
}
"""
  _ASYNC_METHOD = """
void Tpm::%(method_name)s(%(method_args)s) {
  VLOG(1) << __func__;
  base::Callback<void(TPM_RC)> error_reporter =
      base::Bind(%(method_name)sErrorCallback, callback);
  base::Callback<void(const std::string&)> parser =
      base::Bind(%(method_name)sResponseParser,
                 callback,
                 authorization_delegate);
  std::string command;
  TPM_RC rc = SerializeCommand_%(method_name)s(%(method_arg_names)s
      &command,
      authorization_delegate);
  if (rc != TPM_RC_SUCCESS) {
    error_reporter.Run(rc);
    return;
  }
  transceiver_->SendCommand(command, parser);
}
"""
  _SYNC_METHOD = """
TPM_RC Tpm::%(method_name)sSync(%(method_args)s) {
  VLOG(1) << __func__;
  std::string command;
  TPM_RC rc = SerializeCommand_%(method_name)s(%(method_arg_names_in)s
      &command,
      authorization_delegate);
  if (rc != TPM_RC_SUCCESS) {
    return rc;
  }
  std::string response = transceiver_->SendCommandAndWait(command);
  rc = ParseResponse_%(method_name)s(
      response,%(method_arg_names_out)s
      authorization_delegate);
  return rc;
}
"""

  def __init__(self, name):
    """Initializes a Command instance.

    Initially the request_args and response_args attributes are not set.

    Args:
      name: The command name (e.g. 'TPM2_Startup').
    """
    self.name = name
    self.command_code = ''
    self.request_args = None
    self.response_args = None

  def OutputDeclarations(self, out_file):
    """Prints method and callback declaration statements for this command.

    Args:
      out_file: The output file.
    """
    self._OutputCallbackSignature(out_file)
    self._OutputMethodSignatures(out_file)

  def OutputSerializeFunction(self, out_file):
    """Generates a serialize function for the command inputs.

    Args:
      out_file: Generated code is written to this file.
    """
    # Categorize arguments as either handles or parameters.
    handles, parameters = self._SplitArgs(self.request_args)
    response_parameters = self._SplitArgs(self.response_args)[1]
    out_file.write(self._SERIALIZE_FUNCTION_START % {
        'method_name': self._MethodName(),
        'method_args': self._SerializeArgs()})
    out_file.write(self._DECLARE_COMMAND_CODE % {'command_code':
                                                 self.command_code})
    out_file.write(self._DECLARE_BOOLEAN % {
        'var_name': 'is_command_parameter_encryption_possible',
        'value': GetCppBool(parameters and IsTPM2B(parameters[0]['type']))})
    out_file.write(self._DECLARE_BOOLEAN % {
        'var_name': 'is_response_parameter_encryption_possible',
        'value': GetCppBool(response_parameters and
                            IsTPM2B(response_parameters[0]['type']))})
    # Serialize the command code and all the handles and parameters.
    out_file.write(self._SERIALIZE_LOCAL_VAR % {'var_name': 'command_code',
                                                'var_type': 'TPM_CC'})
    for arg in self.request_args:
      out_file.write(self._SERIALIZE_LOCAL_VAR % {'var_name': arg['name'],
                                                  'var_type': arg['type']})
    # Encrypt the first parameter (before doing authorization) if necessary.
    if parameters and IsTPM2B(parameters[0]['type']):
      out_file.write(self._ENCRYPT_PARAMETER % {'var_name':
                                                parameters[0]['name']})
    # Compute the command hash and construct handle and parameter sections.
    out_file.write(self._HASH_START)
    out_file.write(self._HASH_UPDATE % {'var_name': 'command_code_bytes'})
    for handle in handles:
      out_file.write(self._HASH_UPDATE % {'var_name':
                                          '%s_name' % handle['name']})
      out_file.write(self._APPEND_COMMAND_HANDLE % {'var_name':
                                                    handle['name']})
    for parameter in parameters:
      out_file.write(self._HASH_UPDATE % {'var_name':
                                          '%s_bytes' % parameter['name']})
      out_file.write(self._APPEND_COMMAND_PARAMETER % {'var_name':
                                                       parameter['name']})
    # Do authorization based on the hash.
    out_file.write(self._AUTHORIZE_COMMAND)
    # Now that the tag and size are finalized, serialize those.
    out_file.write(self._SERIALIZE_LOCAL_VAR %
                   {'var_name': 'tag',
                    'var_type': 'TPMI_ST_COMMAND_TAG'})
    out_file.write(self._SERIALIZE_LOCAL_VAR % {'var_name': 'command_size',
                                                'var_type': 'UINT32'})
    out_file.write(self._SERIALIZE_FUNCTION_END)

  def OutputParseFunction(self, out_file):
    """Generates a parse function for the command outputs.

    Args:
      out_file: Generated code is written to this file.
    """
    out_file.write(self._RESPONSE_PARSER_START % {
        'method_name': self._MethodName(),
        'method_args': self._ParseArgs()})
    # Parse the header -- this should always exist.
    out_file.write(self._PARSE_LOCAL_VAR % {'var_name': 'tag',
                                            'var_type': 'TPM_ST'})
    out_file.write(self._PARSE_LOCAL_VAR % {'var_name': 'response_size',
                                            'var_type': 'UINT32'})
    out_file.write(self._PARSE_LOCAL_VAR % {'var_name': 'response_code',
                                            'var_type': 'TPM_RC'})
    # Handle the error case.
    out_file.write(self._RESPONSE_ERROR_CHECK)
    # Categorize arguments as either handles or parameters.
    handles, parameters = self._SplitArgs(self.response_args)
    # Parse any handles.
    for handle in handles:
      out_file.write(self._PARSE_ARG_VAR % {'var_name': handle['name'],
                                            'var_type': handle['type']})
    # Setup a serialized command code which is needed for the response hash.
    out_file.write(self._DECLARE_COMMAND_CODE % {'command_code':
                                                 self.command_code})
    out_file.write(self._SERIALIZE_LOCAL_VAR % {'var_name': 'command_code',
                                                'var_type': 'TPM_CC'})
    # Split out the authorization section.
    out_file.write(self._RESPONSE_SECTION_SPLIT)
    # Compute the response hash.
    out_file.write(self._HASH_START)
    out_file.write(self._HASH_UPDATE % {'var_name': 'response_code_bytes'})
    out_file.write(self._HASH_UPDATE % {'var_name': 'command_code_bytes'})
    out_file.write(self._HASH_UPDATE % {'var_name': 'buffer'})
    # Do authorization related stuff.
    out_file.write(self._AUTHORIZE_RESPONSE)
    # Parse response parameters.
    for arg in parameters:
      out_file.write(self._PARSE_ARG_VAR % {'var_name': arg['name'],
                                            'var_type': arg['type']})
    if parameters and IsTPM2B(parameters[0]['type']):
      out_file.write(self._DECRYPT_PARAMETER % {'var_name':
                                                parameters[0]['name'],
                                                'var_type':
                                                parameters[0]['type']})
    out_file.write(self._RESPONSE_PARSER_END)

  def OutputMethodImplementation(self, out_file):
    """Generates the implementation of a Tpm class method for this command.

    The method assembles a command to be sent unmodified to the TPM and invokes
    the CommandTransceiver with the command. Errors are reported directly to the
    response callback via the error callback (see OutputErrorCallback).

    Args:
      out_file: Generated code is written to this file.
    """
    out_file.write(self._ASYNC_METHOD % {
        'method_name': self._MethodName(),
        'method_args': self._AsyncArgs(),
        'method_arg_names': self._ArgNameList(self._RequestArgs(),
                                              trailing_comma=True)})
    out_file.write(self._SYNC_METHOD % {
        'method_name': self._MethodName(),
        'method_args': self._SyncArgs(),
        'method_arg_names_in': self._ArgNameList(self._RequestArgs(),
                                                 trailing_comma=True),
        'method_arg_names_out': self._ArgNameList(self.response_args,
                                                  trailing_comma=True)})

  def OutputErrorCallback(self, out_file):
    """Generates the implementation of an error callback for this command.

    The error callback simply calls the command response callback with the error
    as the first argument and default values for all other arguments.

    Args:
      out_file: Generated code is written to this file.
    """
    out_file.write(self._ERROR_CALLBACK_START % {'method_name':
                                                 self._MethodName()})
    for arg in self.response_args:
      out_file.write(self._ERROR_CALLBACK_ARG % {'arg_type': arg['type']})
    out_file.write(self._ERROR_CALLBACK_END)

  def OutputResponseCallback(self, out_file):
    """Generates the implementation of a response callback for this command.

    The response callback takes the unmodified response from the TPM, parses it,
    and invokes the original response callback with the parsed response args.
    Errors during parsing or from the TPM are reported directly to the response
    callback via the error callback (see OutputErrorCallback).

    Args:
      out_file: Generated code is written to this file.
    """
    out_file.write(self._RESPONSE_CALLBACK_START % {'method_name':
                                                    self._MethodName()})
    for arg in self.response_args:
      out_file.write(self._DECLARE_ARG_VAR % {'var_type': arg['type'],
                                              'var_name': arg['name']})
    out_file.write(self._RESPONSE_CALLBACK_END % {
        'method_name': self._MethodName(),
        'method_arg_names_in': self._ArgNameList(self.response_args,
                                                 leading_comma=True),
        'method_arg_names_out': self._ArgNameList(self.response_args,
                                                  prefix='&',
                                                  trailing_comma=True)})

  def GetNumberOfRequestHandles(self):
    """Returns the number of input handles for this command."""
    return len(self._SplitArgs(self.request_args)[0])

  def GetNumberOfResponseHandles(self):
    """Returns the number of output handles for this command."""
    return len(self._SplitArgs(self.response_args)[0])

  def _OutputMethodSignatures(self, out_file):
    """Prints method declaration statements for this command.

    This includes a method to serialize a request, a method to parse a response,
    and methods for synchronous and asynchronous calls.

    Args:
      out_file: The output file.
    """
    out_file.write('  static TPM_RC SerializeCommand_%s(%s);\n' % (
        self._MethodName(), self._SerializeArgs()))
    out_file.write('  static TPM_RC ParseResponse_%s(%s);\n' % (
        self._MethodName(), self._ParseArgs()))
    out_file.write('  virtual void %s(%s);\n' % (self._MethodName(),
                                                 self._AsyncArgs()))
    out_file.write('  virtual TPM_RC %sSync(%s);\n' % (self._MethodName(),
                                                       self._SyncArgs()))

  def _OutputCallbackSignature(self, out_file):
    """Prints a callback typedef for this command.

    Args:
      out_file: The output file.
    """
    args = self._InputArgList(self.response_args)
    if args:
      args = ',' + args
    args = '\n      TPM_RC response_code' + args
    out_file.write('  typedef base::Callback<void(%s)> %sResponse;\n' %
                   (args, self._MethodName()))

  def _MethodName(self):
    """Creates an appropriate generated method name for the command.

    We use the command name without the TPM2_ prefix.

    Returns:
      The method name.
    """
    if not self.name.startswith('TPM2_'):
      return self.name
    return self.name[5:]

  def _InputArgList(self, args):
    """Formats a list of input arguments for use in a function declaration.

    Args:
      args: An argument list in the same form as the request_args and
          response_args attributes.

    Returns:
      A string which can be used in a function declaration.
    """
    if args:
      arg_list = ['const %(type)s& %(name)s' % a for a in args]
      return '\n      ' + ',\n      '.join(arg_list)
    return ''

  def _OutputArgList(self, args):
    """Formats a list of output arguments for use in a function declaration.

    Args:
      args: An argument list in the same form as the request_args and
          response_args attributes.

    Returns:
      A string which can be used in a function declaration.
    """
    if args:
      arg_list = ['%(type)s* %(name)s' % a for a in args]
      return '\n      ' + ',\n      '.join(arg_list)
    return ''

  def _ArgNameList(self, args, prefix='', leading_comma=False,
                   trailing_comma=False):
    """Formats a list of arguments for use in a function call statement.

    Args:
      args: An argument list in the same form as the request_args and
          response_args attributes.
      prefix: A prefix to be prepended to each argument.
      leading_comma: Whether to include a comma before the first argument.
      trailing_comma: Whether to include a comma after the last argument.

    Returns:
      A string which can be used in a function call statement.
    """
    if args:
      arg_list = [(prefix + a['name']) for a in args]
      header = ''
      if leading_comma:
        header = ','
      trailer = ''
      if trailing_comma:
        trailer = ','
      return header + '\n      ' + ',\n      '.join(arg_list) + trailer
    return ''

  def _SplitArgs(self, args):
    """Splits a list of args into handles and parameters."""
    handles = []
    parameters = []
    # These commands have handles that are serialized into the parameter
    # section.
    command_handle_parameters = {
        'TPM_CC_FlushContext': 'TPMI_DH_CONTEXT',
        'TPM_CC_Hash': 'TPMI_RH_HIERARCHY',
        'TPM_CC_LoadExternal': 'TPMI_RH_HIERARCHY',
        'TPM_CC_SequenceComplete': 'TPMI_RH_HIERARCHY',
    }
    # Handle type that appears in the handle section.
    always_handle = set(['TPM_HANDLE'])
    # Handle types that always appear as command parameters.
    always_parameter = set(['TPMI_RH_ENABLES', 'TPMI_DH_PERSISTENT'])
    if self.command_code in command_handle_parameters:
      always_parameter.add(command_handle_parameters[self.command_code])
    for arg in args:
      if (arg['type'] in always_handle or
          (self._HANDLE_RE.search(arg['type']) and
           arg['type'] not in always_parameter)):
        handles.append(arg)
      else:
        parameters.append(arg)
    return handles, parameters

  def _RequestArgs(self):
    """Computes the argument list for a Tpm request.

    For every handle argument a handle name argument is added.
    """
    handles, parameters = self._SplitArgs(self.request_args)
    args = []
    # Add a name argument for every handle.  We'll need it to compute cpHash.
    for handle in handles:
      args.append(handle)
      args.append({'type': 'std::string',
                   'name': '%s_name' % handle['name']})
    for parameter in parameters:
      args.append(parameter)
    return args

  def _AsyncArgs(self):
    """Returns a formatted argument list for an asynchronous method."""
    args = self._InputArgList(self._RequestArgs())
    if args:
      args += ','
    return (args + self._DELEGATE_ARG + ',' +
            self._CALLBACK_ARG % {'method_name': self._MethodName()})

  def _SyncArgs(self):
    """Returns a formatted argument list for a synchronous method."""
    request_arg_list = self._InputArgList(self._RequestArgs())
    if request_arg_list:
      request_arg_list += ','
    response_arg_list = self._OutputArgList(self.response_args)
    if response_arg_list:
      response_arg_list += ','
    return request_arg_list + response_arg_list + self._DELEGATE_ARG

  def _SerializeArgs(self):
    """Returns a formatted argument list for a request-serialize method."""
    args = self._InputArgList(self._RequestArgs())
    if args:
      args += ','
    return args + self._SERIALIZE_ARG + ',' + self._DELEGATE_ARG

  def _ParseArgs(self):
    """Returns a formatted argument list for a response-parse method."""
    args = self._OutputArgList(self.response_args)
    if args:
      args = ',' + args
    return self._PARSE_ARG + args + ',' + self._DELEGATE_ARG


class CommandParser(object):
  """Command definition parser.

  The input text file is extracted from the PDF file containing the TPM
  command specification from the Trusted Computing Group. The syntax
  of the text file is defined by extract_commands.sh.
  """

  # Regular expressions to pull relevant bits from annotated lines.
  _INPUT_START_RE = re.compile(r'^_INPUT_START\s+(\w+)$')
  _OUTPUT_START_RE = re.compile(r'^_OUTPUT_START\s+(\w+)$')
  _TYPE_RE = re.compile(r'^_TYPE\s+(\w+)$')
  _NAME_RE = re.compile(r'^_NAME\s+(\w+)$')
  # Pull the command code from a comment like: _COMMENT TPM_CC_Startup {NV}.
  _COMMENT_CC_RE = re.compile(r'^_COMMENT\s+(TPM_CC_\w+).*$')
  _COMMENT_RE = re.compile(r'^_COMMENT\s+(.*)')
  # Args which are handled internally by the generated method.
  _INTERNAL_ARGS = ('tag', 'Tag', 'commandSize', 'commandCode', 'responseSize',
                    'responseCode', 'returnCode')

  def __init__(self, in_file):
    """Initializes a CommandParser instance.

    Args:
      in_file: A file as returned by open() which has been opened for reading.
    """
    self._line = None
    self._in_file = in_file

  def _NextLine(self):
    """Gets the next input line.

    Returns:
      The next input line if another line is available, None otherwise.
    """
    try:
      self._line = self._in_file.next()
    except StopIteration:
      self._line = None

  def Parse(self):
    """Parses everything in a commands file.

    Returns:
      A list of extracted Command objects.
    """
    commands = []
    self._NextLine()
    if self._line != '_BEGIN\n':
      print('Invalid format for first line: %s\n' % self._line)
      return commands
    self._NextLine()

    while self._line != '_END\n':
      cmd = self._ParseCommand()
      if not cmd:
        break
      commands.append(cmd)
    return commands

  def _ParseCommand(self):
    """Parses inputs and outputs for a single TPM command.

    Returns:
      A single Command object.
    """
    match = self._INPUT_START_RE.search(self._line)
    if not match:
      print('Cannot match command input from line: %s\n' % self._line)
      return None
    name = match.group(1)
    cmd = Command(name)
    self._NextLine()
    cmd.request_args = self._ParseCommandArgs(cmd)
    match = self._OUTPUT_START_RE.search(self._line)
    if not match or match.group(1) != name:
      print('Cannot match command output from line: %s\n' % self._line)
      return None
    self._NextLine()
    cmd.response_args = self._ParseCommandArgs(cmd)
    request_var_names = set([arg['name'] for arg in cmd.request_args])
    for arg in cmd.response_args:
      if arg['name'] in request_var_names:
        arg['name'] += '_out'
    if not cmd.command_code:
      print('Command code not found for %s' % name)
      return None
    return cmd

  def _ParseCommandArgs(self, cmd):
    """Parses a set of arguments for a command.

    The arguments may be input or output arguments.

    Args:
      cmd: The current Command object. The command_code attribute will be set if
          such a constant is parsed.

    Returns:
      A list of arguments in the same form as the Command.request_args and
      Command.response_args attributes.
    """
    args = []
    match = self._TYPE_RE.search(self._line)
    while match:
      arg_type = match.group(1)
      self._NextLine()
      match = self._NAME_RE.search(self._line)
      if not match:
        print('Cannot match argument name from line: %s\n' % self._line)
        break
      arg_name = match.group(1)
      self._NextLine()
      match = self._COMMENT_CC_RE.search(self._line)
      if match:
        cmd.command_code = match.group(1)
      match = self._COMMENT_RE.search(self._line)
      if match:
        self._NextLine()
      if arg_name not in self._INTERNAL_ARGS:
        args.append({'type': arg_type,
                     'name': FixName(arg_name)})
      match = self._TYPE_RE.search(self._line)
    return args


def GenerateHandleCountFunctions(commands, out_file):
  """Generates the GetNumberOf*Handles functions given a list of commands.

  Args:
    commands: A list of Command objects.
    out_file: The output file.
  """
  out_file.write(_HANDLE_COUNT_FUNCTION_START % {'handle_type': 'Request'})
  for command in commands:
    out_file.write(_HANDLE_COUNT_FUNCTION_CASE %
                   {'command_code': command.command_code,
                    'handle_count': command.GetNumberOfRequestHandles()})
  out_file.write(_HANDLE_COUNT_FUNCTION_END)
  out_file.write(_HANDLE_COUNT_FUNCTION_START % {'handle_type': 'Response'})
  for command in commands:
    out_file.write(_HANDLE_COUNT_FUNCTION_CASE %
                   {'command_code': command.command_code,
                    'handle_count': command.GetNumberOfResponseHandles()})
  out_file.write(_HANDLE_COUNT_FUNCTION_END)


def GenerateHeader(types, constants, structs, defines, typemap, commands):
  """Generates a header file with declarations for all given generator objects.

  Args:
    types: A list of Typedef objects.
    constants: A list of Constant objects.
    structs: A list of Structure objects.
    defines: A list of Define objects.
    typemap: A dict mapping type names to the corresponding object.
    commands: A list of Command objects.
  """
  out_file = open(_OUTPUT_FILE_H, 'w')
  out_file.write(_COPYRIGHT_HEADER)
  guard_name = 'TRUNKS_%s_' % _OUTPUT_FILE_H.upper().replace('.', '_')
  out_file.write(_HEADER_FILE_GUARD_HEADER % {'name': guard_name})
  out_file.write(_HEADER_FILE_INCLUDES)
  out_file.write(_NAMESPACE_BEGIN)
  out_file.write(_FORWARD_DECLARATIONS)
  out_file.write('\n')
  # These types are built-in or defined by <stdint.h>; they serve as base cases
  # when defining type dependencies.
  defined_types = set(_BASIC_TYPES)
  # Generate defines.  These must be generated before any other code.
  for define in defines:
    define.Output(out_file)
  out_file.write('\n')
  # Generate typedefs.  These are declared before structs because they are not
  # likely to depend on structs and when they do a simple forward declaration
  # for the struct can be generated.  This improves the readability of the
  # generated code.
  for typedef in types:
    typedef.Output(out_file, defined_types, typemap)
  out_file.write('\n')
  # Generate constant definitions.  Again, generated before structs to improve
  # readability.
  for constant in constants:
    constant.Output(out_file, defined_types, typemap)
  out_file.write('\n')
  # Generate structs.  All non-struct dependencies should be already declared.
  for struct in structs:
    struct.Output(out_file, defined_types, typemap)
  # Helper function declarations.
  out_file.write(_FUNCTION_DECLARATIONS)
  # Generate serialize / parse function declarations.
  for basic_type in _BASIC_TYPES:
    out_file.write(_SERIALIZE_DECLARATION % {'type': basic_type})
  for typedef in types:
    out_file.write(_SERIALIZE_DECLARATION % {'type': typedef.new_type})
  for struct in structs:
    out_file.write(_SERIALIZE_DECLARATION % {'type': struct.name})
    if struct.IsSimpleTPM2B():
      out_file.write(_SIMPLE_TPM2B_HELPERS_DECLARATION % {'type': struct.name})
    elif struct.IsComplexTPM2B():
      out_file.write(_COMPLEX_TPM2B_HELPERS_DECLARATION % {
          'type': struct.name,
          'inner_type': struct.fields[1][0]})
  # Generate a declaration for a 'Tpm' class, which includes one method for
  # every TPM 2.0 command.
  out_file.write(_CLASS_BEGIN)
  for command in commands:
    command.OutputDeclarations(out_file)
  out_file.write(_CLASS_END)
  out_file.write(_NAMESPACE_END)
  out_file.write(_HEADER_FILE_GUARD_FOOTER % {'name': guard_name})
  out_file.close()


def GenerateImplementation(types, structs, typemap, commands):
  """Generates implementation code for each command.

  Args:
    types: A list of Typedef objects.
    structs: A list of Structure objects.
    typemap: A dict mapping type names to the corresponding object.
    commands: A list of Command objects.
  """
  out_file = open(_OUTPUT_FILE_CC, 'w')
  out_file.write(_COPYRIGHT_HEADER)
  out_file.write(_LOCAL_INCLUDE % {'filename': _OUTPUT_FILE_H})
  out_file.write(_IMPLEMENTATION_FILE_INCLUDES)
  out_file.write(_NAMESPACE_BEGIN)
  GenerateHandleCountFunctions(commands, out_file)
  serialized_types = set(_BASIC_TYPES)
  for basic_type in _BASIC_TYPES:
    out_file.write(_SERIALIZE_BASIC_TYPE % {'type': basic_type})
  for typedef in types:
    typedef.OutputSerialize(out_file, serialized_types, typemap)
  for struct in structs:
    struct.OutputSerialize(out_file, serialized_types, typemap)
  for command in commands:
    command.OutputSerializeFunction(out_file)
    command.OutputParseFunction(out_file)
    command.OutputErrorCallback(out_file)
    command.OutputResponseCallback(out_file)
    command.OutputMethodImplementation(out_file)
  out_file.write(_NAMESPACE_END)
  out_file.close()


def main():
  """A main function.

  Both a TPM structures file and commands file are parsed and C++ header and C++
  implementation file are generated.

  Positional Args:
    structures_file: The extracted TPM structures file.
    commands_file: The extracted TPM commands file.
  """
  parser = argparse.ArgumentParser(description='TPM 2.0 code generator')
  parser.add_argument('structures_file')
  parser.add_argument('commands_file')
  args = parser.parse_args()
  structure_parser = StructureParser(open(args.structures_file))
  types, constants, structs, defines, typemap = structure_parser.Parse()
  command_parser = CommandParser(open(args.commands_file))
  commands = command_parser.Parse()
  GenerateHeader(types, constants, structs, defines, typemap, commands)
  GenerateImplementation(types, structs, typemap, commands)
  print('Processed %d commands.' % len(commands))


if __name__ == '__main__':
  main()
