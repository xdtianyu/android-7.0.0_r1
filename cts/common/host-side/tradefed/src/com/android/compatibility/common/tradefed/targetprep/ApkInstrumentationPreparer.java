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

package com.android.compatibility.common.tradefed.targetprep;

import com.android.compatibility.common.tradefed.build.CompatibilityBuildHelper;
import com.android.compatibility.common.tradefed.util.NoOpTestInvocationListener;
import com.android.ddmlib.testrunner.TestIdentifier;
import com.android.tradefed.build.IBuildInfo;
import com.android.tradefed.config.Option;
import com.android.tradefed.config.Option.Importance;
import com.android.tradefed.config.OptionClass;
import com.android.tradefed.device.DeviceNotAvailableException;
import com.android.tradefed.device.ITestDevice;
import com.android.tradefed.result.ITestInvocationListener;
import com.android.tradefed.targetprep.BuildError;
import com.android.tradefed.targetprep.ITargetCleaner;
import com.android.tradefed.targetprep.TargetSetupError;
import com.android.tradefed.testtype.AndroidJUnitTest;

import java.io.File;
import java.io.FileNotFoundException;
import java.util.ArrayList;
import java.util.List;
import java.util.Map;
import java.util.concurrent.ConcurrentHashMap;

/**
 * Target preparer that instruments an APK.
 */
@OptionClass(alias="apk-instrumentation-preparer")
public class ApkInstrumentationPreparer extends PreconditionPreparer implements ITargetCleaner {

    @Option(name = "apk", description = "Name of the apk to instrument", mandatory = true)
    protected String mApkFileName = null;

    @Option(name = "package", description = "Name of the package", mandatory = true)
    protected String mPackageName = null;

    public enum When {
        BEFORE, AFTER, BOTH;
    }

    @Option(name = "when", description = "When to instrument the apk", mandatory = true)
    protected When mWhen = null;

    protected ConcurrentHashMap<TestIdentifier, Map<String, String>> testMetrics =
            new ConcurrentHashMap<>();
    private ConcurrentHashMap<TestIdentifier, String> testFailures = new ConcurrentHashMap<>();

    /**
     * {@inheritDoc}
     */
    @Override
    public void run(ITestDevice device, IBuildInfo buildInfo) throws TargetSetupError,
            BuildError, DeviceNotAvailableException {
        if (mWhen == When.AFTER) {
            return;
        }
        try {
            if (instrument(device, buildInfo)) {
                logInfo("Target preparation successful");
            } else {
                throw new TargetSetupError("Not all target preparation steps completed");
            }
        } catch (FileNotFoundException e) {
            throw new TargetSetupError("Couldn't find apk to instrument", e);
        }
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void tearDown(ITestDevice device, IBuildInfo buildInfo, Throwable e)
            throws DeviceNotAvailableException {
        if (e instanceof DeviceNotAvailableException) {
            return;
        }
        if (mWhen == When.BEFORE) {
            return;
        }
        try {
            instrument(device, buildInfo);
        } catch (FileNotFoundException e1) {
            logError("Couldn't find apk to instrument");
            logError(e1);
        }
    }

    private boolean instrument(ITestDevice device, IBuildInfo buildInfo)
            throws DeviceNotAvailableException, FileNotFoundException {
        ITestInvocationListener listener = new TargetPreparerListener();
        CompatibilityBuildHelper buildHelper = new CompatibilityBuildHelper(buildInfo);

        File testsDir = buildHelper.getTestsDir();
        File apkFile = new File(testsDir, mApkFileName);
        if (!apkFile.exists()) {
            throw new FileNotFoundException(String.format("%s not found", mApkFileName));
        }

        if (device.getAppPackageInfo(mPackageName) != null) {
            logInfo("Package %s already present on the device, uninstalling ...", mPackageName);
            device.uninstallPackage(mPackageName);
        }

        logInfo("Instrumenting package %s:", mPackageName);
        AndroidJUnitTest instrTest = new AndroidJUnitTest();
        instrTest.setDevice(device);
        instrTest.setInstallFile(apkFile);
        instrTest.setPackageName(mPackageName);
        instrTest.run(listener);
        boolean success = true;
        if (!testFailures.isEmpty()) {
            for (TestIdentifier test : testFailures.keySet()) {
                success = false;
                String trace = testFailures.get(test);
                logError("Target preparation step %s failed.\n%s", test.getTestName(), trace);
            }
        }
        return success;
    }

    public class TargetPreparerListener extends NoOpTestInvocationListener {

        @Override
        public void testEnded(TestIdentifier test, Map<String, String> metrics) {
            testMetrics.put(test, metrics);
        }

        @Override
        public void testFailed(TestIdentifier test, String trace) {
            testFailures.put(test, trace);
        }

    }

}
