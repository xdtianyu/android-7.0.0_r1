#!/usr/bin/python

# Copyright 2015 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""A Main file for command and structure generators.

Takes in structures.txt and commands.txt as outputted by extract_*.sh, then
passes files as input to structure_generator and command_generator objects.
"""

from __future__ import print_function

import os
import re
import subprocess
import sys

import command_generator
import extract_structures
import structure_generator

TMP_DIR = '/tmp'
TYPES_FILE = 'tpm_types.h'


class GeneratorException(Exception):
  """Generator error, a convenience class."""
  pass

usage = ('''
usage: %s [-h|[tar_archive|part2.html part3.html]]

    -h  show this message and exit

    tar_archive - a tarred archive consisting of at least two HTML files,
                  parts 2 and 3 of the TCG TPM2 library specification. File
                  names must include 'part2' and 'part3'. The extracted files
                  could be found in %s after this script finished processing.

    part{23}.html - parts 2 and 3 of the TCG TPM2 library specification in
                    html format.
''' % (os.path.basename(__file__), TMP_DIR))


def _TryUntarring(tar_file_name):
  """Try retrieving parts 2 and 3 from the passed in archive.

  Args:
    tar_file_name: a string, file name of the tar file which is supposed to
        contain parts 2 and 3 of the specification.

  Returns:
    A tuple of strings, two file names in case they were found in the archive
    and successfully extracted.
  """
  part2 = None
  part3 = None
  tar_extract_base = ['tar', '-C', TMP_DIR, '-f']

  components = subprocess.check_output(['tar', 'tf', tar_file_name],
                                       stderr=subprocess.STDOUT)
  for name in components.splitlines():
    if re.search('part2', name, re.IGNORECASE):
      subprocess.check_output(tar_extract_base + [tar_file_name, '-x', name],
                              stderr=subprocess.STDOUT)
      part2 = os.path.join(TMP_DIR, name)
    if re.search('part3', name, re.IGNORECASE):
      subprocess.check_output(tar_extract_base + [tar_file_name, '-x', name],
                              stderr=subprocess.STDOUT)
      part3 = os.path.join(TMP_DIR, name)
  return part2, part3


def _ParseCommandLine(args):

  """Process command line and determine input file names.

  Input files could be supplied by two different ways - as part of a tar
  archive (in which case only one command line parameter is expected), or as
  two separate file names, one for part 2 and one for part 3.

  If a single command line parameter is supplied, and it is not '-h', tar
  extraction is attempted and if successful, two separate files are created in
  TMP_DIR.

  Args:
    args: a list of string, command line parameters retrieved from sys.argv

  Returns:
    A tuple of two strings, two html files to process, part 2 and part 3 of
    the spec.

  Raises:
    GeneratorException: on input errors.
  """
  if len(args) == 1:
    if args[0] == '-h':
      print(usage)
      sys.exit(0)
    try:
      structures_file, commands_file = _TryUntarring(args[0])
    except subprocess.CalledProcessError as e:
      raise GeneratorException("command '%s' failed:\n%s\n%s" %
                               (' '.join(e.cmd), e.output, usage))

  elif len(args) == 2:
    structures_file = args[0]
    commands_file = args[1]
  else:
    raise GeneratorException(usage)
  return structures_file, commands_file


def main(argv):
  """A Main function.

  TPM structures and commands files are parsed and C header and C implementation
  files are generated.

  Args:
    argv: a list of strings, command line parameters.
  """

  structures_file, commands_file = _ParseCommandLine(argv[1:])
  print('parse part2...')
  html_parser = extract_structures.SpecParser()
  tpm_table = html_parser.GetTable()
  # The tables included in the below tuple are defined twice in the
  # specification, once in part 2 and once in part 4. Let's ignore the part 2
  # definitions to avoid collisions.
  tpm_table.SetSkipTables((2, 6, 9, 10, 13))
  html_parser.feed(open(structures_file).read())
  html_parser.close()
  tpm_defines = tpm_table.GetHFile()

  print('parse part3...')
  tpm_table.SetSkipTables(())
  html_parser.feed(open(commands_file).read())
  html_parser.close()

  # Move to the root directory, which is one level above the script.
  os.chdir(os.path.join(os.path.dirname(argv[0]), '..'))

  # Save types include file.
  print('generate output...')
  types_file = open(TYPES_FILE, 'w')
  guard_name = TYPES_FILE.upper()
  guard_name = guard_name.replace('.', '_')
  guard_name = 'TPM2_' + guard_name + '_'
  types_file.write((structure_generator.COPYRIGHT_HEADER +
                    structure_generator.HEADER_FILE_GUARD_HEADER) %
                   {'name': guard_name})
  types_file.write(tpm_defines)
  types_file.write((structure_generator.HEADER_FILE_GUARD_FOOTER) %
                   {'name': guard_name})
  types_file.close()
  typemap = tpm_table.GetTypeMap()
  structure_generator.GenerateHeader(typemap)
  structure_generator.GenerateImplementation(typemap)
  commands = tpm_table.GetCommandList()
  command_generator.GenerateHeader(commands)
  command_generator.GenerateImplementation(commands, typemap)
  print('Processed %d TPM types.' % len(typemap))
  print('Processed %d commands.' % len(commands))

if __name__ == '__main__':
  try:
    main(sys.argv)
  except GeneratorException as e:
    if e.message:
      print(e, file=sys.stderr)
      sys.exit(1)
