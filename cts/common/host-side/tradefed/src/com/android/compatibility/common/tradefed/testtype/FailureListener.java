/*
 * Copyright (C) 2015 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
package com.android.compatibility.common.tradefed.testtype;

import com.android.ddmlib.testrunner.TestIdentifier;
import com.android.tradefed.device.DeviceNotAvailableException;
import com.android.tradefed.device.ITestDevice;
import com.android.tradefed.log.LogUtil.CLog;
import com.android.tradefed.result.ITestInvocationListener;
import com.android.tradefed.result.InputStreamSource;
import com.android.tradefed.result.LogDataType;
import com.android.tradefed.result.ResultForwarder;
import com.android.tradefed.util.RunUtil;

public class FailureListener extends ResultForwarder {

    private static final int MAX_LOGCAT_BYTES = 500 * 1024; // 500K

    private ITestDevice mDevice;
    private boolean mBugReportOnFailure;
    private boolean mLogcatOnFailure;
    private boolean mScreenshotOnFailure;
    private boolean mRebootOnFailure;

    public FailureListener(ITestInvocationListener listener, ITestDevice device,
            boolean bugReportOnFailure, boolean logcatOnFailure, boolean screenshotOnFailure,
            boolean rebootOnFailure) {
        super(listener);
        mDevice = device;
        mBugReportOnFailure = bugReportOnFailure;
        mLogcatOnFailure = logcatOnFailure;
        mScreenshotOnFailure = screenshotOnFailure;
        mRebootOnFailure = rebootOnFailure;
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void testFailed(TestIdentifier test, String trace) {
        super.testFailed(test, trace);
        CLog.i("FailureListener.testFailed %s %b %b %b", test.toString(), mBugReportOnFailure, mLogcatOnFailure, mScreenshotOnFailure);
        if (mBugReportOnFailure) {
           InputStreamSource bugSource = mDevice.getBugreport();
           super.testLog(String.format("%s-bugreport", test.toString()), LogDataType.BUGREPORT,
                   bugSource);
           bugSource.cancel();
        }
        if (mLogcatOnFailure) {
            // sleep 2s to ensure test failure stack trace makes it into logcat capture
            RunUtil.getDefault().sleep(2 * 1000);
            InputStreamSource logSource = mDevice.getLogcat(MAX_LOGCAT_BYTES);
            super.testLog(String.format("%s-logcat", test.toString()), LogDataType.LOGCAT,
                    logSource);
            logSource.cancel();
        }
        if (mScreenshotOnFailure) {
            try {
                InputStreamSource screenSource = mDevice.getScreenshot();
                super.testLog(String.format("%s-screenshot", test.toString()), LogDataType.PNG,
                        screenSource);
                screenSource.cancel();
            } catch (DeviceNotAvailableException e) {
                CLog.e(e);
                CLog.e("Device %s became unavailable while capturing screenshot",
                        mDevice.getSerialNumber());
            }
        }
        if (mRebootOnFailure) {
            try {
                // Rebooting on all failures can hide legitimate issues and platform instabilities,
                // therefore only allowed on "user-debug" and "eng" builds.
                if ("user".equals(mDevice.getProperty("ro.build.type"))) {
                    CLog.e("Reboot-on-failure should only be used during development," +
                            " this is a\" user\" build device");
                } else {
                    mDevice.reboot();
                }
            } catch (DeviceNotAvailableException e) {
                CLog.e(e);
                CLog.e("Device %s became unavailable while rebooting",
                        mDevice.getSerialNumber());
            }
        }
    }

}
