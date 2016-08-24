#!/usr/bin/python
# Copyright 2015 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Module for parsing TCG TPM2 library specification in HTML format.

This module processes parts 2 and 3 of the specification, extracting
information related to tables defined in the documents, feeding the
information into the Table object for further processing and creating the
appropriate TPM2 objects.
"""

from __future__ import print_function

import HTMLParser
import os
import re
import sys

import tpm_table

table_name = re.compile(r'^\s*Table\s+[0-9]+')


class SpecParser(HTMLParser.HTMLParser):
  """A class for parsing TCG specifications in html format."""

  # The state machine of the parser could be in one of the following states.
  ANCHOR = 0       # Look for table title anchor
  TABLE_NAME = 1   # Look for table title in the data stream
  TABLE_BODY = 2   # Scraping the actual table body
  MAYBE_DONE = 3   # Could be over, unless a single spec table is split in
                   # multiple HTML tables (to continue on the next page)
  SKIP_HEADER = 4  # Ignore the header of the split tables

  def __init__(self):
    """Initialize a parser object to default state."""
    HTMLParser.HTMLParser.__init__(self)
    self._state = self.ANCHOR
    self._title = ''
    self._table = tpm_table.Table()
    self._previous_table_number = 0  # Used to check if there are skipped tables

  def _Normalize(self, data):
    """Normalize HTML data.

    HTML files generated from TCG specifications sometimes include utf8
    characters (like long dashes), which appear only in comments/table titles
    and can be safely ignored.

    Args:
     data: a string representing portion of data from the HTML being parsed.

    Returns:
      a string, the input data with characters above ASCII printable range
                 excluded.
    """
    return ' ' + ''.join(x for x in self.unescape(data) if ord(x) < 128)

  def GetTable(self):
    """Return the Table object containing all information parsed so far."""
    return self._table

  def _SetState(self, new_state):
    if self._state != new_state:
      self._state = new_state
      if new_state == self.TABLE_NAME:
        self._title = ''

  def handle_starttag(self, tag, attrs):
    """Invoked each time a new HTML tag is opened.

    This method drives changes in the parser FSM states, its heuristics are
    derived from the format of the HTML files the TCG specs get converted to.

    Each specification table is preceded with a tittle. The title is wrapped
    in an anchor tag with a property 'name' set to 'bookmark#xxx. The title
    text starts with ' Table [0-9]+ '. Once the table title is detected,
    the state machine switches to looking for the actual HTML table, i.e. tags
    'table', 'tr' and 'td' (the generated specs do not use the 'th' tags).

    Large specification tables can be split into multiple HTML tables (so that
    they fit in a page). This is why the presence of the closing 'table' tag
    is not enough to close the parsing of the current specification table.

    In some cases the next table is defined in the spec immediately after the
    current one - this is when the new anchor tag is used as a signal that the
    previous table has been completely consumed.

    Args:
      tag: a string, the HTML tag
      attrs: a tuple of zero or more two-string tuples, the first element -
             the HTML tag's attribute, the second element - the attribute
             value.
    """
    if tag == 'a':
      if [x for x in attrs if x[0] == 'name' and x[1].startswith('bookmark')]:
        if self._state == self.ANCHOR:
          self._SetState(self.TABLE_NAME)
        elif self._state == self.MAYBE_DONE:
          # Done indeed
          self._table.ProcessTable()
          self._table.Init()
          self._SetState(self.TABLE_NAME)
        elif self._state == self.TABLE_NAME:
          self._title = ''
    elif tag == 'p' and self._state == self.TABLE_NAME and not self._title:
      # This was not a valid table start, back to looking for the right anchor.
      self._SetState(self.ANCHOR)
    elif self._state == self.TABLE_NAME and tag == 'table':
      if not table_name.search(self._title):
        # Table title does not match the expected format - back to square one.
        self._SetState(self.ANCHOR)
        return  # will have to start over
      table_number = int(self._title.split()[1])
      self._previous_table_number += 1
      if table_number > self._previous_table_number:
        print('Table(s) %s missing' % ' '.join(
            '%d' % x for x in
            range(self._previous_table_number, table_number)), file=sys.stderr)
        self._previous_table_number = table_number
      self._table.Init(self._title)
      self._SetState(self.TABLE_BODY)
    elif self._state == self.MAYBE_DONE and tag == 'tr':
      self._SetState(self.SKIP_HEADER)
    elif self._state == self.SKIP_HEADER and tag == 'tr':
      self._SetState(self.TABLE_BODY)
      self._table.NewRow()
    elif self._state == self.TABLE_BODY:
      if tag == 'tr':
        self._table.NewRow()
      elif tag == 'td':
        self._table.NewCell()

  def handle_endtag(self, tag):
    """Invoked each time an HTML tag is closed."""
    if tag == 'table' and self._table.InProgress():
      self._SetState(self.MAYBE_DONE)

  def handle_data(self, data):
    """Process data outside HTML tags."""
    if self._state == self.TABLE_NAME:
      self._title += ' %s' % self._Normalize(data)
    elif self._state == self.TABLE_BODY:
      self._table.AddData(self._Normalize(data))
    elif self._state == self.MAYBE_DONE:
      # Done indeed
      self._table.ProcessTable()
      self._table.Init()
      self._SetState(self.ANCHOR)

  def close(self):
    """Finish processing of the HTML buffer."""
    if self._state in (self.TABLE_BODY, self.MAYBE_DONE):
      self._table.ProcessTable()
    self._state = self.ANCHOR

  def handle_entityref(self, name):
    """Process HTML escape sequence."""
    entmap = {
        'amp': '&',
        'gt': '>',
        'lt': '<',
        'quot': '"',
    }
    if name in entmap:
      if self._state == self.TABLE_BODY:
        self._table.AddData(entmap[name])
      elif self._state == self.TABLE_NAME:
        self._title += entmap[name]


def main(structs_html_file_name):
  """When invoked standalone - dump .h file on the console."""
  parser = SpecParser()
  with open(structs_html_file_name) as input_file:
    html_content = input_file.read()
  parser.feed(html_content)
  parser.close()
  print(parser.GetTable().GetHFile())

if __name__ == '__main__':
  if len(sys.argv) != 2:
    print('%s: One parameter is required, the name of the html file '
          'which is the TPM2 library Part 2 specification' %
          os.path.basename(sys.argv[0]), file=sys.stderr)
    sys.exit(1)
  main(sys.argv[1])
