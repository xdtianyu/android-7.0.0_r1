# Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Classes for holding autoupdate_EndToEnd test parameters."""

import common

import test_control


_DEFAULT_AU_SUITE_NAME = 'au'
_additional_suite_names = ', push_to_prod'


class TestEnv(object):
    """Contains and formats the environment arguments of a test."""

    def __init__(self, args):
        """Initial environment arguments object.

        @param args: parsed program arguments, including test environment ones

        """
        self._env_args_str_local = None
        self._env_args_str_afe = None

        # Distill environment arguments from all input arguments.
        self._env_args = {}
        omaha_host = vars(args).get('omaha_host')
        if omaha_host is not None:
            self._env_args['omaha_host'] = omaha_host


    def is_var_set(self, var):
        """Returns true if a variable is set in this environment.

        @param var: the variable we are interested in

        """
        return var in self._env_args


    def get_cmdline_args(self):
        """Return formatted environment arguments for command-line invocation.

        The formatted string is cached for repeated use.

        """
        if self._env_args_str_local is None:
            self._env_args_str_local = ''
            for key, val in self._env_args.iteritems():
                # Convert Booleans to 'yes' / 'no'.
                if val is True:
                    val = 'yes'
                elif val is False:
                    val = 'no'

                self._env_args_str_local += ' %s=%s' % (key, val)

        return self._env_args_str_local


    def get_code_args(self):
        """Return formatted environment arguments for inline assignment.

        The formatted string is cached for repeated use.

        """
        if self._env_args_str_afe is None:
            self._env_args_str_afe = ''
            for key, val in self._env_args.iteritems():
                # Everything becomes a string, except for Booleans.
                if type(val) is bool:
                    self._env_args_str_afe += "%s = %s\n" % (key, val)
                else:
                    self._env_args_str_afe += "%s = '%s'\n" % (key, val)

        return self._env_args_str_afe


class TestConfig(object):
    """A single test configuration.

    Stores and generates arguments for running autotest_EndToEndTest.

    """
    def __init__(self, board, name, is_delta_update, source_release,
                 target_release, source_payload_uri, target_payload_uri,
                 suite_name=_DEFAULT_AU_SUITE_NAME, source_archive_uri=None):
        """Initialize a test configuration.

        @param board: the board being tested (e.g. 'x86-alex')
        @param name: a descriptive name of the test
        @param is_delta_update: whether this is a delta update test (Boolean)
        @param source_release: the source image version (e.g. '2672.0.0')
        @param target_release: the target image version (e.g. '2673.0.0')
        @param source_payload_uri: source payload URI ('gs://...') or None
        @param target_payload_uri: target payload URI ('gs://...')
        @param suite_name: the name of the test suite (default: 'au')
        @param source_archive_uri: location of source build artifacts

        """
        self.board = board
        self.name = name
        self.is_delta_update = is_delta_update
        self.source_release = source_release
        self.target_release = target_release
        self.source_payload_uri = source_payload_uri
        self.target_payload_uri = target_payload_uri
        self.suite_name = suite_name
        self.source_archive_uri = source_archive_uri


    def get_update_type(self):
        return 'delta' if self.is_delta_update else 'full'


    def unique_name_suffix(self):
        """Unique name suffix for the test config given the target version."""
        return '%s_%s_%s' % (self.name,
                             'delta' if self.is_delta_update else 'full',
                             self.source_release)


    def get_autotest_name(self):
        """Returns job name to use when creating an autotest job.

        Returns a job name that conforms to the suite naming style.

        """
        return '%s-release/%s/%s/%s.%s' % (
                self.board, self.target_release, self.suite_name,
                test_control.get_test_name(), self.unique_name_suffix())


    def get_control_file_name(self):
        """Returns the name of the name of the control file to store this in.

        Returns the control file name that should be generated for this test.
        A unique name suffix is used to keep from collisions per target
        release/board.
        """
        return 'control.%s' % self.unique_name_suffix()


    def __str__(self):
        """Short textual representation w/o image/payload URIs."""
        return ('[%s/%s/%s/%s -> %s]' %
                (self.board, self.name, self.get_update_type(),
                 self.source_release, self.target_release))


    def __repr__(self):
        """Full textual representation w/ image/payload URIs."""
        return '\n'.join([str(self),
                          'source payload : %s' % self.source_payload_uri,
                          'target payload : %s' % self.target_payload_uri])


    def _get_args(self, assign, delim, is_quote_val):
        template = "%s%s'%s'" if is_quote_val else "%s%s%s"
        arg_values = [
            ('name', self.name),
            ('update_type', self.get_update_type()),
            ('source_release', self.source_release),
            ('target_release', self.target_release),
            ('target_payload_uri', self.target_payload_uri),
            ('SUITE', self.suite_name)
        ]
        if self.source_payload_uri:
            arg_values.append(('source_payload_uri', self.source_payload_uri))
        if self.source_archive_uri:
            arg_values.append(('source_archive_uri', self.source_archive_uri))

        return delim.join(
                [template % (key, assign, val) for key, val in arg_values])


    def get_cmdline_args(self):
        return self._get_args('=', ' ', False)


    def get_code_args(self):
        args = self._get_args(' = ', '\n', True)
        return args + '\n' if args else ''
