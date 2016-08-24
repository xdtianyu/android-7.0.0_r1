#!/usr/bin/python
# Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Extract list of team members for the root individual."""


import commands
import json
import logging
import optparse
import os
import sys


def ParseArgs(argv):
  """Get input and output file."""
  base_dir = os.path.realpath(os.path.abspath(os.path.join(
      os.getcwd(), os.path.dirname(__file__))))

  parser = optparse.OptionParser()
  parser.add_option('-r', '--root-person', help='initial person to follow',
                    dest='rootperson', default=None)
  parser.add_option('-o', '--output-file', help='output file (json)',
                    dest='outputfile', default="kernel-team.json")
  parser.add_option('-p', '--print-test', help='print the map as a test',
                    dest='printtest', action='store_true', default=False)
  parser.add_option('-v', '--verbose', help='Show more output',
                    dest='verbose', action='store_true', default=False)
  options, args = parser.parse_args()

  if not options.rootperson:
    parser.error('--root-person must be supplied')

  logging_level = logging.INFO
  if options.verbose:
    logging_level = logging.DEBUG

  logging.basicConfig(level=logging_level)

  return options, args, base_dir


def SearchOnePerson(person):
  """Run a command to get details for one person."""
  found = []
  if person:
    command = 'f %s | grep Reportees' % person
    logging.debug(command)
    find_result = commands.getoutput(command)
    if find_result:
      found = find_result.split(' ')[2:]
      logging.debug(found)
  return found


def FindTeamMembers(root_person):
  """Recursively iteratea through the list of team members until done.

  Expect the root_person to have at least 1 report but not needed.
  """
  remaining = [root_person]
  extracted = [root_person]
  while remaining:
    found = SearchOnePerson(remaining.pop(0))
    if found:
      remaining += found
      extracted += found

  return extracted


def WriteJson(outputfile, extracted):
  """Write output in json format."""
  f = open(outputfile, 'w')
  json.dump(extracted, f)
  f.close()


def PrintJson(jsonfile):
  """Read the json file and format-print its contents as a test."""
  f = open(jsonfile, 'r')
  team_list = json.load(f)
  f.close()
  for t in sorted(team_list):
    logging.info('%s', t)


def main(argv):
  """Can generate tables, plots and email."""
  options, args, base_dir = ParseArgs(argv)

  logging.info('Using %s as root.', options.rootperson)
  logging.info('Using output file: %s', options.outputfile)

  team = FindTeamMembers(options.rootperson)
  if team:
    WriteJson(options.outputfile, team)
  if options.printtest:
    PrintJson(options.outputfile)


if __name__ == '__main__':
  main(sys.argv)
