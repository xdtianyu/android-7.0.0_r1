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
import sys
import threading
import time
import traceback
import Queue
sys.path.append(sys.path[0])
from android_device import *

CTS_THEME_dict = {
    120 : "ldpi",
    160 : "mdpi",
    213 : "tvdpi",
    240 : "hdpi",
    320 : "xhdpi",
    480 : "xxhdpi",
    640 : "xxxhdpi",
}

OUT_FILE = "/sdcard/cts-theme-assets.zip"

# pass a function with number of instances to be executed in parallel
# each thread continues until config q is empty.
def executeParallel(tasks, setup, q, numberThreads):
    class ParallelExecutor(threading.Thread):
        def __init__(self, tasks, q):
            threading.Thread.__init__(self)
            self._q = q
            self._tasks = tasks
            self._setup = setup
            self._result = 0

        def run(self):
            try:
                while True:
                    config = q.get(block=True, timeout=2)
                    for t in self._tasks:
                        try:
                            if t(self._setup, config):
                                self._result += 1
                        except KeyboardInterrupt:
                            raise
                        except:
                            print "Failed to execute thread:", sys.exc_info()[0]
                            traceback.print_exc()
                    q.task_done()
            except KeyboardInterrupt:
                raise
            except Queue.Empty:
                pass

        def getResult(self):
            return self._result

    result = 0;
    threads = []
    for i in range(numberThreads):
        t = ParallelExecutor(tasks, q)
        t.start()
        threads.append(t)
    for t in threads:
        t.join()
        result += t.getResult()
    return result;

def printAdbResult(device, out, err):
    print "device: " + device
    if out is not None:
        print "out:\n" + out
    if err is not None:
        print "err:\n" + err

def getResDir(outPath, resName):
    resDir = outPath + "/" + resName
    return resDir

def doCapturing(setup, deviceSerial):
    (themeApkPath, outPath) = setup

    print "Found device: " + deviceSerial
    device = androidDevice(deviceSerial)

    outPath = outPath + "/%d" % (device.getSdkLevel())
    density = device.getDensity()
    if CTS_THEME_dict.has_key(density):
        resName = CTS_THEME_dict[density]
    else:
        resName = str(density) + "dpi"

    device.uninstallApk("android.theme.app")

    (out, err, success) = device.installApk(themeApkPath)
    if not success:
        print "Failed to install APK on " + deviceSerial
        printAdbResult(deviceSerial, out, err)
        return False

    print "Generating images on " + deviceSerial + "..."
    try:
        (out, err) = device.runInstrumentationTest("android.theme.app/android.support.test.runner.AndroidJUnitRunner")
    except KeyboardInterrupt:
        raise
    except:
        (out, err) = device.runInstrumentationTest("android.theme.app/android.test.InstrumentationTestRunner")

    # Detect test failure and abort.
    if "FAILURES!!!" in out.split():
        printAdbResult(deviceSerial, out, err)
        return False

    # Make sure that the run is complete by checking the process itself
    print "Waiting for " + deviceSerial + "..."
    waitTime = 0
    while device.isProcessAlive("android.theme.app"):
        time.sleep(1)
        waitTime = waitTime + 1
        if waitTime > 180:
            print "Timed out"
            break

    time.sleep(10)
    resDir = getResDir(outPath, resName)

    print "Pulling images from " + deviceSerial + " to " + resDir + ".zip"
    device.runAdbCommand("pull " + OUT_FILE + " " + resDir + ".zip")
    device.runAdbCommand("shell rm -rf " + OUT_FILE)
    return True

def main(argv):
    if len(argv) < 3:
        print "run_theme_capture_device.py themeApkPath outDir"
        sys.exit(1)
    themeApkPath = argv[1]
    outPath = os.path.abspath(argv[2])
    os.system("mkdir -p " + outPath)

    tasks = []
    tasks.append(doCapturing)

    devices = runAdbDevices();
    numberThreads = len(devices)

    configQ = Queue.Queue()
    for device in devices:
        configQ.put(device)
    setup = (themeApkPath, outPath)
    result = executeParallel(tasks, setup, configQ, numberThreads)

    if result > 0:
        print 'Generated reference images for %(count)d devices' % {"count": result}
    else:
        print 'Failed to generate reference images'

if __name__ == '__main__':
    main(sys.argv)
