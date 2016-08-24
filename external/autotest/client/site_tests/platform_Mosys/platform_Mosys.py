# Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import re, errno, logging, utils
from autotest_lib.client.bin import test
from autotest_lib.client.common_lib import error

class platform_Mosys(test.test):
    version = 1

    def __TestAllLeafCommands(self):
        """Tests all "leaf" sub-commands return non-error.

        Commands that return ENOSYS or EINVAL are not counted as error.
        Return value are not check for correctness.

        Raises:
            error.TestFail Raised for commands that return non-zero exit code.
        """

        # Find all leaf commands by 'mosys -tv'.
        # Old mosys keeps track root:branch:node numbers; the output for one
        # command may look like, for example,
        #    [leaf 5:5] mosys platform variant
        # Latest mosys removes these numbers:
        #    [leaf] mosys platform variant
        cmd_re = re.compile('\[leaf[^\]]*\] (.+)')
        bad_cmd_list = []
        cmd_list = utils.system_output('mosys -tv')
        for line in cmd_list.splitlines():
            m = cmd_re.search(line)
            if m and not self.__TestOneCommand(m.group(1)):
                bad_cmd_list.append(m.group(1))

        if len(bad_cmd_list) == 1:
            raise error.TestFail('Command not properly implemented: ' +
                                 bad_cmd_list[0])
        elif len(bad_cmd_list) > 1:
            raise error.TestFail('Commands not properly implemented: ' +
                                 ','.join(bad_cmd_list))

    def __TestOneCommand(self, cmd):
        """ Tests one "leaf" sub-command.

        Returns
            True if the command returns 0, ENOSYS or EINVAL; False otherwise.
        """

        # Note that 'mosys eeprom map' takes about 30 sec to complete on Snow.
        # We set timeout=40 to accommodate that.
        result = utils.run(cmd, timeout=40, ignore_status=True)
        rc = result.exit_status
        if rc and rc not in [errno.ENOSYS, errno.EINVAL]:
            # older mosys does not return useful exit code but instead, prints
            # 'Command not supported on this platform' on stderr or prints
            # usage info on stdout when the command needs arguments. These are
            # not considered as error.
            stderr = result.stderr
            stdout = result.stdout
            not_supported = (stderr and
                stderr.startswith('Command not supported on this platform'))
            need_argument = stdout and stdout.startswith('usage:')
            unable_to_determine = (stderr and
                stderr.startswith('Unable to determine'))
            if not_supported:
                logging.info('cmd not supported: "%s"', cmd);
            elif need_argument:
                logging.info('cmd needs argument: "%s"', cmd);
            elif unable_to_determine:
                logging.info('Unable to determine: "%s"', cmd);
            else:
                logging.error('failed to execute "%s"; exit code=%d', cmd, rc);
                return False

        return True

    def run_once(self):
        self.__TestAllLeafCommands()
