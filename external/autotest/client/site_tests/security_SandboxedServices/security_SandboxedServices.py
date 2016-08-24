# Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import csv
import getopt
import logging
import os
import re

from collections import namedtuple

from autotest_lib.client.bin import test
from autotest_lib.client.common_lib import error
from autotest_lib.client.common_lib import utils


PS_FIELDS = "pid,ppid,comm:32,euser:%d,ruser:%d,args"
PsOutput = namedtuple("PsOutput",
                      ' '.join([field.split(':')[0]
                                for field in PS_FIELDS.split(',')]))

MINIJAIL_OPTS = { "mj_uid": "-u",
                  "mj_gid": "-g",
                  "mj_pidns": "-p",
                  "mj_caps": "-c",
                  "mj_filter": "-S" }


class security_SandboxedServices(test.test):
    """Enforces sandboxing restrictions on the processes running
    on the system.
    """

    version = 1


    def get_minijail_opts(self):
        """Parses Minijail's help and generates a getopt string.
        """

        help = utils.system_output("minijail0 -h", ignore_status=True)
        help_lines = help.splitlines()[1:]

        opt_list = []

        for line in help_lines:
            # Example lines:
            #     '  -c <caps>:  restrict caps to <caps>'
            #     '  -s:         use seccomp'
            m = re.search("-(\w)( <.+>)?:", line)

            if m:
                opt_list.append(m.groups()[0])

                if m.groups()[1]:
                    # The option takes an argument
                    opt_list.append(':')

        return ''.join(opt_list)


    def get_running_processes(self):
        """Returns a list of running processes as PsOutput objects."""

        usermax = utils.system_output("cut -d: -f1 /etc/passwd | wc -L",
                                      ignore_status=True)
        usermax = max(int(usermax), 8)
        ps_cmd = "ps --no-headers -ww -eo " + (PS_FIELDS % (usermax, usermax))
        ps_fields_len = len(PS_FIELDS.split(','))

        output = utils.system_output(ps_cmd)
        # crbug.com/422700: Filter out zombie processes.
        running_processes = [PsOutput(*line.split(None, ps_fields_len - 1))
                             for line in output.splitlines()
                             if "<defunct>" not in line]
        return running_processes


    def load_baseline(self):
        """The baseline file lists the services we know and
        whether (and how) they are sandboxed.
        """

        baseline_path = os.path.join(self.bindir, 'baseline')
        dict_reader = csv.DictReader(open(baseline_path))
        return dict([(d["exe"], d) for d in dict_reader])


    def load_exclusions(self):
        """The exclusions file lists running programs
        that we don't care about (for now).
        """

        exclusions_path = os.path.join(self.bindir, 'exclude')
        return set([line.strip() for line in open(exclusions_path)])


    def minijail_ok(self, launcher, expected):
        """Checks whether the Minijail invocation
        has the correct command-line options.

        @param launcher: Minijail command line for the process.
        @param expected: Sandboxing restrictions expected.
        """

        opts, args = getopt.getopt(launcher.args.split()[1:],
                                   self.get_minijail_opts())
        optset = set([opt[0] for opt in opts])

        missing_opts = []
        new_opts = []

        for check, opt in MINIJAIL_OPTS.iteritems():
            if expected[check] == "Yes":
                if opt not in optset:
                    missing_opts.append(check)
            elif expected[check] == "No":
                if opt in optset:
                    new_opts.append(check)

        if len(new_opts) > 0:
            logging.error("New Minijail opts for '%s': %s",
                          expected["exe"], ', '.join(new_opts))

        if len(missing_opts) > 0:
            logging.error("Missing Minijail options for '%s': %s",
                          expected["exe"], ', '.join(missing_opts))

        return (len(new_opts) + len(missing_opts)) == 0


    def dump_services(self, running_services, minijail_processes):
        """Leaves a list of running services in the results dir
        so that we can update the baseline file if necessary.

        @param running_services: list of services to be logged.
        @param minijail_processes: list of Minijail processes used to log how
        each running service is sandboxed.
        """

        csv_file = csv.writer(open(os.path.join(self.resultsdir,
                                                "running_services"), 'w'))

        for service in running_services:
            service_minijail = ""

            if service.ppid in minijail_processes:
                launcher = minijail_processes[service.ppid]
                service_minijail = launcher.args.split("--")[0].strip()

            row = [service.comm, service.euser, service.args, service_minijail]
            csv_file.writerow(row)


    def log_process_list(self, logger, title, list):
        report = "%s: %s" % (title, ', '.join(list))
        logger(report)


    def log_process_list_warn(self, title, list):
        self.log_process_list(logging.warn, title, list)


    def log_process_list_error(self, title, list):
        self.log_process_list(logging.error, title, list)


    def run_once(self):
        """Inspects the process list, looking for root and sandboxed processes
        (with some exclusions). If we have a baseline entry for a given process,
        confirms it's an exact match. Warns if we see root or sandboxed
        processes that we have no baseline for, and warns if we have
        baselines for processes not seen running.
        """

        baseline = self.load_baseline()
        exclusions = self.load_exclusions()
        running_processes = self.get_running_processes()

        kthreadd_pid = -1

        running_services = {}
        minijail_processes = {}

        # Filter running processes list
        for process in running_processes:
            exe = process.comm

            if exe == "kthreadd":
                kthreadd_pid = process.pid
                continue

            # Don't worry about kernel threads
            if process.ppid == kthreadd_pid:
                continue

            if exe in exclusions:
                continue

            # Remember minijail0 invocations
            if exe == "minijail0":
                minijail_processes[process.pid] = process
                continue

            running_services[exe] = process

        # Find differences between running services and baseline
        services_set = set(running_services.keys())
        baseline_set = set(baseline.keys())

        new_services = services_set.difference(baseline_set)
        stale_baselines = baseline_set.difference(services_set)

        # Check baseline
        sandbox_delta = []
        for exe in services_set.intersection(baseline_set):
            process = running_services[exe]

            # If the process is not running as the correct user
            if process.euser != baseline[exe]["euser"]:
                sandbox_delta.append(exe)
                continue

            # If this process is supposed to be sandboxed
            if baseline[exe]["mj_uid"] == "Yes":
                # If it's not being launched from Minijail,
                # it's not sandboxed wrt the baseline.
                if process.ppid not in minijail_processes:
                    sandbox_delta.append(exe)
                else:
                    launcher = minijail_processes[process.ppid]
                    expected = baseline[exe]
                    if not self.minijail_ok(launcher, expected):
                        sandbox_delta.append(exe)

        # Save current run to results dir
        self.dump_services(running_services.values(), minijail_processes)

        if len(stale_baselines) > 0:
            self.log_process_list_warn("Stale baselines", stale_baselines)

        if len(new_services) > 0:
            self.log_process_list_warn("New services", new_services)

        if len(sandbox_delta) > 0:
            self.log_process_list_error("Failed sandboxing", sandbox_delta)
            raise error.TestFail("One or more processes failed sandboxing")
