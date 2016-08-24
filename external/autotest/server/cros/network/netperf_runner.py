# Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import collections
import logging
import math
import numbers
import time
import os.path

from autotest_lib.client.common_lib import error
from autotest_lib.client.common_lib.cros import path_utils


class NetperfResult(object):
    """Encapsulates logic to parse and represent netperf results."""

    @staticmethod
    def from_netperf_results(test_type, results, duration_seconds):
        """Parse the text output of netperf and return a NetperfResult.

        @param test_type string one of NetperfConfig.TEST_TYPE_* below.
        @param results string raw results from netperf.
        @param duration_seconds float number of seconds the test ran for.
        @return NetperfResult result.

        """
        lines = results.splitlines()
        if test_type in NetperfConfig.TCP_STREAM_TESTS:
            """Parses the following (works for both TCP_STREAM, TCP_MAERTS and
            TCP_SENDFILE) and returns a singleton containing throughput.

            TCP STREAM TEST from 0.0.0.0 (0.0.0.0) port 0 AF_INET to \
            foo.bar.com (10.10.10.3) port 0 AF_INET
            Recv   Send    Send
            Socket Socket  Message  Elapsed
            Size   Size    Size     Time     Throughput
            bytes  bytes   bytes    secs.    10^6bits/sec

            87380  16384  16384    2.00      941.28
            """
            if len(lines) < 7:
                return None

            result = NetperfResult(test_type, duration_seconds,
                                   throughput=float(lines[6].split()[4]))
        elif test_type in NetperfConfig.UDP_STREAM_TESTS:
            """Parses the following and returns a tuple containing throughput
            and the number of errors.

            UDP UNIDIRECTIONAL SEND TEST from 0.0.0.0 (0.0.0.0) port 0 AF_INET \
            to foo.bar.com (10.10.10.3) port 0 AF_INET
            Socket  Message  Elapsed      Messages
            Size    Size     Time         Okay Errors   Throughput
            bytes   bytes    secs            #      #   10^6bits/sec

            129024   65507   2.00         3673      0     961.87
            131072           2.00         3673            961.87
            """
            if len(lines) < 6:
                return None

            udp_tokens = lines[5].split()
            result = NetperfResult(test_type, duration_seconds,
                                   throughput=float(udp_tokens[5]),
                                   errors=float(udp_tokens[4]))
        elif test_type in NetperfConfig.REQUEST_RESPONSE_TESTS:
            """Parses the following which works for both rr (TCP and UDP)
            and crr tests and returns a singleton containing transfer rate.

            TCP REQUEST/RESPONSE TEST from 0.0.0.0 (0.0.0.0) port 0 AF_INET \
            to foo.bar.com (10.10.10.3) port 0 AF_INET
            Local /Remote
            Socket Size   Request  Resp.   Elapsed  Trans.
            Send   Recv   Size     Size    Time     Rate
            bytes  Bytes  bytes    bytes   secs.    per sec

            16384  87380  1        1       2.00     14118.53
            16384  87380
            """
            if len(lines) < 7:
                return None

            result = NetperfResult(test_type, duration_seconds,
                                   transaction_rate=float(lines[6].split()[5]))
        else:
            raise error.TestFail('Invalid netperf test type: %r.' % test_type)

        logging.info('%r', result)
        return result


    @staticmethod
    def _get_stats(samples, field_name):
        if any(map(lambda x: getattr(x, field_name) is None, samples)):
            return (None, None)

        values = map(lambda x: getattr(x, field_name), samples)
        N = len(samples)
        mean = math.fsum(values) / N
        deviation = None
        if N > 1:
            differences = map(lambda x: math.pow(mean - x, 2), values)
            deviation = math.sqrt(math.fsum(differences) / (N - 1))
        return mean, deviation


    @staticmethod
    def from_samples(samples):
        """Build an averaged NetperfResult from |samples|.

        Calculate an representative sample with averaged values
        and standard deviation from samples.

        @param samples list of NetperfResult objects.
        @return NetperfResult object.

        """
        if len(set([x.test_type for x in samples])) != 1:
            # We have either no samples or multiple test types.
            return None

        duration_seconds, duration_seconds_dev = NetperfResult._get_stats(
                samples, 'duration_seconds')
        throughput, throughput_dev = NetperfResult._get_stats(
                samples, 'throughput')
        errors, errors_dev = NetperfResult._get_stats(samples, 'errors')
        transaction_rate, transaction_rate_dev = NetperfResult._get_stats(
                samples, 'transaction_rate')
        return NetperfResult(
                samples[0].test_type,
                duration_seconds, duration_seconds_dev=duration_seconds_dev,
                throughput=throughput, throughput_dev=throughput_dev,
                errors=errors, errors_dev=errors_dev,
                transaction_rate=transaction_rate,
                transaction_rate_dev=transaction_rate_dev)


    @property
    def human_readable_tag(self):
        """@return string human readable test description."""
        return NetperfConfig.test_type_to_human_readable_tag(self.test_type)


    @property
    def tag(self):
        """@return string very short test description."""
        return NetperfConfig.test_type_to_tag(self.test_type)


    def __init__(self, test_type, duration_seconds, duration_seconds_dev=None,
                 throughput=None, throughput_dev=None,
                 errors=None, errors_dev=None,
                 transaction_rate=None, transaction_rate_dev=None):
        """Construct a NetperfResult.

        @param duration_seconds float how long the test took.
        @param throughput float test throughput in Mbps.
        @param errors int number of UDP errors in test.
        @param transaction_rate float transactions per second.

        """
        self.test_type = test_type
        self.duration_seconds = duration_seconds
        self.duration_seconds_dev = duration_seconds_dev
        self.throughput = throughput
        self.throughput_dev = throughput_dev
        self.errors = errors
        self.errors_dev = errors_dev
        self.transaction_rate = transaction_rate
        self.transaction_rate_dev = transaction_rate_dev
        if throughput is None and transaction_rate is None and errors is None:
            logging.error('Created a NetperfResult with no data.')


    def __repr__(self):
        fields = ['test_type=%s' % self.test_type]
        fields += ['%s=%0.2f' % item
                   for item in vars(self).iteritems()
                   if item[1] is not None
                   and isinstance(item[1], numbers.Number)]
        return '%s(%s)' % (self.__class__.__name__, ', '.join(fields))


    def all_deviations_less_than_fraction(self, fraction):
        """Check that this result is "acurate" enough.

        We say that a NetperfResult is "acurate" enough when for each
        measurement X with standard deviation d(X), d(X)/X <= |fraction|.

        @param fraction float used in constraint above.
        @return True on above condition.

        """
        for measurement in ['throughput', 'errors', 'transaction_rate']:
            value = getattr(self, measurement)
            dev = getattr(self, measurement + '_dev')
            if value is None or dev is None:
                continue

            if not dev and not value:
                # 0/0 is undefined, but take this to be good for our purposes.
                continue

            if dev and not value:
                # Deviation is non-zero, but the average is 0.  Deviation
                # as a fraction of the value is undefined but in theory
                # a "very large number."
                return False

            if dev / value > fraction:
                return False

        return True


    def get_keyval(self, prefix='', suffix=''):
        ret = {}
        if prefix:
            prefix = prefix + '_'
        if suffix:
            suffix = '_' + suffix

        for measurement in ['throughput', 'errors', 'transaction_rate']:
            value = getattr(self, measurement)
            dev = getattr(self, measurement + '_dev')
            if dev is None:
                margin = ''
            else:
                margin = '+-%0.2f' % dev
            if value is not None:
                ret[prefix + measurement + suffix] = '%0.2f%s' % (value, margin)
        return ret


class NetperfAssertion(object):
    """Defines a set of expectations for netperf results."""

    def _passes(self, result, field):
        value = getattr(result, field)
        deviation = getattr(result, field + '_dev')
        bounds = getattr(self, field + '_bounds')
        if bounds[0] is None and bounds[1] is None:
            return True

        if value is None:
            # We have bounds requirements, but no value to check?
            return False

        if bounds[0] is not None and bounds[0] > value + deviation:
            return False

        if bounds[1] is not None and bounds[1] < value - deviation:
            return False

        return True


    def __init__(self, duration_seconds_min=None, duration_seconds_max=None,
                 throughput_min=None, throughput_max=None,
                 error_min=None, error_max=None,
                 transaction_rate_min=None, transaction_rate_max=None):
        """Construct a NetperfAssertion.

        Leaving bounds undefined sets them to values which are permissive.

        @param duration_seconds_min float minimal test duration in seconds.
        @param duration_seconds_max float maximal test duration in seconds.
        @param throughput_min float minimal throughput in Mbps.
        @param throughput_max float maximal throughput in Mbps.
        @param error_min int minimal number of UDP frame errors.
        @param error_max int max number of UDP frame errors.
        @param transaction_rate_min float minimal number of transactions
                per second.
        @param transaction_rate_max float max number of transactions per second.

        """
        Bound = collections.namedtuple('Bound', ['lower', 'upper'])
        self.duration_seconds_bounds = Bound(duration_seconds_min,
                                             duration_seconds_max)
        self.throughput_bounds = Bound(throughput_min, throughput_max)
        self.errors_bounds = Bound(error_min, error_max)
        self.transaction_rate_bounds = Bound(transaction_rate_min,
                                             transaction_rate_max)


    def passes(self, result):
        """Check that a result matches the given assertion.

        @param result NetperfResult object produced by a test.
        @return True iff all this assertion passes for the give result.

        """
        passed = [self._passes(result, field)
                  for field in ['duration_seconds', 'throughput',
                                'errors', 'transaction_rate']]
        if all(passed):
            return True

        return False


    def __repr__(self):
        fields = {'duration_seconds_min': self.duration_seconds_bounds.lower,
                  'duration_seconds_max': self.duration_seconds_bounds.upper,
                  'throughput_min': self.throughput_bounds.lower,
                  'throughput_max': self.throughput_bounds.upper,
                  'error_min': self.errors_bounds.lower,
                  'error_max': self.errors_bounds.upper,
                  'transaction_rate_min': self.transaction_rate_bounds.lower,
                  'transaction_rate_max': self.transaction_rate_bounds.upper}
        return '%s(%s)' % (self.__class__.__name__,
                           ', '.join(['%s=%r' % item
                                      for item in fields.iteritems()
                                      if item[1] is not None]))


class NetperfConfig(object):
    """Defines a single netperf run."""

    DEFAULT_TEST_TIME = 10
    # Measures how many times we can connect, request a byte, and receive a
    # byte per second.
    TEST_TYPE_TCP_CRR = 'TCP_CRR'
    # MAERTS is stream backwards.  Measure bitrate of a stream from the netperf
    # server to the client.
    TEST_TYPE_TCP_MAERTS = 'TCP_MAERTS'
    # Measures how many times we can request a byte and receive a byte per
    # second.
    TEST_TYPE_TCP_RR = 'TCP_RR'
    # This is like a TCP_STREAM test except that the netperf client will use
    # a platform dependent call like sendfile() rather than the simple send()
    # call.  This can result in better performance.
    TEST_TYPE_TCP_SENDFILE = 'TCP_SENDFILE'
    # Measures throughput sending bytes from the client to the server in a
    # TCP stream.
    TEST_TYPE_TCP_STREAM = 'TCP_STREAM'
    # Measures how many times we can request a byte from the client and receive
    # a byte from the server.  If any datagram is dropped, the client or server
    # will block indefinitely.  This failure is not evident except as a low
    # transaction rate.
    TEST_TYPE_UDP_RR = 'UDP_RR'
    # Test UDP throughput sending from the client to the server.  There is no
    # flow control here, and generally sending is easier that receiving, so
    # there will be two types of throughput, both receiving and sending.
    TEST_TYPE_UDP_STREAM = 'UDP_STREAM'
    # This isn't a real test type, but we can emulate a UDP stream from the
    # server to the DUT by running the netperf server on the DUT and the
    # client on the server and then doing a UDP_STREAM test.
    TEST_TYPE_UDP_MAERTS = 'UDP_MAERTS'
    # Different kinds of tests have different output formats.
    REQUEST_RESPONSE_TESTS = [ TEST_TYPE_TCP_CRR,
                               TEST_TYPE_TCP_RR,
                               TEST_TYPE_UDP_RR ]
    TCP_STREAM_TESTS = [ TEST_TYPE_TCP_MAERTS,
                         TEST_TYPE_TCP_SENDFILE,
                         TEST_TYPE_TCP_STREAM ]
    UDP_STREAM_TESTS = [ TEST_TYPE_UDP_STREAM,
                         TEST_TYPE_UDP_MAERTS ]

    SHORT_TAGS = { TEST_TYPE_TCP_CRR: 'tcp_crr',
                   TEST_TYPE_TCP_MAERTS: 'tcp_rx',
                   TEST_TYPE_TCP_RR: 'tcp_rr',
                   TEST_TYPE_TCP_SENDFILE: 'tcp_stx',
                   TEST_TYPE_TCP_STREAM: 'tcp_tx',
                   TEST_TYPE_UDP_RR: 'udp_rr',
                   TEST_TYPE_UDP_STREAM: 'udp_tx',
                   TEST_TYPE_UDP_MAERTS: 'udp_rx' }

    READABLE_TAGS = { TEST_TYPE_TCP_CRR: 'tcp_connect_roundtrip_rate',
                      TEST_TYPE_TCP_MAERTS: 'tcp_downstream',
                      TEST_TYPE_TCP_RR: 'tcp_roundtrip_rate',
                      TEST_TYPE_TCP_SENDFILE: 'tcp_upstream_sendfile',
                      TEST_TYPE_TCP_STREAM: 'tcp_upstream',
                      TEST_TYPE_UDP_RR: 'udp_roundtrip',
                      TEST_TYPE_UDP_STREAM: 'udp_upstream',
                      TEST_TYPE_UDP_MAERTS: 'udp_downstream' }


    @staticmethod
    def _assert_is_valid_test_type(test_type):
        """Assert that |test_type| is one of TEST_TYPE_* above.

        @param test_type string test type.

        """
        if (test_type not in NetperfConfig.REQUEST_RESPONSE_TESTS and
            test_type not in NetperfConfig.TCP_STREAM_TESTS and
            test_type not in NetperfConfig.UDP_STREAM_TESTS):
            raise error.TestFail('Invalid netperf test type: %r.' % test_type)


    @staticmethod
    def test_type_to_tag(test_type):
        """Convert a test type to a concise unique tag.

        @param test_type string, one of TEST_TYPE_* above.
        @return string very short test description.

        """
        return NetperfConfig.SHORT_TAGS.get(test_type, 'unknown')


    @staticmethod
    def test_type_to_human_readable_tag(test_type):
        """Convert a test type to a unique human readable tag.

        @param test_type string, one of TEST_TYPE_* above.
        @return string human readable test description.

        """
        return NetperfConfig.READABLE_TAGS.get(test_type, 'unknown')

    @property
    def human_readable_tag(self):
        """@return string human readable test description."""
        return self.test_type_to_human_readable_tag(self.test_type)


    @property
    def netperf_test_type(self):
        """@return string test type suitable for passing to netperf."""
        if self.test_type == self.TEST_TYPE_UDP_MAERTS:
            return self.TEST_TYPE_UDP_STREAM

        return self.test_type


    @property
    def server_serves(self):
        """False iff the server and DUT should switch roles for running netperf.

        @return True iff netserv should be run on server host.  When false
                this indicates that the DUT should run netserv and netperf
                should be run on the server against the client.

        """
        return self.test_type != self.TEST_TYPE_UDP_MAERTS


    @property
    def tag(self):
        """@return string very short test description."""
        return self.test_type_to_tag(self.test_type)


    def __init__(self, test_type, test_time=DEFAULT_TEST_TIME):
        """Construct a NetperfConfig.

        @param test_type string one of TEST_TYPE_* above.
        @param test_time int number of seconds to run the test for.

        """
        self.test_type = test_type
        self.test_time = test_time
        self._assert_is_valid_test_type(self.netperf_test_type)


    def __repr__(self):
        return '%s(test_type=%r, test_time=%r' % (
                self.__class__.__name__,
                self.test_type,
                self.test_time)


class NetperfRunner(object):
    """Delegate to run netperf on a client/server pair."""

    NETPERF_DATA_PORT = 12866
    NETPERF_PORT = 12865
    NETSERV_STARTUP_WAIT_TIME = 3
    NETPERF_COMMAND_TIMEOUT_MARGIN = 120


    def __init__(self, client_proxy, server_proxy, config):
        """Construct a NetperfRunner.

        @param client WiFiClient object.
        @param server LinuxSystem object.

        """
        self._client_proxy = client_proxy
        self._server_proxy = server_proxy
        if config.server_serves:
            self._server_host = server_proxy.host
            self._client_host = client_proxy.host
            self._target_ip = server_proxy.wifi_ip
        else:
            self._server_host = client_proxy.host
            self._client_host = server_proxy.host
            self._target_ip = client_proxy.wifi_ip
        self._command_netserv = path_utils.must_be_installed(
                'netserver', host=self._server_host)
        self._command_netperf = path_utils.must_be_installed(
                'netperf', host=self._client_host)
        self._config = config


    def __enter__(self):
        self._restart_netserv()
        return self


    def __exit__(self, exc_type, exc_value, traceback):
        self._client_proxy.firewall_cleanup()
        self._kill_netserv()


    def _kill_netserv(self):
        """Kills any existing netserv process on the serving host."""
        self._server_host.run('pkill %s' %
                              os.path.basename(self._command_netserv),
                              ignore_status=True)


    def _restart_netserv(self):
        logging.info('Starting netserver...')
        self._kill_netserv()
        self._server_host.run('%s -p %d >/dev/null 2>&1' %
                              (self._command_netserv, self.NETPERF_PORT))
        startup_time = time.time()
        self._client_proxy.firewall_open('tcp', self._server_proxy.wifi_ip)
        self._client_proxy.firewall_open('udp', self._server_proxy.wifi_ip)
        # Wait for the netserv to come up.
        while time.time() - startup_time < self.NETSERV_STARTUP_WAIT_TIME:
            time.sleep(0.1)


    def run(self, ignore_failures=False, retry_count=3):
        """Run netperf and take a performance measurement.

        @param ignore_failures bool True iff netperf runs that fail should be
                ignored.  If this happens, run will return a None value rather
                than a NetperfResult.
        @param retry_count int number of times to retry the netperf command if
                it fails due to an internal timeout within netperf.
        @return NetperfResult summarizing a netperf run.

        """
        netperf = '%s -H %s -p %s -t %s -l %d -- -P 0,%d' % (
                self._command_netperf,
                self._target_ip,
                self.NETPERF_PORT,
                self._config.netperf_test_type,
                self._config.test_time,
                self.NETPERF_DATA_PORT)
        logging.debug('Running netperf client.')
        logging.info('Running netperf for %d seconds.', self._config.test_time)
        timeout = self._config.test_time + self.NETPERF_COMMAND_TIMEOUT_MARGIN
        for attempt in range(retry_count):
            start_time = time.time()
            result = self._client_host.run(netperf, ignore_status=True,
                                           ignore_timeout=ignore_failures,
                                           timeout=timeout)
            if not result:
                logging.info('Retrying netperf after empty result.')
                continue

            # Exit retry loop on success.
            if not result.exit_status:
                break

            # Only retry for known retryable conditions.
            if 'Interrupted system call' in result.stderr:
                logging.info('Retrying netperf after internal timeout error.')
                continue

            if 'establish the control connection' in result.stdout:
                logging.info('Restarting netserv after client failed connect.')
                self._restart_netserv()
                continue

            # We are in an unhandled error case.
            logging.info('Retrying netperf after an unknown error.')

        if result.exit_status and not ignore_failures:
            raise error.CmdError(netperf, result,
                                 "Command returned non-zero exit status")

        duration = time.time() - start_time
        if result is None or result.exit_status:
            return None

        return NetperfResult.from_netperf_results(
                self._config.test_type, result.stdout, duration)
