# Copyright (c) 2015 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging
import os
import StringIO

SERVER_TEST_ROOT = os.path.dirname(__file__)
CLIENT_TEST_ROOT = '/usr/local/autotest/tests/native_Benchmarks'

def run_check(host, cmd, err_msg):
    """Run command on a host object.
    It checks and logs if error occurred.

    @param host: the host object
    @param cmd: the command to run
    @param err_msg: what to print when error occurred.
    @return: stdout of the cmd.
    """
    logging.info('(%s) Running: %s', host, cmd)
    stdout = StringIO.StringIO()
    stderr = StringIO.StringIO()
    try:
        result = host.run(cmd, stdout_tee=stdout, stderr_tee=stderr)
    except:
        logging.info('%s:\n%s\n%s\n', err_msg,
                                      stdout.getvalue(),
                                      stderr.getvalue())
        raise
    finally:
        stdout_str = stdout.getvalue()
        stdout.close()
        stderr.close()
    return stdout_str

def rcp_check(client, src, dst, err_msg):
    """Copy src on the running machine to dst on client.
    It checks and logs if error occurred.

    @param client: a host object representing client.
    @param src: path on the running machine.
    @param dst: path on client.
    @param err_msg: what to print when error occurred.
    """
    logging.info('Copying: %s -> %s', src, dst)
    try:
        client.send_file(src, dst)
    except:
        logging.info('%s: %s %s', err_msg, src, dst)
        raise

def def_flag(d, k, v):
    """Define a flag: k=v in d
    Warn if k is already in d.

    @param d: the flag dictionary
    @param k: key
    @param v: value
    """
    if k in d:
        logging.info('WARNING: Overriding flag %s: from %s to %s', k, d[k], v)
    d[k] = v
