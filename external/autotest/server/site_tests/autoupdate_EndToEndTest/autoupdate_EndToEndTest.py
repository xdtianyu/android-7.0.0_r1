# Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import collections
import json
import logging
import os
import socket
import time
import urllib2
import urlparse

from autotest_lib.client.bin import utils as client_utils
from autotest_lib.client.common_lib import error, global_config
from autotest_lib.client.common_lib.cros import autoupdater, dev_server
from autotest_lib.client.common_lib.cros.graphite import autotest_stats
from autotest_lib.server import autotest, hosts, test
from autotest_lib.server.cros.dynamic_suite import tools


def _wait(secs, desc=None):
    """Emits a log message and sleeps for a given number of seconds."""
    msg = 'Waiting %s seconds' % secs
    if desc:
        msg += ' (%s)' % desc
    logging.info(msg)
    time.sleep(secs)


class ExpectedUpdateEventChainFailed(error.TestFail):
    """Raised if we fail to receive an expected event in a chain."""

class RequiredArgumentMissing(error.TestError):
    """Raised if the test is missing a required argument."""


# Update event types.
EVENT_TYPE_DOWNLOAD_COMPLETE = '1'
EVENT_TYPE_INSTALL_COMPLETE = '2'
EVENT_TYPE_UPDATE_COMPLETE = '3'
EVENT_TYPE_DOWNLOAD_STARTED = '13'
EVENT_TYPE_DOWNLOAD_FINISHED = '14'
EVENT_TYPE_REBOOTED_AFTER_UPDATE = '54'

# Update event results.
EVENT_RESULT_ERROR = '0'
EVENT_RESULT_SUCCESS = '1'
EVENT_RESULT_SUCCESS_REBOOT = '2'
EVENT_RESULT_UPDATE_DEFERRED = '9'

# Omaha event types/results, from update_engine/omaha_request_action.h
# These are stored in dict form in order to easily print out the keys.
EVENT_TYPE_DICT = {
        EVENT_TYPE_DOWNLOAD_COMPLETE: 'download_complete',
        EVENT_TYPE_INSTALL_COMPLETE: 'install_complete',
        EVENT_TYPE_UPDATE_COMPLETE: 'update_complete',
        EVENT_TYPE_DOWNLOAD_STARTED: 'download_started',
        EVENT_TYPE_DOWNLOAD_FINISHED: 'download_finished',
        EVENT_TYPE_REBOOTED_AFTER_UPDATE: 'rebooted_after_update'
}

EVENT_RESULT_DICT = {
        EVENT_RESULT_ERROR: 'error',
        EVENT_RESULT_SUCCESS: 'success',
        EVENT_RESULT_SUCCESS_REBOOT: 'success_reboot',
        EVENT_RESULT_UPDATE_DEFERRED: 'update_deferred'
}


def snippet(text):
    """Returns the text with start/end snip markers around it.

    @param text: The snippet text.

    @return The text with start/end snip markers around it.
    """
    snip = '---8<---' * 10
    start = '-- START -'
    end = '-- END -'
    return ('%s%s\n%s\n%s%s' %
            (start, snip[len(start):], text, end, snip[len(end):]))


class ExpectedUpdateEvent(object):
    """Defines an expected event in an update process."""

    _ATTR_NAME_DICT_MAP = {
            'event_type': EVENT_TYPE_DICT,
            'event_result': EVENT_RESULT_DICT,
    }

    _VALID_TYPES = set(EVENT_TYPE_DICT.keys())
    _VALID_RESULTS = set(EVENT_RESULT_DICT.keys())

    def __init__(self, event_type=None, event_result=None, version=None,
                 previous_version=None, on_error=None):
        """Initializes an event expectation.

        @param event_type: Expected event type.
        @param event_result: Expected event result code.
        @param version: Expected reported image version.
        @param previous_version: Expected reported previous image version.
        @param on_error: This is either an object to be returned when a received
                         event mismatches the expectation, or a callable used
                         for generating one. In the latter case, takes as
                         input two attribute dictionaries (expected and actual)
                         and an iterable of mismatched keys. If None, a generic
                         message is returned.
        """
        if event_type and event_type not in self._VALID_TYPES:
            raise ValueError('event_type %s is not valid.' % event_type)

        if event_result and event_result not in self._VALID_RESULTS:
            raise ValueError('event_result %s is not valid.' % event_result)

        self._expected_attrs = {
            'event_type': event_type,
            'event_result': event_result,
            'version': version,
            'previous_version': previous_version,
        }
        self._on_error = on_error


    @staticmethod
    def _attr_val_str(attr_val, helper_dict, default=None):
        """Returns an enriched attribute value string, or default."""
        if not attr_val:
            return default

        s = str(attr_val)
        if helper_dict:
            s += ':%s' % helper_dict.get(attr_val, 'unknown')

        return s


    def _attr_name_and_values(self, attr_name, expected_attr_val,
                              actual_attr_val=None):
        """Returns an attribute name, expected and actual value strings.

        This will return (name, expected, actual); the returned value for
        actual will be None if its respective input is None/empty.

        """
        helper_dict = self._ATTR_NAME_DICT_MAP.get(attr_name)
        expected_attr_val_str = self._attr_val_str(expected_attr_val,
                                                   helper_dict,
                                                   default='any')
        actual_attr_val_str = self._attr_val_str(actual_attr_val, helper_dict)

        return attr_name, expected_attr_val_str, actual_attr_val_str


    def _attrs_to_str(self, attrs_dict):
        return ' '.join(['%s=%s' %
                         self._attr_name_and_values(attr_name, attr_val)[0:2]
                         for attr_name, attr_val in attrs_dict.iteritems()])


    def __str__(self):
        return self._attrs_to_str(self._expected_attrs)


    def verify(self, actual_event):
        """Verify the attributes of an actual event.

        @param actual_event: a dictionary containing event attributes

        @return An error message, or None if all attributes as expected.

        """
        mismatched_attrs = [
                attr_name for attr_name, expected_attr_val
                in self._expected_attrs.iteritems()
                if (expected_attr_val and
                    not self._verify_attr(attr_name, expected_attr_val,
                                          actual_event.get(attr_name)))]
        if not mismatched_attrs:
            return None
        if callable(self._on_error):
            return self._on_error(self._expected_attrs, actual_event,
                                  mismatched_attrs)
        if self._on_error is None:
            return ('Received event (%s) does not match expectation (%s)' %
                    (self._attrs_to_str(actual_event), self))
        return self._on_error


    def _verify_attr(self, attr_name, expected_attr_val, actual_attr_val):
        """Verifies that an actual log event attributes matches expected on.

        @param attr_name: name of the attribute to verify
        @param expected_attr_val: expected attribute value
        @param actual_attr_val: actual attribute value

        @return True if actual value is present and matches, False otherwise.

        """
        # None values are assumed to be missing and non-matching.
        if actual_attr_val is None:
            logging.error('No value found for %s (expected %s)',
                          *self._attr_name_and_values(attr_name,
                                                      expected_attr_val)[0:2])
            return False

        # We allow expected version numbers (e.g. 2940.0.0) to be contained in
        # actual values (2940.0.0-a1); this is necessary for the test to pass
        # with developer / non-release images.
        if (actual_attr_val == expected_attr_val or
            ('version' in attr_name and expected_attr_val in actual_attr_val)):
            return True

        return False


    def get_attrs(self):
        """Returns a dictionary of expected attributes."""
        return dict(self._expected_attrs)


class ExpectedUpdateEventChain(object):
    """Defines a chain of expected update events."""
    def __init__(self):
        self._expected_events_chain = []


    def add_event(self, expected_events, timeout, on_timeout=None):
        """Adds an expected event to the chain.

        @param expected_events: The ExpectedEvent, or a list thereof, to wait
                                for. If a list is passed, it will wait for *any*
                                of the provided events, but only one of those.
        @param timeout: A timeout (in seconds) to wait for the event.
        @param on_timeout: An error string to use if the event times out. If
                           None, a generic message is used.
        """
        if isinstance(expected_events, ExpectedUpdateEvent):
            expected_events = [expected_events]
        self._expected_events_chain.append(
                (expected_events, timeout, on_timeout))


    @staticmethod
    def _format_event_with_timeout(expected_events, timeout):
        """Returns a string representation of the event, with timeout."""
        until = 'within %s seconds' % timeout if timeout else 'indefinitely'
        return '%s, %s' % (' OR '.join(map(str, expected_events)), until)


    def __str__(self):
        return ('[%s]' %
                ', '.join(
                    [self._format_event_with_timeout(expected_events, timeout)
                     for expected_events, timeout, _
                     in self._expected_events_chain]))


    def __repr__(self):
        return str(self._expected_events_chain)


    def verify(self, get_next_event):
        """Verifies that an actual stream of events complies.

        @param get_next_event: a function returning the next event

        @raises ExpectedUpdateEventChainFailed if we failed to verify an event.

        """
        for expected_events, timeout, on_timeout in self._expected_events_chain:
            logging.info('Expecting %s',
                         self._format_event_with_timeout(expected_events,
                                                         timeout))
            err_msg = self._verify_event_with_timeout(
                    expected_events, timeout, on_timeout, get_next_event)
            if err_msg is not None:
                logging.error('Failed expected event: %s', err_msg)
                raise ExpectedUpdateEventChainFailed(err_msg)


    @staticmethod
    def _verify_event_with_timeout(expected_events, timeout, on_timeout,
                                   get_next_event):
        """Verify an expected event occurs within a given timeout.

        @param expected_events: the list of possible events expected next
        @param timeout: specified in seconds
        @param on_timeout: A string to return if timeout occurs, or None.
        @param get_next_event: function returning the next event in a stream

        @return None if event complies, an error string otherwise.

        """
        base_timestamp = curr_timestamp = time.time()
        expired_timestamp = base_timestamp + timeout
        while curr_timestamp <= expired_timestamp:
            new_event = get_next_event()
            if new_event:
                logging.info('Event received after %s seconds',
                             round(curr_timestamp - base_timestamp, 1))
                results = [event.verify(new_event) for event in expected_events]
                return None if None in results else ' AND '.join(results)

            # No new events, sleep for one second only (so we don't miss
            # events at the end of the allotted timeout).
            time.sleep(1)
            curr_timestamp = time.time()

        logging.error('Timeout expired')
        if on_timeout is None:
            return ('Waiting for event %s timed out after %d seconds' %
                    (' OR '.join(map(str, expected_events)), timeout))
        return on_timeout


class UpdateEventLogVerifier(object):
    """Verifies update event chains on a devserver update log."""
    def __init__(self, event_log_url, url_request_timeout=None):
        self._event_log_url = event_log_url
        self._url_request_timeout = url_request_timeout
        self._event_log = []
        self._num_consumed_events = 0


    def verify_expected_events_chain(self, expected_event_chain):
        """Verify a given event chain.

        @param expected_event_chain: instance of expected event chain.

        @raises ExpectedUpdateEventChainFailed if we failed to verify the an
                event.
        """
        expected_event_chain.verify(self._get_next_log_event)


    def _get_next_log_event(self):
        """Returns the next event in an event log.

        Uses the URL handed to it during initialization to obtain the host log
        from a devserver. If new events are encountered, the first of them is
        consumed and returned.

        @return The next new event in the host log, as reported by devserver;
                None if no such event was found or an error occurred.

        """
        # (Re)read event log from devserver, if necessary.
        if len(self._event_log) <= self._num_consumed_events:
            try:
                if self._url_request_timeout:
                    conn = urllib2.urlopen(self._event_log_url,
                                           timeout=self._url_request_timeout)
                else:
                    conn = urllib2.urlopen(self._event_log_url)
            except urllib2.URLError, e:
                logging.warning('Failed to read event log url: %s', e)
                return None
            except socket.timeout, e:
                logging.warning('Timed out reading event log url: %s', e)
                return None

            event_log_resp = conn.read()
            conn.close()
            self._event_log = json.loads(event_log_resp)

        # Return next new event, if one is found.
        if len(self._event_log) > self._num_consumed_events:
            new_event = {
                    key: str(val) for key, val
                    in self._event_log[self._num_consumed_events].iteritems()
            }
            self._num_consumed_events += 1
            logging.info('Consumed new event: %s', new_event)
            return new_event


class OmahaDevserverFailedToStart(error.TestError):
    """Raised when a omaha devserver fails to start."""


class OmahaDevserver(object):
    """Spawns a test-private devserver instance."""
    # How long to wait for a devserver to start.
    _WAIT_FOR_DEVSERVER_STARTED_SECONDS = 30

    # How long to sleep (seconds) between checks to see if a devserver is up.
    _WAIT_SLEEP_INTERVAL = 1

    # Max devserver execution time (seconds); used with timeout(1) to ensure we
    # don't have defunct instances hogging the system.
    _DEVSERVER_TIMELIMIT_SECONDS = 12 * 60 * 60


    def __init__(self, omaha_host, devserver_dir, update_payload_staged_url):
        """Starts a private devserver instance, operating at Omaha capacity.

        @param omaha_host: host address where the devserver is spawned.
        @param devserver_dir: path to the devserver source directory
        @param update_payload_staged_url: URL to provision for update requests.

        """
        if not update_payload_staged_url:
            raise error.TestError('Missing update payload url')

        self._omaha_host = omaha_host
        self._devserver_pid = 0
        self._devserver_port = 0  # Determined later from devserver portfile.
        self._devserver_dir = devserver_dir
        self._update_payload_staged_url = update_payload_staged_url

        self._devserver_ssh = hosts.SSHHost(self._omaha_host,
                                            user=os.environ['USER'])

        # Temporary files for various devserver outputs.
        self._devserver_logfile = None
        self._devserver_stdoutfile = None
        self._devserver_portfile = None
        self._devserver_pidfile = None
        self._devserver_static_dir = None


    def _cleanup_devserver_files(self):
        """Cleans up the temporary devserver files."""
        for filename in (self._devserver_logfile, self._devserver_stdoutfile,
                         self._devserver_portfile, self._devserver_pidfile):
            if filename:
                self._devserver_ssh.run('rm -f %s' % filename,
                                        ignore_status=True)

        if self._devserver_static_dir:
            self._devserver_ssh.run('rm -rf %s' % self._devserver_static_dir,
                                    ignore_status=True)


    def _create_tempfile_on_devserver(self, label, dir=False):
        """Creates a temporary file/dir on the devserver and returns its path.

        @param label: Identifier for the file context (string, no whitespaces).
        @param dir: If True, create a directory instead of a file.

        @raises test.TestError: If we failed to invoke mktemp on the server.
        @raises OmahaDevserverFailedToStart: If tempfile creation failed.
        """
        remote_cmd = 'mktemp --tmpdir devserver-%s.XXXXXX' % label
        if dir:
            remote_cmd += ' --directory'

        try:
            result = self._devserver_ssh.run(remote_cmd, ignore_status=True)
        except error.AutoservRunError as e:
            self._log_and_raise_remote_ssh_error(e)
        if result.exit_status != 0:
            raise OmahaDevserverFailedToStart(
                    'Could not create a temporary %s file on the devserver, '
                    'error output: "%s"' % (label, result.stderr))
        return result.stdout.strip()

    @staticmethod
    def _log_and_raise_remote_ssh_error(e):
        """Logs failure to ssh remote, then raises a TestError."""
        logging.debug('Failed to ssh into the devserver: %s', e)
        logging.error('If you are running this locally it means you did not '
                      'configure ssh correctly.')
        raise error.TestError('Failed to ssh into the devserver: %s' % e)


    def _read_int_from_devserver_file(self, filename):
        """Reads and returns an integer value from a file on the devserver."""
        return int(self._get_devserver_file_content(filename).strip())


    def _wait_for_devserver_to_start(self):
        """Waits until the devserver starts within the time limit.

        Infers and sets the devserver PID and serving port.

        Raises:
            OmahaDevserverFailedToStart: If the time limit is reached and we
                                         cannot connect to the devserver.
        """
        # Compute the overall timeout.
        deadline = time.time() + self._WAIT_FOR_DEVSERVER_STARTED_SECONDS

        # First, wait for port file to be filled and determine the server port.
        logging.warning('Waiting for devserver to start up.')
        while time.time() < deadline:
            try:
                self._devserver_pid = self._read_int_from_devserver_file(
                        self._devserver_pidfile)
                self._devserver_port = self._read_int_from_devserver_file(
                        self._devserver_portfile)
                logging.info('Devserver pid is %d, serving on port %d',
                             self._devserver_pid, self._devserver_port)
                break
            except Exception:  # Couldn't read file or corrupt content.
                time.sleep(self._WAIT_SLEEP_INTERVAL)
        else:
            try:
                self._devserver_ssh.run_output('uptime')
            except error.AutoservRunError as e:
                logging.debug('Failed to run uptime on the devserver: %s', e)
            raise OmahaDevserverFailedToStart(
                    'The test failed to find the pid/port of the omaha '
                    'devserver after %d seconds. Check the dumped devserver '
                    'logs and devserver load for more information.' %
                    self._WAIT_FOR_DEVSERVER_STARTED_SECONDS)

        # Check that the server is reponsding to network requests.
        logging.warning('Waiting for devserver to accept network requests.')
        url = 'http://%s' % self.get_netloc()
        while time.time() < deadline:
            if dev_server.DevServer.devserver_healthy(url, timeout_min=0.1):
                break

            # TODO(milleral): Refactor once crbug.com/221626 is resolved.
            time.sleep(self._WAIT_SLEEP_INTERVAL)
        else:
            raise OmahaDevserverFailedToStart(
                    'The test failed to establish a connection to the omaha '
                    'devserver it set up on port %d. Check the dumped '
                    'devserver logs for more information.' %
                    self._devserver_port)


    def start_devserver(self):
        """Starts the devserver and confirms it is up.

        Raises:
            test.TestError: If we failed to spawn the remote devserver.
            OmahaDevserverFailedToStart: If the time limit is reached and we
                                         cannot connect to the devserver.
        """
        update_payload_url_base, update_payload_path = self._split_url(
                self._update_payload_staged_url)

        # Allocate temporary files for various server outputs.
        self._devserver_logfile = self._create_tempfile_on_devserver('log')
        self._devserver_stdoutfile = self._create_tempfile_on_devserver(
                'stdout')
        self._devserver_portfile = self._create_tempfile_on_devserver('port')
        self._devserver_pidfile = self._create_tempfile_on_devserver('pid')
        self._devserver_static_dir = self._create_tempfile_on_devserver(
                'static', dir=True)

        # Invoke the Omaha/devserver on the remote server. Will attempt to kill
        # it with a SIGTERM after a predetermined timeout has elapsed, followed
        # by SIGKILL if not dead within 30 seconds from the former signal.
        cmdlist = [
                'timeout', '-s', 'TERM', '-k', '30',
                str(self._DEVSERVER_TIMELIMIT_SECONDS),
                '%s/devserver.py' % self._devserver_dir,
                '--payload=%s' % update_payload_path,
                '--port=0',
                '--pidfile=%s' % self._devserver_pidfile,
                '--portfile=%s' % self._devserver_portfile,
                '--logfile=%s' % self._devserver_logfile,
                '--remote_payload',
                '--urlbase=%s' % update_payload_url_base,
                '--max_updates=1',
                '--host_log',
                '--static_dir=%s' % self._devserver_static_dir,
        ]
        remote_cmd = '( %s ) </dev/null >%s 2>&1 &' % (
                ' '.join(cmdlist), self._devserver_stdoutfile)

        logging.info('Starting devserver with %r', remote_cmd)
        try:
            self._devserver_ssh.run_output(remote_cmd)
        except error.AutoservRunError as e:
            self._log_and_raise_remote_ssh_error(e)

        try:
            self._wait_for_devserver_to_start()
        except OmahaDevserverFailedToStart:
            self._kill_remote_process()
            self._dump_devserver_log()
            self._cleanup_devserver_files()
            raise


    def _kill_remote_process(self):
        """Kills the devserver and verifies it's down; clears the remote pid."""
        def devserver_down():
            """Ensure that the devserver process is down."""
            return not self._remote_process_alive()

        if devserver_down():
            return

        for signal in 'SIGTERM', 'SIGKILL':
            remote_cmd = 'kill -s %s %s' % (signal, self._devserver_pid)
            self._devserver_ssh.run(remote_cmd)
            try:
                client_utils.poll_for_condition(
                        devserver_down, sleep_interval=1, desc='devserver down')
                break
            except client_utils.TimeoutError:
                logging.warning('Could not kill devserver with %s.', signal)
        else:
            logging.warning('Failed to kill devserver, giving up.')

        self._devserver_pid = None


    def _remote_process_alive(self):
        """Tests whether the remote devserver process is running."""
        if not self._devserver_pid:
            return False
        remote_cmd = 'test -e /proc/%s' % self._devserver_pid
        result = self._devserver_ssh.run(remote_cmd, ignore_status=True)
        return result.exit_status == 0


    def get_netloc(self):
        """Returns the netloc (host:port) of the devserver."""
        if not (self._devserver_pid and self._devserver_port):
            raise error.TestError('No running omaha/devserver')

        return '%s:%s' % (self._omaha_host, self._devserver_port)


    def get_update_url(self):
        """Returns the update_url you can use to update via this server."""
        return urlparse.urlunsplit(('http', self.get_netloc(), '/update',
                                    '', ''))


    def _get_devserver_file_content(self, filename):
        """Returns the content of a file on the devserver."""
        return self._devserver_ssh.run_output('cat %s' % filename,
                                              stdout_tee=None)


    def _get_devserver_log(self):
        """Obtain the devserver output."""
        return self._get_devserver_file_content(self._devserver_logfile)


    def _get_devserver_stdout(self):
        """Obtain the devserver output in stdout and stderr."""
        return self._get_devserver_file_content(self._devserver_stdoutfile)


    def _dump_devserver_log(self, logging_level=logging.ERROR):
        """Dump the devserver log to the autotest log.

        @param logging_level: logging level (from logging) to log the output.
        """
        logging.log(logging_level, "Devserver stdout and stderr:\n" +
                    snippet(self._get_devserver_stdout()))
        logging.log(logging_level, "Devserver log file:\n" +
                    snippet(self._get_devserver_log()))


    @staticmethod
    def _split_url(url):
        """Splits a URL into the URL base and path."""
        split_url = urlparse.urlsplit(url)
        url_base = urlparse.urlunsplit(
                (split_url.scheme, split_url.netloc, '', '', ''))
        url_path = split_url.path
        return url_base, url_path.lstrip('/')


    def stop_devserver(self):
        """Kill remote process and wait for it to die, dump its output."""
        if not self._devserver_pid:
            logging.error('No running omaha/devserver.')
            return

        logging.info('Killing omaha/devserver')
        self._kill_remote_process()
        logging.debug('Final devserver log before killing')
        self._dump_devserver_log(logging.DEBUG)
        self._cleanup_devserver_files()


class TestPlatform(object):
    """An interface and factory for platform-dependent functionality."""

    # Named tuple containing urls for staged urls needed for test.
    # source_url: url to find the update payload for the source image.
    # source_stateful_url: url to find the stateful payload for the source
    #                      image.
    # target_url: url to find the update payload for the target image.
    # target_stateful_url: url to find the stateful payload for the target
    #                      image.
    StagedURLs = collections.namedtuple(
            'StagedURLs',
            ['source_url', 'source_stateful_url', 'target_url',
             'target_stateful_url'])


    def __init__(self):
        assert False, 'Cannot instantiate this interface'


    @staticmethod
    def create(host):
        """Returns a TestPlatform implementation based on the host type.

        *DO NOT* override this method.

        @param host: a host object representing the DUT

        @return A TestPlatform implementation.
        """
        os_type = host.get_os_type()
        if os_type == 'cros':
            return ChromiumOSTestPlatform(host)
        if os_type == 'brillo':
            return BrilloTestPlatform(host)

        raise error.TestError('Unknown OS type reported by host: %s' % os_type)


    def initialize(self, autotest_devserver, devserver_dir):
        """Initialize the object.

        @param autotest_devserver: Instance of client.common_lib.dev_server to
                                   use to reach the devserver instance for this
                                   build.
        @param devserver_dir: Path to devserver source tree.
        """
        raise NotImplementedError


    def prep_artifacts(self, test_conf):
        """Prepares update artifacts for the test.

        The test config must include 'source_payload_uri' and
        'target_payload_uri'. In addition, it may include platform-specific
        values as determined by the test control file.

        @param test_conf: Dictionary containing the test configuration.

        @return A tuple of staged URLs.

        @raise error.TestError on failure.
        """
        raise NotImplementedError


    def reboot_device(self):
        """Reboots the device."""
        raise NotImplementedError


    def prep_device_for_update(self, source_release):
        """Prepares the device for update.

        @param source_release: Source release version (string), or None.

        @raise error.TestError on failure.
        """
        raise NotImplementedError


    def get_active_slot(self):
        """Returns the active boot slot of the device."""
        raise NotImplementedError


    def start_update_perf(self, bindir):
        """Starts performance monitoring (if available).

        @param bindir: Directory containing test binary files.
        """
        raise NotImplementedError


    def stop_update_perf(self):
        """Stops performance monitoring and returns data (if available).

        @return Dictionary containing performance attributes.
        """
        raise NotImplementedError


    def trigger_update(self, omaha_devserver):
        """Kicks off an update.

        @param omaha_devserver: OmahaDevserver instance.
        """
        raise NotImplementedError


    def finalize_update(self):
        """Performs post-update procedures."""
        raise NotImplementedError


    def get_update_log(self, num_lines):
        """Returns the update log.

        @param num_lines: Number of log lines to return (tail), zero for all.

        @return String containing the last |num_lines| from the update log.
        """
        raise NotImplementedError


    def check_device_after_update(self, target_release):
        """Runs final sanity checks.

        @param target_release: Target release version (string), or None.

        @raise error.TestError on failure.
        """
        raise NotImplementedError


class ChromiumOSTestPlatform(TestPlatform):
    """A TestPlatform implementation for Chromium OS."""

    _STATEFUL_UPDATE_FILENAME = 'stateful.tgz'
    _LOGINABLE_MINIMUM_RELEASE = 5110

    def __init__(self, host):
        self._host = host
        self._autotest_devserver = None
        self._devserver_dir = None
        self._staged_urls = None
        self._perf_mon_pid = None


    def _stage_payload(self, devserver_label, filename, archive_url=None):
        """Stage the given payload onto the devserver.

        Works for either a stateful or full/delta test payload. Expects the
        gs_path or a combo of devserver_label + filename.

        @param devserver_label: The build name e.g. x86-mario-release/<version>.
                                If set, assumes default gs archive bucket and
                                requires filename to be specified.
        @param filename: In conjunction with devserver_label, if just specifying
                         the devserver label name, this is which file are you
                         downloading.
        @param archive_url: An optional GS archive location, if not using the
                            devserver's default.

        @return URL of the staged payload on the server.

        @raise error.TestError if there's a problem with staging.

        """
        try:
            self._autotest_devserver.stage_artifacts(
                    image=devserver_label, files=[filename],
                    archive_url=archive_url)
            return self._autotest_devserver.get_staged_file_url(filename,
                                                                devserver_label)
        except dev_server.DevServerException, e:
            raise error.TestError('Failed to stage payload: %s' % e)


    def _stage_payload_by_uri(self, payload_uri):
        """Stage a payload based on its GS URI.

        This infers the build's label, filename and GS archive from the
        provided GS URI.

        @param payload_uri: The full GS URI of the payload.

        @return URL of the staged payload on the server.

        @raise error.TestError if there's a problem with staging.

        """
        archive_url, _, filename = payload_uri.rpartition('/')
        devserver_label = urlparse.urlsplit(archive_url).path.strip('/')
        return self._stage_payload(devserver_label, filename,
                                   archive_url=archive_url)


    @staticmethod
    def _payload_to_update_url(payload_url):
        """Given a update or stateful payload url, returns the update url."""
        # We want to transform it to the correct omaha url which is
        # <hostname>/update/...LABEL.
        base_url = payload_url.rpartition('/')[0]
        return base_url.replace('/static/', '/update/')


    def _get_stateful_uri(self, build_uri):
        """Returns a complete GS URI of a stateful update given a build path."""
        return '/'.join([build_uri.rstrip('/'), self._STATEFUL_UPDATE_FILENAME])


    def _payload_to_stateful_uri(self, payload_uri):
        """Given a payload GS URI, returns the corresponding stateful URI."""
        build_uri = payload_uri.rpartition('/')[0]
        return self._get_stateful_uri(build_uri)


    def _update_via_test_payloads(self, omaha_host, payload_url, stateful_url,
                                  clobber):
        """Given the following update and stateful urls, update the DUT.

        Only updates the rootfs/stateful if the respective url is provided.

        @param omaha_host: If updating rootfs, redirect updates through this
            host. Should be None iff payload_url is None.
        @param payload_url: If set, the specified url to find the update
            payload.
        @param stateful_url: If set, the specified url to find the stateful
            payload.
        @param clobber: If True, do a clean install of stateful.
        """
        def perform_update(url, is_stateful):
            """Perform a rootfs/stateful update using given URL.

            @param url: URL to update from.
            @param is_stateful: Whether this is a stateful or rootfs update.
            """
            if url:
                updater = autoupdater.ChromiumOSUpdater(url, host=self._host)
                if is_stateful:
                    updater.update_stateful(clobber=clobber)
                else:
                    updater.update_image()

        # We create a OmahaDevserver to redirect blah.bin to update/. This
        # allows us to use any payload filename to serve an update.
        temp_devserver = None
        try:
            if payload_url:
                temp_devserver = OmahaDevserver(
                        omaha_host, self._devserver_dir, payload_url)
                temp_devserver.start_devserver()
                payload_url = temp_devserver.get_update_url()

            stateful_url = self._payload_to_update_url(stateful_url)

            perform_update(payload_url, False)
            perform_update(stateful_url, True)
        finally:
            if temp_devserver:
                temp_devserver.stop_devserver()


    def _install_source_version(self, devserver_hostname, image_url,
                                stateful_url):
        """Prepare the specified host with the image given by the urls.

        @param devserver_hostname: If updating rootfs, redirect updates
                                   through this host. Should be None iff
                                   image_url is None.
        @param image_url: If set, the specified url to find the source image
                          or full payload for the source image.
        @param stateful_url: If set, the specified url to find the stateful
                             payload.
        """
        try:
            # Reboot to get us into a clean state.
            self._host.reboot()
            # Since we are installing the source image of the test, clobber
            # stateful.
            self._update_via_test_payloads(devserver_hostname, image_url,
                                           stateful_url, True)
            self._host.reboot()
        except OmahaDevserverFailedToStart as e:
            logging.fatal('Failed to start private devserver for installing '
                          'the source image (%s) on the DUT', image_url)
            raise error.TestError(
                    'Failed to start private devserver for installing the '
                    'source image on the DUT: %s' % e)
        except error.AutoservRunError as e:
            logging.fatal('Error re-imaging or rebooting the DUT with the '
                          'source image from %s', image_url)
            raise error.TestError('Failed to install the source image or '
                                  'reboot the DUT: %s' % e)


    def _stage_artifacts_onto_devserver(self, test_conf):
        """Stages artifacts that will be used by the test onto the devserver.

        @param test_conf: a dictionary containing test configuration values

        @return a StagedURLs tuple containing the staged urls.
        """
        logging.info('Staging images onto autotest devserver (%s)',
                     self._autotest_devserver.url())

        staged_source_url = None
        staged_source_stateful_url = None
        try:
            source_payload_uri = test_conf['source_payload_uri']
        except KeyError:
            # TODO(garnold) Remove legacy key support once control files on all
            # release branches have caught up.
            source_payload_uri = test_conf['source_image_uri']
        if source_payload_uri:
            staged_source_url = self._stage_payload_by_uri(source_payload_uri)

            # In order to properly install the source image using a full
            # payload we'll also need the stateful update that comes with it.
            # In general, tests may have their source artifacts in a different
            # location than their payloads. This is determined by whether or
            # not the source_archive_uri attribute is set; if it isn't set,
            # then we derive it from the dirname of the source payload.
            source_archive_uri = test_conf.get('source_archive_uri')
            if source_archive_uri:
                source_stateful_uri = self._get_stateful_uri(source_archive_uri)
            else:
                source_stateful_uri = self._payload_to_stateful_uri(
                        source_payload_uri)

            staged_source_stateful_url = self._stage_payload_by_uri(
                    source_stateful_uri)

            # Log source image URLs.
            logging.info('Source full payload from %s staged at %s',
                         source_payload_uri, staged_source_url)
            if staged_source_stateful_url:
                logging.info('Source stateful update from %s staged at %s',
                             source_stateful_uri, staged_source_stateful_url)

        target_payload_uri = test_conf['target_payload_uri']
        staged_target_url = self._stage_payload_by_uri(target_payload_uri)
        target_stateful_uri = None
        staged_target_stateful_url = None
        target_archive_uri = test_conf.get('target_archive_uri')
        if target_archive_uri:
            target_stateful_uri = self._get_stateful_uri(target_archive_uri)
        else:
            # Attempt to get the job_repo_url to find the stateful payload for
            # the target image.
            try:
                job_repo_url = self._host.lookup_job_repo_url()
            except KeyError:
                # If this failed, assume the stateful update is next to the
                # update payload.
                target_stateful_uri = self._payload_to_stateful_uri(
                    target_payload_uri)
            else:
                _, devserver_label = tools.get_devserver_build_from_package_url(
                        job_repo_url)
                staged_target_stateful_url = self._stage_payload(
                        devserver_label, self._STATEFUL_UPDATE_FILENAME)

        if not staged_target_stateful_url and target_stateful_uri:
            staged_target_stateful_url = self._stage_payload_by_uri(
                    target_stateful_uri)

        # Log target payload URLs.
        logging.info('%s test payload from %s staged at %s',
                     test_conf['update_type'], target_payload_uri,
                     staged_target_url)
        logging.info('Target stateful update from %s staged at %s',
                     target_stateful_uri or 'standard location',
                     staged_target_stateful_url)

        return self.StagedURLs(staged_source_url, staged_source_stateful_url,
                               staged_target_url, staged_target_stateful_url)


    def _run_login_test(self, release_string):
        """Runs login_LoginSuccess test if it is supported by the release."""
        # Only do login tests with recent builds, since they depend on
        # some binary compatibility with the build itself.
        # '5116.0.0' -> ('5116', '0', '0') -> 5116
        if not release_string:
            logging.info('No release provided, skipping login test.')
        elif int(release_string.split('.')[0]) > self._LOGINABLE_MINIMUM_RELEASE:
            # Login, to prove we can before/after the update.
            logging.info('Attempting to login (release %s).', release_string)

            client_at = autotest.Autotest(self._host)
            client_at.run_test('login_LoginSuccess')
        else:
            logging.info('Not attempting login test because %s is older than '
                         '%d.', release_string, self._LOGINABLE_MINIMUM_RELEASE)


    def _start_perf_mon(self, bindir):
        """Starts monitoring performance and resource usage on a DUT.

        Call _stop_perf_mon() with the returned PID to stop monitoring
        and collect the results.

        @param bindir: Directoy containing monitoring script.

        @return The PID of the newly created DUT monitoring process.
        """
        # We can't assume much about the source image so we copy the
        # performance monitoring script to the DUT directly.
        path = os.path.join(bindir, 'update_engine_performance_monitor.py')
        self._host.send_file(path, '/tmp')
        cmd = 'python /tmp/update_engine_performance_monitor.py --start-bg'
        return int(self._host.run(cmd).stdout)


    def _stop_perf_mon(self, perf_mon_pid):
        """Stops monitoring performance and resource usage on a DUT.

        @param perf_mon_pid: the PID returned from _start_perf_mon().

        @return Dictionary containing performance attributes, or None if
                unavailable.
        """
        # Gracefully handle problems with performance monitoring by
        # just returning None.
        try:
            cmd = ('python /tmp/update_engine_performance_monitor.py '
                   '--stop-bg=%d') % perf_mon_pid
            perf_json_txt = self._host.run(cmd).stdout
            return json.loads(perf_json_txt)
        except Exception as e:
            logging.warning('Failed to parse output from '
                            'update_engine_performance_monitor.py: %s', e)
        return None


    # Interface overrides.
    #
    def initialize(self, autotest_devserver, devserver_dir):
        self._autotest_devserver = autotest_devserver
        self._devserver_dir = devserver_dir


    def reboot_device(self):
        self._host.reboot()


    def prep_artifacts(self, test_conf):
        self._staged_urls = self._stage_artifacts_onto_devserver(test_conf)
        return self._staged_urls


    def prep_device_for_update(self, source_release):
        # Install the source version onto the DUT.
        if self._staged_urls.source_url:
            logging.info('Installing a source image on the DUT')
            devserver_hostname = urlparse.urlparse(
                    self._autotest_devserver.url()).hostname
            self._install_source_version(devserver_hostname,
                                         self._staged_urls.source_url,
                                         self._staged_urls.source_stateful_url)

        # Make sure we can login before the update.
        self._run_login_test(source_release)


    def get_active_slot(self):
        return self._host.run('rootdev -s').stdout.strip()


    def start_update_perf(self, bindir):
        if self._perf_mon_pid is None:
            self._perf_mon_pid = self._start_perf_mon(bindir)


    def stop_update_perf(self):
        perf_data = None
        if self._perf_mon_pid is not None:
            perf_data = self._stop_perf_mon(self._perf_mon_pid)
            self._perf_mon_pid = None

        return perf_data


    def trigger_update(self, omaha_devserver):
        updater = autoupdater.ChromiumOSUpdater(
                omaha_devserver.get_update_url(), host=self._host)
        updater.trigger_update()


    def finalize_update(self):
        self._update_via_test_payloads(
                None, None, self._staged_urls.target_stateful_url, False)


    def get_update_log(self, num_lines):
        return self._host.run_output(
                'tail -n %d /var/log/update_engine.log' % num_lines,
                stdout_tee=None)


    def check_device_after_update(self, target_release):
        # Make sure we can login after update.
        self._run_login_test(target_release)


class BrilloTestPlatform(TestPlatform):
    """A TestPlatform implementation for Brillo."""

    _URL_DEFAULT_PORT = 80
    _DUT_LOCALHOST = '127.0.0.1'

    def __init__(self, host):
        self._host = host
        self._autotest_devserver = None
        self._devserver_dir = None
        self._staged_urls = None
        self._forwarding_ports = set()


    @classmethod
    def _get_host_port(cls, url):
        """Returns the host and port values from a given URL.

        @param url: The URL from which the values are extracted.

        @return A pair consisting of the host and port strings.
        """
        host, _, port = urlparse.urlsplit(url).netloc.partition(':')
        return host, port or str(cls._URL_DEFAULT_PORT)


    def _install_rev_forwarding(self, port=None):
        """Installs reverse forwarding rules via ADB.

        @param port: The TCP port we want forwarded; if None, installs all
                     previously configured ports.
        """
        ports = self._forwarding_ports if port is None else [port]
        for port in ports:
            port_spec = 'tcp:%s' % port
            self._host.add_forwarding(port_spec, port_spec, reverse=True)


    def _add_rev_forwarding(self, url):
        """Configures reverse port forwarding and adjusts the given URL.

        This extracts the port from the URL, adds it to the set of configured
        forwarding ports, installs it to the DUT, then returns the adjusted URL
        for use by the DUT.

        @param url: The URL for which we need to establish forwarding.

        @return: The adjusted URL for use on the DUT.
        """
        if url:
            host, port = self._get_host_port(url)
            if port not in self._forwarding_ports:
                self._forwarding_ports.add(port)
                self._install_rev_forwarding(port=port)
            url = url.replace(host, self._DUT_LOCALHOST, 1)
        return url


    def _remove_rev_forwarding(self, url=None):
        """Removes a reverse port forwarding.

        @param url: The URL for which forwarding was established; if None,
                    removes all previously configured ports.
        """
        ports = set()
        if url is None:
            ports.update(self._forwarding_ports)
        else:
            _, port = self._get_host_port(url)
            if port in self._forwarding_ports:
                ports.add(port)

        # TODO(garnold) Enable once ADB port removal is fixed (b/24771474):
        # for port in ports:
        #     self._host.remove_forwarding(src='tcp:%s' % port, reverse=True)

        self._forwarding_ports.difference_update(ports)


    def _install_source_version(self, devserver_hostname, payload_url):
        """Installs a source version onto the test device.

        @param devserver_hostname: Redirect updates through this host.
        @param payload_url: URL of staged payload for installing a source image.
        """
        try:
            # Start a private Omaha server and update the DUT.
            temp_devserver = None
            url = None
            try:
                temp_devserver = OmahaDevserver(
                        devserver_hostname, self._devserver_dir, payload_url)
                temp_devserver.start_devserver()
                url = self._add_rev_forwarding(temp_devserver.get_update_url())
                updater = autoupdater.BrilloUpdater(url, host=self._host)
                updater.update_image()
            finally:
                if url:
                    self._remove_rev_forwarding(url)
                if temp_devserver:
                    temp_devserver.stop_devserver()

            # Reboot the DUT.
            self.reboot_device()
        except OmahaDevserverFailedToStart as e:
            logging.fatal('Failed to start private devserver for installing '
                          'the source payload (%s) on the DUT', payload_url)
            raise error.TestError(
                    'Failed to start private devserver for installing the '
                    'source image on the DUT: %s' % e)
        except error.AutoservRunError as e:
            logging.fatal('Error re-imaging or rebooting the DUT with the '
                          'source image from %s', payload_url)
            raise error.TestError('Failed to install the source image or '
                                  'reboot the DUT: %s' % e)


    # Interface overrides.
    #
    def initialize(self, autotest_devserver, devserver_dir):
        self._autotest_devserver = autotest_devserver
        self._devserver_dir = devserver_dir


    def reboot_device(self):
        self._host.reboot()
        self._install_rev_forwarding()


    def prep_artifacts(self, test_conf):
        # TODO(garnold) Currently we don't stage anything and assume that the
        # provided URLs are of pre-staged payloads. We should implement staging
        # support once a release scheme for Brillo is finalized.
        self._staged_urls = self.StagedURLs(
                self._add_rev_forwarding(test_conf['source_payload_uri']), None,
                self._add_rev_forwarding(test_conf['target_payload_uri']), None)
        return self._staged_urls


    def prep_device_for_update(self, source_release):
        # Install the source version onto the DUT.
        if self._staged_urls.source_url:
            logging.info('Installing a source image on the DUT')
            devserver_hostname = urlparse.urlparse(
                    self._autotest_devserver.url()).hostname
            self._install_source_version(devserver_hostname,
                                         self._staged_urls.source_url)


    def get_active_slot(self):
        return self._host.run('rootdev -s /system').stdout.strip()


    def start_update_perf(self, bindir):
        pass


    def stop_update_perf(self):
        pass


    def trigger_update(self, omaha_devserver):
        url = self._add_rev_forwarding(omaha_devserver.get_update_url())
        updater = autoupdater.BrilloUpdater(url, host=self._host)
        updater.trigger_update()


    def finalize_update(self):
        pass


    def get_update_log(self, num_lines):
        return self._host.run_output(
                'logcat -d -s update_engine | tail -n %d' % num_lines,
                stdout_tee=None)


    def check_device_after_update(self, target_release):
        self._remove_rev_forwarding()
        # TODO(garnold) Port forwarding removal is broken in ADB (b/24771474).
        # Instead we reboot the device, which has the side-effect of flushing
        # all forwarding rules. Once fixed, the following should be removed.
        self.reboot_device()


class autoupdate_EndToEndTest(test.test):
    """Complete update test between two Chrome OS releases.

    Performs an end-to-end test of updating a ChromeOS device from one version
    to another. The test performs the following steps:

      1. Stages the source (full) and target update payload on the central
         devserver.
      2. Spawns a private Omaha-like devserver instance, configured to return
         the target (update) payload URL in response for an update check.
      3. Reboots the DUT.
      4. Installs a source image on the DUT (if provided) and reboots to it.
      5. Triggers an update check at the DUT.
      6. Watches as the DUT obtains an update and applies it.
      7. Reboots and repeats steps 5-6, ensuring that the next update check
         shows the new image version.

    Some notes on naming:
      devserver: Refers to a machine running the Chrome OS Update Devserver.
      autotest_devserver: An autotest wrapper to interact with a devserver.
                          Can be used to stage artifacts to a devserver. While
                          this can also be used to update a machine, we do not
                          use it for that purpose in this test as we manage
                          updates with out own devserver instances (see below).
      omaha_devserver: This test's wrapper of a devserver running for the
                       purposes of emulating omaha. This test controls the
                       lifetime of this devserver instance and is separate
                       from the autotest lab's devserver's instances which are
                       only used for staging and hosting artifacts (because they
                       scale). These are run on the same machines as the actual
                       autotest devservers which are used for staging but on
                       different ports.
      *staged_url's: In this case staged refers to the fact that these items
                     are available to be downloaded statically from these urls
                     e.g. 'localhost:8080/static/my_file.gz'. These are usually
                     given after staging an artifact using a autotest_devserver
                     though they can be re-created given enough assumptions.
      *update_url's: Urls refering to the update RPC on a given omaha devserver.
                     Since we always use an instantiated omaha devserver to run
                     updates, these will always reference an existing instance
                     of an omaha devserver that we just created for the purposes
                     of updating.
      devserver_hostname: At the start of each test, we choose a devserver
                          machine in the lab for the test. We use the devserver
                          instance there (access by autotest_devserver) to stage
                          artifacts. However, we also use the same host to start
                          omaha devserver instances for updating machines with
                          (that reference the staged paylaods on the autotest
                          devserver instance). This hostname refers to that
                          machine we are using (since it's always the same for
                          both staging/omaha'ing).

    """
    version = 1

    # Timeout periods, given in seconds.
    _WAIT_AFTER_SHUTDOWN_SECONDS = 10
    _WAIT_AFTER_UPDATE_SECONDS = 20
    _WAIT_FOR_USB_INSTALL_SECONDS = 4 * 60
    _WAIT_FOR_MP_RECOVERY_SECONDS = 8 * 60
    _WAIT_FOR_INITIAL_UPDATE_CHECK_SECONDS = 12 * 60
    # TODO(sosa): Investigate why this needs to be so long (this used to be
    # 120 and regressed).
    _WAIT_FOR_DOWNLOAD_STARTED_SECONDS = 4 * 60
    _WAIT_FOR_DOWNLOAD_COMPLETED_SECONDS = 10 * 60
    _WAIT_FOR_UPDATE_COMPLETED_SECONDS = 4 * 60
    _WAIT_FOR_UPDATE_CHECK_AFTER_REBOOT_SECONDS = 15 * 60
    _DEVSERVER_HOSTLOG_REQUEST_TIMEOUT_SECONDS = 30

    # Logs and their whereabouts.
    _WHERE_UPDATE_LOG = ('update_engine log (in sysinfo or on the DUT, also '
                         'included in the test log)')
    _WHERE_OMAHA_LOG = 'Omaha-devserver log (included in the test log)'


    def initialize(self):
        """Sets up variables that will be used by test."""
        self._host = None
        self._omaha_devserver = None
        self._source_image_installed = False

        self._devserver_dir = global_config.global_config.get_config_value(
                'CROS', 'devserver_dir', default=None)
        if self._devserver_dir is None:
            raise error.TestError(
                    'Path to devserver source tree not provided; please define '
                    'devserver_dir under [CROS] in your shadow_config.ini')


    def cleanup(self):
        """Kill the omaha devserver if it's still around."""
        if self._omaha_devserver:
            self._omaha_devserver.stop_devserver()

        self._omaha_devserver = None


    def _dump_update_engine_log(self, test_platform):
        """Dumps relevant AU error log."""
        try:
            error_log = test_platform.get_update_log(80)
            logging.error('Dumping snippet of update_engine log:\n%s',
                          snippet(error_log))
        except Exception:
            # Mute any exceptions we get printing debug logs.
            pass


    def _report_perf_data(self, perf_data):
        """Reports performance and resource data.

        Currently, performance attributes are expected to include 'rss_peak'
        (peak memory usage in bytes).

        @param perf_data: A dictionary containing performance attributes.
        """
        rss_peak = perf_data.get('rss_peak')
        if rss_peak:
            rss_peak_kib = rss_peak / 1024
            logging.info('Peak memory (RSS) usage on DUT: %d KiB', rss_peak_kib)
            self.output_perf_value(description='mem_usage_peak',
                                   value=int(rss_peak_kib),
                                   units='KiB',
                                   higher_is_better=False)
        else:
            logging.warning('No rss_peak key in JSON returned by '
                            'update_engine_performance_monitor.py')


    def _error_initial_check(self, expected, actual, mismatched_attrs):
        if 'version' in mismatched_attrs:
            err_msg = ('Initial update check was received but the reported '
                       'version is different from what was expected.')
            if self._source_image_installed:
                err_msg += (' The source payload we installed was probably '
                            'incorrect or corrupt.')
            else:
                err_msg += (' The DUT is probably not running the correct '
                            'source image.')
            return err_msg

        return 'A test bug occurred; inspect the test log.'


    def _error_intermediate(self, expected, actual, mismatched_attrs, action,
                            problem):
        if 'event_result' in mismatched_attrs:
            event_result = actual.get('event_result')
            reported = (('different than expected (%s)' %
                         EVENT_RESULT_DICT[event_result])
                        if event_result else 'missing')
            return ('The updater reported result code is %s. This could be an '
                    'updater bug or a connectivity problem; check the %s. For '
                    'a detailed log of update events, check the %s.' %
                    (reported, self._WHERE_UPDATE_LOG, self._WHERE_OMAHA_LOG))
        if 'event_type' in mismatched_attrs:
            event_type = actual.get('event_type')
            reported = ('different (%s)' % EVENT_TYPE_DICT[event_type]
                        if event_type else 'missing')
            return ('Expected the updater to %s (%s) but received event type '
                    'is %s. This could be an updater %s; check the '
                    '%s. For a detailed log of update events, check the %s.' %
                    (action, EVENT_TYPE_DICT[expected['event_type']], reported,
                     problem, self._WHERE_UPDATE_LOG, self._WHERE_OMAHA_LOG))
        if 'version' in mismatched_attrs:
            return ('The updater reported an unexpected version despite '
                    'previously reporting the correct one. This is most likely '
                    'a bug in update engine; check the %s.' %
                    self._WHERE_UPDATE_LOG)

        return 'A test bug occurred; inspect the test log.'


    def _error_download_started(self, expected, actual, mismatched_attrs):
        return self._error_intermediate(expected, actual, mismatched_attrs,
                                        'begin downloading',
                                        'bug, crash or provisioning error')


    def _error_download_finished(self, expected, actual, mismatched_attrs):
        return self._error_intermediate(expected, actual, mismatched_attrs,
                                        'finish downloading', 'bug or crash')


    def _error_update_complete(self, expected, actual, mismatched_attrs):
        return self._error_intermediate(expected, actual, mismatched_attrs,
                                        'complete the update', 'bug or crash')


    def _error_reboot_after_update(self, expected, actual, mismatched_attrs):
        if 'event_result' in mismatched_attrs:
            event_result = actual.get('event_result')
            reported = ('different (%s)' % EVENT_RESULT_DICT[event_result]
                        if event_result else 'missing')
            return ('The updater was expected to reboot (%s) but reported '
                    'result code is %s. This could be a failure to reboot, an '
                    'updater bug or a connectivity problem; check the %s and '
                    'the system log. For a detailed log of update events, '
                    'check the %s.' %
                    (EVENT_RESULT_DICT[expected['event_result']], reported,
                     self._WHERE_UPDATE_LOG, self._WHERE_OMAHA_LOG))
        if 'event_type' in mismatched_attrs:
            event_type = actual.get('event_type')
            reported = ('different (%s)' % EVENT_TYPE_DICT[event_type]
                        if event_type else 'missing')
            return ('Expected to successfully reboot into the new image (%s) '
                    'but received event type is %s. This probably means that '
                    'the new image failed to verify after reboot, possibly '
                    'because the payload is corrupt. This might also be an '
                    'updater bug or crash; check the %s. For a detailed log of '
                    'update events, check the %s.' %
                    (EVENT_TYPE_DICT[expected['event_type']], reported,
                     self._WHERE_UPDATE_LOG, self._WHERE_OMAHA_LOG))
        if 'version' in mismatched_attrs:
            return ('The DUT rebooted after the update but reports a different '
                    'image version than the one expected. This probably means '
                    'that the payload we applied was incorrect or corrupt.')
        if 'previous_version' in mismatched_attrs:
            return ('The DUT rebooted after the update and reports the '
                    'expected version. However, it reports a previous version '
                    'that is different from the one previously reported. This '
                    'is most likely a bug in update engine; check the %s.' %
                    self._WHERE_UPDATE_LOG)

        return 'A test bug occurred; inspect the test log.'


    def _timeout_err(self, desc, timeout, event_type=None):
        if event_type is not None:
            desc += ' (%s)' % EVENT_TYPE_DICT[event_type]
        return ('Failed to receive %s within %d seconds. This could be a '
                'problem with the updater or a connectivity issue. For more '
                'details, check the %s.' %
                (desc, timeout, self._WHERE_UPDATE_LOG))


    def run_update_test(self, test_platform, test_conf):
        """Runs the actual update test once preconditions are met.

        @param test_platform: TestPlatform implementation.
        @param test_conf: A dictionary containing test configuration values

        @raises ExpectedUpdateEventChainFailed if we failed to verify an update
                event.
        """

        # Record the active root partition.
        source_active_slot = test_platform.get_active_slot()
        logging.info('Source active slot: %s', source_active_slot)

        source_release = test_conf['source_release']
        target_release = test_conf['target_release']

        # Start the performance monitoring process on the DUT.
        test_platform.start_update_perf(self.bindir)
        try:
            # Trigger an update.
            test_platform.trigger_update(self._omaha_devserver)

            # Track update progress.
            omaha_netloc = self._omaha_devserver.get_netloc()
            omaha_hostlog_url = urlparse.urlunsplit(
                    ['http', omaha_netloc, '/api/hostlog',
                     'ip=' + self._host.ip, ''])
            logging.info('Polling update progress from omaha/devserver: %s',
                         omaha_hostlog_url)
            log_verifier = UpdateEventLogVerifier(
                    omaha_hostlog_url,
                    self._DEVSERVER_HOSTLOG_REQUEST_TIMEOUT_SECONDS)

            # Verify chain of events in a successful update process.
            chain = ExpectedUpdateEventChain()
            chain.add_event(
                    ExpectedUpdateEvent(
                        version=source_release,
                        on_error=self._error_initial_check),
                    self._WAIT_FOR_INITIAL_UPDATE_CHECK_SECONDS,
                    on_timeout=self._timeout_err(
                            'an initial update check',
                            self._WAIT_FOR_INITIAL_UPDATE_CHECK_SECONDS))
            chain.add_event(
                    ExpectedUpdateEvent(
                        event_type=EVENT_TYPE_DOWNLOAD_STARTED,
                        event_result=EVENT_RESULT_SUCCESS,
                        version=source_release,
                        on_error=self._error_download_started),
                    self._WAIT_FOR_DOWNLOAD_STARTED_SECONDS,
                    on_timeout=self._timeout_err(
                            'a download started notification',
                            self._WAIT_FOR_DOWNLOAD_STARTED_SECONDS,
                            event_type=EVENT_TYPE_DOWNLOAD_STARTED))
            chain.add_event(
                    ExpectedUpdateEvent(
                        event_type=EVENT_TYPE_DOWNLOAD_FINISHED,
                        event_result=EVENT_RESULT_SUCCESS,
                        version=source_release,
                        on_error=self._error_download_finished),
                    self._WAIT_FOR_DOWNLOAD_COMPLETED_SECONDS,
                    on_timeout=self._timeout_err(
                            'a download finished notification',
                            self._WAIT_FOR_DOWNLOAD_COMPLETED_SECONDS,
                            event_type=EVENT_TYPE_DOWNLOAD_FINISHED))
            chain.add_event(
                    ExpectedUpdateEvent(
                        event_type=EVENT_TYPE_UPDATE_COMPLETE,
                        event_result=EVENT_RESULT_SUCCESS,
                        version=source_release,
                        on_error=self._error_update_complete),
                    self._WAIT_FOR_UPDATE_COMPLETED_SECONDS,
                    on_timeout=self._timeout_err(
                            'an update complete notification',
                            self._WAIT_FOR_UPDATE_COMPLETED_SECONDS,
                            event_type=EVENT_TYPE_UPDATE_COMPLETE))

            log_verifier.verify_expected_events_chain(chain)

            # Wait after an update completion (safety margin).
            _wait(self._WAIT_AFTER_UPDATE_SECONDS, 'after update completion')
        finally:
            # Terminate perf monitoring process and collect its output.
            perf_data = test_platform.stop_update_perf()
            if perf_data:
                self._report_perf_data(perf_data)

        # Only update the stateful partition (the test updated the rootfs).
        test_platform.finalize_update()

        # Reboot the DUT after the update.
        test_platform.reboot_device()

        # Trigger a second update check (again, test vs MP).
        test_platform.trigger_update(self._omaha_devserver)

        # Observe post-reboot update check, which should indicate that the
        # image version has been updated.
        chain = ExpectedUpdateEventChain()
        expected_events = [
            ExpectedUpdateEvent(
                event_type=EVENT_TYPE_UPDATE_COMPLETE,
                event_result=EVENT_RESULT_SUCCESS_REBOOT,
                version=target_release,
                previous_version=source_release,
                on_error=self._error_reboot_after_update),
            # Newer versions send a "rebooted_after_update" message after reboot
            # with the previous version instead of another "update_complete".
            ExpectedUpdateEvent(
                event_type=EVENT_TYPE_REBOOTED_AFTER_UPDATE,
                event_result=EVENT_RESULT_SUCCESS,
                version=target_release,
                previous_version=source_release,
                on_error=self._error_reboot_after_update),
        ]
        chain.add_event(
                expected_events,
                self._WAIT_FOR_UPDATE_CHECK_AFTER_REBOOT_SECONDS,
                on_timeout=self._timeout_err(
                        'a successful reboot notification',
                        self._WAIT_FOR_UPDATE_CHECK_AFTER_REBOOT_SECONDS,
                        event_type=EVENT_TYPE_UPDATE_COMPLETE))

        log_verifier.verify_expected_events_chain(chain)

        # Make sure we're using a different slot after the update.
        target_active_slot = test_platform.get_active_slot()
        if target_active_slot == source_active_slot:
            err_msg = 'The active image slot did not change after the update.'
            if None in (source_release, target_release):
                err_msg += (' The DUT likely rebooted into the old image, which '
                            'probably means that the payload we applied was '
                            'corrupt. But since we did not check the source '
                            'and/or target version we cannot say for sure.')
            elif source_release == target_release:
                err_msg += (' Given that the source and target versions are '
                            'identical, the DUT likely rebooted into the old '
                            'image. This probably means that the payload we '
                            'applied was corrupt.')
            else:
                err_msg += (' This is strange since the DUT reported the '
                            'correct target version. This is probably a system '
                            'bug; check the DUT system log.')
            raise error.TestFail(err_msg)

        logging.info('Target active slot changed as expected: %s',
                     target_active_slot)

        logging.info('Update successful, test completed')


    # TODO(garnold) Remove the use_servo argument once control files on all
    # release branches have caught up.
    def run_once(self, host, test_conf, use_servo=False):
        """Performs a complete auto update test.

        @param host: a host object representing the DUT
        @param test_conf: a dictionary containing test configuration values
        @param use_servo: DEPRECATED

        @raise error.TestError if anything went wrong with setting up the test;
               error.TestFail if any part of the test has failed.

        """

        self._host = host

        # Find a devserver to use. We first try to pick a devserver with the
        # least load. In case all devservers' load are higher than threshold,
        # fall back to the old behavior by picking a devserver based on the
        # payload URI, with which ImageServer.resolve will return a random
        # devserver based on the hash of the URI.
        least_loaded_devserver = dev_server.get_least_loaded_devserver()
        if least_loaded_devserver:
            logging.debug('Choose the least loaded devserver: %s',
                          least_loaded_devserver)
            autotest_devserver = dev_server.ImageServer(least_loaded_devserver)
        else:
            logging.warning('No devserver meets the maximum load requirement. '
                            'Pick a random devserver to use.')
            autotest_devserver = dev_server.ImageServer.resolve(
                    test_conf['target_payload_uri'])
        devserver_hostname = urlparse.urlparse(
                autotest_devserver.url()).hostname
        counter_key = dev_server.ImageServer.create_stats_str(
                'paygen', devserver_hostname, artifacts=None)
        metadata = {'devserver': devserver_hostname,
                    '_type': 'devserver_paygen'}
        metadata.update(test_conf)
        autotest_stats.Counter(counter_key, metadata=metadata).increment()

        # Obtain a test platform implementation.
        test_platform = TestPlatform.create(host)
        test_platform.initialize(autotest_devserver, self._devserver_dir)

        # Stage source images and update payloads onto a devserver.
        staged_urls = test_platform.prep_artifacts(test_conf)
        self._source_image_installed = bool(staged_urls.source_url)

        # Prepare the DUT (install source version etc).
        test_platform.prep_device_for_update(test_conf['source_release'])

        self._omaha_devserver = OmahaDevserver(
                devserver_hostname, self._devserver_dir,
                staged_urls.target_url)
        self._omaha_devserver.start_devserver()

        try:
            self.run_update_test(test_platform, test_conf)
        except ExpectedUpdateEventChainFailed:
            self._dump_update_engine_log(test_platform)
            raise

        test_platform.check_device_after_update(test_conf['target_release'])
