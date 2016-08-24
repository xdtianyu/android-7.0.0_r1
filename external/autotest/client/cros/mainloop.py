# Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import gobject, logging, sys, traceback

import common
from autotest_lib.client.common_lib import error

# TODO(rochberg): Take another shot at fixing glib to allow this
# behavior when desired
def ExceptionForward(func):
  """Decorator that saves exceptions for forwarding across a glib
  mainloop.

  Exceptions thrown by glib callbacks are swallowed if they reach the
  glib main loop. This decorator collaborates with
  ExceptionForwardingMainLoop to save those exceptions so that it can
  reraise them."""
  def wrapper(self, *args, **kwargs):
    try:
      return func(self, *args, **kwargs)
    except Exception, e:
      logging.warning('Saving exception: %s' % e)
      logging.warning(''.join(traceback.format_exception(*sys.exc_info())))
      self._forwarded_exception = e
      self.main_loop.quit()
      return False
  return wrapper

class ExceptionForwardingMainLoop(object):
  """Wraps a glib mainloop so that exceptions raised by functions
  called by the mainloop cause the mainloop to terminate and reraise
  the exception.

  Any function called by the main loop (including dbus callbacks and
  glib callbacks like add_idle) must be wrapped in the
  @ExceptionForward decorator."""

  def __init__(self, main_loop, timeout_s=-1):
    self._forwarded_exception = None
    self.main_loop = main_loop
    if timeout_s == -1:
      logging.warning('ExceptionForwardingMainLoop: No timeout specified.')
      logging.warning('(Specify timeout_s=0 explicitly for no timeout.)')
    self.timeout_s = timeout_s

  def idle(self):
    raise Exception('idle must be overridden')

  def timeout(self):
    pass

  @ExceptionForward
  def _timeout(self):
    self.timeout()
    raise error.TestFail('main loop timed out')

  def quit(self):
    self.main_loop.quit()

  def run(self):
    gobject.idle_add(self.idle)
    if self.timeout_s > 0:
      timeout_source = gobject.timeout_add(self.timeout_s * 1000, self._timeout)
    self.main_loop.run()
    if self.timeout_s > 0:
      gobject.source_remove(timeout_source)

    if self._forwarded_exception:
      raise self._forwarded_exception

class GenericTesterMainLoop(ExceptionForwardingMainLoop):
  """Runs a glib mainloop until it times out or all requirements are
  satisfied."""

  def __init__(self, test, main_loop, **kwargs):
    super(GenericTesterMainLoop, self).__init__(main_loop, **kwargs)
    self.test = test
    self.property_changed_actions = {}

  def idle(self):
    self.perform_one_test()

  def perform_one_test(self):
    """Subclasses override this function to do their testing."""
    raise Exception('perform_one_test must be overridden')

  def after_main_loop(self):
    """Children can override this to clean up after the main loop."""
    pass

  def build_error_handler(self, name):
    """Returns a closure that fails the test with the specified name."""
    @ExceptionForward
    def to_return(self, e):
      raise error.TestFail('Dbus call %s failed: %s' % (name, e))
    # Bind the returned handler function to this object
    return to_return.__get__(self, GenericTesterMainLoop)

  @ExceptionForward
  def ignore_handler(*ignored_args, **ignored_kwargs):
    pass

  def requirement_completed(self, requirement, warn_if_already_completed=True):
    """Record that a requirement was completed.  Exit if all are."""
    should_log = True
    try:
      self.remaining_requirements.remove(requirement)
    except KeyError:
      if warn_if_already_completed:
        logging.warning('requirement %s was not present to be completed',
                        requirement)
      else:
        should_log = False

    if not self.remaining_requirements:
      logging.info('All requirements satisfied')
      self.quit()
    else:
      if should_log:
        logging.info('Requirement %s satisfied.  Remaining: %s' %
                     (requirement, self.remaining_requirements))

  def timeout(self):
    logging.error('Requirements unsatisfied upon timeout: %s' %
                    self.remaining_requirements)

  @ExceptionForward
  def dispatch_property_changed(self, property, *args, **kwargs):
    action = self.property_changed_actions.pop(property, None)
    if action:
      logging.info('Property_changed dispatching %s' % property)
      action(property, *args, **kwargs)

  def assert_(self, arg):
    self.test.assert_(self, arg)

  def run(self, *args, **kwargs):
    self.test_args = args
    self.test_kwargs = kwargs
    ExceptionForwardingMainLoop.run(self)
    self.after_main_loop()
