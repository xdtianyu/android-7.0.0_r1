# Copyright (c) 2010,2013 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging
from autotest_lib.client.bin import test, utils
from autotest_lib.client.common_lib import error

class kernel_TPMPing(test.test):
  """See control file for doc"""
  version = 2

  def run_once(self):
    tpm_version = utils.system_output("tpm_version")
    if tpm_version.find("Version Info") == -1:
      raise error.TestFail("Invalid tpm_version output:\n%s\n" % tpm_version)
    else:
      logging.info(tpm_version)

    # This autotest is not compatible with kernel version < 3.8
    version = utils.system_output('/bin/uname -r').strip()
    logging.info(version)

    # If the "[gentle shutdown]" string  followed by 'Linux Version'
    # is missing from the /var/log/messages,
    # we forgot to carry over an important patch.
    if version >= '3.8':
      result = utils.system_output('awk \'/Linux version [0-9]+\./ '
                                   '{gentle=0;} /\[gentle shutdown\]/ '
                                   '{gentle=1;} END {print gentle}\' '
                                   '$(ls -t /var/log/messages* | tac)',
                                    ignore_status=True)

      # We only care about the most recent instance of the TPM driver message.
      if result == '0':
        raise error.TestFail('no \'gentle shutdown\' TPM driver init message')
    else:
      logging.info('Bypassing the test as kernel version is < 3.8')
