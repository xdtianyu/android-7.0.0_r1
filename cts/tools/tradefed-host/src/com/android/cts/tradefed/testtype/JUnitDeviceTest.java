/*
 * Copyright (C) 2014 The Android Open Source Project
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

import com.android.cts.tradefed.build.CtsBuildHelper;
import com.android.ddmlib.testrunner.TestIdentifier;
import com.android.tradefed.build.IBuildInfo;
import com.android.tradefed.config.Option;
import com.android.tradefed.config.Option.Importance;
import com.android.tradefed.device.DeviceNotAvailableException;
import com.android.tradefed.device.ITestDevice;
import com.android.tradefed.log.LogUtil.CLog;
import com.android.tradefed.result.ITestInvocationListener;
import com.android.tradefed.testtype.IAbi;
import com.android.tradefed.testtype.IBuildReceiver;
import com.android.tradefed.testtype.IDeviceTest;
import com.android.tradefed.testtype.IRemoteTest;
import com.android.tradefed.util.AbiFormatter;
import com.android.tradefed.util.ArrayUtil;

import junit.framework.Assert;
import junit.framework.Test;

import java.io.File;
import java.io.FileNotFoundException;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Collection;
import java.util.Collections;
import java.util.HashSet;
import java.util.List;
import java.util.Set;
import java.util.concurrent.TimeUnit;

/**
 * {@link Test} for running CTS JUnit tests on the device.
 */
public class JUnitDeviceTest implements IDeviceTest, IRemoteTest, IBuildReceiver {

    private static final String TMP_DIR = "/data/local/tmp/";

    @Option(name = "junit-device-runtime",
            description = "The name of the runtime to use on the device",
            importance = Importance.ALWAYS)
    private String mRuntimePath = "dalvikvm|#ABI#|";

    @Option(name = "junit-device-tmpdir", description = "Device path where to store the test jars."
            , importance = Importance.IF_UNSET)
    private String mDeviceTestTmpPath = TMP_DIR;



    // default to no timeout
    private long mMaxTimeToOutputResponse = 0;

    private ITestDevice mDevice;
    private String mRunName;
    private Collection<TestIdentifier> mTests;
    private CtsBuildHelper mCtsBuild = null;

    private List<String> mJarPaths = new ArrayList<String>();

    private String mRuntimeArgs;

    private IAbi mAbi;

    private static final String JUNIT_JAR = "cts-junit.jar";

    private Set<String> mTestJars = new HashSet<String>(Arrays.asList(JUNIT_JAR));

    /**
     * @param abi The ABI to run the test on
     */
    public void setAbi(IAbi abi) {
        mAbi = abi;
    }

    @Override
    public ITestDevice getDevice() {
        return mDevice;
    }

    @Override
    public void setDevice(ITestDevice device) {
        mDevice = device;
    }

    public void addTestJarFileName(String jarFileName) {
        mTestJars.add(jarFileName);
    }

    public void setRunName(String runName) {
        mRunName = runName;
    }

    public void setTests(Collection<TestIdentifier> tests) {
        mTests = tests;
    }

    public Collection<TestIdentifier> getTests() {
        return mTests;
    }

    @Override
    public void run(ITestInvocationListener listener) throws DeviceNotAvailableException {
        addTestJarFileName(JUNIT_JAR);
        checkFields();
        long startTime = System.currentTimeMillis();
        listener.testRunStarted(mRunName, mTests.size());
        try {
            installJars();
            String jarPath = ArrayUtil.join(":", mJarPaths);
            for (TestIdentifier testId : mTests) {
                SingleJUnitTestResultParser resultParser = new SingleJUnitTestResultParser(
                        testId, listener);
                String cmdLine = String.format("ANDROID_DATA=%s %s -cp %s %s " +
                        "com.android.cts.junit.SingleJUnitTestRunner %s#%s",
                        mDeviceTestTmpPath, mRuntimePath, jarPath, mRuntimeArgs,
                        testId.getClassName(), testId.getTestName());
                String cmd = AbiFormatter.formatCmdForAbi(cmdLine, mAbi.getBitness());
                CLog.d("Running %s", cmd);
                listener.testStarted(testId);
                mDevice.executeShellCommand(cmd, resultParser, mMaxTimeToOutputResponse,
                        TimeUnit.MILLISECONDS, 0);
            }
        } finally {
            listener.testRunEnded(System.currentTimeMillis() - startTime,
                    Collections.<String, String> emptyMap());
            // Remove jar files from device
            removeJars();
        }
    }

    /**
     * Installs the jar files on the device under test.
     *
     * @throws DeviceNotAvailableException
     */
    protected void installJars() throws DeviceNotAvailableException {
        for (String f : mTestJars) {
            CLog.d("Installing %s on %s", f, getDevice().getSerialNumber());
            File jarFile;
            try {
                String fullJarPath = String.format("%s%s", mDeviceTestTmpPath, f);
                jarFile = mCtsBuild.getTestApp(f);
                boolean result = getDevice().pushFile(jarFile, fullJarPath);
                Assert.assertTrue(String.format("Failed to push file to %s", fullJarPath), result);
                mJarPaths.add(fullJarPath);
            } catch (FileNotFoundException e) {
                Assert.fail(String.format("Could not find file %s", f));
            }
        }
    }

    /**
     * Cleans up the jar files from the device under test.
     *
     * @throws DeviceNotAvailableException
     */
    protected void removeJars() throws DeviceNotAvailableException {
        for (String f : mTestJars) {
            String fullJarPath = String.format("%s%s", mDeviceTestTmpPath, f);
            CLog.d("Uninstalling %s on %s", fullJarPath, getDevice().getSerialNumber());
            getDevice().executeShellCommand(String.format("rm %s", fullJarPath));
        }
    }

    @Override
    public void setBuild(IBuildInfo buildInfo) {
        mCtsBuild = CtsBuildHelper.createBuildHelper(buildInfo);
    }

    /**
     * Checks that all mandatory member fields has been set.
     */
    protected void checkFields() {
        if (mRunName == null) {
            throw new IllegalArgumentException("run name has not been set");
        }
        if (mDevice == null) {
            throw new IllegalArgumentException("Device has not been set");
        }
        if (mTestJars.isEmpty()) {
            throw new IllegalArgumentException("No test jar has been set");
        }
        if (mTests == null) {
            throw new IllegalArgumentException("tests has not been set");
        }
        if (mCtsBuild == null) {
            throw new IllegalArgumentException("build has not been set");
        }
        for (String f : mTestJars) {
            try {

                mCtsBuild.getTestApp(f);
            } catch (FileNotFoundException e) {
                throw new IllegalArgumentException(String.format(
                        "Could not find jar %s in CTS build %s", f,
                        mCtsBuild.getRootDir().getAbsolutePath()));
            }
        }
    }

    /**
     * Add runtime arguments to run the tests with.
     *
     * @param mRunTimeArgs
     */
    public void addRunTimeArgs(String mRunTimeArgs) {
        mRuntimeArgs = mRunTimeArgs;
    }
}
