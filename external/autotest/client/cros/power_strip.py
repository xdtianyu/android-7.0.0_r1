# Copyright (c) 2010 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import telnetlib, threading, time

# Controls Server Technology CW-16V1-C20M switched CDUs over Telnet
# Opens a new connection for every command to
# avoid threading and address space conflicts

class PowerStrip():
    def __init__(self, host, user='admn', password='admn'):
        self.host = host
        self.user = user
        self.password = password

    def reboot(self, outlet, delay=0):
        self.command('reboot', outlet, delay)

    def off(self, outlet, delay=0):
        self.command('off', outlet, delay)

    def on(self, outlet, delay=0):
        self.command('on', outlet, delay)

    def command(self, command, outlet=1, delay=0):
        if delay == 0:
            self._do_command(command, outlet)
        else:
            threading.Timer(delay, self._do_command, (command, outlet)).start()

    def _do_command(self, command, outlet=1):
        tn = telnetlib.Telnet(self.host)
        tn.read_until('Username: ')
        tn.write(self.user + '\n')
        tn.read_until('Password: ')
        tn.write(self.password + '\n')
        tn.read_until('Switched CDU: ')
        tn.write('%s .a%d\n' % (command, outlet))
        tn.read_some()
        tn.close()
