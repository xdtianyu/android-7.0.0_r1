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
import com.android.tradefed.build.IBuildInfo;
import com.android.tradefed.config.Option;
import com.android.tradefed.device.DeviceNotAvailableException;
import com.android.tradefed.device.ITestDevice;
import com.android.tradefed.log.LogUtil.CLog;
import com.android.tradefed.targetprep.BuildError;
import com.android.tradefed.targetprep.ITargetCleaner;
import com.android.tradefed.targetprep.TargetSetupError;
import com.android.tradefed.util.FileUtil;

import java.io.BufferedReader;
import java.io.BufferedWriter;
import java.io.File;
import java.io.FileNotFoundException;
import java.io.FileReader;
import java.io.FileWriter;
import java.io.IOException;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

/**
 * An {@link ITargetCleaner} that prepares and pulls report logs.
 */
public class ReportLogCollector implements ITargetCleaner {

    @Option(name= "src-dir", description = "The directory to copy to the results dir")
    private String mSrcDir;

    @Option(name = "dest-dir", description = "The directory under the result to store the files")
    private String mDestDir;

    @Option(name = "temp-dir", description = "The temp directory containing host-side report logs")
    private String mTempReportFolder;

    public ReportLogCollector() {
    }

    @Override
    public void setUp(ITestDevice device, IBuildInfo buildInfo) throws TargetSetupError,
            BuildError, DeviceNotAvailableException {
        prepareReportLogContainers(device, buildInfo);
    }

    private void addBuildInfo(ITestDevice device, IBuildInfo buildInfo, String key, String value)
            throws DeviceNotAvailableException {
    }

    private void prepareReportLogContainers(ITestDevice device, IBuildInfo buildInfo) {
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
        } catch (FileNotFoundException fnfe) {
            fnfe.printStackTrace();
        }
    }

    @Override
    public void tearDown(ITestDevice device, IBuildInfo buildInfo, Throwable e) {
        // Pull report log files from device.
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
            final File hostReportDir = FileUtil.createNamedTempDir(mTempReportFolder);
            if (!hostReportDir.isDirectory()) {
                CLog.e("%s is not a directory", hostReportDir.getAbsolutePath());
                return;
            }
            pull(device, mSrcDir, hostReportDir, resultDir);
            reformatRepeatedStreams(resultDir);
        } catch (Exception exception) {
            exception.printStackTrace();
        }
    }

    private void pull(ITestDevice device, String deviceSrc, File hostDir, File destDir) {
        String hostSrc = hostDir.getAbsolutePath();
        String dest = destDir.getAbsolutePath();
        String deviceSideCommand = String.format("adb -s %s pull %s %s", device.getSerialNumber(),
                deviceSrc, dest);
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

    private void reformatRepeatedStreams(File resultDir) throws IOException, FileNotFoundException {
        File[] reportLogs = resultDir.listFiles();
        // Sometimes report logs are in a sub-directory.
        if (reportLogs.length == 1 && reportLogs[0].isDirectory()) {
            reportLogs = reportLogs[0].listFiles();
        }
        for (File reportLog : reportLogs) {
            try (BufferedReader metricsReader = new BufferedReader(new FileReader(reportLog))) {
                // Get metrics as string.
                StringBuilder metricBuilder = new StringBuilder();
                String line;
                while ((line = metricsReader.readLine()) != null) {
                    metricBuilder.append(line);
                }
                String metrics = metricBuilder.toString();
                // Create map of stream names and metrics.
                HashMap<String, List<String>> metricsMap = new HashMap<>();
                String pattern = "\\\"([a-z0-9_]*)\\\":(\\{[^{}]*\\})";
                Pattern p = Pattern.compile(pattern);
                Matcher m = p.matcher(metrics);
                while (m.find()) {
                    String key = m.group(1);
                    String value = m.group(2);
                    if (!metricsMap.containsKey(key)) {
                        metricsMap.put(key, new ArrayList<String>());
                    }
                    metricsMap.get(key).add(value);
                }
                // Rewrite metrics as arrays.
                StringBuilder newMetricsBuilder = new StringBuilder();
                newMetricsBuilder.append("{");
                boolean firstLine = true;
                for (String key: metricsMap.keySet()) {
                    if (!firstLine) {
                        newMetricsBuilder.append(",");
                    } else {
                        firstLine = false;
                    }
                    newMetricsBuilder.append("\"").append(key).append("\":[");
                    boolean firstValue = true;
                    for (String stream : metricsMap.get(key)) {
                        if (!firstValue) {
                            newMetricsBuilder.append(",");
                        }
                        else {
                            firstValue = false;
                        }
                        newMetricsBuilder.append(stream);
                    }
                    newMetricsBuilder.append("]");
                }
                newMetricsBuilder.append("}");
                reportLog.createNewFile();
                try (BufferedWriter metricsWriter = new BufferedWriter(new
                        FileWriter(reportLog))) {
                    String newMetrics = newMetricsBuilder.toString();
                    metricsWriter.write(newMetrics, 0, newMetrics.length());
                }
            }
        }
    }
}
