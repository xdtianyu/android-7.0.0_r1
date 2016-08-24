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
package com.android.compatibility.common.tradefed.build;

import com.android.compatibility.SuiteInfo;
import com.android.tradefed.build.BuildInfo;
import com.android.tradefed.build.BuildRetrievalError;
import com.android.tradefed.build.DeviceBuildInfo;
import com.android.tradefed.build.IBuildInfo;
import com.android.tradefed.build.IBuildProvider;
import com.android.tradefed.build.IDeviceBuildProvider;
import com.android.tradefed.config.Option;
import com.android.tradefed.config.OptionClass;
import com.android.tradefed.device.DeviceNotAvailableException;
import com.android.tradefed.device.ITestDevice;

import java.util.regex.Pattern;
/**
 * A simple {@link IBuildProvider} that uses a pre-existing Compatibility install.
 */
@OptionClass(alias="compatibility-build-provider")
public class CompatibilityBuildProvider implements IDeviceBuildProvider {

    private static final Pattern RELEASE_BUILD = Pattern.compile("^[A-Z]{3}\\d{2}[A-Z]{0,1}$");

    @Option(name="branch", description="build branch name to supply.")
    private String mBranch = null;

    @Option(name="use-device-build-info", description="Bootstrap build info from device")
    private boolean mUseDeviceBuildInfo = false;

    @Option(name="test-tag", description="test tag name to supply.")
    private String mTestTag = "cts";

    /**
     * {@inheritDoc}
     */
    @Override
    public IBuildInfo getBuild() {
        // Create a blank BuildInfo which will get populated later.
        String version = SuiteInfo.BUILD_NUMBER;
        if (version == null) {
            version = IBuildInfo.UNKNOWN_BUILD_ID;
        }
        IBuildInfo ctsBuild = new BuildInfo(version, mTestTag, mTestTag);
        if (mBranch  != null) {
            ctsBuild.setBuildBranch(mBranch);
        }

        return ctsBuild;
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public IBuildInfo getBuild(ITestDevice device)
            throws BuildRetrievalError, DeviceNotAvailableException {
        if (!mUseDeviceBuildInfo) {
            // return a regular build info without extracting device attributes into standard
            // build info fields
            return getBuild();
        } else {
            String buildId = device.getBuildId();
            String buildFlavor = device.getBuildFlavor();
            IBuildInfo info = new DeviceBuildInfo(buildId, mTestTag, buildFlavor);
            if (mBranch == null) {
                // if branch is not specified via param, make a pseudo branch name based on platform
                // version and product info from device
                mBranch = String.format("%s-%s-%s-%s",
                        device.getProperty("ro.product.brand"),
                        device.getProperty("ro.product.name"),
                        device.getProductVariant(),
                        device.getProperty("ro.build.version.release"));
            }
            info.setBuildBranch(mBranch);
            info.setBuildFlavor(buildFlavor);
            String buildAlias = device.getBuildAlias();
            if (RELEASE_BUILD.matcher(buildAlias).matches()) {
                info.addBuildAttribute("build_alias", buildAlias);
            }
            return info;
        }
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void buildNotTested(IBuildInfo info) {
        // ignore
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void cleanUp(IBuildInfo info) {
        // ignore
    }
}
