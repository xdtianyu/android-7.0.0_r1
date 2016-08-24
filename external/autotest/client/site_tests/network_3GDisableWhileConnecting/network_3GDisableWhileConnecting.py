# Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import gobject
import logging
import time

from autotest_lib.client.bin import test
from autotest_lib.client.common_lib import error
from autotest_lib.client.cros.cellular import modem_utils
from autotest_lib.client.cros.mainloop import GenericTesterMainLoop
from autotest_lib.client.cros.mainloop import ExceptionForward

DEFAULT_TEST_TIMEOUT_S = 600


class DisableTester(GenericTesterMainLoop):
  """Base class containing main test logic."""
  def __init__(self, *args, **kwargs):
    super(DisableTester, self).__init__(*args, **kwargs)

  @ExceptionForward
  def perform_one_test(self):
    """Called by GenericMainTesterMainLoop to execute the test."""
    self._configure()
    disable_delay_ms = (
        self.test_kwargs.get('delay_before_disable_ms', 0) +
        self.test.iteration *
        self.test_kwargs.get('disable_delay_per_iteration_ms', 0))
    gobject.timeout_add(disable_delay_ms, self._start_disable)
    self._start_test()

  @ExceptionForward
  def _connect_success_handler(self, *ignored_args):
    logging.info('connect succeeded')
    self.requirement_completed('connect')

  @ExceptionForward
  def _connect_error_handler(self, e):
    # We disabled while connecting; error is OK
    logging.info('connect errored: %s', e)
    self.requirement_completed('connect')

  @ExceptionForward
  def _start_disable(self):
    logging.info('disabling')
    self.disable_start = time.time()
    self._enable(False)

  @ExceptionForward
  def _disable_success_handler(self):
    disable_elapsed = time.time() - self.disable_start
    self.requirement_completed('disable')

  @ExceptionForward
  def _get_status_success_handler(self, status):
    logging.info('Got status')
    self.requirement_completed('get_status', warn_if_already_completed=False)
    if self.status_delay_ms:
      gobject.timeout_add(self.status_delay_ms, self._start_get_status)

  def after_main_loop(self):
    """Called by GenericTesterMainLoop after the main loop has exited."""
    enabled = self._enabled()
    logging.info('Modem enabled: %s', enabled)
    # Will return happily if no Gobi present
    modem_utils.ClearGobiModemFaultInjection()


class ShillDisableTester(DisableTester):
  """Tests that disable-while-connecting works at the shill level.
  Expected control flow:

  * self._configure() called; registers self._disable_property_changed
    to be called when device is en/disabled

  * Parent class sets a timer that calls self._enable(False) when it expires.

  * _start_test calls _start_connect() which sends a connect request to
    the device.

  * we wait for the modem to power off, at which point
    _disable_property_changed (registered above) will get called

  * _disable_property_changed() completes the 'disable' requirement,
    and we're done.

  """
  def __init__(self, *args, **kwargs):
    super(ShillDisableTester, self).__init__(*args, **kwargs)

  def _disable_property_changed(self, property, value, *args, **kwargs):
    self._disable_success_handler()

  def _start_test(self):
    # We would love to add requirements based on connect, but in many
    # scenarios, there is no observable response to a cancelled
    # connect: We issue a connect, it returns instantly to let us know
    # that the connect has started, but then the disable takes effect
    # and the connect fails.  We don't get a state change because no
    # state change has happened: the modem never got to a different
    # state before we cancelled
    self.remaining_requirements = set(['disable'])
    self._start_connect()

  def _configure(self):
    self.cellular_device = \
        self.test.test_env.shill.find_cellular_device_object()
    if self.cellular_device is None:
      raise error.TestError("Could not find cellular device")

    self.cellular_service = \
        self.test.test_env.shill.find_cellular_service_object()

    self.test.test_env.bus.add_signal_receiver(
            self.dispatch_property_changed,
            signal_name='PropertyChanged',
            dbus_interface=self.cellular_device.dbus_interface,
            path=self.cellular_device.object_path)

  @ExceptionForward
  def _expect_einprogress_handler(self, e):
    pass

  def _enable(self, value):
    self.property_changed_actions['Powered'] = self._disable_property_changed

    if value:
      self.cellular_device.Enable(
          reply_handler=self.ignore_handler,
          error_handler=self._expect_einprogress_handler)
    else:
      self.cellular_device.Disable(
          reply_handler=self.ignore_handler,
          error_handler=self._expect_einprogress_handler)

  @ExceptionForward
  def _start_connect(self):
    logging.info('connecting')

    def _log_connect_event(property, value, *ignored_args):
      logging.info('%s property changed: %s', property, value)

    self.property_changed_actions['Connected'] = _log_connect_event

    # Contrary to documentation, Connect just returns when it has
    # fired off the lower-level dbus messages.  So a success means
    # nothing to us.  But a failure means it didn't even try.
    self.cellular_service.Connect(
        reply_handler=self.ignore_handler,
        error_handler=self.build_error_handler('Connect'))

  def _enabled(self):
    return self.cellular_device.GetProperties()['Powered']


class ModemDisableTester(DisableTester):
  """Tests that disable-while-connecting works at the modem-manager level.

  Expected control flow:

  * _configure() is called.

  * Parent class sets a timer that calls self._enable(False) when it
    expires.

  * _start_test calls _start_connect() which sends a connect request to
    the device, also sets a timer that calls GetStatus on the modem.

  * wait for all three (connect, disable, get_status) to complete.

  """
  def __init__(self, *args, **kwargs):
    super(ModemDisableTester, self).__init__(*args, **kwargs)

  def _is_gobi(self):
    return 'Gobi' in self.test.test_env.modem.path

  def _start_test(self):
    self.remaining_requirements = set(['connect', 'disable'])

    # Only cromo/gobi-cromo-plugin maintain the invariant that GetStatus
    # will always succeed, so we only check it if the modem is a Gobi.
    if self._is_gobi():
      self.remaining_requirements.add('get_status')
      self.status_delay_ms = self.test_kwargs.get('status_delay_ms', 200)
      gobject.timeout_add(self.status_delay_ms, self._start_get_status)

    self._start_connect()

  def _configure(self):
    self.simple_modem = self.test.test_env.modem.SimpleModem()

    logging.info('Modem path: %s', self.test.test_env.modem.path)

    if self._is_gobi():
      self._configure_gobi()
    else:
      self._configure_non_gobi()

    service = self.test.test_env.shill.wait_for_cellular_service_object()
    if not service:
      raise error.TestError('Modem failed to register with the network after '
                            're-enabling.')

  def _configure_gobi(self):
    gobi_modem = self.test.test_env.modem.GobiModem()

    if 'async_connect_sleep_ms' in self.test_kwargs:
      sleep_ms = self.test_kwargs.get('async_connect_sleep_ms', 0)
      logging.info('Sleeping %d ms before connect', sleep_ms)
      gobi_modem.InjectFault('AsyncConnectSleepMs', sleep_ms)

    if 'connect_fails_with_error_sending_qmi_request' in self.test_kwargs:
      logging.info('Injecting QMI failure')
      gobi_modem.InjectFault('ConnectFailsWithErrorSendingQmiRequest', 1)

  def _configure_non_gobi(self):
    # Check to make sure no Gobi-specific arguments were specified.
    if 'async_connect_sleep_ms' in self.test_kwargs:
      raise error.TestError('async_connect_sleep_ms on non-Gobi modem')
    if 'connect_fails_with_error_sending_qmi_request' in self.test_kwargs:
      raise error.TestError(
          'connect_fails_with_error_sending_qmi_request on non-Gobi modem')

  @ExceptionForward
  def _start_connect(self):
    logging.info('connecting')

    retval = self.simple_modem.Connect(
        {},
        reply_handler=self._connect_success_handler,
        error_handler=self._connect_error_handler)
    logging.info('connect call made.  retval = %s', retval)


  @ExceptionForward
  def _start_get_status(self):
    # Keep on calling get_status to make sure it works at all times
    self.simple_modem.GetStatus(
        reply_handler=self._get_status_success_handler,
        error_handler=self.build_error_handler('GetStatus'))

  def _enabled(self):
    return self.test.test_env.modem.GetModemProperties().get('Enabled', -1)

  def _enable(self, value):
    self.test.test_env.modem.Enable(
        value,
        reply_handler=self._disable_success_handler,
        error_handler=self.build_error_handler('Enable'))


class network_3GDisableWhileConnecting(test.test):
  """Check that the modem can handle a disconnect while connecting."""
  version = 1

  def run_once(self, test_env, **kwargs):
    self.test_env = test_env
    timeout_s = kwargs.get('timeout_s', DEFAULT_TEST_TIMEOUT_S)
    gobject_main_loop = gobject.MainLoop()

    with test_env:
      logging.info('Shill-level test')
      shill_level_test = ShillDisableTester(self,
                                            gobject_main_loop,
                                            timeout_s=timeout_s)
      shill_level_test.run(**kwargs)

    with test_env:
      try:
        logging.info('Modem-level test')
        modem_level_test = ModemDisableTester(self,
                                              gobject_main_loop,
                                              timeout_s=timeout_s)
        modem_level_test.run(**kwargs)
      finally:
        modem_utils.ClearGobiModemFaultInjection()
