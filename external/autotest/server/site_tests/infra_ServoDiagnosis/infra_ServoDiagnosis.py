# Copyright (c) 2014 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging

from autotest_lib.client.common_lib import error
from autotest_lib.client.common_lib.cros import servo_afe_board_map
from autotest_lib.server import test
from autotest_lib.server.cros.servo import servo


def _successful(result_value):
    return result_value and not isinstance(result_value, Exception)


class _DiagnosticTest(object):
    """Data needed to handle one diagnostic test on a Servo host.

    The class encapsulates two basic elements:
     1. A pre-requisite test that must have passed.  The
        pre-requisite is recorded as a key in the results
        dictionary.
     2. A function that performs the actual diagnostic test.

    All tests have the implicit pre-requisite that the servo host
    can be reached on the network via ping.

    Pre-requisites are meant to capture relationships of the form
    "if test X cant't pass, test Y will always fail".  Typically,
    that means that test X tests a capability used by test Y.

    This implementation is a bit naive:  It assumes only a single
    pre-requisite, and it assumes the only outcome is a simple
    pass/fail.  The design also doesn't account for relationships
    of the form "if test X fails, run test Y to try and distinguish
    possible causes".

    """

    def __init__(self, prerequisite, get_result):
        self._prerequisite = prerequisite
        self._get_result = get_result

    def can_run(self, results):
        """Return whether this test's pre-requisite is satisfied.

        @param results The results dictionary with the status of
                       this test's pre-requisite.

        """
        if self._prerequisite is None:
            return True
        return _successful(results[self._prerequisite])

    def run_diagnostic(self, servo_host, servod):
        """Run the diagnostic test, and return the result.

        The test receives ServoHost and Servo objects to be tested;
        typically a single test uses one or the other, but not both.

        @param servo_host A ServoHost object to be the target of the
                          test.
        @param servod     A Servo object to be the target of the
                          test.
        @return If the test returns normally, return its result.  If
                the test raises an exception, return the exception.

        """
        try:
            return self._get_result(servo_host, servod)
        except Exception as e:
            return e


def _ssh_test(servo_host, servod):
    """Test whether the servo host answers to ssh.

    This test serves as a basic pre-requisite for tests that
    use ssh to test other conditions.

    Pre-requisite: There are no pre-requisites for this test aside
    from the implicit pre-requisite that the host answer to ping.

    @param servo_host The ServoHost object to talk to via ssh.
    @param servod     Ignored.

    """
    return servo_host.is_up()


def _servod_connect(servo_host, servod):
    """Test whether connection to servod succeeds.

    This tests the connection to the target servod with a simple
    method call.  As a side-effect, all hardware signals are
    initialized to default values.

    This function always returns success.  The test can only fail if
    the underlying call to servo raises an exception.

    Pre-requisite: There are no pre-requisites for this test aside
    from the implicit pre-requisite that the host answer to ping.

    @return `True`

    """
    # TODO(jrbarnette) We need to protect this call so that it
    # will time out if servod doesn't respond.
    servod.initialize_dut()
    return True


def _pwr_button_test(servo_host, servod):
    """Test whether the 'pwr_button' signal is correct.

    This tests whether the state of the 'pwr_button' signal is
    'release'.  When the servo flex cable is not attached, the
    signal will be stuck at 'press'.

    Pre-requisite:  This test depends on successful initialization
    of servod.

    Rationale:  The initialization step sets 'pwr_button' to
    'release', which is required to justify the expectations of this
    test.  Also, if initialization fails, we can reasonably expect
    that all communication with servod will fail.

    @param servo_host Ignored.
    @param servod     The Servo object to be tested.

    """
    return servod.get('pwr_button') == 'release'


def _lid_test(servo_host, servod):
    """Test whether the 'lid_open' signal is correct.

    This tests whether the state of the 'lid_open' signal has a
    correct value.  There is a manual switch on the servo board; if
    that switch is set wrong, the signal will be stuck at 'no'.
    Working units may return a setting of 'yes' (meaning the lid is
    open) or 'not_applicable' (meaning the device has no lid).

    Pre-requisite:  This test depends on the 'pwr_button' test.

    Rationale:  If the 'pwr_button' test fails, the flex cable may
    be disconnected, which means any servo operation to read a
    hardware signal will fail.

    @param servo_host Ignored.
    @param servod     The Servo object to be tested.

    """
    return servod.get('lid_open') != 'no'


def _command_test(servo_host, command):
    """Utility to return the output of a command on a servo host.

    The command is expected to produce at most one line of
    output.  A trailing newline, if any, is stripped.

    @return Output from the command with the trailing newline
            removed.

    """
    return servo_host.run(command).stdout.strip('\n')


def _brillo_test(servo_host, servod):
    """Get the version of Brillo running on the servo host.

    Reads the setting of CHROMEOS_RELEASE_VERSION from
    /etc/lsb-release on the servo host.  An empty string will
    returned if there is no such setting.

    Pre-requisite:  This test depends on the ssh test.

    @param servo_host The ServoHost object to be queried.
    @param servod     Ignored.

    @return Returns a Brillo version number or an empty string.

    """
    command = ('sed "s/CHROMEOS_RELEASE_VERSION=//p ; d" '
                   '/etc/lsb-release')
    return _command_test(servo_host, command)


def _board_test(servo_host, servod):
    """Get the board for which the servo is configured.

    Reads the setting of BOARD from /var/lib/servod/config.  An
    empty string is returned if the board is unconfigured.

    Pre-requisite:  This test depends on the brillo version test.

    Rationale: The /var/lib/servod/config file is used by the servod
    upstart job, which is specific to Brillo servo builds.  This
    test has no meaning if the target servo host isn't running
    Brillo.

    @param servo_host The ServoHost object to be queried.
    @param servod     Ignored.

    @return The confgured board or an empty string.

    """
    command = ('CONFIG=/var/lib/servod/config\n'
               '[ -f $CONFIG ] && . $CONFIG && echo $BOARD')
    return _command_test(servo_host, command)


def _servod_test(servo_host, servod):
    """Get the status of the servod upstart job.

    Ask upstart for the status of the 'servod' job.  Return whether
    the job is reported running.

    Pre-requisite:  This test depends on the brillo version test.

    Rationale: The servod upstart job is specific to Brillo servo
    builds.  This test has no meaning if the target servo host isn't
    running Brillo.

    @param servo_host The ServoHost object to be queried.
    @param servod     Ignored.

    @return `True` if the job is running, or `False` otherwise.

    """
    command = 'status servod | sed "s/,.*//"'
    return _command_test(servo_host, command) == 'servod start/running'


_DIAGNOSTICS_LIST = [
    ('ssh_responds',
        _DiagnosticTest(None, _ssh_test)),
    ('servod_connect',
        _DiagnosticTest(None, _servod_connect)),
    ('pwr_button',
        _DiagnosticTest('servod_connect', _pwr_button_test)),
    ('lid_open',
        _DiagnosticTest('pwr_button', _lid_test)),
    ('brillo_version',
        _DiagnosticTest('ssh_responds', _brillo_test)),
    ('board',
        _DiagnosticTest('brillo_version', _board_test)),
    ('servod',
        _DiagnosticTest('brillo_version', _servod_test)),
]


class infra_ServoDiagnosis(test.test):
    """Test a servo and diagnose common failures."""

    version = 1

    def _run_results(self, servo_host, servod):
        results = {}
        for key, tester in _DIAGNOSTICS_LIST:
            if tester.can_run(results):
                results[key] = tester.run_diagnostic(servo_host, servod)
                logging.info('Test %s result %s', key, results[key])
            else:
                results[key] = None
                logging.info('Skipping %s', key)
        return results

    def run_once(self, host):
        """Test and diagnose the servo for the given host.

        @param host Host object for a DUT with Servo.

        """
        # TODO(jrbarnette):  Need to handle ping diagnoses:
        #   + Specifically report if servo host isn't a lab host.
        #   + Specifically report if servo host is in lab but
        #     doesn't respond to ping.
        servo_host = host._servo_host
        servod = host.servo
        if servod is None:
            servod = servo.Servo(servo_host)
        results = self._run_results(servo_host, servod)

        if not _successful(results['ssh_responds']):
            raise error.TestFail('ssh connection to %s failed' %
                                     servo_host.hostname)

        if not _successful(results['brillo_version']):
            raise error.TestFail('Servo host %s is not running Brillo' %
                                     servo_host.hostname)

        # Make sure servo board matches DUT label
        board = host._get_board_from_afe()
        board = servo_afe_board_map.map_afe_board_to_servo_board(board)
        if (board and results['board'] is not None and
                board != results['board']):
            logging.info('AFE says board should be %s', board)
            if results['servod']:
                servo_host.run('stop servod', ignore_status=True)
            servo_host.run('start servod BOARD=%s' % board)
            results = self._run_results(servo_host, servod)

        # TODO(jrbarnette): The brillo update check currently
        # lives in ServoHost; it needs to move here.

        # Repair actions:
        #   if servod is dead or running but not working
        #     reboot and re-run results

        if (not _successful(results['servod']) or
                not _successful(results['servod_connect'])):
            # TODO(jrbarnette):  For now, allow reboot failures to
            # raise their exceptions up the stack.  This event
            # shouldn't happen, so smarter handling should wait
            # until we have a use case to guide the requirements.
            servo_host.reboot()
            results = self._run_results(servo_host, servod)
            if not _successful(results['servod']):
                # write result value to log
                raise error.TestFail('servod failed to start on %s' %
                                         servo_host.hostname)

            if not _successful(results['servod_connect']):
                raise error.TestFail('Servo failure on %s' %
                                         servo_host.hostname)

        if not _successful(results['pwr_button']):
            raise error.TestFail('Stuck power button on %s' %
                                     servo_host.hostname)

        if not _successful(results['lid_open']):
            raise error.TestFail('Lid stuck closed on %s' %
                                     servo_host.hostname)
