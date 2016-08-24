# Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging
import math
import re

from autotest_lib.client.bin import utils
from autotest_lib.client.common_lib import error


class PingConfig(object):
    """Describes the parameters for a ping command."""

    DEFAULT_COUNT = 10
    PACKET_WAIT_MARGIN_SECONDS = 120

    @property
    def ping_args(self):
        """@return list of parameters to ping."""
        args = []
        args.append('-c %d' % self.count)
        if self.size is not None:
            args.append('-s %d' % self.size)
        if self.interval is not None:
            args.append('-i %f' % self.interval)
        if self.qos is not None:
            if self.qos == 'be':
                args.append('-Q 0x04')
            elif self.qos == 'bk':
                args.append('-Q 0x02')
            elif self.qos == 'vi':
                args.append('-Q 0x08')
            elif self.qos == 'vo':
                args.append('-Q 0x10')
            else:
                raise error.TestFail('Unknown QoS value: %s' % self.qos)

        # The last argument is the IP addres to ping.
        args.append(self.target_ip)
        return args


    def __init__(self, target_ip, count=DEFAULT_COUNT, size=None,
                 interval=None, qos=None,
                 ignore_status=False, ignore_result=False):
        super(PingConfig, self).__init__()
        self.target_ip = target_ip
        self.count = count
        self.size = size
        self.interval = interval
        if qos:
            qos = qos.lower()
        self.qos = qos
        self.ignore_status = ignore_status
        self.ignore_result = ignore_result
        interval_seconds = self.interval or 1
        command_time = math.ceil(interval_seconds * self.count)
        self.command_timeout_seconds = int(command_time +
                                           self.PACKET_WAIT_MARGIN_SECONDS)


class PingResult(object):
    """Represents a parsed ping command result.

    On error, some statistics may be missing entirely from the output.

    An example of output with some errors is:

    PING 192.168.0.254 (192.168.0.254) 56(84) bytes of data.
    From 192.168.0.124 icmp_seq=1 Destination Host Unreachable
    From 192.168.0.124 icmp_seq=2 Destination Host Unreachable
    From 192.168.0.124 icmp_seq=3 Destination Host Unreachable
    64 bytes from 192.168.0.254: icmp_req=4 ttl=64 time=1171 ms
    [...]
    64 bytes from 192.168.0.254: icmp_req=10 ttl=64 time=1.95 ms

    --- 192.168.0.254 ping statistics ---
    10 packets transmitted, 7 received, +3 errors, 30% packet loss, time 9007ms
    rtt min/avg/max/mdev = 1.806/193.625/1171.174/403.380 ms, pipe 3

    A more normal run looks like:

    PING google.com (74.125.239.137) 56(84) bytes of data.
    64 bytes from 74.125.239.137: icmp_req=1 ttl=57 time=1.77 ms
    64 bytes from 74.125.239.137: icmp_req=2 ttl=57 time=1.78 ms
    [...]
    64 bytes from 74.125.239.137: icmp_req=5 ttl=57 time=1.79 ms

    --- google.com ping statistics ---
    5 packets transmitted, 5 received, 0% packet loss, time 4007ms
    rtt min/avg/max/mdev = 1.740/1.771/1.799/0.042 ms

    We also sometimes see result lines like:
    9 packets transmitted, 9 received, +1 duplicates, 0% packet loss, time 90 ms

    """

    @staticmethod
    def _regex_int_from_string(regex, value):
        m = re.search(regex, value)
        if m is None:
            return None

        return int(m.group(1))


    @staticmethod
    def parse_from_output(ping_output):
        """Construct a PingResult from ping command output.

        @param ping_output string stdout from a ping command.

        """
        loss_line = (filter(lambda x: x.find('packets transmitted') > 0,
                            ping_output.splitlines()) or [''])[0]
        sent = PingResult._regex_int_from_string('([0-9]+) packets transmitted',
                                                 loss_line)
        received = PingResult._regex_int_from_string('([0-9]+) received',
                                                     loss_line)
        loss = PingResult._regex_int_from_string('([0-9]+)% packet loss',
                                                 loss_line)
        if None in (sent, received, loss):
            raise error.TestFail('Failed to parse transmission statistics.')

        m = re.search('(round-trip|rtt) min[^=]*= '
                      '([0-9.]+)/([0-9.]+)/([0-9.]+)/([0-9.]+)', ping_output)
        if m is not None:
            return PingResult(sent, received, loss,
                              min_latency=float(m.group(2)),
                              avg_latency=float(m.group(3)),
                              max_latency=float(m.group(4)),
                              dev_latency=float(m.group(5)))
        if received > 0:
            raise error.TestFail('Failed to parse latency statistics.')

        return PingResult(sent, received, loss)


    def __init__(self, sent, received, loss,
                 min_latency=-1.0, avg_latency=-1.0,
                 max_latency=-1.0, dev_latency=-1.0):
        """Construct a PingResult.

        @param sent: int number of packets sent.
        @param received: int number of replies received.
        @param loss: int loss as a percentage (0-100)
        @param min_latency: float min response latency in ms.
        @param avg_latency: float average response latency in ms.
        @param max_latency: float max response latency in ms.
        @param dev_latency: float response latency deviation in ms.

        """
        super(PingResult, self).__init__()
        self.sent = sent
        self.received = received
        self.loss = loss
        self.min_latency = min_latency
        self.avg_latency = avg_latency
        self.max_latency = max_latency
        self.dev_latency = dev_latency


    def __repr__(self):
        return '%s(%s)' % (self.__class__.__name__,
                           ', '.join(['%s=%r' % item
                                      for item in vars(self).iteritems()]))


class PingRunner(object):
    """Delegate to run the ping command on a local or remote host."""
    DEFAULT_PING_COMMAND = 'ping'
    PING_LOSS_THRESHOLD = 20  # A percentage.


    def __init__(self, command_ping=DEFAULT_PING_COMMAND, host=None):
        """Construct a PingRunner.

        @param command_ping optional path or alias of the ping command.
        @param host optional host object when a remote host is desired.

        """
        super(PingRunner, self).__init__()
        self._run = utils.run
        if host is not None:
            self._run = host.run
        self.command_ping = command_ping


    def simple_ping(self, host_name):
        """Quickly test that a hostname or IPv4 address responds to ping.

        @param host_name: string name or IPv4 address.
        @return True iff host_name responds to at least one ping.

        """
        ping_config = PingConfig(host_name, count=3,
                                 interval=0.5, ignore_result=True,
                                 ignore_status=True)
        ping_result = self.ping(ping_config)
        if ping_result is None or ping_result.received == 0:
            return False
        return True


    def ping(self, ping_config):
        """Run ping with the given |ping_config|.

        Will assert that the ping had reasonable levels of loss unless
        requested not to in |ping_config|.

        @param ping_config PingConfig object describing the ping to run.

        """
        command_pieces = [self.command_ping] + ping_config.ping_args
        command = ' '.join(command_pieces)
        command_result = self._run(command,
                                   timeout=ping_config.command_timeout_seconds,
                                   ignore_status=True,
                                   ignore_timeout=True)
        if not command_result:
            if ping_config.ignore_status:
                logging.warning('Ping command timed out; cannot parse output.')
                return PingResult(ping_config.count, 0, 100)

            raise error.TestFail('Ping command timed out unexpectedly.')

        if not command_result.stdout:
            logging.warning('Ping command returned no output; stderr was %s.',
                            command_result.stderr)
            if ping_config.ignore_result:
                return PingResult(ping_config.count, 0, 100)
            raise error.TestFail('Ping command failed to yield any output')

        if command_result.exit_status and not ping_config.ignore_status:
            raise error.TestFail('Ping command failed with code=%d' %
                                 command_result.exit_status)

        ping_result = PingResult.parse_from_output(command_result.stdout)
        if ping_config.ignore_result:
            return ping_result

        if ping_result.loss > self.PING_LOSS_THRESHOLD:
            raise error.TestFail('Lost ping packets: %r.' % ping_result)

        logging.info('Ping successful.')
        return ping_result
