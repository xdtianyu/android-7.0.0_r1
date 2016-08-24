# Copyright (c) 2015 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging

import common
from autotest_lib.client.cros.cellular.mbim_compliance import entity
from autotest_lib.client.cros.cellular.mbim_compliance import mbim_errors


class Sequence(entity.Entity):
    """ Base class for all sequences. """

    def run(self, **kwargs):
        """
        Run the sequence.

        @returns The result of the sequence. The type of value returned varies
                 by the sequence type.

        """
        logging.info('---- Sequence (%s) begin ----', self.name())
        result = self.run_internal(**kwargs)
        logging.info('---- Sequence (%s) end ----', self.name())
        return result

    def run_internal(self):
        """
        The actual method runs the sequence.
        Subclasses should override this method to run their own sequence.

        """
        mbim_errors.log_and_raise(NotImplementedError)


    def name(self):
        """ Return str name. """
        return self.__class__.__name__


    def detach_kernel_driver_if_active(self, interface_number):
        """
        Check if interfaces are occupied by kernel driver. If kernel driver is
        active, then we can't exclusively use the inteface for tests.

        @param interface_number: The bInterfaceNumber value of the interface
                under check.

        """
        if self.device_context.device.is_kernel_driver_active(interface_number):
            self.device_context.device.detach_kernel_driver(interface_number)
