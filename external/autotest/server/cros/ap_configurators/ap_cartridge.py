# Copyright (c) 2012 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging
import Queue
import time
import traceback

from autotest_lib.client.common_lib.cros.network import ap_constants
from autotest_lib.server.cros.ap_configurators import ap_configurator
from threading import Thread

# Maximum configurators to run at once
THREAD_MAX = 15


class APCartridge(object):
    """Class to run multiple configurators in parallel."""


    def __init__(self):
        self.cartridge = Queue.Queue()


    def push_configurators(self, configurators):
        """Adds multiple configurators to the cartridge.

        @param configurators: a list of configurator objects.
        """
        for configurator in configurators:
            self.cartridge.put(configurator)


    def push_configurator(self, configurator):
        """Adds a configurator to the cartridge.

        @param configurator: a configurator object.
        """
        self.cartridge.put(configurator)


    def _apply_settings(self, broken_pdus):
        while True:
            configurator = self.cartridge.get()
            try:
                # Don't run this thread if the PDU in question was found to be
                # down by any previous thread.
                if configurator.pdu in broken_pdus:
                   configurator.configuration_success = ap_constants.PDU_FAIL
                   raise ap_configurator.PduNotResponding(configurator.pdu)
                configurator.apply_settings()
            except ap_configurator.PduNotResponding as e:
                if configurator.pdu not in broken_pdus:
                    broken_pdus.append(configurator.pdu)
            except Exception:
                configurator.configuration_success = ap_constants.CONFIG_FAIL
                trace = ''.join(traceback.format_exc())
                configurator.store_config_failure(trace)
                logging.error('Configuration failed for AP: %s\n%s',
                               configurator.name, trace)
            finally:
                configurator.reset_command_list()
                logging.info('Configuration of AP %s complete.',
                              configurator.name)
                self.cartridge.task_done()


    def run_configurators(self, broken_pdus):
        """Runs apply_settings for all configurators in the cartridge.

        @param broken_pdus: List of all the PDUs that are down.
        """
        for i in range(THREAD_MAX):
            t = Thread(target=self._apply_settings,args=(broken_pdus,))
            t.daemon = True
            t.start()
        self.cartridge.join()
