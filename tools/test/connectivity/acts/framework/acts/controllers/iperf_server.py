#!/usr/bin/env python3.4
#
#   Copyright 2016 - The Android Open Source Project
#
#   Licensed under the Apache License, Version 2.0 (the "License");
#   you may not use this file except in compliance with the License.
#   You may obtain a copy of the License at
#
#       http://www.apache.org/licenses/LICENSE-2.0
#
#   Unless required by applicable law or agreed to in writing, software
#   distributed under the License is distributed on an "AS IS" BASIS,
#   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#   See the License for the specific language governing permissions and
#   limitations under the License.

import os
from signal import SIGTERM
import subprocess

from acts.utils import create_dir
from acts.utils import start_standing_subprocess
from acts.utils import stop_standing_subprocess

ACTS_CONTROLLER_CONFIG_NAME = "IPerfServer"
ACTS_CONTROLLER_REFERENCE_NAME = "iperf_servers"

def create(configs, logger):
    log_path = os.path.dirname(logger.handlers[1].baseFilename)
    results = []
    for c in configs:
        try:
            results.append(IPerfServer(c, log_path))
        except:
            pass
    return results

def destroy(objs):
    for ipf in objs:
        try:
            ipf.stop()
        except:
            pass

class IPerfServer():
    """Class that handles iperf3 operations.
    """
    def __init__(self, port, log_path):
        self.port = port
        self.log_path = os.path.join(os.path.expanduser(log_path), "iPerf")
        self.iperf_str = "iperf3 -s -p {}".format(port)
        self.iperf_process = None
        self.exec_count = 0
        self.started = False

    def start(self, extra_args="", tag=""):
        """Starts iperf server on specified port.

        Args:
            extra_args: A string representing extra arguments to start iperf
                server with.
            tag: Appended to log file name to identify logs from different
                iperf runs.
        """
        if self.started:
            return
        create_dir(self.log_path)
        self.exec_count += 1
        if tag:
            tag = tag + ','
        out_file_name = "IPerfServer,{},{}{}.log".format(self.port, tag,
            self.exec_count)
        full_out_path = os.path.join(self.log_path, out_file_name)
        cmd = "{} {} > {}".format(self.iperf_str, extra_args, full_out_path)
        self.iperf_process = start_standing_subprocess(cmd)
        self.started = True

    def stop(self):
        if self.started:
            stop_standing_subprocess(self.iperf_process)
            self.started = False
