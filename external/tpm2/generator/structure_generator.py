# Copyright 2015 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""A code generator for TPM 2.0 structures.

The structure generator provides classes to create various objects
(structures, unions, constants, etc.) and then convert the set of generated
objects into valid C files named tpm_generated.{h,c}.

"""

from __future__ import print_function

import datetime
import re

from subprocess import call

_BASIC_TYPES = ['uint8_t', 'int8_t', 'uint16_t', 'int16_t', 'uint32_t',
                'int32_t', 'uint64_t', 'int64_t']
_OUTPUT_FILE_H = 'tpm_generated.h'
_OUTPUT_FILE_CC = 'tpm_generated.c'
COPYRIGHT_HEADER = (
    '// Copyright %d The Chromium OS Authors. All rights reserved.\n'
    '// Use of this source code is governed by a BSD-style license that can '
    'be\n'
    '// found in the LICENSE file.\n'
    '\n'
    '// THIS CODE IS GENERATED - DO NOT MODIFY!\n' %
    datetime.datetime.now().year)
HEADER_FILE_GUARD_HEADER = """
#ifndef %(name)s
#define %(name)s
"""
HEADER_FILE_GUARD_FOOTER = """
#endif  // %(name)s
"""
_HEADER_FILE_INCLUDES = """
#include <endian.h>
#include <string.h>

#include "TPM_Types.h"
#include "Tpm.h"
"""
_IMPLEMENTATION_FILE_INCLUDES = """
#include "tpm_generated.h"
"""
# Function signatures for generated marshaling code are specified in TCG TPM2.0
# Library Specification, Part 4: Supporting Routines, sections 4.2.2 and 4.2.3.
_MARSHAL_BASIC_TYPE = """
UINT16 %(type)s_Marshal(%(type)s *source, BYTE **buffer, INT32 *size) {
  %(type)s value_net = *source;
  if (!size || *size < sizeof(%(type)s)) {
    return 0;  // Nothing has been marshaled.
  }
  switch (sizeof(%(type)s)) {
    case 2:
      value_net = htobe16(*source);
      break;
    case 4:
      value_net = htobe32(*source);
      break;
    case 8:
      value_net = htobe64(*source);
      break;
    default:
      break;
  }
  memcpy(*buffer, &value_net, sizeof(%(type)s));
  *buffer += sizeof(%(type)s);
  *size -= sizeof(%(type)s);
  return sizeof(%(type)s);
}

TPM_RC %(type)s_Unmarshal(%(type)s *target, BYTE **buffer, INT32 *size) {
  %(type)s value_net = 0;
  if (!size || *size < sizeof(%(type)s)) {
    return TPM_RC_INSUFFICIENT;
  }
  memcpy(&value_net, *buffer, sizeof(%(type)s));
  switch (sizeof(%(type)s)) {
    case 2:
      *target = be16toh(value_net);
      break;
    case 4:
      *target = be32toh(value_net);
      break;
    case 8:
      *target = be64toh(value_net);
      break;
    default:
      *target = value_net;
  }
  *buffer += sizeof(%(type)s);
  *size -= sizeof(%(type)s);
  return TPM_RC_SUCCESS;
}
"""
_STANDARD_MARSHAL_DECLARATION = """
UINT16 %(type)s_Marshal(
    %(type)s *source,
    BYTE **buffer,
    INT32 *size);

TPM_RC %(type)s_Unmarshal(
    %(type)s *target,
    BYTE **buffer,
    INT32 *size);
"""


def _IsTPM2B(name):
  return name.startswith('TPM2B_')


class Field(object):
  """Represents a field in TPM structure or union.

  This object is used in several not fully overlapping cases, not all
  attributes apply to all use cases.

  The 'array_size' and 'run_time_size' attributes below are related to the
  following code example:

  struct {
    int size;
    byte array[MAX_SIZE]
  } object.

  In this structure the actual number of bytes in the array could be anything
  from zero to MAX_SIZE. The field 'size' denotes the actual number of
  elements at run time. So, when this object is constructed, array_size is
  'MAX_SIZE' and run_time_size is 'size'.

  The 'selector_value' attribute is used to associate union fields with
  certain object types. For instance

  typedef union {
    TPM2B_PUBLIC_KEY_RSA  rsa;
    TPMS_ECC_POINT        ecc;
  } TPMU_PUBLIC_ID;

  the field named 'rsa' will have its 'selector_value' set to 'TPM_ALG_RSA'.

  Attributes:
    field_type: a string, the type of field.
    field_name: a string, the name of the field.
    array_size: a string, see example above
    run_time_size: a string, see example above
    selector_value: a string, see example above
    conditional_value: a string, necessary for validation when unmarshaling.
                       Some types have a value that is allowed for some
                       commands but not others. E.g. 'TPM_RS_PW' is a
                       conditional value for the 'TPMI_SH_AUTH_SESSION' type
                       and TPM_ALG_NULL is a conditional value for the
                       TPMI_ALG_HASH type.
  """
  _MARSHAL_FIELD_ARRAY = """
  for (i = 0; i < source->%(array_length)s; ++i) {
    total_size += %(type)s_Marshal(&source->%(name)s[i], buffer, size);
  }"""
  _UNMARSHAL_FIELD_ARRAY = """
  for (i = 0; i < target->%(array_length)s; ++i) {
    result = %(type)s_Unmarshal(&target->%(name)s[i], buffer, size);
    if (result != TPM_RC_SUCCESS) {
      return result;
    }
  }"""

  def __init__(self, field_type, field_name,
               selector=None, array_size='',
               conditional_value='FALSE',
               run_time_size=None):
    """Initializes a Field instance.

    Args:
      field_type: Initial value for the field type attribute.
      field_name: Initial value for the field name attribute.
      selector: Initial value for the selector attribute.
      array_size: Initial value for the array_size attribute.
      conditional_value: Initial value of the conditional_value attribute.
      run_time_size: Initial value of the run_time_size attribute
    """
    if not field_type:
      # Some tables include rows without data type, for instance 'Table 70 -
      # Definition of TPMU_HA Union' in part 2. These rows are supposed to
      # cause another case added to the switch in the marshaling function
      # (processing of TPM_ALG_NULL in this example). Setting field name to ''
      # makes sure that the actual generated structure/union does not have an
      # entry for this field, setting type of such field to some value
      # simplifies functions generating the marshaling code.
      self.field_type = 'BYTE'
      self.field_name = ''
    else:
      self.field_type = field_type
      self.field_name = field_name
    self.array_size = array_size
    self.selector_value = selector
    self.conditional_value = conditional_value
    self.run_time_size = run_time_size

  def OutputMarshal(self, out_file, typemap):
    """Write a call to marshal the field this instance represents.

    Args:
      out_file: The output file.
      typemap: A dict mapping type names to the corresponding object.
    """
    if self.array_size:
      if self.run_time_size:
        real_size = self.run_time_size
      else:
        real_size = self.array_size
      out_file.write(
          self._MARSHAL_FIELD_ARRAY % {'type': self.field_type,
                                       'name': self.field_name,
                                       'array_length': real_size})
    else:
      typemap[self.field_type].OutputMarshalCall(out_file, self)

  def OutputUnmarshal(self, out_file, typemap):
    """Write a call to unmarshal the field this instance represents.

    Args:
      out_file: The output file.
      typemap: A dict mapping type names to the corresponding object.
    """
    if self.array_size:
      if self.run_time_size:
        real_size = self.run_time_size
      else:
        real_size = self.array_size
      out_file.write(
          self._UNMARSHAL_FIELD_ARRAY % {'type': self.field_type,
                                         'name': self.field_name,
                                         'array_length': real_size})
    else:
      typemap[self.field_type].OutputUnmarshalCall(out_file, self)


class TPMType(object):
  """Base type for all TPMTypes.

     Contains functions and string literals common to all TPM types.

     Attributes:
      _base_type: a string, when set - the very basic type this type is
                  derived from (should be used for marshaling/unmarshaling to
                  shortcut multiple nested invocations).
  """
  # A function to marshal a TPM typedef.
  _TYPEDEF_MARSHAL_FUNCTION = """
UINT16 %(new_type)s_Marshal(
    %(new_type)s *source,
    BYTE **buffer,
    INT32 *size) {
  return %(old_type)s_Marshal(source, buffer, size);
}
"""
  # The function signature and unmarshaling call to the base type of a TPM
  # typedef. After the value is unmarshaled, additional validation code is
  # generated based on tables in TCG TPM2.0 Library Specification, Part 2:
  # Structures.
  _TYPEDEF_UNMARSHAL_START = """
TPM_RC %(new_type)s_Unmarshal(
    %(new_type)s *target,
    BYTE **buffer,
    INT32 *size) {
  TPM_RC result;
  result = %(old_type)s_Unmarshal(target, buffer, size);
  if (result != TPM_RC_SUCCESS) {
    return result;
  }"""
  _UNMARSHAL_END = '\n  return TPM_RC_SUCCESS;\n}\n'
  # Snippets of code for value validation.
  _VALUE_START_SWITCH = '\n  switch (%(name)s) {'
  _VALUE_CASE = '\n    case %(value)s:'
  _VALUE_CASE_IFDEF = '\n#ifdef %(value)s\n    case %(value)s:\n#endif'
  _VALUE_END_SWITCH = """
      break;
    default:
      return %(error_code)s;
  }"""
  # A declaration for marshaling and unmarshaling functions for a TPM type.
  _MARSHAL_DECLARATION = _STANDARD_MARSHAL_DECLARATION
  # Snippets of code which make calls to marshaling functions. Marshals a value
  # of type 'type' into a field 'name' within a structure. This is used in
  # generation of structure and command marshaling code.
  _MARSHAL_CALL = """
  total_size += %(type)s_Marshal(
      &source->%(name)s, buffer, size);"""
  _UNMARSHAL_CALL = """
  result = %(type)s_Unmarshal(
      &target->%(name)s, buffer, size);
  if (result != TPM_RC_SUCCESS) {
    return result;
  }"""

  def __init__(self):
    self._base_type = None

  def SetBaseType(self, base_type):
    self._base_type = base_type

  def _GetBaseType(self, out_file, marshalled_types, typemap):
    '''Return base type for this object.

    The base type is used for shortcutting marshaling/unmarshaling code.

    If _base_type is not set, return the old_type value as the base type.

    If the base type's marshaling/unmarshaling code has not been generated
    yet, issue it before continuing processing.

    Args:
      out_file: The output file.
      marshalled_types: A set of types for which marshal and unmarshal functions
          have already been generated.
      typemap: A dict mapping type names to the corresponding object.

    Returns:
      A string, the name of the type to use for marshaling/unmarshaling.

    '''
    if self._base_type:
      base_type = self._base_type
    else:
      base_type = self.old_type
    if base_type not in marshalled_types:
      typemap[base_type].OutputMarshalImpl(
        out_file, marshalled_types, typemap)
    return base_type

  def HasConditional(self):
    """Returns true if TPMType has a conditional value."""
    return False

  def OutputMarshalCall(self, out_file, field):
    """Write a call to Marshal function for TPMType to |out_file|.

       Accumulates a variable 'total_size' with the result of marshaling
       field |field_name| in structure 'source'.

    Args:
      out_file: The output file.
      field: A Field object describing this type.
    """
    out_file.write(self._MARSHAL_CALL % {'type': field.field_type,
                                         'name': field.field_name})

  def OutputUnmarshalCall(self, out_file, field):
    """Write a call to Unmarshal function for TPMType to |out_file|.

       Assigns result of unmarshaling field |field_name| in structure 'source'
       to variable 'result'. Returns if the unmarshalling was unsuccessful.

    Args:
      out_file: The output file.
      field: A Field object describing this type.
    """
    obj_type = field.field_type
    if obj_type == 'TPM_CC':
      obj_type = 'UINT32'
    out_file.write(self._UNMARSHAL_CALL % {'type': obj_type,
                                           'name': field.field_name})

  def _OutputTypedefMarshalDecl(self, out_file, declared_types, typemap):
    """Write marshal declarations for TPM typedefs to |out_file|.

       Can only be called on Typedef, ConstantType, AttributeStruct, and
       Interface objects.

    Args:
      out_file: The output file.
      declared_types: A set of types for which marshal and unmarshal function
          declarations have already been generated.
      typemap: A dict mapping type names to the corresponding object.
    """
    if self.new_type in declared_types:
      return
    if self.old_type not in declared_types and self.old_type in typemap:
      typemap[self.old_type].OutputMarshalDecl(
          out_file, declared_types, typemap)
    out_file.write(self._MARSHAL_DECLARATION % {'type': self.new_type})
    declared_types.add(self.new_type)

  def _OutputStructOrUnionMarshalDecl(self, out_file, declared_types):
    """Write marshal declarations for a TPM Structure or Union.

       Can only be called on Structure and Union objects.

    Args:
      out_file: The output file.
      declared_types: A set of types for which marshal and unmarshal function
        declarations have already been generated.
    """
    # TPMU_NAME and TPMU_ENCRYPTED_SECRET type are never used across the
    # interface.
    if (self.name in declared_types or
        self.name == 'TPMU_NAME' or
        self.name == 'TPMU_ENCRYPTED_SECRET'):
      return
    out_file.write(self._MARSHAL_DECLARATION % {'type': self.name})
    declared_types.add(self.name)


class Typedef(TPMType):
  """Represents a TPM typedef.

  Attributes:
    old_type: The existing type in a typedef statement.
    new_type: The new type in a typedef statement.
  """
  # A function to unmarshal a TPM typedef with no extra validation.
  _TYPEDEF_UNMARSHAL_FUNCTION = """
TPM_RC %(new_type)s_Unmarshal(
    %(new_type)s *target,
    BYTE **buffer,
    INT32 *size) {
  return %(old_type)s_Unmarshal(target, buffer, size);
}
"""

  def __init__(self, old_type, new_type):
    """Initializes a Typedef instance.

    Args:
      old_type: The base type of the attribute structure.
      new_type: The name of the type.
    """
    super(Typedef, self).__init__()
    self.old_type = old_type
    self.new_type = new_type

  def OutputMarshalImpl(self, out_file, marshalled_types, typemap):
    """Writes marshal implementations for Typedef to |out_file|.

    Args:
      out_file: The output file.
      marshalled_types: A set of types for which marshal and unmarshal functions
          have already been generated.
      typemap: A dict mapping type names to the corresponding object.
    """
    if self.new_type in marshalled_types:
      return
    base_type = self._GetBaseType(out_file, marshalled_types, typemap)
    out_file.write(self._TYPEDEF_MARSHAL_FUNCTION % {'old_type': base_type,
                                                     'new_type': self.new_type})
    out_file.write(
        self._TYPEDEF_UNMARSHAL_FUNCTION % {'old_type': base_type,
                                            'new_type': self.new_type})
    marshalled_types.add(self.new_type)

  def OutputMarshalDecl(self, out_file, declared_types, typemap):
    """Writes marshal declarations for Typedef to |out_file|.

    Args:
      out_file: The output file.
      declared_types: A set of types for which marshal and unmarshal function
          declarations have already been generated.
      typemap: A dict mapping type names to the corresponding object.
    """
    self._OutputTypedefMarshalDecl(out_file, declared_types, typemap)


class ConstantType(TPMType):
  """Represents a TPM Constant type definition.

  Attributes:
     old_type: The base type of the constant (e.g. 'int').
     new_type: The name of the type (e.g. 'TPM_RC').
     valid_values: The list of valid values this type can take (e.g.
         'TPM_RC_SUCCESS').
     error_code: Error to be returned when unmarshalling is unsuccessful.
  """
  _CHECK_VALUE = """
  if (*target == %(value)s) {
    return TPM_RC_SUCCESS;
  }"""
  _CHECK_VALUE_IFDEF = """
#ifdef %(value)s
  if (*target == %(value)s) {
    return TPM_RC_SUCCESS;
  }
#endif"""
  _UNMARSHAL_END = """
  return %(error_code)s;
}
"""
  _IFDEF_TYPE_RE = re.compile(r'^TPM_(ALG|CC).*')

  def __init__(self, old_type, new_type):
    """Initializes a ConstantType instance.

    Values are added to valid_values attribute during parsing.

    Args:
      old_type: The base type of the constant type.
      new_type: The name of the type.
    """
    super(ConstantType, self).__init__()
    self.old_type = old_type
    self.new_type = new_type
    self.valid_values = []
    self.error_code = 'TPM_RC_VALUE'

  def _NeedsIfdef(self):
    """Returns True if new_type is a type which needs ifdef enclosing."""
    return self._IFDEF_TYPE_RE.search(self.new_type)

  def OutputMarshalImpl(self, out_file, marshalled_types, typemap):
    """Writes marshal implementations for ConstantType to |out_file|.

    Args:
      out_file: The output file.
      marshalled_types: A set of types for which marshal and unmarshal functions
          have already been generated.
      typemap: A dict mapping type names to the corresponding object.
    """
    if self.new_type in marshalled_types:
      return
    base_type = self._GetBaseType(out_file, marshalled_types, typemap)
    out_file.write(self._TYPEDEF_MARSHAL_FUNCTION % {'old_type': base_type,
                                                     'new_type': self.new_type})
    out_file.write(self._TYPEDEF_UNMARSHAL_START % {'old_type': base_type,
                                                    'new_type': self.new_type})
    for value in self.valid_values:
      if self._NeedsIfdef():
        out_file.write(self._CHECK_VALUE_IFDEF % {'value': value})
      else:
        out_file.write(self._CHECK_VALUE % {'value': value})
    out_file.write(self._UNMARSHAL_END % {'error_code': self.error_code})
    marshalled_types.add(self.new_type)

  def OutputMarshalDecl(self, out_file, declared_types, typemap):
    """Writes marshal declarations for ConstantType to |out_file|.

    Args:
      out_file: The output file.
      declared_types: A set of types for which marshal and unmarshal function
          declarations have already been generated.
      typemap: A dict mapping type names to the corresponding object.
    """
    self._OutputTypedefMarshalDecl(out_file, declared_types, typemap)


class AttributeStructure(TPMType):
  """Represents a TPM attribute structure type definition.

  Attributes:
     old_type: The base type of the constant (e.g. 'int').
     new_type: The name of the type (e.g. 'TPMA_OBJECT').
     reserved: The list of bit bounds where bits must be 0 (e.g. ['10_2','3']).
  """
  # Attribute structures need an explicit cast to the base type.
  _ATTRIBUTE_MARSHAL_FUNCTION = """
UINT16 %(new_type)s_Marshal(
    %(new_type)s *source,
    BYTE **buffer,
    INT32 *size) {
  return %(old_type)s_Marshal((%(old_type)s*)source, buffer, size);
}
"""
  _ATTRIBUTE_UNMARSHAL_START = """
TPM_RC %(new_type)s_Unmarshal(
    %(new_type)s *target,
    BYTE **buffer,
    INT32 *size) {
  TPM_RC result;
  result = %(old_type)s_Unmarshal((%(old_type)s*)target, buffer, size);
  if (result != TPM_RC_SUCCESS) {
    return result;
  }"""
  _CHECK_RESERVED = """
  if (target->reserved%(bits)s != 0) {
    return TPM_RC_RESERVED_BITS;
  }"""

  def __init__(self, old_type, new_type):
    """Initializes an AttributeStructure instance.

    Values may be added to reserved attribute during parsing.

    Args:
      old_type: The base type of the attribute structure.
      new_type: The name of the type.
    """
    super(AttributeStructure, self).__init__()
    self.old_type = old_type
    self.new_type = new_type
    self.reserved = []

  def OutputMarshalImpl(self, out_file, marshalled_types, typemap):
    """Writes marshal implementations for AttributStructure to |out_file|.

    Args:
      out_file: The output file.
      marshalled_types: A set of types for which marshal and unmarshal functions
          have already been generated.
      typemap: A dict mapping type names to the corresponding object.
    """
    if self.new_type in marshalled_types:
      return
    base_type = self._GetBaseType(out_file, marshalled_types, typemap)
    out_file.write(self._ATTRIBUTE_MARSHAL_FUNCTION %
                   {'old_type': base_type,
                    'new_type': self.new_type})
    out_file.write(self._ATTRIBUTE_UNMARSHAL_START %
                   {'old_type': base_type,
                    'new_type': self.new_type})
    for bits in self.reserved:
      out_file.write(self._CHECK_RESERVED % {'bits': bits})
    out_file.write(self._UNMARSHAL_END)
    marshalled_types.add(self.new_type)

  def OutputMarshalDecl(self, out_file, declared_types, typemap):
    """Writes marshal declarations for AttributeStructure to |out_file|.

    Args:
      out_file: The output file.
      declared_types: A set of types for which marshal and unmarshal function
          declarations have already been generated.
      typemap: A dict mapping type names to the corresponding object.
    """
    self._OutputTypedefMarshalDecl(out_file, declared_types, typemap)


class Interface(TPMType):
  """Represents a TPM interface type definition.

  Attributes:
     old_type: The base type of the interface (e.g. 'TPM_HANDLE').
     new_type: The name of the type (e.g. 'TPMI_DH_OBJECT').
     valid_values: List of valid values for new_type. If this is not empty,
         valid values for new_type is explicitly defined in the spec.
     bounds: List of pairs representing bounds. If nonempty, target must fall
         between one of these bounds.
     conditional_value: Name of conditionally allowed value. If there is no
         such value, this variable will be None.
     supported_values: String literal indicating the name of a list of supported
         values to be substituted at compile time (e.g. 'AES_KEY_SIZES_BITS').
         If this is not None, valid values for new_type depends on the
         implementation.
     error_code: Return code when an unmarshalling error occurs.
  """
  _INTERFACE_CONDITIONAL_UNMARSHAL_START = """
TPM_RC %(new_type)s_Unmarshal(
    %(new_type)s *target,
    BYTE **buffer,
    INT32 *size,
    BOOL allow_conditional_value) {
  TPM_RC result;"""
  _INTERFACE_UNMARSHAL_START = """
TPM_RC %(new_type)s_Unmarshal(
    %(new_type)s *target,
    BYTE **buffer,
    INT32 *size) {
  TPM_RC result;"""
  _UNMARSHAL_VALUE = """
  result = %(old_type)s_Unmarshal(target, buffer, size);
  if (result != TPM_RC_SUCCESS) {
    return result;
  }"""
  _UNMARSHAL_VALUE_ALLOW_RC_VALUE = """
  result = %(old_type)s_Unmarshal(target, buffer, size);
  if ((result != TPM_RC_VALUE) && (result != TPM_RC_SUCCESS)) {
    return result;
  }"""
  _SETUP_CHECK_SUPPORTED_VALUES = """
  uint16_t supported_values[] = %(supported_values)s;
  size_t length = sizeof(supported_values)/sizeof(supported_values[0]);
  size_t i;
  BOOL is_supported_value = FALSE;"""
  _CHECK_SUPPORTED_VALUES = """
  for (i = 0; i < length; ++i) {
    if (*target == supported_values[i]) {
      is_supported_value = TRUE;
      break;
    }
  }
  if (!is_supported_value) {
    return %(error_code)s;
  }"""
  _CHECK_CONDITIONAL = """
  if (*target == %(name)s) {
    return allow_conditional_value ? TPM_RC_SUCCESS : %(error_code)s;
  }"""
  _SETUP_CHECK_VALUES = '\n  BOOL has_valid_value = FALSE;'
  _VALUE_END_SWITCH = """
      has_valid_value = TRUE;
      break;
  }"""
  _CHECK_BOUND = """
  if((*target >= %(lower)s) && (*target <= %(upper)s)) {
    has_valid_value = TRUE;
  }"""
  _CHECK_VALUES_END = """
  if (!has_valid_value) {
    return %(error_code)s;
  }"""
  _CONDITIONAL_MARSHAL_DECLARATION = """
UINT16 %(type)s_Marshal(
    %(type)s *source,
    BYTE **buffer,
    INT32 *size);

TPM_RC %(type)s_Unmarshal(
    %(type)s *target,
    BYTE **buffer,
    INT32 *size,
    BOOL allow_conditioanl_value);
"""
  _CONDITIONAL_UNMARSHAL_CALL = """
  result = %(type)s_Unmarshal(
      &target->%(name)s, buffer, size, %(flag)s);
  if (result != TPM_RC_SUCCESS) {
    return result;
  }"""
  _IFDEF_TYPE_RE = re.compile(r'^TPMI_(ALG|ECC)_.*')

  def __init__(self, old_type, new_type):
    """Initializes an Interface instance.

    Values may be added/assigned to valid_values, bounds, conditional_value,
    supported_values, and error_code attributes new values during parsing.

    Args:
      old_type: The base type of the interface.
      new_type: The name of the type.
    """
    super(Interface, self).__init__()
    self.old_type = old_type
    self.new_type = new_type
    self.valid_values = []
    self.bounds = []
    self.conditional_value = None
    self.supported_values = None
    self.error_code = 'TPM_RC_VALUE'

  def HasConditional(self):
    """Returns true if Interface has a valid conditional_value."""
    return self.conditional_value is not None

  def _NeedsIfdef(self):
    """Returns True if new_type is a type which needs ifdef enclosing."""
    return self._IFDEF_TYPE_RE.search(self.new_type)

  def OutputMarshalImpl(self, out_file, marshalled_types, typemap):
    """Writes marshal implementation for Interface to |out_file|.

    Args:
      out_file: The output file.
      marshalled_types: A set of types for which marshal and unmarshal functions
          have already been generated.
      typemap: A dict mapping type names to the corresponding object.
    """
    if self.new_type in marshalled_types:
      return
    base_type = self._GetBaseType(out_file, marshalled_types, typemap)
    out_file.write(self._TYPEDEF_MARSHAL_FUNCTION % {'old_type': base_type,
                                                     'new_type': self.new_type})
    if self.conditional_value:
      out_file.write(self._INTERFACE_CONDITIONAL_UNMARSHAL_START %
                     {'old_type': base_type,
                      'new_type': self.new_type})
    else:
      out_file.write(
          self._INTERFACE_UNMARSHAL_START % {'old_type': base_type,
                                             'new_type': self.new_type})
    # Creating necessary local variables.
    if self.supported_values:
      out_file.write(self._SETUP_CHECK_SUPPORTED_VALUES %
                     {'supported_values': self.supported_values})
    if len(self.valid_values)+len(self.bounds) > 0:
      out_file.write(self._SETUP_CHECK_VALUES)
      out_file.write(self._UNMARSHAL_VALUE_ALLOW_RC_VALUE %
                     {'old_type': base_type})
    else:
      out_file.write(self._UNMARSHAL_VALUE % {'old_type': base_type})

    if self.supported_values:
      out_file.write(self._CHECK_SUPPORTED_VALUES %
                     {'supported_values': self.supported_values,
                      'error_code': self.error_code})
    if self.conditional_value:
      out_file.write(
          self._CHECK_CONDITIONAL % {'name': self.conditional_value,
                                     'error_code': self.error_code})
    # Checking for valid values.
    if len(self.valid_values)+len(self.bounds) > 0:
      if self.valid_values:
        out_file.write(self._VALUE_START_SWITCH % {'name': '*target'})
        for value in self.valid_values:
          if self._NeedsIfdef():
            out_file.write(self._VALUE_CASE_IFDEF % {'value': value})
          else:
            out_file.write(self._VALUE_CASE % {'value': value})
        out_file.write(self._VALUE_END_SWITCH)
      for (lower, upper) in self.bounds:
        out_file.write(
            self._CHECK_BOUND % {'lower': lower, 'upper': upper})
      out_file.write(self._CHECK_VALUES_END % {'error_code': self.error_code})

    out_file.write(self._UNMARSHAL_END)
    marshalled_types.add(self.new_type)

  def OutputMarshalDecl(self, out_file, declared_types, typemap):
    """Writes marshal declarations for Interface to |out_file|.

       Outputted declaration depends on whether Interface type has a
       conditionally valid value.

    Args:
      out_file: The output file.
      declared_types: A set of types for which marshal and unmarshal function
          declarations have already been generated.
      typemap: A dict mapping type names to the corresponding object.
    """
    if self.new_type in declared_types:
      return
    if self.old_type not in declared_types:
      typemap[self.old_type].OutputMarshalDecl(
          out_file, declared_types, typemap)
    if self.HasConditional():
      out_file.write(
          self._CONDITIONAL_MARSHAL_DECLARATION % {'type': self.new_type})
    else:
      out_file.write(self._MARSHAL_DECLARATION % {'type': self.new_type})
    declared_types.add(self.new_type)

  def OutputUnmarshalCall(
      self, out_file, field):
    """Write a call to Unmarshal function for Interface type to |out_file|.

       Override TPMType OutputUnmarshalCall because when an Interface type has
       a conditionally valid value, a BOOL value (|conditional_valid|) is passed
       as a parameter.

    Args:
      out_file: The output file.
      field: A Field object representing an element of this interface.
    """
    if self.conditional_value:
      out_file.write(
          self._CONDITIONAL_UNMARSHAL_CALL % {'type': field.field_type,
                                              'name': field.field_name,
                                              'flag': field.conditional_value})
    else:
      out_file.write(self._UNMARSHAL_CALL % {'type': field.field_type,
                                             'name': field.field_name})


class Structure(TPMType):
  """Represents a TPM structure.

  Attributes:
    name: The name of the structure.
    fields: A list of Field objects representing struct fields.
    upper_bounds: A dictionary of (name, val) tuples mapping name to max val.
    lower_bounds: A dictionary of (name, val) tuples mapping name to min val.
    size_check: Set if TPM2B structure must be size checked (triggered by size=)
    valid_tag_values: A list of values field tag is allowed to take.
    error_code: The return code to be returned if an error occurs
  """
  _STRUCTURE_MARSHAL_START = """
UINT16 %(name)s_Marshal(
    %(name)s *source,
    BYTE **buffer,
    INT32 *size) {
  UINT16 total_size = 0;"""
  _STRUCTURE_UNMARSHAL_START = """
TPM_RC %(name)s_Unmarshal(
    %(name)s *target,
    BYTE **buffer,
    INT32 *size) {
  TPM_RC result;"""
  _MARSHAL_END = '\n  return total_size;\n}\n'
  _SETUP_ARRAY_FIELD = '\n  INT32 i;'
  _CHECK_SIZE_START = """
  UINT32 start_size = *size;
  UINT32 struct_size;"""
  _CHECK_SIZE_END = """
  struct_size = start_size - *size - sizeof(target->t.size);
  if (struct_size != target->t.size) {
    return TPM_RC_SIZE;
  }"""
  _TPM2B_ZERO_SIZE = """
  if (target->t.size == 0) {
    return %(return_value)s;
  }"""
  _CHECK_BOUND = """
  if (target->%(name)s %(operator)s %(bound_value)s) {
    return %(error_code)s;
  }"""
  _FIX_SIZE_FIELD = """
  {
    BYTE *size_location = *buffer - total_size;
    INT32 size_field_size = sizeof(%(size_field_type)s);
    UINT16 payload_size = total_size - (UINT16)size_field_size;
    %(size_field_type)s_Marshal(&payload_size,
      &size_location, &size_field_size);
  }"""

  def __init__(self, name):
    """Initializes a Structure instance.

    Initially the instance will have no fields, upper_bounds, lower_bounds, or
    valid_tag_values. Those can be added with AddField(), AddUpperBound(),
    AddLowerBound(), and AddTagVal() methods.

    Args:
      name: The name of the structure.
    """
    super(Structure, self).__init__()
    self.name = name
    self.fields = []
    self.upper_bounds = {}
    self.lower_bounds = {}
    self.size_check = False
    self.valid_tag_values = []
    self.error_code = 'TPM_RC_VALUE'

  def AddField(self, field):
    """Adds a field to fields attribute in Structure.

    Args:
      field: Instance of Field
    """
    self.fields.append(field)

  def AddUpperBound(self, field_name, value):
    """Adds an upper bound for a field.

    Args:
       field_name: Name of field with bound.
       value: Value of upper bound.
    """
    if _IsTPM2B(self.name):
      field_name = 't.' + field_name
    self.upper_bounds[field_name] = value

  def AddLowerBound(self, field_name, value):
    """Adds a lower bound for a field.

    Args:
       field_name: Name of field with bound.
       value: Value of lower bound.
    """
    if _IsTPM2B(self.name):
      field_name = 't.' + field_name
    self.lower_bounds[field_name] = value

  def _AddTagValue(self, value):
    """Adds a valid value for tag field.

    Args:
       value: Valid value for tag field.
    """
    self.valid_tag_values.append(value)

  def _GetFieldTypes(self):
    """Creates a set which holds all current field types.

    Returns:
      A set of field types.
    """
    return set([field.field_type for field in self.fields])

  def OutputMarshalImpl(self, out_file, marshalled_types, typemap):
    """Writes marshal implementations for Structure to |out_file|.

    Args:
      out_file: The output file.
      marshalled_types: A set of types for which marshal and unmarshal functions
          have already been generated.
      typemap: A dict mapping type names to the corresponding object.
    """
    if self.name in marshalled_types:
      return

    # Make sure any dependencies already have marshal functions defined.
    for field_type in self._GetFieldTypes():
      if field_type not in marshalled_types:
        typemap[field_type].OutputMarshalImpl(
            out_file, marshalled_types, typemap)
        marshalled_types.add(field_type)

    out_file.write(self._STRUCTURE_MARSHAL_START % {'name': self.name})
    # If any field is an array, create local variable INT32 i.
    for field in self.fields:
      if field.array_size:
        out_file.write(self._SETUP_ARRAY_FIELD)
        break
    for field in self.fields:
      # Each TPM2B is a union of two sized buffers, one which is type specific
      # (the 't' element) and the other is a generic value (the 'b' element).
      # For this reason a 't.' is prepended for fields in a TPM2B type. See
      # section 9.11.6 in TCG TPM2.0 Library Specification, Part 2: Structures
      # for more details.
      if _IsTPM2B(self.name):
        field.field_name = 't.' + field.field_name
        if field.run_time_size:
          field.run_time_size = 't.' + field.run_time_size
      field.OutputMarshal(out_file, typemap)
    if self.size_check:
      out_file.write(self._FIX_SIZE_FIELD % {'size_field_type': self.fields[0].field_type})
    out_file.write(self._MARSHAL_END)

    out_file.write(self._STRUCTURE_UNMARSHAL_START % {'name': self.name})
    if self.size_check:
      out_file.write(self._CHECK_SIZE_START)
    # If any field is an array, create local variable INT32 i.
    for field in self.fields:
      if field.array_size:
        out_file.write(self._SETUP_ARRAY_FIELD)
        break
    for field in self.fields:
      field.OutputUnmarshal(out_file, typemap)
      return_value = self.error_code
      if field.field_name == 't.size' and self.size_check:
        out_file.write(self._TPM2B_ZERO_SIZE % {'return_value': 'TPM_RC_SIZE'})
      if field.field_name == 't.size' and not self.size_check:
        out_file.write(
            self._TPM2B_ZERO_SIZE % {'return_value': 'TPM_RC_SUCCESS'})
      if field.field_name in self.upper_bounds:
        if (field.field_name == 'count' or
            field.field_name == 't.size' or
            field.field_name == 'size'):
          return_value = 'TPM_RC_SIZE'
        out_file.write(self._CHECK_BOUND %
                       {'name': field.field_name,
                        'operator': '>',
                        'bound_value': self.upper_bounds[field.field_name],
                        'error_code': return_value})
      if field.field_name in self.lower_bounds:
        if (field.field_name == 'count' or
            field.field_name == 't.size' or
            field.field_name == 'size'):
          return_value = 'TPM_RC_SIZE'
        out_file.write(self._CHECK_BOUND %
                       {'name': field.field_name,
                        'operator': '<',
                        'bound_value': self.lower_bounds[field.field_name],
                        'error_code': return_value})
      if field.field_name == 'tag' and self.valid_tag_values:
        out_file.write(self._VALUE_START_SWITCH % {'name': 'target->tag'})
        for value in self.valid_tag_values:
          out_file.write(self._VALUE_CASE % {'value': value})
        out_file.write(self._VALUE_END_SWITCH % {'error_code': 'TPM_RC_TAG'})
    if self.size_check:
      out_file.write(self._CHECK_SIZE_END)
    if not self.fields:
      # The spec includes a definition of an empty structure, as a side effect
      # the marshaling/unmarshaling functions become empty, the compiler
      # warning is suppressed by the below statement.
      out_file.write('  (void)result;\n')
    out_file.write(self._UNMARSHAL_END)

    marshalled_types.add(self.name)

  def OutputMarshalDecl(self, out_file, declared_types, _):
    """Writes marshal declarations for Structure to |out_file|.

    Args:
      out_file: The output file.
      declared_types: A set of types for which marshal and unmarshal function
          declarations have already been generated.
    """
    self._OutputStructOrUnionMarshalDecl(out_file, declared_types)


class Union(TPMType):
  """Represents a TPM union.

  Attributes:
    name: The name of the union.
    fields: A list of Field objects representing union fields.
  """

  _UNION_MARSHAL_START = """
UINT16 %(name)s_Marshal(
    %(name)s *source,
    BYTE **buffer,
    INT32 *size,
    UINT32 selector) {
  %(array_extras)s
  switch(selector) {"""
  _UNION_UNMARSHAL_START = """
TPM_RC %(name)s_Unmarshal(
    %(name)s *target,
    BYTE **buffer,
    INT32 *size,
    UINT32 selector) {
  switch(selector) {"""
  _MARSHAL_END = '\n  }\n  return 0;\n}\n'
  _UNMARSHAL_END = '\n  }\n  return TPM_RC_SELECTOR;\n}\n'
  _MARSHAL_DECLARATION = """
UINT16 %(type)s_Marshal(
    %(type)s *source,
    BYTE **buffer,
    INT32 *size,
    UINT32 selector);

TPM_RC %(type)s_Unmarshal(
    %(type)s *target,
    BYTE **buffer,
    INT32 *size,
    UINT32 selector);
"""
  _CASE_SELECTOR = """
    case %(selector)s:"""
  _MARSHAL_EMPTY = """
      return 0;"""
  _UNMARSHAL_EMPTY = """
      return TPM_RC_SUCCESS;"""
  _MARSHAL_FIELD = """
      return %(type)s_Marshal(
          (%(type)s*)&source->%(name)s, buffer, size);"""
  _UNMARSHAL_FIELD = """
      return %(type)s_Unmarshal(
          (%(type)s*)&target->%(name)s, buffer, size);"""
  _SETUP_MARSHAL_FIELD_ARRAY = """
    INT32 i;
    UINT16 total_size = 0;"""
  _SETUP_UNMARSHAL_FIELD_ARRAY = """
    INT32 i;
    TPM_RC result = TPM_RC_SUCCESS;"""
  _MARSHAL_FIELD_ARRAY = """
    for (i = 0; i < %(array_length)s; ++i) {
      total_size += %(type)s_Marshal(
          &source->%(name)s[i], buffer, size);
    }
    return total_size;"""
  _UNMARSHAL_FIELD_ARRAY = """
    for (i = 0; i < %(array_length)s; ++i) {
      result = %(type)s_Unmarshal(
          &target->%(name)s[i], buffer, size);
      if (result != TPM_RC_SUCCESS) {
        return result;
      }
    }
    return TPM_RC_SUCCESS;"""
  _UNMARSHAL_FIELD_CONDITIONAL = """
    return %(type)s_Unmarshal(
        &target->%(name)s, buffer, size, FALSE);"""
  _UNION_MARSHAL_CALL = """
  total_size += %(type)s_Marshal(
      &source->%(name)s, buffer, size, source->%(selector)s);"""
  _UNION_UNMARSHAL_CALL = """
  result = %(type)s_Unmarshal(
      &target->%(name)s, buffer, size, target->%(selector)s);
  if (result != TPM_RC_SUCCESS) {
    return result;
  }"""
  _IFDEF = '\n#ifdef %(type)s'
  _ENDIF = '\n#endif'
  _IFDEF_TYPE_RE = re.compile(r'^TPM_(ALG|CC).*')

  def __init__(self, name):
    """Initializes a Union instance.

    Initially the instance will have no fields. Fields are added with the
    AddField() method.

    Args:
      name: The name of the structure.
    """
    super(Union, self).__init__()
    self.name = name
    self.fields = []

  def _NeedsIfdef(self, selector):
    """Returns True if selector is a type which needs ifdef enclosing."""
    return self._IFDEF_TYPE_RE.search(selector)

  def AddField(self, field):
    """Adds a field to fields attribute in Union.

    Args:
      field: instance of Field
    """
    # xor is a C++ keyword and must be fixed.
    if field.field_name == 'xor':
      field.field_name = 'xor_'
    self.fields.append(field)

  def _OutputMarshalField(
      self, out_file, field_type, field_name, array_length):
    """Write a call to marshal a field in this union.

    Args:
      out_file: The output file.
      field_type: The type of field.
      field_name: The name of the field.
      array_length: Variable indicating length of array, None if field is not
          an array.
    """
    if array_length:
      out_file.write(self._MARSHAL_FIELD_ARRAY % {'type': field_type,
                                                  'name': field_name,
                                                  'array_length': array_length})
    else:
      out_file.write(self._MARSHAL_FIELD % {'type': field_type,
                                            'name': field_name})

  def _OutputUnmarshalField(
      self, out_file, field_type, field_name, array_length, typemap):
    """Write a call to unmarshal a field in this union.

    Args:
      out_file: The output file object.
      field_type: The type of field.
      field_name: The name of the field.
      array_length: Variable indicating length of array, None if field is not
          an array.
      typemap: A dict mapping type names to the corresponding object.
    """
    if array_length:
      out_file.write(
          self._UNMARSHAL_FIELD_ARRAY % {'type': field_type,
                                         'name': field_name,
                                         'array_length': array_length})
    elif typemap[field_type].HasConditional():
      out_file.write(
          self._UNMARSHAL_FIELD_CONDITIONAL % {'type': field_type,
                                               'name': field_name})
    else:
      out_file.write(self._UNMARSHAL_FIELD % {'type': field_type,
                                              'name': field_name})

  def OutputMarshalImpl(self, out_file, marshalled_types, typemap):
    """Writes marshal implementations for Union to |out_file|.

    Args:
      out_file: The output file.
      marshalled_types: A set of types for which marshal and unmarshal functions
          have already been generated.
      typemap: A dict mapping type names to the corresponding object.
    """
    if (self.name in marshalled_types or
        self.name == 'TPMU_NAME' or
        self.name == 'TPMU_ENCRYPTED_SECRET' or
        not self.fields):
      return

    field_types = {f.field_name: f.field_type for f in self.fields}
    array_lengths = {}
    for f in self.fields:
      if f.array_size:
        array_lengths[f.field_name] = f.array_size
      else:
        array_lengths[f.field_name] = None

    # Make sure any dependencies already have marshal functions defined.
    for field_type in field_types.itervalues():
      if field_type not in marshalled_types:
        typemap[field_type].OutputMarshalImpl(
            out_file, marshalled_types, typemap)
        marshalled_types.add(field_type)
    if self.fields[0].array_size:
      array_extras = self._SETUP_MARSHAL_FIELD_ARRAY
    else:
      array_extras = ''
    out_file.write(self._UNION_MARSHAL_START % {'name': self.name,
                                                'array_extras': array_extras})
    # Set up variables if Union is an array type.
    for field in self.fields:
      selector = field.selector_value
      if not selector:
        continue
      field_name = field.field_name
      if self._NeedsIfdef(selector):
        out_file.write(self._IFDEF % {'type': selector})
      out_file.write(self._CASE_SELECTOR % {'selector': selector})
      # Selector is not associated with a name, so no marshaling occurs.
      if not field_name:
        out_file.write(self._MARSHAL_EMPTY)
        if self._NeedsIfdef(selector):
          out_file.write(self._ENDIF)
        continue
      field_type = field_types[field_name]
      array_length = array_lengths[field_name]
      self._OutputMarshalField(out_file, field_type, field_name, array_length)
      if self._NeedsIfdef(selector):
        out_file.write(self._ENDIF)
    out_file.write(self._MARSHAL_END)
    out_file.write(self._UNION_UNMARSHAL_START % {'name': self.name})
    # Set up variables if Union is an array type.
    if self.fields[0].array_size:
      out_file.write(self._SETUP_UNMARSHAL_FIELD_ARRAY)
    for field in self.fields:
      selector = field.selector_value
      if not selector:
        continue
      field_name = field.field_name
      if self._NeedsIfdef(selector):
        out_file.write(self._IFDEF % {'type': selector})
      out_file.write(self._CASE_SELECTOR % {'selector': selector})
      # Selector is not associated with a name, so no unmarshaling occurs.
      if not field_name:
        out_file.write(self._UNMARSHAL_EMPTY)
        if self._NeedsIfdef(selector):
          out_file.write(self._ENDIF)
        continue
      field_type = field_types[field_name]
      array_length = array_lengths[field_name]
      self._OutputUnmarshalField(
          out_file, field_type, field_name, array_length, typemap)
      if self._NeedsIfdef(selector):
        out_file.write(self._ENDIF)
    out_file.write(self._UNMARSHAL_END)
    marshalled_types.add(self.name)

  def OutputMarshalDecl(self, out_file, declared_types, _):
    """Writes marshal declarations for Union to |out_file|.

    Args:
      out_file: The output file.
      declared_types: A set of types for which marshal and unmarshal function
          declarations have already been generated.
    """
    self._OutputStructOrUnionMarshalDecl(out_file, declared_types)

  def OutputMarshalCall(self, out_file, field):
    """Write a call to marshal function for Union type to |out_file|.

       Override TPMType OutputMarshalCall to pass in selector value.

    Args:
      out_file: The output file.
      field: A Field object representing a member of this union
    """
    out_file.write(self._UNION_MARSHAL_CALL %
                   {'type': field.field_type,
                    'name': field.field_name,
                    'selector': field.selector_value})

  def OutputUnmarshalCall(self, out_file, field):
    """Write a call to unmarshal function for Union type to |out_file|.

       Override TPMType OutputUnmashalCall to pass in selector value.

    Args:
      out_file: The output file.
      field: A Field object representing a member of this union
    """
    out_file.write(self._UNION_UNMARSHAL_CALL %
                   {'type': field.field_type,
                    'name': field.field_name,
                    'selector': field.selector_value})


def GenerateHeader(typemap):
  """Generates a header file with declarations for all given generator objects.

  Args:
    typemap: A dict mapping type names to the corresponding object.
  """
  out_file = open(_OUTPUT_FILE_H, 'w')
  out_file.write(COPYRIGHT_HEADER)
  guard_name = 'TPM2_%s_' % _OUTPUT_FILE_H.upper().replace('.', '_')
  out_file.write(HEADER_FILE_GUARD_HEADER % {'name': guard_name})
  out_file.write(_HEADER_FILE_INCLUDES)
  # These types are built-in or defined by <stdint.h>; they serve as base cases
  # when defining type dependencies.
  declared_types = set(_BASIC_TYPES)
  # Generate serialize / parse function declarations.
  for basic_type in _BASIC_TYPES:
    out_file.write(_STANDARD_MARSHAL_DECLARATION % {'type': basic_type})
  for tpm_type in [typemap[x] for x in sorted(typemap.keys())]:
    tpm_type.OutputMarshalDecl(out_file, declared_types, typemap)
  out_file.write(HEADER_FILE_GUARD_FOOTER % {'name': guard_name})
  out_file.close()
  call(['clang-format', '-i', '-style=Chromium', 'tpm_generated.h'])


def GenerateImplementation(typemap):
  """Generates implementation code for each type.

  Args:
    typemap: A dict mapping string type names to the corresponding object.
  """
  out_file = open(_OUTPUT_FILE_CC, 'w')
  out_file.write(COPYRIGHT_HEADER)
  out_file.write(_IMPLEMENTATION_FILE_INCLUDES)
  marshalled_types = set(_BASIC_TYPES)
  for basic_type in _BASIC_TYPES:
    out_file.write(_MARSHAL_BASIC_TYPE % {'type': basic_type})
  for tpm_type in [typemap[x] for x in sorted(typemap.keys())]:
    tpm_type.OutputMarshalImpl(out_file, marshalled_types, typemap)
  out_file.close()
  call(['clang-format', '-i', '-style=Chromium', 'tpm_generated.c'])
