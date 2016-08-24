#!/usr/bin/python
# Copyright (c) 2010 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Utilities to test the autoupdate process.
"""

from autotest_lib.client.common_lib import error, utils
import logging, os, socket, subprocess, urllib2

DEVSERVER_PORT = 8080

CMD_TIMEOUT = 120

CWD = os.getcwd()
DEVSERVER_SRC = os.path.join('/home', os.environ['USER'], 'trunk',
                             'src', 'platform', 'dev')
DEVSERVER_LOG = os.path.join(CWD, 'devserver.log')

class AutoUpdateTester():

    def __init__(self):
        """Copy devserver source into current working directory.
        """
        self.devserver_url = 'http://%s:%s' % (socket.gethostname(),
                                               DEVSERVER_PORT)


    def is_devserver_running(self):
        try:
            resp = urllib2.urlopen(self.devserver_url)
        except urllib2.URLError:
            return False
        if resp is None:
            return False
        return True


    def start_devserver(self, image_path):
        """Start devserver
        """
        if self.is_devserver_running():
            logging.info('Devserver is already running')
            raise error.TestFail('Please kill devserver before running test.')

        logging.info('Starting devserver...')

        opts = '--image %s' % image_path
        cmd = 'python devserver.py %s >%s 2>&1 &' % (opts, DEVSERVER_LOG)
        logging.info('devserver cmd: %s' % cmd)

        try:
          subprocess.Popen(cmd, shell=True, cwd=DEVSERVER_SRC)
        except OSError, e:
          raise Exception('Could not start devserver: %s' % e.child_traceback)


    def kill_devserver(self):
        """Kill devserver.
        """
        logging.info('Killing devserver...')
        pkill_cmd = 'pkill -f devserver'
        subprocess.Popen(pkill_cmd, shell=True)


    def get_devserver_url(self):
        """Return devserver_url"""
        return self.devserver_url
