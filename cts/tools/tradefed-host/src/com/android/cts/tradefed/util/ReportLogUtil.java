/*
 * Copyright (C) 2016 The Android Open Source Project
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

package com.android.cts.tradefed.util;

import com.android.cts.tradefed.result.CtsXmlResultReporter;
import com.android.tradefed.build.IBuildInfo;
import com.android.tradefed.device.ITestDevice;
import com.android.tradefed.log.LogUtil.CLog;
import com.android.tradefed.util.FileUtil;

import java.io.File;

/**
 * Collects report logs from device and host after cts_v1 test runs.
 */
public class ReportLogUtil{

    /**
     * Directory values must match the src-dir, dest-dir and temp-dir values configured in
     * ReportLogCollector target preparer in
     * cts/tools/cts-tradefed/res/config/cts-preconditions.xml.
     */
    private static final String SRC_DIR = "/sdcard/report-log-files/";
    private static final String DEST_DIR = "report-log-files/";
    private static final String TEMP_REPORT_DIR= "temp-report-logs/";

    public static void prepareReportLogContainers(ITestDevice device, IBuildInfo buildInfo) {
        try {
            // Delete earlier report logs if present on device.
            String command = String.format("adb -s %s shell rm -rf %s", device.getSerialNumber(),
                    SRC_DIR);
            CLog.e(command);
            if (device.doesFileExist(SRC_DIR)) {
                Process process = Runtime.getRuntime().exec(new String[]{"/bin/bash", "-c",
                        command});
                if (process.waitFor() != 0) {
                    CLog.e("Failed to run %s", command);
                }
            }
            // Create folder in result directory to store report logs.
            File resultDir = new File(buildInfo.getBuildAttributes().get(
                    CtsXmlResultReporter.CTS_RESULT_DIR));
            if (DEST_DIR != null) {
                resultDir = new File(resultDir, DEST_DIR);
            }
            resultDir.mkdirs();
            if (!resultDir.isDirectory()) {
                CLog.e("%s is not a directory", resultDir.getAbsolutePath());
                return;
            }
        } catch (Exception e) {
            e.printStackTrace();
        }
    }

    public static void collectReportLogs(ITestDevice device, IBuildInfo buildInfo) {
        // Pull report log files from device and host.
        try {
            File resultDir = new File(buildInfo.getBuildAttributes().get(
                    CtsXmlResultReporter.CTS_RESULT_DIR));
            if (DEST_DIR != null) {
                resultDir = new File(resultDir, DEST_DIR);
            }
            resultDir.mkdirs();
            if (!resultDir.isDirectory()) {
                CLog.e("%s is not a directory", resultDir.getAbsolutePath());
                return;
            }
            final File hostReportDir = FileUtil.createNamedTempDir(TEMP_REPORT_DIR);
            if (!hostReportDir.isDirectory()) {
                CLog.e("%s is not a directory", hostReportDir.getAbsolutePath());
                return;
            }
            pull(device, SRC_DIR, hostReportDir, resultDir);
        } catch (Exception e) {
            e.printStackTrace();
        }
    }

    private static void pull(ITestDevice device, String deviceSrc, File hostDir, File destDir) {
        String hostSrc = hostDir.getAbsolutePath();
        String dest = destDir.getAbsolutePath();
        String deviceSideCommand = String.format("adb -s %s pull %s %s", device.getSerialNumber(),
                deviceSrc, dest);
        CLog.e(deviceSideCommand);
        try {
            if (device.doesFileExist(deviceSrc)) {
                Process deviceProcess = Runtime.getRuntime().exec(new String[]{"/bin/bash", "-c",
                        deviceSideCommand});
                if (deviceProcess.waitFor() != 0) {
                    CLog.e("Failed to run %s", deviceSideCommand);
                }
            }
            FileUtil.recursiveCopy(hostDir, destDir);
            FileUtil.recursiveDelete(hostDir);
        } catch (Exception e) {
            CLog.e("Caught exception during pull.");
            CLog.e(e);
        }
    }
}
