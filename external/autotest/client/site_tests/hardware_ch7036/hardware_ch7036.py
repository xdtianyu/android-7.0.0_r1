# -*- coding: utf-8 -*-
# Copyright (c) 2010 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.


import logging
import os
import re
import time

from autotest_lib.client.bin import test
from autotest_lib.client.common_lib import error, utils


_DPMS_DOWN = ['standby', 'suspend', 'stop']
_LOG_CHECKLIST = ['Found device', 'CH7036 MCU ver']
_SERVER = 'chrontel'
_XENV = 'LD_LIBRARY_PATH=/usr/local/lib ' + \
    'XAUTHORITY=/var/run/chromelogin.auth DISPLAY=:0'


def run_cmd(cmd):
    try:
        cmd_out = utils.system_output(cmd, retain_output=True)
    except error.CmdError, e:
        logging.debug(e)
        raise error.TestFail(cmd)
    return cmd_out


def check_server(server):
    ups_str = run_cmd('initctl status %s' % (server))
    ups_list = ups_str.strip().split()
    if ups_list[1] != 'start/running,':
        raise error.TestFail('%s not running :: %s' % (server, ups_list[1]))
    return ups_list[3]


def stress_server():
    # Make sure it responds favorably to DPMS events
    utils.assert_has_X_server()
    for dpms_verb in _DPMS_DOWN:
        run_cmd("%s xset dpms force %s" % (_XENV, dpms_verb))
        time.sleep(5)
        run_cmd("%s xset dpms force on" % (_XENV))
        time.sleep(5)


def check_log():
    # two listings (stdout, stderr) + header
    lsof_out = run_cmd('lsof -c /ch7036_monitor/ ' + \
                           '-a -u root -a +D /var/log | tail -1')
    if not lsof_out:
        raise error.TestFail('Unable to locate logfile in lsof output')

    lsof_list = lsof_out.rstrip().split()
    log = lsof_list[8]
    log.rstrip()
    logging.debug('log = %s' % (log))
    if not os.path.isfile(log):
        raise error.TestFail('no log at %s' % (log))

    fd = open(log)
    found = dict((k, False) for k in _LOG_CHECKLIST)
    found_cnt = 0
    for ln in fd:
        ln.rstrip()
        for k in found:
            if re.search(k, ln, re.I):
                logging.debug(ln)
                found[k] = True
                found_cnt += 1
        if found_cnt == len(found):
            break

    if found_cnt < len(found):
        errs = ''
        for k in found:
            if not found[k]:
                errs += "%s " % (k)
        raise error.TestFail('Failed to validate log for %s' % (errs))
    fd.close()
    return log


class hardware_ch7036(test.test):
    version = 1


    def run_once(self):
        pid1 = check_server(_SERVER)
        stress_server()
        self._log = check_log()
        pid2 = check_server(_SERVER)
        logging.debug("pids %s %s" % (pid1, pid2))
        if pid1 != pid2:
            raise error.TestFail('Appears server %s restarted (%s != %s)' %
                                 (_SERVER, pid1, pid2))
