#!/usr/bin/python
# Copyright 2014 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import errno
import json
import mmap
import optparse
import os
import signal
import sys
import syslog
import time

# The prefix of FIFO files used when using background processes.
RESULT_FIFO_PREFIX = '/tmp/update_engine_performance_monitor_fifo'

class UpdateEnginePerformanceMonitor(object):
    """Performance and resource usage monitor script.

    This script is intended to run on the DUT and will dump
    performance data as a JSON document when done. It can be run in
    the background using the --start-bg and --stop-bg options.
    """

    def __init__(self, verbose, timeout_seconds):
        """Instance initializer.

        @param verbose:  if True, prints debug info stderr.

        @param timeout_seconds: maximum amount of time to monitor for.
        """
        self.verbose = verbose
        self.timeout_seconds = timeout_seconds


    @staticmethod
    def get_update_engine_pids():
        """Gets all processes (tasks) in the update-engine cgroup.

        @return  a list of process identifiers.
        """
        with open('/sys/fs/cgroup/cpu/update-engine/tasks') as f:
            return [int(i) for i in f.read().split()]


    @staticmethod
    def get_info_for_pid(pid, pids_processed):
        """Get information about a process.

        The returned information is a tuple where the first element is
        the process name and the second element is the RSS size in
        bytes. The task and its siblings (e.g. tasks belonging to the
        same process) will be set in the |pids_processed| set.

        @param pid:            the task to get information about.

        @param pids_processed: set of process identifiers.

        @return                a tuple with information.
        """
        try:
            with open('/proc/%d/stat' % pid) as f:
                fields = f.read().split()
            # According to the proc(4) man page, field 23 is the
            # number of pages in the resident set.
            comm = fields[1]
            rss = int(fields[23]) * mmap.PAGESIZE
            tasks = os.listdir('/proc/%d/task'%pid)
            # Mark all tasks belonging to the process to avoid
            # double-counting their RSS.
            for t in tasks:
                pids_processed.add(int(t))
            return rss, comm
        except (IOError, OSError) as e:
            # It's possible that the task vanished in the window
            # between reading the 'tasks' file and when attempting to
            # read from it (ditto for iterating over the 'task'
            # directory). Handle this gracefully.
            if e.errno == errno.ENOENT:
                return 0, ''
            raise


    def do_sample(self):
        """Sampling method.

        This collects information about all the processes in the
        update-engine cgroup. The information is used to e.g. maintain
        historical peaks etc.
        """
        if self.verbose:
            sys.stderr.write('========================================\n')
        rss_total = 0
        pids = self.get_update_engine_pids()
        pids_processed = set()
        # Loop over all PIDs (tasks) in the update-engine cgroup and
        # be careful not to double-count PIDs (tasks) belonging to the
        # same process.
        for pid in pids:
            if pid not in pids_processed:
                rss, comm = self.get_info_for_pid(pid, pids_processed)
                rss_total += rss
                if self.verbose:
                    sys.stderr.write('pid %d %s -> %d KiB\n' %
                                     (pid, comm, rss/1024))
            else:
                if self.verbose:
                    sys.stderr.write('pid %d already counted\n' % pid)
        self.rss_peak = max(rss_total, self.rss_peak)
        if self.verbose:
            sys.stderr.write('Total = %d KiB\n' % (rss_total / 1024))
            sys.stderr.write('Peak  = %d KiB\n' % (self.rss_peak / 1024))


    def signal_handler(self, signal, frame):
        """Signal handler used to terminate monitoring.

        @param signal: the signal delivered.

        @param frame:  the interrupted stack frame.
        """
        self.request_exit = True


    def run(self, signum):
        """Main sampling loop.

        Periodically sample and process performance data until the
        signal specified by |signum| is sent to the
        process. Returns recorded data as a string.

        @param signum:  the signal to wait (e.g. signal.SIGTERM) or None.

        @return  a string with JSON data or None if the timeout
                 deadline has been exceeded.
        """
        if signum:
            signal.signal(signum, self.signal_handler)
        self.rss_peak = 0
        self.request_exit = False
        timeout_deadline = time.time() + self.timeout_seconds
        while not self.request_exit:
            monitor.do_sample()
            time.sleep(0.1)
            if time.time() > timeout_deadline:
                return None
        return json.dumps({'rss_peak': self.rss_peak})


class WriteToSyslog:
    """File-like object to log messages to syslog.

    Instances of this object can be assigned to e.g. sys.stderr to log
    errors/backtraces to syslog.
    """

    def __init__(self, ident):
        """Instance initializer.

        @param ident:  string to identify program by.
        """
        syslog.openlog(ident, syslog.LOG_PID, syslog.LOG_DAEMON)


    def write(self, data):
        """Overridden write() method.

        @param data:  the data to write.
        """
        syslog.syslog(syslog.LOG_ERR, data)


def daemonize_and_print_pid_on_stdout():
    """Daemonizes and prints the daemon process pid on stdout and
    exits.

    When this function returns, the process is a properly detached daemon
    process parented by pid 1. This is basically the standard double-fork
    daemonization dance as described in W. Richard Stevens, "Advanced
    Programming in the Unix Environment", 1992, Addison-Wesley, ISBN
    0-201-56317-7
    """
    first_child = os.fork()
    if first_child != 0:
        # Exit first child.
        sys.exit(0)
    os.chdir('/')
    os.setsid()
    os.umask(0)
    second_child = os.fork()
    if second_child != 0:
        # Parent, write child pid to stdout and exit.
        print second_child
        sys.exit(0)
    # Redirect native stdin, stdout, stderr file descriptors to /dev/null.
    si = open(os.devnull, 'r')
    so = open(os.devnull, 'a+')
    se = open(os.devnull, 'a+', 0)
    os.dup2(si.fileno(), sys.stdin.fileno())
    os.dup2(so.fileno(), sys.stdout.fileno())
    os.dup2(se.fileno(), sys.stderr.fileno())
    # Send stderr to syslog. Note that this will only work for Python
    # code in this process - it will not work for native code or child
    # processes. If this is ever needed, use subprocess.Popen() to
    # spawn logger(1) and connect its stdin fd with the stderr fd in
    # this process.
    sys.stderr = WriteToSyslog('update_engine_performance_monitor.py')


if __name__ == '__main__':
    parser = optparse.OptionParser()
    parser.add_option('-v', '--verbose', action='store_true',
                      dest='verbose', help='print debug info to stderr')
    parser.add_option('--timeout', action='store', type='int', default=3600,
                      dest='timeout_seconds', metavar='<SECONDS>',
                      help='maximum amount of time to monitor for')
    parser.add_option('--start-bg', action='store_true',
                      dest='start_bg', help='start background instance '
                      'and print its PID on stdout')
    parser.add_option('--stop-bg', action='store', type='int', default=0,
                      dest='stop_bg', metavar='<PID>',
                      help='stop running background instance and dump '
                      'its recorded data')
    (options, args) = parser.parse_args()

    monitor = UpdateEnginePerformanceMonitor(options.verbose,
                                             options.timeout_seconds)
    if options.start_bg:
        # If starting a background instance, fork a child and write
        # its PID on stdout in the parent process. In the child
        # process, setup a FIFO and monitor until SIGTERM is
        # called. When that happes, write the JSON result to the FIFO.
        #
        # Since this is expected to be called via ssh we need to
        # completely detach from the session - otherwise the remote
        # ssh(1) invocation will hang until our background instance is
        # gone.
        daemonize_and_print_pid_on_stdout()
        # Prepare the FIFO ahead of time since it'll serve as an extra
        # sanity check in --stop-bg before sending SIGTERM to the
        # given pid.
        instance_pid = os.getpid()
        fifo_path = RESULT_FIFO_PREFIX + ('-pid-%d' % instance_pid)
        if os.path.exists(fifo_path):
            os.unlink(fifo_path)
        os.mkfifo(fifo_path)
        # Now monitor.
        sys.stderr.write('Starting background collection.\n')
        json_str = monitor.run(signal.SIGTERM)
        sys.stderr.write('Stopping background collection.\n')
        if json_str:
            fifo = open(fifo_path, 'w')
            fifo.write(json_str)
            fifo.close()
        os.unlink(fifo_path)
    elif options.stop_bg:
        # If stopping a background instance, check that the FIFO is
        # really there and if so, signal the monitoring process and
        # wait for it to write the JSON result on the FIFO.
        instance_pid = options.stop_bg
        fifo_path = RESULT_FIFO_PREFIX + ('-pid-%d' % instance_pid)
        if not os.path.exists(fifo_path):
            sys.stderr.write('No instance with PID %d. Check syslog for '
                             'messages.\n' % instance_pid)
            sys.exit(1)
        os.kill(instance_pid, signal.SIGTERM)
        fifo = open(fifo_path, 'r')
        json_str = fifo.read()
        print json_str
        fifo.close()
    else:
        # Monitor in foreground until Ctrl+C is pressed, then dump
        # JSON on stdout. This is useful for hacking on this script,
        # especially in conjunction with --verbose.
        json_str = monitor.run(signal.SIGINT)
        if json_str:
            print json_str
