#!/usr/bin/python
#
# Copyright 2010 Google Inc. All Rights Reserved.

"""Script to clear lock directories for latest builds; rescheduling tests.

Running this script will remove all test locks for the latest builds of every
board. Which will cause all tests whose locks were removed to be rescheduled the
next time Test Scheduler runs. The latest build is determined by the contents of
the LATEST file in each board directory.

By default the script will process all boards under DEFAULT_IMAGES_PATH. To
process only specific boards and/or a different images directory, use the -b
and/or -i options.

Expected images directory structure looks like:

   <board_0>\LATEST
   <board_0>\<build_0>\netbook_<test_0>
   <board_0>\<build_0>\netbook_<test_1>
   .
   .
   .
   <board_0>\<build_0>\netbook_<test_n>
   .
   .
   .
   <board_n>\LATEST
   <board_n>\<build_0>\netbook_<test_0>
   <board_n>\<build_0>\netbook_<test_n>
"""

__author__ = 'dalecurtis@google.com (Dale Curtis)'

import optparse
import os


# Path to Dev Server's images directory.
DEFAULT_IMAGES_PATH = '/usr/local/google/images'


def ParseOptions():
  """Parse command line options. Returns options structure."""

  parser = optparse.OptionParser('usage: %prog [options]')

  parser.add_option('-b', '--boards', dest='boards',
                    help='Comma separated list of boards to process. By default'
                    ' all boards in the images directory will be processed.')
  parser.add_option('-i', '--images', dest='images',
                    help='Path to Dev Server images directory. Defaults to %s' %
                    DEFAULT_IMAGES_PATH, default=DEFAULT_IMAGES_PATH)

  options = parser.parse_args()[0]

  if not os.path.exists(options.images):
    parser.error('The specified images directory (%s) does not exist. Please '
                 'specify another.' % options.images)

  if options.boards:
    options.boards = options.boards.split(',')

  return options


def main():
  options = ParseOptions()

  os.chdir(options.images)

  # Build board listing.
  if options.boards:
    boards = options.boards
  else:
    boards = [board for board in os.listdir('.') if os.path.isdir(board)]

  for board in boards:
    latest_path = os.path.join(board, 'LATEST')

    # Make sure directory contains a LATEST file.
    if not os.path.exists(latest_path):
      continue

    build_path = os.path.join(board, open(latest_path, 'r').read().strip())

    # Make sure LATEST file points to a valid build.
    if not os.path.exists(build_path):
      continue

    # Remove test locks in latest build directory.
    for test in os.listdir(build_path):
      test_path = os.path.join(build_path, test)

      # Only remove directories we know (well, pretty sure) are test locks.
      if not os.path.isdir(test_path) or not test.startswith('netbook_'):
        continue

      print 'Removing lock %s' % test_path
      os.rmdir(test_path)


if __name__ == '__main__':
  main()
