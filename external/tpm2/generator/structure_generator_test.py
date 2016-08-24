#!/usr/bin/python2

# Copyright 2015 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Unit tests for the libtpm2 structure_generator."""

from __future__ import print_function

import StringIO
import os
import unittest

import extract_structures
import structure_generator


class TestGenerators(unittest.TestCase):
  """Test structure_generator classes."""

  def testTypedefMarshal(self):
    """Test generation of marshaling code for typedefs."""
    marshalled_types = set(['int'])
    typedef = structure_generator.Typedef('int', 'INT')
    typedef2 = structure_generator.Typedef('INT', 'INT2')
    typemap = {'INT': typedef}
    out_file = StringIO.StringIO()
    typedef2.OutputMarshalImpl(out_file, marshalled_types, typemap)
    self.assertIn('INT', marshalled_types)
    self.assertIn('INT2', marshalled_types)
    out_file.close()

  def testConstantTypeMarshal(self):
    """Test generation of marshaling code for constant types."""
    marshalled_types = set(['int'])
    typedef = structure_generator.Typedef('int', 'UINT16')
    constant = structure_generator.ConstantType('UINT16', 'TPM_TYPE')
    constant.valid_values.append('VALUE0')
    constant.valid_values.append('VALUE1')
    typemap = {'UINT16': typedef}
    out_file = StringIO.StringIO()
    constant.OutputMarshalImpl(out_file, marshalled_types, typemap)
    self.assertIn('UINT16', marshalled_types)
    self.assertIn('TPM_TYPE', marshalled_types)
    out_file.close()

  def testAttributeStructureMarshal(self):
    """Test generation of marshaling code for attribute structures."""
    marshalled_types = set(['int'])
    typedef = structure_generator.Typedef('int', 'UINT16')
    attributeStruct = structure_generator.AttributeStructure(
        'UINT16', 'TPM_TYPE')
    attributeStruct.reserved.append('4_7')
    attributeStruct.reserved.append('1')
    typemap = {'UINT16': typedef}
    out_file = StringIO.StringIO()
    attributeStruct.OutputMarshalImpl(out_file, marshalled_types, typemap)
    self.assertIn('UINT16', marshalled_types)
    self.assertIn('TPM_TYPE', marshalled_types)
    out_file.close()

  def testInterfacemarshal(self):
    """test generation of marshaling code for interfaces."""
    marshalled_types = set(['int'])
    typedef = structure_generator.Typedef('int', 'UINT16')
    interface = structure_generator.Interface('UINT16', 'TPM_TYPE')
    interface.conditional = 'TPM_VALUE_NULL'
    interface.bounds.append(('TPM_MIN', 'TPM_MAX'))
    interface.valid_values.append('VALUE0')
    interface.valid_values.append('VALUE1')
    typemap = {'UINT16': typedef}
    out_file = StringIO.StringIO()
    interface.OutputMarshalImpl(out_file, marshalled_types, typemap)
    self.assertIn('UINT16', marshalled_types)
    self.assertIn('TPM_TYPE', marshalled_types)
    out_file.close()

  def testStructMarshal(self):
    """Test generation of marshaling code for structures."""
    marshalled_types = set(['int'])
    struct = structure_generator.Structure('TEST_STRUCT')
    struct.AddField(structure_generator.Field('UINT16', 'type', None, False))
    struct.AddField(structure_generator.Field('TPMI_TYPE', 'interfaceField0',
                                              'TRUE', False))
    struct.AddField(structure_generator.Field('TPMI_TYPE', 'interfaceField1',
                                              'FALSE', False))
    struct.AddField(structure_generator.Field('TPMU_SYM_MODE', 'unionField',
                                              'type', False))
    struct.AddField(structure_generator.Field('UINT16', 'arrayField',
                                              'MAX_VALUE', True))
    typedef = structure_generator.Typedef('int', 'UINT16')
    interface = structure_generator.Interface('UINT16', 'TPMI_TYPE')
    # Choose TPMU_SYM_MODE because it exists in selectors definition and it
    # has few fields.
    union = structure_generator.Union('TPMU_SYM_MODE')
    union.AddField(structure_generator.Field('UINT16', 'aes', None))
    union.AddField(structure_generator.Field('UINT16', 'SM4', None))
    typemap = {
        'UINT16': typedef,
        'TPMI_TYPE': interface,
        'TPMU_SYM_MODE': union
    }
    out_file = StringIO.StringIO()
    struct.OutputMarshalImpl(out_file, marshalled_types, typemap)
    self.assertIn('UINT16', marshalled_types)
    self.assertIn('TPMI_TYPE', marshalled_types)
    self.assertIn('TPMU_SYM_MODE', marshalled_types)
    self.assertIn('TEST_STRUCT', marshalled_types)
    out_file.close()

  def testUnionMarshal(self):
    """Test generation of marshaling code for unions."""
    marshalled_types = set(['int'])
    union = structure_generator.Union('TPMU_SYM_MODE')
    union.AddField(structure_generator.Field('UINT16', 'aes', None))
    union.AddField(structure_generator.Field('UINT16', 'SM4', None))
    typedef = structure_generator.Typedef('int', 'UINT16')
    typemap = {'UINT16': typedef}
    out_file = StringIO.StringIO()
    union.OutputMarshalImpl(out_file, marshalled_types, typemap)
    self.assertIn('UINT16', marshalled_types)
    self.assertIn('TPMU_SYM_MODE', marshalled_types)
    out_file.close()

  def _MakeArg(self, arg_type, arg_name):
    return {'type': arg_type,
            'name': arg_name,
            'command_code': None,
            'description': None}


class TestParser(unittest.TestCase):
  """Test structure parser."""

  def testStructureParser(self):
    """Test the structure parser with valid data.

       One of each typedef, constant type, attribute structure, interface,
       structure, and union. Should appear in types array in that order.
    """
    html_parser = extract_structures.SpecParser()
    html_file_name = os.path.join(os.path.dirname(__file__),
                                  'test_structure_generator.html')
    html_parser.feed(open(html_file_name).read())
    html_parser.close()
    types = html_parser.GetTable().GetTypeMap()
    self.assertEqual(len(types), 6)
    tpm_obj = types['UINT16']
    self.assertEqual(tpm_obj.old_type, 'uint16_t')
    self.assertEqual(tpm_obj.new_type, 'UINT16')
    tpm_obj = types['TPMA_LOCALITY']
    self.assertEqual(tpm_obj.old_type, 'base_type')
    self.assertEqual(tpm_obj.new_type, 'TPMA_LOCALITY')
    self.assertEqual(tpm_obj.reserved[0], '4_7')
    self.assertEqual(tpm_obj.reserved[1], '9')
    tpm_obj = types['const_type']
    self.assertEqual(tpm_obj.old_type, 'base_type')
    self.assertEqual(tpm_obj.new_type, 'const_type')
    self.assertEqual(tpm_obj.valid_values[0], 'const_name')
    self.assertEqual(tpm_obj.error_code, 'return_name')
    tpm_obj = types['TPMI_DH_OBJECT']
    self.assertEqual(tpm_obj.old_type, 'base_type')
    self.assertEqual(tpm_obj.new_type, 'TPMI_DH_OBJECT')
    self.assertEqual(tpm_obj.bounds[0][0], 'min_name')
    self.assertEqual(tpm_obj.bounds[0][1], 'max_name')
    self.assertEqual(tpm_obj.valid_values[0], 'const_name')
    self.assertEqual(tpm_obj.conditional_value, 'null_name')
    self.assertEqual(tpm_obj.error_code, 'return_name')
    tpm_obj = types['struct_type']
    self.assertEqual(tpm_obj.name, 'struct_type')
    self.assertEqual(tpm_obj.fields[0].field_type, 'UINT16')
    self.assertEqual(tpm_obj.fields[0].field_name, 'field1')
    self.assertEqual(tpm_obj.fields[1].field_type, 'UINT16')
    self.assertEqual(tpm_obj.fields[1].field_name, 'field2')
    self.assertEqual(tpm_obj.fields[2].field_type, 'UINT16')
    self.assertEqual(tpm_obj.fields[2].field_name, 'field3')
    self.assertEqual(tpm_obj.fields[2].run_time_size, 'field1')
    self.assertEqual(tpm_obj.fields[3].field_type, 'UINT16')
    self.assertEqual(tpm_obj.fields[3].field_name, 'field4')
    self.assertEqual(tpm_obj.fields[3].selector_value, 'field2')
    self.assertEqual(tpm_obj.fields[4].field_type, 'interface_type')
    self.assertEqual(tpm_obj.fields[4].field_name, 'field5')
    self.assertEqual(tpm_obj.upper_bounds['field1'], 'max')
    self.assertEqual(tpm_obj.lower_bounds['field1'], 'min')
    tpm_obj = types['union_type']
    self.assertEqual(tpm_obj.name, 'union_type')
    self.assertEqual(tpm_obj.fields[0].field_type, 'field1_type')
    self.assertEqual(tpm_obj.fields[0].field_name, 'field1')
    self.assertEqual(tpm_obj.fields[1].field_type, 'field2_type')
    self.assertEqual(tpm_obj.fields[1].field_name, 'field2')

if __name__ == '__main__':
  unittest.main()
