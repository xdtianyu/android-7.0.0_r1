#!/usr/bin/env python
#
# Copyright (C) 2015 The Android Open Source Project
#
# Licensed under the Apache License, Version 2.0 (the 'License');
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an 'AS IS' BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#

import os
import re
import sys
import threading
import subprocess
import time

# class for running android device from python
# it will fork the device processor
class androidDevice(object):
    def __init__(self, adbDevice):
        self._adbDevice = adbDevice

    def runAdbCommand(self, cmd):
        self.waitForAdbDevice()
        adbCmd = "adb -s %s %s" %(self._adbDevice, cmd)
        adbProcess = subprocess.Popen(adbCmd.split(" "), bufsize = -1, stdout = subprocess.PIPE, stderr = subprocess.PIPE)
        return adbProcess.communicate()

    def runShellCommand(self, cmd):
        return self.runAdbCommand("shell " + cmd)

    def waitForAdbDevice(self):
        os.system("adb -s %s wait-for-device" %self._adbDevice)

    def waitForBootComplete(self, timeout = 240):
        boot_complete = False
        attempts = 0
        wait_period = 5
        while not boot_complete and (attempts*wait_period) < timeout:
            (output, err) = self.runShellCommand("getprop dev.bootcomplete")
            output = output.strip()
            if output == "1":
                boot_complete = True
            else:
                time.sleep(wait_period)
                attempts += 1
        if not boot_complete:
            print "***boot not complete within timeout. will proceed to the next step"
        return boot_complete

    def installApk(self, apkPath):
        (out, err) = self.runAdbCommand("install -r -d -g " + apkPath)
        result = err.split()
        return (out, err, "Success" in result)

    def uninstallApk(self, package):
        (out, err) = self.runAdbCommand("uninstall " + package)
        result = err.split()
        return "Success" in result

    def runInstrumentationTest(self, option):
        return self.runShellCommand("am instrument -w " + option)

    def isProcessAlive(self, processName):
        (out, err) = self.runShellCommand("ps")
        names = out.split()
        # very lazy implementation as it does not filter out things like uid
        # should work mostly unless processName is too simple to overlap with
        # uid. So only use name like com.android.xyz
        return processName in names

    def getDensity(self):
        if "emulator" in self._adbDevice:
          return int(self.runShellCommand("getprop qemu.sf.lcd_density")[0])
        else:
          return int(self.runShellCommand("getprop ro.sf.lcd_density")[0])

    def getSdkLevel(self):
        return int(self.runShellCommand("getprop ro.build.version.sdk")[0])

    def getOrientation(self):
        return int(self.runShellCommand("dumpsys | grep SurfaceOrientation")[0].split()[1])

def runAdbDevices():
    devices = subprocess.check_output(["adb", "devices"])
    devices = devices.split('\n')[1:]

    deviceSerial = []

    for device in devices:
        if device is not "":
            info = device.split('\t')
            if info[1] == "device":
                deviceSerial.append(info[0])

    return deviceSerial

if __name__ == '__main__':
    main(sys.argv)
