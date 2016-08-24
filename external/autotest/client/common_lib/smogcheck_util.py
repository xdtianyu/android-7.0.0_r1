# Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""A Python utility library for TPM module testing."""

import logging, subprocess, time
from autotest_lib.client.common_lib import error


def runInSubprocess(args, rc_list=None):
    """Run a command in subprocess and return stdout.

    Args:
      args: a list of string, command to run.
      rc_list: a list of int, acceptable return code values.

    Returns:
      out: a string, stdout of the command executed.
      err: a string, stderr of the command executed, or None.

    Raises:
      RuntimeError: if subprocess return code is non-zero and not in rc_list.
    """
    if rc_list is None:
        rc_list = []

    # Sleep for 1 second so we don't overwhelm I2C bus with too many commands
    time.sleep(1)
    logging.debug('runInSubprocess args = %r; rc_list = %r', args, rc_list)
    proc = subprocess.Popen(args,
                            stdout=subprocess.PIPE,
                            stderr=subprocess.PIPE)
    out, err = proc.communicate()
    logging.error('runInSubprocess %s: out=%r, err=%r', args[0], out, err)
    if proc.returncode and proc.returncode not in rc_list:
        raise RuntimeError('runInSubprocess %s failed with returncode %d: %s' %
                           (args[0], proc.returncode, out))
    return str(out), str(err)


def enableI2C():
    """Enable i2c-dev so i2c-tools can be used.

    Dependency: 'i2cdetect' is a command from 'i2c-tools' package, which comes
                with Chrom* OS image and is available from inside chroot.

    Raises:
      TestFail: if i2c-dev can't be enabled.
    """
    args = ['i2cdetect', '-l']
    out, _ = runInSubprocess(args)
    if not out:
        logging.info('i2c-dev disabled. Enabling it with modprobe')
        out, _ = runInSubprocess(['modprobe', 'i2c-dev'])
        if out:
            raise error.TestFail('Error enable i2c-dev: %s' % out)
        out, _ = runInSubprocess(args)
    logging.info('i2c-dev ready to go:\n%s', out)


def computeTimeElapsed(end, start):
    """Computes time difference in microseconds.

    Args:
      end: a datetime.datetime() object, end timestamp.
      start: a datetime.datetime() object, start timestamp.

    Returns:
      usec: an int, difference between end and start in microseconds.
    """
    t = end - start
    usec = 1000000 * t.seconds + t.microseconds
    logging.info('Elapsed time = %d usec', usec)
    return usec
