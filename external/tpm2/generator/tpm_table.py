"""Module for processing TCG TPM2 library object descriptions.

The descriptions are scraped from the tables of parts 2 and 3 of the
specification by a different module and fed through this module for
processing.
"""

from __future__ import print_function

import re
import sys

from command_generator import Command
from structure_generator import AttributeStructure
from structure_generator import ConstantType
from structure_generator import Field
from structure_generator import Interface
from structure_generator import Structure
from structure_generator import Typedef
from structure_generator import Union


def _DebugLog(*args, **kwargs):
  """When used - sends its inputs to stderr.

  This function can be used when debugging this module. Its footprint is
  similar to print(), but the default destination is sys.stderr, which is
  handy when the script generates stdio output redirected into a file.

  Args:
    *args: a list of items of various types to print. Printed space separated,
           each one converted to str before being printed.
    **kwargs: a dictionary of variables to pass to print(), if any. In fact the
              only key this function cares about is 'endl', which allows to
              suppress adding a newline to the printed string.
  """
  endl = kwargs.get('endl', '\n')
  print(' '.join(str(x) for x in args), end=endl, file=sys.stderr)


class Table(object):
  """Representation of TCG TPM2 library specification tables.

  The purpose of this class is to both generate new TPM2 objects and to keep
  track of the previously generated objects for post processing (generating C
  code).

  The HTML scraper finds tables in the specifications and builds up the
  tables' contents in this object, one at a time. This object's table
  representation includes table title, table header and one or more then table
  rows.

  The table title must start with 'Table ### xxx', where ### is monotonously
  increasing table number and xxx is some description allowing to deduce the
  type of the object defined by this table.

  The cells of the table include names and types of the components, various
  decorations are used to convey additional information: array boundaries,
  values' limits, return values, selectors, etc, etc.

  Once the entire table is scraped, the scraper invokes a method to process it
  (ProcessTable). The title of the table is examined by this module and the
  appropriate processing method is invoked to actually convert the internal
  table representation into a TPM2 object.

  Two maps are maintained persistently over the life time of this object, the
  map of types (keyed by the type name scraped from part 2) and map of
  commands (keyed by the command name, scraped from part 3).

  One other thing this module produces is the text for the .h file defining
  all structures and types this module encountered.

  Attributes:

    _alg_id_table: actual table of various TPM2 algorithms, a copy of Table 9
                   from part 2. It is used to convert encoded algorithm specs
                   used in other tables into a list of matching algorithms.
    _h_file: a multiline string, the accumulated .h file defining all TPM
                   objects processed so far.
    _type_map: a dictionary of various TPM types, keyed by the string - the
                   type name
    _command_map: a dictionary of command_generator.Command objects, keyed by
                   the string, the command name
    skip_tables: a tuple of integers, the numbers of tables which should not
                   be included in the .h file, as the same information was
                   derived from part 4 earlier.
    _title: a string, title of the currently being processed specification
                   table
    _title_type: a string, base type of the object defined by the currently
                   being processed specification table
    _alg_type: a string, in some tables included in the title in curly
                   brackets, to indicate what type of the algorithm this table
                   deals with (usually RSA or ECC)
    _body: a list of strings, rows of the currently being processed
                   specification table
    _has_selector_column: a Boolean, set to True if the third column of the
                   table is the selector to be used to process this row (as in
                   picking the object type when the table represents a union)
    _skip_printing: a Boolean, set to True if the table contents should not be
                   included on tpm_types.h - some tables are also defined in
                   files extracted from Part 4 of the specification.

  """

  # Match table titles with processing routines.
  TABLE_ROUTER = (
      (re.compile('(Constants|Defines for Logic Values)'), '_ProcessConstants'),
      (re.compile('(of Types for|Base Types)'), '_ProcessTypes'),
      (re.compile('Definition of .* Type'), '_ProcessInterfaceOrType'),
      (re.compile('Unmarshaling Errors'), '_ProcessEnum'),
      (re.compile(r'Definition of [\S]+ (Structure|Union)'),
       '_ProcessStructureOrUnion'),
      (re.compile('Definition of .* Bits'), '_ProcessBits'),
      (re.compile(r' TPM2_\S+ (Command|Response)'), '_ProcessCommand'),
  )


  # The TCG spec in some cases uses so called 'Algorithm macros' to describe
  # all algorithms a type should apply to. The macros are described in details
  # in section 4.12 of part 2 of the spec.
  #
  # Basically, the macro consists of the prefix '!ALG' followed by dot
  # separated descriptions of algorithm types this marco applies to.
  #
  # The algorithm types are expressed as sequences or lower or upper case
  # letters, and should match the third column of Table 9 either inclusively
  # (in case the type letters are in upper case, or exclusively, in case the
  # type letters are in lower case.
  _alg_macro = re.compile(r'!ALG\.([a-z\.]+)', re.IGNORECASE)

  def __init__(self):
    """Create a Table class instance."""
    self._alg_id_table = []
    # Allow re-initializing attributes outside __init__() (in Init())
    self.Init()
    self._h_file = ''
    self._type_map = {}
    self._command_map = {}
    self.skip_tables = ()

  def Init(self, title=''):
    """Initialize a new table.

    This function is invoked each time a new table is encountered in the spec.

    A typical table header could look like this:

    'Table 10 - Definition of (UINT16) {ECC} TPM_ECC_CURVE Constants'

    The algorithm type in curly brackets, if present, is redundant, it is
    stripped off before the table header comment is generated for the .h file.

    Some titles include the parenthesized base type the defined object should
    be typedef'ed from.

    Args:
      title: a string, the title of the table as included in the TCG spec.
    """
    title_bracket_filter = re.compile(r'({.*?}) ?')
    title_type_filter = re.compile(r'(\(.*?\)) ?')
    # Retrieve base type, if present in the table title.
    m = title_type_filter.search(title)
    if m:
      # the header shown in the docstring above would result in the match of
      # '(UINT16)', remove the parenthesis and save the base type.
      self._title_type = m.groups()[0][1:-1]
      self._title = title_type_filter.sub('', title).strip()
    else:
      self._title_type = ''
      self._title = title.strip()
    # Now retrieve algorithm type, if present in the table title.
    m = title_bracket_filter.search(self._title)
    self._alg_type = ''
    if m:
      self._title = title_bracket_filter.sub('', self._title).strip()
      alg_type = m.groups()[0][1:-1].strip()
      if not alg_type.startswith('!'):
        self._alg_type = alg_type
    self._body = []
    self._has_selector_column = False
    self._skip_printing = False

  def _SplitByAlg(self, word):
    """Split the passed in word by the regex used to pick TPM algorithms.

    The TPM algorithm regex is used all over the spec in situations where
    similar code needs to be generated for different algorithms of a certain
    type.

    A string in the spec might look like one of the following:
    TPMI_!ALG.S_KEY_BITS or !ALG.S_KEY_SIZES_BITS.

    The split would result in a three element list: the part before !ALG
    (could be empty), the letters between '!ALG.' and _ or end of the string,
    and the part after the letters included in the algorithm regex.

    TPMI_!ALG.S_KEY_BITS => ['TPMI_', 'S', '_KEY_BITS']
    !ALG.S_KEY_SIZES_BITS => ['', 'S', '_KEY_SIZES_BITS']

    The first and last elements of the split are used as the prefix and suffix
    of the type names included in the generated file.

    In some cases there is no regex suffix present, only the !ALG string, as
    in the selector column in table 127 (TPM_ALG_!ALG) In this case the split
    by the !ALG string is attempted, and the generated list has just two
    elements.

    In yet some other cases, say in Table 126 where the type field does not
    change at all set to TPMI_ALG_SYM_MODE for all fields. In such cases the
    split returns a single element list, the second element set to None is
    added to the list.

    Args:
      word: a string, the encoded algorithm string to be split.

    Returns:
      a tuple of two strings, first and last elements of the split, either one
      could be empty.

    """
    parts = self._alg_macro.split(word)
    if len(parts) == 1:
      parts = word.split('!ALG')
      if len(parts) == 1:
        return word, None
    return parts[0].strip('_'), parts[-1].strip('_')

  def SetSkipTables(self, skip_tables):
    """Set the tuple of table numbers to be ignored by the parser."""
    self.skip_tables = skip_tables

  def _AddToHfile(self, text=''):
    self._h_file += text + '\n'

  def _SetBaseType(self, old_type, tpm_obj):
    """Set the base type for a new object.

    Many TPM objects are typedefed hierarchically, for instance

    uint16_t => UINT16 => TPM_ALG_ID_Marshal => TPMI_ALG_HASH_Marshal

    This causes excessive nesting when marshaling and unmarshaling, which is
    bad from both performance and stack size requirements point of view.

    This function will discover the 'basest' type and set it in the tpm
    object, this would help to generate direct marshaling/unmarshaling
    functions.

    Args:
      old_type: a string, name of the immediate type this tpm object typedefed
                from.
      tpm_obj: a tpm object, derived from TPMType
    """
    base_type = old_type
    while base_type in self._type_map:
      try:
        base_type = self._type_map[base_type].old_type
      except AttributeError:
        break  # The underlying type is not a typedef
    tpm_obj.SetBaseType(base_type)

  def _AddTypedef(self, old_type, new_type):
    if not self._skip_printing:
      self._AddToHfile('typedef %s %s;' % (old_type, new_type))
    # No need to generate marshaling/unmarshaling code for BOOL type.
    if new_type != 'BOOL':
      self._type_map[new_type] = Typedef(old_type, new_type)
      self._SetBaseType(old_type, self._type_map[new_type])

  def InProgress(self):
    """Return True when the parser is in the middle of a table."""
    return self._title

  def _GetMaxLengths(self, table):
    """Find out maximum lengths of the table columns.

    This function helps to generate nicely aligned definitions in the output
    .h file, by making sure that each field's name starts in the same column,
    far enough for all fields' types to fit.

    Args:
      table: a list of string lists. Each component consists of at least two
             elements, the first one the field or constant type, the
             second one the field name or constant value.

    Returns:
      a tuple of two integers, the first one - the length of the longest
              string in the first colume, the second one - the length of the
              longest string in the second column.
    """
    lengths = [0, 0]
    for row in table:
      for i in range(len(lengths)):
        if len(row[i]) > lengths[i]:
          lengths[i] = len(row[i])
    return tuple(lengths)

  def NewRow(self):
    """Start a new row in the internal table representation."""
    self._body.append([])

  def NewCell(self):
    """Start a new cell in the last row."""
    self._body[-1].append('')

  def AddData(self, data):
    """Add data to the last cell of the last row."""
    if not self._body:
      return  # Ignore end of line and whitespace formatting.
    self._body[-1][-1] += data

  def ProcessTable(self):
    """Process accumulated table contents.

    This function is invoked when the parser state machine detects that the
    entire HTML table has been processed. The received contents is handled
    based on the table title by finding the matching entry in TABLE_ROUTER
    tuple.

    The result of processing is adding a new TPM type to the _type_map
    dictionary, or a new command descriptor to the _command_map dictionary.
    """

    # The table has a selector column if it has at least three columns, and
    # the third column is titled 'Selector'.
    self._has_selector_column = (len(self._body[0]) >= 3 and
                                 self._body[0][2].strip() == 'Selector')
    # Preprocess representation of the table scraped from the spec. Namely,
    # remove the header row, and strip all other cells before adding them to
    # self._body[], which becomes a list including all scraped table cells,
    # stripped.
    self._body = [[cell.strip() for cell in row] for row in self._body[1:]]
    if 'TPM_ALG_ID Constants' in self._title:
      self._alg_id_table = [[x[0], x[2].replace(' ', ''), x[3]]
                            for x in self._body]

    # The name of the type defined in the table, when present, is always the
    # fifth element in the stripped header, for instance:
    # 'Table 10 - Definition of TPM_ECC_CURVE Constants'
    try:
      type_name = self._title.split()[4]
    except IndexError:
      type_name = ''

    # Based on the table title, find the function to process the table and
    # generate a TPM specification object of a certain type.
    table_func = ''
    for regex, func in self.TABLE_ROUTER:
      if regex.search(self._title):
        table_func = func
        break
    else:
      self._AddToHfile('// Unprocessed: %s' % self._title)
      return

    if int(self._title.split()[1]) in self.skip_tables:
      self._skip_printing = True
      self._AddToHfile('// Skipped: %s' % self._title)
    else:
      self._AddToHfile('// %s' % self._title)

    # Invoke a TPM type specific processing function.
    getattr(self, table_func)(type_name)

  def _ProcessCommand(self, _):
    """Process command description table from part 3.

    Each TPM command has two tables associated with it, one describing the
    request structure, and another one describing the response structure. The
    first time a TPM command is encountered, a Command object is created and
    its 'request_args' property is set, the second time it is encountered -
    the existing object's 'response_args' property is set.
    """
    command_name = self._title.split()[2]
    if command_name not in self._command_map:
      command = Command(command_name)
      self._command_map[command_name] = command
    else:
      command = self._command_map[command_name]
    params = []
    # The first three fields in each request and response are always the same
    # and are not included in the generated structures. Let's iterate over the
    # rest of the fields.
    for row in self._body[3:]:
      # A dictionary describing a request or response structure field.
      field = {}
      # Ignore the '@' decoration for now.
      field_type, field['name'] = row[0], row[1].lstrip('@')
      # The '+' decoration means this field can be conditional.
      if field_type.endswith('+'):
        field_type = field_type[:-1]
        field['has_conditional'] = 'TRUE'
      else:
        field['has_conditional'] = 'FALSE'
      field['type'] = field_type
      if len(row) > 2:
        field['description'] = row[2]
      else:
        field['description'] = ''
      # Add the new field to the list of request or response fields.
      params.append(field)
    if ' Command' in self._title:
      command.request_args = params
    else:
      command.response_args = params

  def _PickAlgEntries(self, alg_type_str):
    """Process algorithm id table and find all matching entries.

    See comments to _alg_macro above.

    Args:
     alg_type_str: a string, one or more dot separated encoded algorithm types.

    Returns:
      A table of alg_type (Table 9 of part 2) entries matching the passed in
      encoded type string.
    """
    filtered_table = []
    for alg_type in alg_type_str.split('.'):
      if re.match('^[A-Z]+$', alg_type):
        # Exclusive selection, must exactly match algorithm type from table 9
        # (which is in the second column). Add to the return value all
        # matching rows of table 9.
        extension = []
        for row in self._alg_id_table:
          if row[1] == alg_type:
            if self._alg_type and self._alg_type != row[2]:
              continue
            extension.append(row)
        filtered_table.extend(extension)
      elif re.match('^[a-z]+$', alg_type):
        # Inclusive selection. All type letters must be present in the type
        # column, but no exact match is required.
        for row in self._alg_id_table:
          for char in alg_type.upper():
            if char not in row[1]:
              break
          else:
            if not self._alg_type or self._alg_type == row[2]:
              filtered_table.append(row)
    return filtered_table

  def _ParseAlgorithmRegex(self, token):
    """Process a token as an algorithm regex.

    This function tries to interpret the passed in token as an encoded
    algorithm specification.

    In case the encoded algorithm regex matches, the function splits the token
    into prefix, algorithm description and suffix, and then retrieves the list
    of all algorithms matching the algorithm description.

    Args:
      token: a string, potentially including the algorithm regex.

    Returns:
      in case the regex matched returns a tri-tuple of two strings (prefix and
      suffix, either one could be empty) and a list of matching algorithms
      from the algorithm descriptors table. If there has been no match -
      returns None.
    """
    elements = self._alg_macro.split(token)
    if len(elements) == 3:
      # The token matched the algorithm regex, Find out prefix and suffix to
      # be used on the generated type names, and the algorithm regex suffix to
      # use to find matching entries in the algorithm table.
      name_prefix, alg_suffix, name_suffix = tuple(elements)
      name_prefix = name_prefix.strip('_')
      name_suffix = name_suffix.strip('_')
      return name_prefix, name_suffix, self._PickAlgEntries(alg_suffix)

  def _ProcessInterface(self, type_name):
    """Processes spec tables describing interfaces."""
    result = self._ParseAlgorithmRegex(type_name)
    if result:
      name_prefix, name_suffix, alg_list = tuple(result)
      # Process all matching algorithm types
      for alg_desc in alg_list:
        alg_base = alg_desc[0].replace('TPM_ALG_', '')
        new_name = '_'.join([name_prefix,
                             alg_base, name_suffix]).strip('_')
        new_if = Interface(self._title_type, new_name)
        self._AddTypedef(self._title_type, new_name)
        for row in self._body:
          new_value = row[0]
          if new_value.startswith('$!ALG'):
            new_if.supported_values = alg_base + '_' + '_'.join(
                new_value.split('_')[1:])
          elif new_value.startswith('$'):
            new_if.supported_values = new_value[1:]
          elif new_value.startswith('#'):
            new_if.error_code = new_value[1:]
        self._type_map[new_name] = new_if
      self._AddToHfile('\n')
      return
    new_if = Interface(self._title_type, type_name)
    self._AddTypedef(self._title_type, type_name)
    self._type_map[type_name] = new_if
    self._SetBaseType(type_name, new_if)
    for row in self._body:
      new_value = row[0]
      result = self._ParseAlgorithmRegex(new_value)
      if result:
        # The field is described using the algorithm regex. The above comment
        # applies.
        name_prefix, name_suffix, alg_list = tuple(result)
        for alg_desc in alg_list:
          alg_base = alg_desc[0].replace('TPM_ALG_', '')
          new_if.valid_values.append('_'.join(
              [name_prefix, alg_base, name_suffix]).strip('_'))
      else:
        if new_value.startswith('{'):
          bounds = tuple(
              [x.strip() for x in new_value[1:-1].strip().split(':')])
          new_if.bounds.append(bounds)
        elif new_value.startswith('+'):
          new_if.conditional_value = new_value[1:]
        elif new_value.startswith('#'):
          new_if.error_code = new_value[1:]
        elif new_value.startswith('$'):
          new_if.supported_values = new_value[1:]
        else:
          new_if.valid_values.append(new_value)
    return

  def _ProcessTypedefs(self, type_name):
    """Processes spec tables defining new types."""
    result = self._ParseAlgorithmRegex(type_name)
    if result:
      name_prefix, name_suffix, alg_list = tuple(result)
      for alg_desc in alg_list:
        alg_base = alg_desc[0].replace('TPM_ALG_', '')
        new_type = '%s_%s_%s' % (name_prefix, alg_base, name_suffix)
        self._AddTypedef(self._title_type, new_type)
      self._AddToHfile('\n')
    else:
      self._AddTypedef(self._title_type, type_name)

  def _ProcessBits(self, type_name):
    """Processes spec tables describing attributes (bit fields)."""
    bits_lines = []
    base_bit = 0
    tpm_obj = AttributeStructure(self._title_type, type_name)
    self._type_map[type_name] = tpm_obj
    self._SetBaseType(self._title_type, tpm_obj)
    for bits_line in self._body:
      field, name = tuple(bits_line[:2])
      if not field:
        continue
      if name.startswith('TPM_'):
        # Spec inconsistency fix.
        name_pieces = [x.lower() for x in name.split('_')[1:]]
        name = name_pieces[0]
        for piece in name_pieces[1:]:
          name += piece[0].upper() + piece[1:]
      bit_range = [x.replace(' ', '') for x in field.split(':')]
      field_base = int(bit_range[-1])
      if field_base != base_bit:
        field_name = 'reserved%d' % base_bit
        field_width = field_base - base_bit
        if field_width > 1:
          field_name += '_%d' % (field_base - 1)
        bits_lines.append(['%s : %d' % (field_name, field_width)])
        tpm_obj.reserved.append(field_name.replace('reserved', ''))
      if len(bit_range) > 1:
        field_width = int(bit_range[0]) - field_base + 1
      else:
        field_width = 1
      if re.match('reserved', name, re.IGNORECASE):
        name = 'reserved%d' % field_base
        if field_width > 1:
          name += '_%d' % (field_base + field_width - 1)
        tpm_obj.reserved.append(name.replace('reserved', ''))
      bits_lines.append([name, ': %d' % field_width])
      base_bit = field_base + field_width
    max_type_len, _ = self._GetMaxLengths(bits_lines)
    self._AddToHfile('typedef struct {')
    for bits_line in bits_lines:
      self._AddToHfile('  %s %-*s %s;' % (self._title_type, max_type_len,
                                          bits_line[0], bits_line[1]))
    self._AddToHfile('} %s;\n' % type_name)

  def _ExpandAlgs(self, row):
    """Find all algorithms encoded in the variable name.

    Args:
      row: a list of strings, a row of a structure or union table scraped from
           part 2.

    Returns:
      A list for structure_generator.Field objects, one per expanded
      algorithm.

    """
    alg_spec = row[0].split()
    expansion = []
    m = self._alg_macro.search(alg_spec[0])
    if m:
      alg_type = m.groups()[0]
      # Find all algorithms of this type in the alg id table
      alg_entries = self._PickAlgEntries(alg_type)
      if len(alg_spec) == 2 and alg_spec[1][0] == '[':
        # This is the case of a union of arrays.
        raw_size_parts = self._alg_macro.split(alg_spec[1][1:-1])
        size_prefix = raw_size_parts[0].strip('_')
        size_suffix = raw_size_parts[2].strip('_')
        for alg_desc in alg_entries:
          alg_base = alg_desc[0].replace('TPM_ALG_', '')
          size = '_'.join([size_prefix, alg_base, size_suffix]).strip('_')
          if self._has_selector_column:
            selector_parts = self._alg_macro.split(row[2])
            selector_prefix = selector_parts[0].strip('_')
            selector_suffix = selector_parts[2].strip('_')
            selector = '_'.join([selector_prefix,
                                 alg_base, selector_suffix]).strip('_')
          else:
            selector = ''
          expansion.append(Field(row[1], alg_base.lower(),
                                 selector=selector, array_size=size))
      else:
        type_prefix, type_suffix = self._SplitByAlg(row[1])
        if self._has_selector_column:
          selector_prefix, selector_suffix = self._SplitByAlg(row[2])
        else:
          selector = ''
        for alg_desc in alg_entries:
          alg_base = alg_desc[0].replace('TPM_ALG_', '')
          if type_suffix is not None:
            var_type = '_'.join([type_prefix, alg_base, type_suffix]).strip('_')
          else:
            var_type = type_prefix
          if self._has_selector_column:
            selector = '_'.join([selector_prefix, alg_base,
                                 selector_suffix]).strip('_')
          expansion.append(Field(var_type, alg_base.lower(),
                                 selector=selector))
    return expansion

  def _ProcessInterfaceOrType(self, type_name):
    if type_name.startswith('TPMI_'):
      self._ProcessInterface(type_name)
    else:
      self._ProcessTypedefs(type_name)

  def _StructOrUnionToHfile(self, body_fields, type_name, union_mode, tpm_obj):
    body_lines = []
    for field in body_fields:
      tpm_obj.AddField(field)
      body_lines.append([field.field_type, field.field_name])
      if field.array_size:
        body_lines[-1][-1] += '[%s]' % field.array_size
      if field.selector_value:
        body_lines[-1].append(field.selector_value)
    max_type_len, _ = self._GetMaxLengths(body_lines)
    tpm2b_mode = type_name.startswith('TPM2B_')
    space_prefix = ''
    if union_mode:
      self._AddToHfile('typedef union {')
    else:
      if tpm2b_mode:
        self._AddToHfile('typedef union {')
        space_prefix = '  '
        self._AddToHfile('  struct {')
      else:
        self._AddToHfile('typedef struct {')
    for line in body_lines:
      guard_required = len(line) > 2 and line[2].startswith('TPM_ALG_')
      if not line[1]:
        continue
      if guard_required:
        self._AddToHfile('#ifdef %s' % line[2])
      self._AddToHfile(space_prefix + '  %-*s  %s;' % (
          max_type_len, line[0], line[1]))
      if guard_required:
        self._AddToHfile('#endif')
    if tpm2b_mode:
      self._AddToHfile('  } t;')
      self._AddToHfile('  TPM2B b;')
    self._AddToHfile('} %s;\n' % type_name)
    self._type_map[type_name] = tpm_obj


  def _ProcessStructureOrUnion(self, type_name):
    """Processes spec tables describing structure and unions.

    Both of these object types share a lot of similarities. Union types have
    the word 'Union' in the table title.

    Args:
      type_name: a string, name of the TPM object type
    """
    union_mode = 'Union' in self._title
    if union_mode:
      tpm_obj = Union(type_name)
    else:
      tpm_obj = Structure(type_name)
    body_fields = []
    for row in self._body:
      if row[0].startswith('#'):
        tpm_obj.error_code = row[0][1:]
        continue
      if (len(row) < 2 or
          row[1].startswith('#') or
          row[0].startswith('//')):
        continue
      value = row[0]
      if value.endswith('='):
        value = value[:-1]
        tpm_obj.size_check = True
      if self._alg_macro.search(value):
        body_fields.extend(self._ExpandAlgs(row))
        continue
      array_size = None
      run_time_size = None
      vm = re.search(r'^(\S+)\s*\[(\S+)\]\s*\{(.*:*)\}', value)
      selector = ''
      if vm:
        value, run_time_size, bounds = vm.groups()
        lower, upper = [x.strip() for x in bounds.split(':')]
        if upper:
          array_size = upper
          tpm_obj.AddUpperBound(run_time_size, upper)
        else:
          array_size = run_time_size
        if lower:
          tpm_obj.AddLowerBound(run_time_size, lower)
      else:
        vm = re.search(r'^\[(\S+)\]\s*(\S+)', value)
        if vm:
          selector, value = vm.groups()
        else:
          vm = re.search(r'^(\S+)\s*\{(.+)\}', value)
          if vm:
            value, bounds = vm.groups()
            if ':' in bounds:
              lower, upper = [x.strip() for x in bounds.split(':')]
              if upper:
                tpm_obj.AddUpperBound(value, upper)
              if lower:
                tpm_obj.AddLowerBound(value, lower)
      if self._has_selector_column:
        selector = row[2]
      field_type = row[1]
      if field_type.startswith('+') or field_type.endswith('+'):
        field_type = field_type.strip('+')
        conditional = 'TRUE'
      else:
        conditional = 'FALSE'
      if field_type or value:
        body_fields.append(Field(field_type,
                                 value,
                                 array_size=array_size,
                                 run_time_size=run_time_size,
                                 selector=selector,
                                 conditional_value=conditional))

    self._StructOrUnionToHfile(body_fields, type_name, union_mode, tpm_obj)

  def _ProcessEnum(self, _):
    """Processes spec tables describing enums."""
    if self._skip_printing:
      return
    self._AddToHfile('enum {')
    for value, row in enumerate(self._body):
      self._AddToHfile('  %s = %d,' % (row[0], value + 1))
    self._AddToHfile('};\n')

  def _ProcessTypes(self, _):
    """Processes spec tables describing new types."""
    for type_, name_, _ in self._body:
      m = self._alg_macro.search(name_)
      if not m:
        self._AddTypedef(type_, name_)
        continue
      qualifier = [x for x in ('ECC', 'RSA') if x == type_.split('_')[-1]]
      alg_suffix = m.groups()[0]
      type_base = self._alg_macro.split(name_)[0]
      for alg_desc in self._PickAlgEntries(alg_suffix):
        if qualifier and alg_desc[2] != qualifier[0]:
          continue
        self._AddTypedef(type_, alg_desc[0].replace('TPM_ALG_', type_base))
    self._AddToHfile()

  def _ProcessConstants(self, type_name):
    """Processes spec tables describing constants."""
    if self._title_type:
      self._AddTypedef(self._title_type, type_name)
    constant_defs = []
    tpm_obj = ConstantType(self._title_type, type_name)
    for row in self._body:
      name = row[0].strip()
      if not name:
        continue
      if name.startswith('#'):
        tpm_obj.error_code = name[1:]
        continue
      if name == 'reserved' or len(row) < 2:
        continue
      value = row[1].strip()
      rm = re.match(r'^(\(.*?\))', value)
      if rm:
        value = '%s' % rm.groups()[0]
      else:
        v_list = value.split()
        if len(v_list) > 2 and v_list[1] == '+':
          value = '((%s)(%s))' % (type_name, ' '.join(v_list[:3]))
      if ' ' in value and not value.startswith('('):
        value = '(%s)' % value
      constant_defs.append([name, value])
      tpm_obj.valid_values.append(name)
    if self._title_type:
      self._type_map[type_name] = tpm_obj
      self._SetBaseType(self._title_type, tpm_obj)
    if self._skip_printing:
      return
    max_name_len, max_value_len = self._GetMaxLengths(constant_defs)
    for row in constant_defs:
      self._AddToHfile('#define %-*s  %*s' % (max_name_len, row[0],
                                              max_value_len, row[1]))
    self._AddToHfile()

  def GetHFile(self):
    return self._h_file

  def GetCommandList(self):
    return sorted(self._command_map.values(),
                  cmp=lambda x, y: cmp(x.name, y.name))

  def GetTypeMap(self):
    return self._type_map
