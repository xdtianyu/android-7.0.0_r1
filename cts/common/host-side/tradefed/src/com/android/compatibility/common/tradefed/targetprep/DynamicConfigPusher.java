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
import com.android.compatibility.common.util.DynamicConfig;
import com.android.compatibility.common.util.DynamicConfigHandler;
import com.android.ddmlib.Log;
import com.android.tradefed.build.IBuildInfo;
import com.android.tradefed.config.Option;
import com.android.tradefed.config.OptionClass;
import com.android.tradefed.device.DeviceNotAvailableException;
import com.android.tradefed.device.ITestDevice;
import com.android.tradefed.log.LogUtil;
import com.android.tradefed.targetprep.BuildError;
import com.android.tradefed.targetprep.ITargetCleaner;
import com.android.tradefed.targetprep.TargetSetupError;
import com.android.tradefed.util.FileUtil;
import com.android.tradefed.util.StreamUtil;

import org.json.JSONException;
import org.xmlpull.v1.XmlPullParserException;

import java.io.File;
import java.io.FileNotFoundException;
import java.io.IOException;
import java.net.URL;

/**
 * Pushes dynamic config files from config repository
 */
@OptionClass(alias="dynamic-config-pusher")
public class DynamicConfigPusher implements ITargetCleaner {
    public enum TestTarget {
        DEVICE,
        HOST
    }

    private static final String LOG_TAG = DynamicConfigPusher.class.getSimpleName();

    @Option(name = "cleanup", description = "Whether to remove config files from the test " +
            "target after test completion.")
    private boolean mCleanup = true;

    @Option(name="config-filename", description = "The module name for module-level " +
            "configurations, or the suite name for suite-level configurations", mandatory = true)
    private String mModuleName;

    @Option(name = "target", description = "The test target, \"device\" or \"host\"",
            mandatory = true)
    private TestTarget mTarget;

    @Option(name = "version", description = "The version of the configuration to retrieve " +
            "from the server, e.g. \"1.0\". Defaults to suite version string.")
    private static String mVersion;


    private String mFilePushed;

    void setModuleName(String moduleName) {
        mModuleName = moduleName;
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void setUp(ITestDevice device, IBuildInfo buildInfo) throws TargetSetupError, BuildError,
            DeviceNotAvailableException {

        CompatibilityBuildHelper buildHelper = new CompatibilityBuildHelper(buildInfo);

        File localConfigFile = null;
        try {
            localConfigFile = DynamicConfig.getConfigFile(buildHelper.getTestsDir(), mModuleName);
        } catch (FileNotFoundException e) {
            throw new TargetSetupError(
                    "Cannot get local dynamic config file from test directory", e);
        }

        if (mVersion == null) {
            mVersion = buildHelper.getSuiteVersion();
        }

        String apfeConfigInJson = null;
        String originUrl = buildHelper.getDynamicConfigUrl();

        if (originUrl != null) {
            String requestUrl = originUrl;
            try {
                requestUrl = originUrl
                        .replace("{module}", mModuleName).replace("{version}", mVersion);
                java.net.URL request = new URL(requestUrl);
                apfeConfigInJson = StreamUtil.getStringFromStream(request.openStream());
            } catch (IOException e) {
                LogUtil.printLog(Log.LogLevel.WARN, LOG_TAG,
                        "Cannot download and parse json config from URL " + requestUrl);
            }
        } else {
            LogUtil.printLog(Log.LogLevel.INFO, LOG_TAG,
                    "Dynamic config override URL is not set, using local configuration values");
        }

        File src = null;
        try {
            src = DynamicConfigHandler.getMergedDynamicConfigFile(
                    localConfigFile, apfeConfigInJson, mModuleName);
        } catch (IOException | XmlPullParserException | JSONException e) {
            throw new TargetSetupError("Cannot get merged dynamic config file", e);
        }

        switch (mTarget) {
            case DEVICE:
                String deviceDest = DynamicConfig.CONFIG_FOLDER_ON_DEVICE + src.getName();
                if (!device.pushFile(src, deviceDest)) {
                    throw new TargetSetupError(String.format(
                            "Failed to push local '%s' to remote '%s'",
                            src.getAbsolutePath(), deviceDest));
                } else {
                    mFilePushed = deviceDest;
                    buildHelper.addDynamicConfigFile(mModuleName, src);
                }
                break;

            case HOST:
                File storageDir = new File(DynamicConfig.CONFIG_FOLDER_ON_HOST);
                if (!storageDir.exists()) {
                    storageDir.mkdir();
                }
                File hostDest = new File(DynamicConfig.CONFIG_FOLDER_ON_HOST + src.getName());
                try {
                    FileUtil.copyFile(src, hostDest);
                } catch (IOException e) {
                    throw new TargetSetupError(String.format("Failed to copy file from %s to %s",
                            src.getAbsolutePath(), hostDest.getAbsolutePath()), e);
                }
                mFilePushed = hostDest.getAbsolutePath();
                buildHelper.addDynamicConfigFile(mModuleName, src);
                break;
        }
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void tearDown(ITestDevice device, IBuildInfo buildInfo, Throwable e)
            throws DeviceNotAvailableException {
        switch (mTarget) {
            case DEVICE:
                if (!(e instanceof DeviceNotAvailableException)
                        && mCleanup && mFilePushed != null) {
                    device.executeShellCommand("rm -r " + mFilePushed);
                }
                break;
            case HOST:
                if (mFilePushed != null) {
                    FileUtil.deleteFile(new File(mFilePushed));
                }
        }
    }
}
