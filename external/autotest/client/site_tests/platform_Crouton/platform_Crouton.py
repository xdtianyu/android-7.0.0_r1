# Copyright 2014 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging
import os

from autotest_lib.client.bin import test, utils
from autotest_lib.client.common_lib import file_utils
from autotest_lib.client.common_lib import logging_manager


class platform_Crouton(test.test):
    """
    Tests crouton
    """
    version = 1


    def _parse_args(self, args):
        self._repo = "dnschneid/crouton"
        self._branch = "master"
        self._runargs = "00"
        self._env = ""

        for option_name, value in args.iteritems():
            if option_name == 'repo':
                self._repo = value
            elif option_name == 'branch':
                self._branch = value
            elif option_name == 'runargs':
                self._runargs = value
            elif option_name == 'env':
                self._env = value


    def run_once(self, args={}):
        self._parse_args(args)

        logging.info("Running crouton test:")
        logging.info(" - repo: %s", self._repo);
        logging.info(" - branch: %s", self._branch);
        logging.info(" - runargs: %s", self._runargs);
        logging.info(" - env:%s", self._env);
        logging.debug(" - resultsdir: %s", self.resultsdir)
        logging.debug(' - tmpdir: %s', self.tmpdir)

        crouton_temp_file = os.path.join(self.tmpdir, "archive.tar.gz")
        crouton_url = 'https://github.com/%s/archive/%s.tar.gz' \
                                            % (self._repo, self._branch)

        logging.info('Downloading crouton tarball: "%s".', crouton_url)
        file_utils.download_file(crouton_url, crouton_temp_file)

        os.chdir(self.tmpdir)
        utils.system('tar xvf %s --strip-components 1' % crouton_temp_file)

        # Set environment. Only allow setting CROUTON_MIRROR_* variables
        for env_pair in self._env.split(";"):
            keyval = env_pair.split("=")
            if len(keyval) == 2 and keyval[0].find("CROUTON_MIRROR_") == 0:
                logging.debug('Setting env %s=%s', keyval[0], keyval[1])
                os.environ[keyval[0]] = keyval[1]

        # Pass arguments separately to avoid problems with Little Bobby Tables.
        args = ['test/run.sh', '-l', self.resultsdir] + self._runargs.split()
        utils.run('sh', args=args,
                  timeout=None, ignore_status=False,
                  stderr_tee=logging_manager.LoggingFile(level=logging.INFO))


    def cleanup(self):
        # Reset hung task panic, see crbug.com/420094
        utils.system('echo 1 > /proc/sys/kernel/hung_task_panic')
