/*
 * Copyright 2012 The Android Open Source Project
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

package com.android.cts.tradefed.testtype;

import com.android.compatibility.common.util.AbiUtils;
import com.android.cts.tradefed.build.CtsBuildHelper;
import com.android.ddmlib.testrunner.ITestRunListener;
import com.android.tradefed.build.IBuildInfo;
import com.android.tradefed.device.DeviceNotAvailableException;
import com.android.tradefed.device.ITestDevice;
import com.android.tradefed.log.LogUtil.CLog;
import com.android.tradefed.result.ITestInvocationListener;
import com.android.tradefed.testtype.IAbi;
import com.android.tradefed.testtype.IBuildReceiver;
import com.android.tradefed.testtype.IDeviceTest;
import com.android.tradefed.testtype.IRemoteTest;

import java.io.File;
import java.io.FileNotFoundException;

/**
 * Test runner for wrapped (native) GTests
 */
public class WrappedGTest implements IBuildReceiver, IDeviceTest, IRemoteTest {

    private static final String LOG_TAG = WrappedGTest.class.getSimpleName();

    private int mMaxTestTimeMs = 1 * 60 * 1000;

    private CtsBuildHelper mCtsBuild;
    private ITestDevice mDevice;
    private IAbi mAbi;

    private final String mAppNameSpace;
    private final String mPackageName;
    private final String mName;
    private final String mRunner;

    public WrappedGTest(String appNameSpace, String packageName, String name, String runner) {
        mAppNameSpace = appNameSpace;
        mPackageName = packageName;
        mName = name;
        mRunner = runner;
    }

    /**
     * @param abi The ABI to run the test on
     */
    public void setAbi(IAbi abi) {
        mAbi = abi;
    }

    @Override
    public void setBuild(IBuildInfo buildInfo) {
        mCtsBuild = CtsBuildHelper.createBuildHelper(buildInfo);
    }

    @Override
    public void setDevice(ITestDevice device) {
        mDevice = device;
    }

    @Override
    public ITestDevice getDevice() {
        return mDevice;
    }

    @Override
    public void run(ITestInvocationListener listener) throws DeviceNotAvailableException {
        if (installTest()) {
            runTest(listener);
            uninstallTest();
        } else {
            CLog.e("Failed to install test");
        }
    }

    private boolean installTest() throws DeviceNotAvailableException {
        try {
            File testApp = mCtsBuild.getTestApp(String.format("%s.apk", mName));
            String[] options = {AbiUtils.createAbiFlag(mAbi.getName())};
            String installCode = mDevice.installPackage(testApp, true, options);

            if (installCode != null) {
                CLog.e("Failed to install %s.apk on %s. Reason: %s", mName,
                    mDevice.getSerialNumber(), installCode);
                return false;
            }
        }
        catch (FileNotFoundException e) {
            CLog.e("Package %s.apk not found", mName);
            return false;
        }
        return true;
    }

    private void runTest(ITestRunListener listener) throws DeviceNotAvailableException {
        String id = AbiUtils.createId(mAbi.getName(), mPackageName);
        WrappedGTestResultParser resultParser = new WrappedGTestResultParser(id, listener);
        resultParser.setFakePackagePrefix(mPackageName + ".");
        try {
            String options = mAbi == null ? "" : String.format("--abi %s ", mAbi.getName());
            String command = String.format("am instrument -w %s%s/.%s", options, mAppNameSpace, mRunner);
            mDevice.executeShellCommand(command, resultParser, mMaxTestTimeMs, 0);
        } catch (DeviceNotAvailableException e) {
            resultParser.flush();
            throw e;
        } catch (RuntimeException e) {
            resultParser.flush();
            throw e;
        }
    }

    private void uninstallTest() throws DeviceNotAvailableException {
        mDevice.uninstallPackage(mAppNameSpace);
    }
}
