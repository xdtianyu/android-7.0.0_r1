#!/usr/bin/env python3.4
#
#   Copyright 2016 - The Android Open Source Project
#
#   Licensed under the Apache License, Version 2.0 (the "License");
#   you may not use this file except in compliance with the License.
#   You may obtain a copy of the License at
#
#       http://www.apache.org/licenses/LICENSE-2.0
#
#   Unless required by applicable law or agreed to in writing, software
#   distributed under the License is distributed on an "AS IS" BASIS,
#   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#   See the License for the specific language governing permissions and
#   limitations under the License.

from acts.controllers.android_device import AndroidDevice
from acts.controllers.adb import is_port_available
from acts.controllers.adb import get_available_host_port
import acts.controllers.native as native
from subprocess import call

import time

#TODO(tturney): Merge this into android device

ACTS_CONTROLLER_CONFIG_NAME = "NativeAndroidDevice"
ACTS_CONTROLLER_REFERENCE_NAME = "native_android_devices"

def create(configs, logger):
    ads = get_instances(configs, logger)
    for ad in ads:
        try:
            ad.get_droid()
        except:
            logger.exception("Failed to start sl4n on %s" % ad.serial)
    return ads

def destroy(ads):
    pass

def get_instances(serials, logger=None):
    """Create AndroidDevice instances from a list of serials.

    Args:
        serials: A list of android device serials.
        logger: A logger to be passed to each instance.

    Returns:
        A list of AndroidDevice objects.
    """
    results = []
    for s in serials:
        results.append(NativeAndroidDevice(s, logger=logger))
    return results

class NativeAndroidDeviceError(Exception):
    pass

class NativeAndroidDevice(AndroidDevice):

    def __del__(self):
        if self.h_port:
            self.adb.forward("--remove tcp:%d" % self.h_port)

    def get_droid(self, handle_event=True):
        """Create an sl4n connection to the device.

        Return the connection handler 'droid'. By default, another connection
        on the same session is made for EventDispatcher, and the dispatcher is
        returned to the caller as well.
        If sl4n server is not started on the device, try to start it.

        Args:
            handle_event: True if this droid session will need to handle
                events.

        Returns:
            droid: Android object useds to communicate with sl4n on the android
                device.
            ed: An optional EventDispatcher to organize events for this droid.

        Examples:
            Don't need event handling:
            >>> ad = NativeAndroidDevice()
            >>> droid = ad.get_droid(False)

            Need event handling:
            >>> ad = NativeAndroidDevice()
            >>> droid, ed = ad.get_droid()
        """
        if not self.h_port or not is_port_available(self.h_port):
            self.h_port = get_available_host_port()
        self.adb.tcp_forward(self.h_port, self.d_port)
        pid = self.adb.shell(
            "ps | grep sl4n | awk '{print $2}'").decode('ascii')
        while (pid):
            self.adb.shell("kill {}".format(pid))
            pid = self.adb.shell(
                "ps | grep sl4n | awk '{print $2}'").decode('ascii')
        call(["adb -s " + self.serial + " shell sh -c \"/system/bin/sl4n\" &"],
            shell=True)
        try:
            time.sleep(3)
            droid = self.start_new_session()
        except:
            droid = self.start_new_session()
        return droid

    def start_new_session(self):
        """Start a new session in sl4n.

        Also caches the droid in a dict with its uid being the key.

        Returns:
            An Android object used to communicate with sl4n on the android
                device.

        Raises:
            sl4nException: Something is wrong with sl4n and it returned an
            existing uid to a new session.
        """
        droid = native.NativeAndroid(port=self.h_port)
        if droid.uid in self._droid_sessions:
            raise bt.SL4NException(("SL4N returned an existing uid for a "
                "new session. Abort."))
            return droid
        self._droid_sessions[droid.uid] = [droid]
        return droid
