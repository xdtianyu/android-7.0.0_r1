# Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.


'''
A library to prespawn autotest processes to minimize startup overhead.
'''

import cPickle as pickle, os, sys
from setproctitle import setproctitle


if len(sys.argv) == 2 and sys.argv[1] == '--prespawn_autotest':
    # Run an autotest process, and on stdin, wait for a pickled environment +
    # argv (as a tuple); see spawn() below.  Once we receive these, start
    # autotest.

    # Do common imports (to save startup time).
    # pylint: disable=W0611
    import common
    import autotest_lib.client.bin.job

    if os.environ.get('CROS_DISABLE_SITE_SYSINFO'):
        from autotest_lib.client.bin import sysinfo, base_sysinfo
        sysinfo.sysinfo = autotest_lib.client.bin.base_sysinfo.base_sysinfo

    # Wait for environment and autotest arguments.
    env, sys.argv = pickle.load(sys.stdin)
    # Run autotest and exit.
    if env:
        os.environ.clear()
        os.environ.update(env)
        proc_title = os.environ.get('CROS_PROC_TITLE')
        if proc_title:
            setproctitle(proc_title)

        execfile('autotest')
    sys.exit(0)


import logging, subprocess, threading
from Queue import Queue


NUM_PRESPAWNED_PROCESSES = 1


class Prespawner():
    def __init__(self):
        self.prespawned = Queue(NUM_PRESPAWNED_PROCESSES)
        self.thread = None
        self.terminated = False

    def spawn(self, args, env_additions=None):
        '''
        Spawns a new autotest (reusing an prespawned process if available).

        @param args: A list of arguments (sys.argv)
        @param env_additions: Items to add to the current environment
        '''
        new_env = dict(os.environ)
        if env_additions:
            new_env.update(env_additions)

        process = self.prespawned.get()
        # Write the environment and argv to the process's stdin; it will launch
        # autotest once these are received.
        pickle.dump((new_env, args), process.stdin, protocol=2)
        process.stdin.close()
        return process

    def start(self):
        '''
        Starts a thread to pre-spawn autotests.
        '''
        def run():
            while not self.terminated:
                process = subprocess.Popen(
                    ['python', '-u', os.path.realpath(__file__),
                     '--prespawn_autotest'],
                    cwd=os.path.dirname(os.path.realpath(__file__)),
                    stdin=subprocess.PIPE)
                logging.debug('Pre-spawned an autotest process %d', process.pid)
                self.prespawned.put(process)

            # Let stop() know that we are done
            self.prespawned.put(None)

        if not self.thread:
            self.thread = threading.Thread(target=run, name='Prespawner')
            self.thread.start()

    def stop(self):
        '''
        Stops the pre-spawn thread gracefully.
        '''
        if not self.thread:
            # Never started
            return

        self.terminated = True
        # Wait for any existing prespawned processes.
        while True:
            process = self.prespawned.get()
            if not process:
                break
            # Send a 'None' environment and arg list to tell the prespawner
            # processes to exit.
            pickle.dump((None, None), process.stdin, protocol=2)
            process.stdin.close()
            process.wait()
        self.thread = None
