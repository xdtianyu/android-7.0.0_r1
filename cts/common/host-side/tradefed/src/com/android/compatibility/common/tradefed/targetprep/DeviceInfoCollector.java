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
import com.android.compatibility.common.tradefed.testtype.CompatibilityTest;
import com.android.tradefed.build.IBuildInfo;
import com.android.tradefed.config.Option;
import com.android.tradefed.device.DeviceNotAvailableException;
import com.android.tradefed.device.ITestDevice;
import com.android.tradefed.log.LogUtil.CLog;
import com.android.tradefed.targetprep.BuildError;
import com.android.tradefed.targetprep.TargetSetupError;
import com.android.tradefed.util.ArrayUtil;

import java.io.File;
import java.io.FileNotFoundException;
import java.util.HashMap;
import java.util.Map;
import java.util.Map.Entry;

/**
 * An {@link ApkInstrumentationPreparer} that collects device info.
 */
public class DeviceInfoCollector extends ApkInstrumentationPreparer {

    private static final Map<String, String> BUILD_KEYS = new HashMap<>();
    static {
        BUILD_KEYS.put("cts:build_id", "ro.build.id");
        BUILD_KEYS.put("cts:build_product", "ro.product.name");
        BUILD_KEYS.put("cts:build_device", "ro.product.device");
        BUILD_KEYS.put("cts:build_board", "ro.product.board");
        BUILD_KEYS.put("cts:build_manufacturer", "ro.product.manufacturer");
        BUILD_KEYS.put("cts:build_brand", "ro.product.brand");
        BUILD_KEYS.put("cts:build_model", "ro.product.model");
        BUILD_KEYS.put("cts:build_type", "ro.build.type");
        BUILD_KEYS.put("cts:build_tags", "ro.build.tags");
        BUILD_KEYS.put("cts:build_fingerprint", "ro.build.fingerprint");
        BUILD_KEYS.put("cts:build_abi", "ro.product.cpu.abi");
        BUILD_KEYS.put("cts:build_abi2", "ro.product.cpu.abi2");
        BUILD_KEYS.put("cts:build_abis", "ro.product.cpu.abilist");
        BUILD_KEYS.put("cts:build_abis_32", "ro.product.cpu.abilist32");
        BUILD_KEYS.put("cts:build_abis_64", "ro.product.cpu.abilist64");
        BUILD_KEYS.put("cts:build_serial", "ro.serialno");
        BUILD_KEYS.put("cts:build_version_release", "ro.build.version.release");
        BUILD_KEYS.put("cts:build_version_sdk", "ro.build.version.sdk");
        BUILD_KEYS.put("cts:build_version_base_os", "ro.build.version.base_os");
        BUILD_KEYS.put("cts:build_version_security_patch", "ro.build.version.security_patch");
        BUILD_KEYS.put("cts:build_reference_fingerprint", "ro.build.reference.fingerprint");
    }

    @Option(name = CompatibilityTest.SKIP_DEVICE_INFO_OPTION,
            shortName = 'd',
            description = "Whether device info collection should be skipped")
    private boolean mSkipDeviceInfo = false;

    @Option(name= "src-dir", description = "The directory to copy to the results dir")
    private String mSrcDir;

    @Option(name = "dest-dir", description = "The directory under the result to store the files")
    private String mDestDir;

    public DeviceInfoCollector() {
        mWhen = When.BEFORE;
    }

    @Override
    public void setUp(ITestDevice device, IBuildInfo buildInfo) throws TargetSetupError,
            BuildError, DeviceNotAvailableException {
        for (Entry<String, String> entry : BUILD_KEYS.entrySet()) {
            buildInfo.addBuildAttribute(entry.getKey(),
                    ArrayUtil.join(",", device.getProperty(entry.getValue())));
        }
        if (mSkipDeviceInfo) {
            return;
        }
        run(device, buildInfo);
        getDeviceInfoFiles(device, buildInfo);
    }

    private void getDeviceInfoFiles(ITestDevice device, IBuildInfo buildInfo) {
        CompatibilityBuildHelper buildHelper = new CompatibilityBuildHelper(buildInfo);
        try {
            File resultDir = buildHelper.getResultDir();
            if (mDestDir != null) {
                resultDir = new File(resultDir, mDestDir);
            }
            resultDir.mkdirs();
            if (!resultDir.isDirectory()) {
                CLog.e("%s is not a directory", resultDir.getAbsolutePath());
                return;
            }
            String resultPath = resultDir.getAbsolutePath();
            pull(device, mSrcDir, resultPath);
        } catch (FileNotFoundException fnfe) {
            fnfe.printStackTrace();
        }
    }

    private void pull(ITestDevice device, String src, String dest) {
        String command = String.format("adb -s %s pull %s %s", device.getSerialNumber(), src, dest);
        try {
            Process p = Runtime.getRuntime().exec(new String[] {"/bin/bash", "-c", command});
            if (p.waitFor() != 0) {
                CLog.e("Failed to run %s", command);
            }
        } catch (Exception e) {
            CLog.e("Caught exception during pull.");
            CLog.e(e);
        }
    }
}
