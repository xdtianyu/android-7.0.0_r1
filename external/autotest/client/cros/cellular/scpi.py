#!/usr/bin/python
# Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import cellular_logging
import cellular_system_error

log = cellular_logging.SetupCellularLogging('scpi_driver')


class _ErrorCheckerContext(object):
    """Reference-count our error-checking state and only check for
    errors when we take the first ref or drop the last ref.

    This way, we can minimize the number of checks; each one takes a
    bit of time.  You will likely want to set always_check to True when
    debugging new SCPI interactions.

    On first entry, we check for errors, but do not stop if we find
    them; these are errors that were accumulated on the device before
    this test ran.
    """

    def __init__(self, scpi):
        self.always_check = True  # True for serious debugging
        self.scpi = scpi
        self.depth = 0
        self.raise_on_error = True

    def __enter__(self):
        log.debug('ErrorCheckerContext Depth: %s' % self.depth)
        if self.depth == 0 or self.always_check:
            errors = self.scpi._WaitAndFetchErrors(
                raise_on_error=False)  # Never raise when clearing old errors
        self.depth += 1
        return self

    def __exit__(self, type, value, traceback):
        self.depth -= 1
        if self.depth <= 0 or self.always_check:
            self.scpi._WaitAndFetchErrors()
        return


class Scpi(object):
    """Wrapper for SCPI.

    SCPI = "standard commands for programmable instruments",
    a relative of GPIB.

    The SCPI driver must export:  Query, Send, Reset and Close
    """

    def __init__(self, driver, opc_on_stanza=False):
        self.driver = driver
        self.opc_on_stanza = opc_on_stanza
        self.checker_context = _ErrorCheckerContext(self)

    def Query(self, command):
        """Send the SCPI command and return the response."""
        response = self.driver.Query(command)
        return response

    def Send(self, command):
        """Send the SCPI command."""
        self.driver.Send(command)

    def Reset(self):
        """Tell the device to reset with *RST."""
        # Some devices (like the prologix) require special handling for
        # reset.
        self.driver.Reset()

    def Close(self):
        """Close the device."""
        self.driver.Close()

    def RetrieveErrors(self):
        """Retrieves all SYSTem:ERRor messages from the device."""
        errors = []
        while True:
            error = self.Query('SYSTem:ERRor?')
            if '+0,"No error"' in error:
                # We've reached the end of the error stack
                break

            if '-420' in error and 'Query UNTERMINATED' in error:
                # This is benign; the GPIB bridge asked for a response when
                # the device didn't have one to give.

                # TODO(rochberg): This is a layering violation; we should
                # really only accept -420 if the underlying driver is in a
                # mode that is known to cause this
                continue

            if '+292' in error and 'Data arrived on unknown SAPI' in error:
                # This may be benign; It is known to occur when we do a switch
                # from GPRS to WCDMA
                continue

            errors.append(error)

        self.Send('*CLS')           # Clear status
        errors.reverse()
        return errors

    def _WaitAndFetchErrors(self, raise_on_error=True):
        """Waits for command completion, returns errors."""
        self.Query('*OPC?')      # Wait for operation complete
        errors = self.RetrieveErrors()
        if errors and raise_on_error:
            raise cellular_system_error.BadScpiCommand('\n'.join(errors))
        return errors

    def SimpleVerify(self, command, arg):
        """Sends "command arg", then "command?", expecting arg back.

        Arguments:
          command: SCPI command
          arg: Argument.  We currently check for exact equality: you should
            send strings quoted with " because that's what the 8960 returns.
            We also fail if you send 1 and receive +1 back.

        Raises:
          Error:  Verification failed
        """
        self.always_check = False
        with self.checker_context:
            self.Send('%s %s' % (command, arg))
            result = self.Query('%s?' % (command,))
            if result != arg:
                raise cellular_system_error.BadScpiCommand(
                    'Error on %s: sent %s, got %s' % (command, arg, result))

    def SendStanza(self, commands):
        """
        Sends a list of commands and verifies that they complete correctly.
        """
        with self.checker_context:
            for c in commands:
                if self.opc_on_stanza:
                    self.Send(c)
                    self.Query('*OPC?')
                else:
                    self.Send(c)
