# Copyright 2015 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Argument processing for the DUT deployment tool.

The argument processing is mostly a conventional client of
`argparse`, except that if the command is invoked without required
arguments, code here will start a line-oriented text dialog with the
user to get the arguments.

These are the arguments:
  * (required) Board of the DUTs to be deployed.
  * (required) Hostnames of the DUTs to be deployed.
  * (optional) Version of the test image to be made the stable
    repair image for the board to be deployed.  If omitted, the
    existing setting is retained.

The interactive dialog is invoked if the board and hostnames
are omitted from the command line.

"""

import argparse
import os
import re
import subprocess
import sys
import time


# _BUILD_URI_FORMAT
# A format template for a Google storage URI that designates
# one build.  The template is to be filled in with a board
# name and build version number.

_BUILD_URI_FORMAT = 'gs://chromeos-image-archive/%s-release/%s'


# _BUILD_PATTERNS
# For user convenience, argument parsing allows various formats
# for build version strings.  The function _normalize_build_name()
# is used to convert the recognized syntaxes into the name as
# it appears in Google storage.
#
# _BUILD_PATTERNS describe the recognized syntaxes for user-supplied
# build versions, and information about how to convert them.  See the
# normalize function for details.
#
# For user-supplied build versions, the following forms are supported:
#   ####        - Indicates a canary; equivalent to ####.0.0.
#   ####.#.#    - A full build version without the leading R##- prefix.
#   R##-###.#.# - Canonical form of a build version.

_BUILD_PATTERNS = [
    (re.compile(r'^R\d+-\d+\.\d+\.\d+$'),   None),
    (re.compile(r'^\d+\.\d+\.\d+$'),        'LATEST-%s'),
    (re.compile(r'^\d+$'),                  'LATEST-%s.0.0'),
]


# _VALID_HOSTNAME_PATTERNS
# A list of REs describing patterns that are acceptable as names
# for DUTs in the test lab.  Names that don't match one of the
# patterns will be rejected as invalid.

_VALID_HOSTNAME_PATTERNS = [
    re.compile(r'chromeos\d+-row\d+-rack\d+-host\d+')
]


def _build_path_exists(board, buildpath):
    """Return whether a given build file exists in Google storage.

    The `buildpath` refers to a specific file associated with
    release builds for `board`.  The path may be one of the "LATEST"
    files (e.g. "LATEST-7356.0.0"), or it could refer to a build
    artifact (e.g. "R46-7356.0.0/image.zip").

    The function constructs the full GS URI from the arguments, and
    then tests for its existence with `gsutil ls`.

    @param board        Board to be tested.
    @param buildpath    Partial path of a file in Google storage.

    @return Return a true value iff the designated file exists.
    """
    try:
        gsutil_cmd = [
                'gsutil', 'ls',
                _BUILD_URI_FORMAT % (board, buildpath)
        ]
        status = subprocess.call(gsutil_cmd,
                                 stdout=open('/dev/null', 'w'),
                                 stderr=subprocess.STDOUT)
        return status == 0
    except:
        return False


def _normalize_build_name(board, build):
    """Convert a user-supplied build version to canonical form.

    Canonical form looks like  R##-####.#.#, e.g. R46-7356.0.0.
    Acceptable user-supplied forms are describe under
    _BUILD_PATTERNS, above.  The returned value will be the name of
    a directory containing build artifacts from a release builder
    for the board.

    Walk through `_BUILD_PATTERNS`, trying to convert a user
    supplied build version name into a directory name for valid
    build artifacts.  Searching stops at the first pattern matched,
    regardless of whether the designated build actually exists.

    `_BUILD_PATTERNS` is a list of tuples.  The first element of the
    tuple is an RE describing a valid user input.  The second
    element of the tuple is a format pattern for a "LATEST" filename
    in storage that can be used to obtain the full build version
    associated with the user supplied version.  If the second element
    is `None`, the user supplied build version is already in canonical
    form.

    @param board    Board to be tested.
    @param build    User supplied version name.

    @return Return the name of a directory in canonical form, or
            `None` if the build doesn't exist.
    """
    for regex, fmt in _BUILD_PATTERNS:
        if not regex.match(build):
            continue
        if fmt is not None:
            try:
                gsutil_cmd = [
                    'gsutil', 'cat',
                    _BUILD_URI_FORMAT % (board, fmt % build)
                ]
                return subprocess.check_output(
                        gsutil_cmd, stderr=open('/dev/null', 'w'))
            except:
                return None
        elif _build_path_exists(board, '%s/image.zip' % build):
            return build
        else:
            return None
    return None


def _validate_board(board):
    """Return whether a given board exists in Google storage.

    For purposes of this function, a board exists if it has a
    "LATEST-master" file in its release builder's directory.

    N.B. For convenience, this function prints an error message
    on stderr in certain failure cases.  This is currently useful
    for argument processing, but isn't really ideal if the callers
    were to get more complicated.

    @param board    The board to be tested for existence.
    @return Return a true value iff the board exists.
    """
    # In this case, the board doesn't exist, but we don't want
    # an error message.
    if board is None:
        return False
    # Check Google storage; report failures on stderr.
    if _build_path_exists(board, 'LATEST-master'):
        return True
    else:
        sys.stderr.write('Board %s doesn\'t exist.\n' % board)
        return False


def _validate_build(board, build):
    """Return whether a given build exists in Google storage.

    N.B. For convenience, this function prints an error message
    on stderr in certain failure cases.  This is currently useful
    for argument processing, but isn't really ideal if the callers
    were to get more complicated.

    @param board    The board to be tested for a build
    @param build    The version of the build to be tested for.  This
                    build may be in a user-specified (non-canonical)
                    form.
    @return If the given board+build exists, return its canonical
            (normalized) version string.  If the build doesn't
            exist, return a false value.
    """
    canonical_build = _normalize_build_name(board, build)
    if not canonical_build:
        sys.stderr.write(
                'Build %s is not a valid build version for %s.\n' %
                (build, board))
    return canonical_build


def _validate_hostname(hostname):
    """Return whether a given hostname is valid for the test lab.

    This is a sanity check meant to guarantee that host names follow
    naming requirements for the test lab.

    N.B. For convenience, this function prints an error message
    on stderr in certain failure cases.  This is currently useful
    for argument processing, but isn't really ideal if the callers
    were to get more complicated.

    @param hostname The host name to be checked.
    @return Return a true value iff the hostname is valid.
    """
    for p in _VALID_HOSTNAME_PATTERNS:
        if p.match(hostname):
            return True
    sys.stderr.write(
            'Hostname %s doesn\'t match a valid location name.\n' %
                hostname)
    return False


def _validate_arguments(arguments):
    """Check command line arguments, and account for defaults.

    Check that all command-line argument constraints are satisfied.
    If errors are found, they are reported on `sys.stderr`.

    If there are any fields with defined defaults that couldn't be
    calculated when we constructed the argument parser, calculate
    them now.

    @param arguments  Parsed results from
                      `ArgumentParser.parse_args()`.
    @return Return `True` if there are no errors to report, or
            `False` if there are.
    """
    if (not arguments.hostnames and
            (arguments.board or arguments.build)):
        sys.stderr.write(
                'DUT hostnames are required with board or build.\n')
        return False
    if arguments.board is not None:
        if not _validate_board(arguments.board):
            return False
        if (arguments.build is not None and
                not _validate_build(arguments.board, arguments.build)):
            return False
    return True


def _read_with_prompt(input, prompt):
    """Print a prompt and then read a line of text.

    @param input File-like object from which to read the line.
    @param prompt String to print to stderr prior to reading.
    @return Returns a string, stripped of whitespace.
    """
    full_prompt = '%s> ' % prompt
    sys.stderr.write(full_prompt)
    return input.readline().strip()


def _read_board(input, default_board):
    """Read a valid board name from user input.

    Prompt the user to supply a board name, and read one line.  If
    the line names a valid board, return the board name.  If the
    line is blank and `default_board` is a non-empty string, returns
    `default_board`.  Retry until a valid input is obtained.

    `default_board` isn't checked; the caller is responsible for
    ensuring its validity.

    @param input          File-like object from which to read the
                          board.
    @param default_board  Value to return if the user enters a
                          blank line.
    @return Returns `default_board` or a validated board name.
    """
    if default_board:
        board_prompt = 'board name [%s]' % default_board
    else:
        board_prompt = 'board name'
    new_board = None
    while not _validate_board(new_board):
        new_board = _read_with_prompt(input, board_prompt).lower()
        if new_board:
            sys.stderr.write('Checking for valid board.\n')
        elif default_board:
            return default_board
    return new_board


def _read_build(input, board):
    """Read a valid build version from user input.

    Prompt the user to supply a build version, and read one line.
    If the line names an existing version for the given board,
    return the canonical build version.  If the line is blank,
    return `None` (indicating the build shouldn't change).

    @param input    File-like object from which to read the build.
    @param board    Board for the build.
    @return Returns canonical build version, or `None`.
    """
    build = False
    prompt = 'build version (optional)'
    while not build:
        build = _read_with_prompt(input, prompt)
        if not build:
            return None
        sys.stderr.write('Checking for valid build.\n')
        build = _validate_build(board, build)
    return build


def _read_hostnames(input):
    """Read a list of host names from user input.

    Prompt the user to supply a list of host names.  Any number of
    lines are allowed; input is terminated at the first blank line.
    Any number of hosts names are allowed on one line.  Names are
    separated by whitespace.

    Only valid host names are accepted.  Invalid host names are
    ignored, and a warning is printed.

    @param input    File-like object from which to read the names.
    @return Returns a list of validated host names.
    """
    hostnames = []
    y_n = 'yes'
    while not 'no'.startswith(y_n):
        sys.stderr.write('enter hosts (blank line to end):\n')
        while True:
            new_hosts = input.readline().strip().split()
            if not new_hosts:
                break
            for h in new_hosts:
                if _validate_hostname(h):
                    hostnames.append(h)
        if not hostnames:
            sys.stderr.write('Must provide at least one hostname.\n')
            continue
        prompt = 'More hosts? [y/N]'
        y_n = _read_with_prompt(input, prompt).lower() or 'no'
    return hostnames


def _read_arguments(input, arguments):
    """Dialog to read all needed arguments from the user.

    The user is prompted in turn for a board, a build, and
    hostnames.  Responses are stored in `arguments`.  The user is
    given opportunity to accept or reject the responses before
    continuing.

    @param input      File-like object from which to read user
                      responses.
    @param arguments  Arguments object returned from
                      `ArgumentParser.parse_args()`.  Results are
                      stored here.
    """
    y_n = 'no'
    while not 'yes'.startswith(y_n):
        arguments.board = _read_board(input, arguments.board)
        arguments.build = _read_build(input, arguments.board)
        prompt = '%s build %s? [Y/n]' % (
                arguments.board, arguments.build)
        y_n = _read_with_prompt(input, prompt).lower() or 'yes'
    arguments.hostnames = _read_hostnames(input)


def _parse(argv):
    """Parse the command line arguments.

    Create an argument parser for this command's syntax, parse the
    command line, and return the result of the ArgumentParser
    parse_args() method.

    @param argv Standard command line argument vector; argv[0] is
                assumed to be the command name.
    @return Result returned by ArgumentParser.parse_args().
    """
    parser = argparse.ArgumentParser(
            prog=argv[0],
            description='Install a test image on newly deployed DUTs')
    # frontend.AFE(server=None) will use the default web server,
    # so default for --web is `None`.
    parser.add_argument('-w', '--web', metavar='SERVER', default=None,
                        help='specify web server')
    parser.add_argument('-d', '--dir',
                        help='directory for logs')
    parser.add_argument('-i', '--build',
                        help='select stable test build version')
    parser.add_argument('-n', '--noinstall', action='store_true',
                        help='skip install (for script testing)')
    parser.add_argument('-s', '--nostage', action='store_true',
                        help='skip staging test image (for script testing)')
    parser.add_argument('-t', '--nostable', action='store_true',
                        help='skip changing stable test image '
                             '(for script testing)')
    parser.add_argument('board', nargs='?', metavar='BOARD',
                        help='board for DUTs to be installed')
    parser.add_argument('hostnames', nargs='*', metavar='HOSTNAME',
                        help='host names of DUTs to be installed')
    return parser.parse_args(argv[1:])


def parse_command(argv, full_deploy):
    """Get arguments for install from `argv` or the user.

    Create an argument parser for this command's syntax, parse the
    command line, and return the result of the ArgumentParser
    parse_args() method.

    If mandatory arguments are missing, execute a dialog with the
    user to read the arguments from `sys.stdin`.  Fill in the
    return value with the values read prior to returning.

    @param argv         Standard command line argument vector;
                        argv[0] is assumed to be the command name.
    @param full_deploy  Whether this is for full deployment or
                        repair.

    @return Result, as returned by ArgumentParser.parse_args().
    """
    arguments = _parse(argv)
    arguments.full_deploy = full_deploy
    if arguments.board is None:
        _read_arguments(sys.stdin, arguments)
    elif not _validate_arguments(arguments):
        return None
    if not arguments.dir:
        basename = 'deploy.%s.%s' % (
                time.strftime('%Y-%m-%d.%H:%M:%S'),
                arguments.board)
        arguments.dir = os.path.join(os.environ['HOME'],
                                     'Documents', basename)
        os.makedirs(arguments.dir)
    elif not os.path.isdir(arguments.dir):
        os.mkdir(arguments.dir)
    return arguments
