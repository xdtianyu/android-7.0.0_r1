# Copyright 2014 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import gzip, logging, os, re
from autotest_lib.client.bin import utils
from autotest_lib.client.common_lib import error

class KernelConfig():
    """
    Parse the kernel config and enable us to query it.
    Used to verify the kernel config (see kernel_ConfigVerify).
    """

    def _passed(self, msg):
        logging.info('ok: %s', msg)

    def _failed(self, msg):
        logging.error('FAIL: %s', msg)
        self._failures.append(msg)

    def failures(self):
        """Return the list of failures that occured during the test.

        @return a list of string describing errors that occured since
                initialization.
        """
        return self._failures

    def _fatal(self, msg):
        logging.error('FATAL: %s', msg)
        raise error.TestError(msg)

    def get(self, key, default):
        """Get the value associated to key or default if it does not exist

        @param key: key to look for.
        @param default: value returned if key is not set in self._config
        """
        return self._config.get(key, default)

    def _config_required(self, name, wanted):
        value = self._config.get(name, None)
        if value in wanted:
            self._passed('"%s" was "%s" in kernel config' % (name, value))
        else:
            states = []
            for state in wanted:
                if state == None:
                    states.append("unset")
                else:
                    states.append(state)
            self._failed('"%s" was "%s" (wanted one of "%s") in kernel config' %
                         (name, value, '|'.join(states)))

    def has_value(self, name, value):
        """Determine if the name config item has a specific value.

        @param name: name of config item to test
        @param value: value expected for the given config name
        """
        self._config_required('CONFIG_%s' % (name), value)

    def has_builtin(self, name):
        """Check if the specific config item is built-in (present but not
        built as a module).

        @param name: name of config item to test
        """
        self.has_value(name, ['y'])

    def has_module(self, name):
        """Check if the specific config item is a module (present but not
        built-in).

        @param name: name of config item to test
        """
        self.has_value(name, ['m'])

    def is_enabled(self, name):
        """Check if the specific config item is present (either built-in or
        a module).

        @param name: name of config item to test
        """
        self.has_value(name, ['y', 'm'])

    def is_missing(self, name):
        """Check if the specific config item is not present (neither built-in
        nor a module).

        @param name: name of config item to test
        """
        self.has_value(name, [None])

    def is_exclusive(self, exclusive):
        """Given a config item regex, make sure only the expected items
        are present in the kernel configs.

        @param exclusive: hash containing "missing", "builtin", "module",
                          each to be checked with the corresponding has_*
                          function based on config items matching the
                          "regex" value.
        """
        expected = set()
        for name in exclusive['missing']:
            self.is_missing(name)
        for name in exclusive['builtin']:
            self.has_builtin(name)
            expected.add('CONFIG_%s' % (name))
        for name in exclusive['module']:
            self.has_module(name)
            expected.add('CONFIG_%s' % (name))

        # Now make sure nothing else with the specified regex exists.
        regex = r'CONFIG_%s' % (exclusive['regex'])
        for name in self._config:
            if not re.match(regex, name):
                continue
            if not name in expected:
                self._failed('"%s" found for "%s" when only "%s" allowed' %
                             (name, regex, "|".join(expected)))

    def _open_config(self):
        """Open the kernel's build config file. Attempt to use the built-in
        symbols from /proc first, then fall back to looking for a text file
        in /boot.

        @return fileobj for open config file
        """
        filename = '/proc/config.gz'
        if not os.path.exists(filename):
            utils.system("modprobe configs", ignore_status=True)
        if os.path.exists(filename):
            return gzip.open(filename, "r")

        filename = '/boot/config-%s' % utils.system_output('uname -r')
        if os.path.exists(filename):
            logging.info('Falling back to reading %s', filename)
            return file(filename, "r")

        self._fatal("Cannot locate suitable kernel config file")

    def initialize(self):
        """Load the kernel configuration and parse it.
        """
        fileobj = self._open_config()
        # Import kernel config variables into a dictionary for each searching.
        config = dict()
        for item in fileobj.readlines():
            item = item.strip()
            if not '=' in item:
                continue
            key, value = item.split('=', 1)
            config[key] = value

        # Make sure we actually loaded something sensible.
        if len(config) == 0:
            self._fatal('No CONFIG variables found!')

        self._config = config
        self._failures = []

