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
package com.android.compatibility.common.tradefed.result;

import com.android.compatibility.common.tradefed.build.CompatibilityBuildHelper;
import com.android.compatibility.common.tradefed.testtype.CompatibilityTest;
import com.android.compatibility.common.util.ICaseResult;
import com.android.compatibility.common.util.IInvocationResult;
import com.android.compatibility.common.util.IModuleResult;
import com.android.compatibility.common.util.ITestResult;
import com.android.compatibility.common.util.InvocationResult;
import com.android.compatibility.common.util.MetricsStore;
import com.android.compatibility.common.util.ReportLog;
import com.android.compatibility.common.util.ResultHandler;
import com.android.compatibility.common.util.ResultUploader;
import com.android.compatibility.common.util.TestStatus;
import com.android.ddmlib.Log;
import com.android.ddmlib.Log.LogLevel;
import com.android.ddmlib.testrunner.TestIdentifier;
import com.android.tradefed.build.IBuildInfo;
import com.android.tradefed.config.Option;
import com.android.tradefed.config.Option.Importance;
import com.android.tradefed.config.OptionClass;
import com.android.tradefed.config.OptionCopier;
import com.android.tradefed.log.LogUtil.CLog;
import com.android.tradefed.result.ILogSaver;
import com.android.tradefed.result.ILogSaverListener;
import com.android.tradefed.result.IShardableListener;
import com.android.tradefed.result.ITestInvocationListener;
import com.android.tradefed.result.ITestSummaryListener;
import com.android.tradefed.result.InputStreamSource;
import com.android.tradefed.result.LogDataType;
import com.android.tradefed.result.LogFile;
import com.android.tradefed.result.LogFileSaver;
import com.android.tradefed.result.TestSummary;
import com.android.tradefed.util.FileUtil;
import com.android.tradefed.util.StreamUtil;
import com.android.tradefed.util.TimeUtil;
import com.android.tradefed.util.ZipUtil;

import org.xmlpull.v1.XmlPullParserException;

import java.io.File;
import java.io.FileInputStream;
import java.io.FileNotFoundException;
import java.io.IOException;
import java.io.InputStream;
import java.text.SimpleDateFormat;
import java.util.Collections;
import java.util.Date;
import java.util.HashSet;
import java.util.List;
import java.util.Map;
import java.util.Map.Entry;
import java.util.Set;

/**
 * Collect test results for an entire invocation and output test results to disk.
 */
@OptionClass(alias="result-reporter")
public class ResultReporter implements ILogSaverListener, ITestInvocationListener,
       ITestSummaryListener, IShardableListener {

    private static final String UNKNOWN_DEVICE = "unknown_device";
    private static final String RESULT_KEY = "COMPATIBILITY_TEST_RESULT";
    private static final String CTS_PREFIX = "cts:";
    private static final String BUILD_INFO = CTS_PREFIX + "build_";
    private static final String[] RESULT_RESOURCES = {
        "compatibility_result.css",
        "compatibility_result.xsd",
        "compatibility_result.xsl",
        "logo.png"};

    @Option(name = CompatibilityTest.RETRY_OPTION,
            shortName = 'r',
            description = "retry a previous session.",
            importance = Importance.IF_UNSET)
    private Integer mRetrySessionId = null;

    @Option(name = "result-server", description = "Server to publish test results.")
    private String mResultServer;

    @Option(name = "disable-result-posting", description = "Disable result posting into report server.")
    private boolean mDisableResultPosting = false;

    @Option(name = "include-test-log-tags", description = "Include test log tags in report.")
    private boolean mIncludeTestLogTags = false;

    @Option(name = "use-log-saver", description = "Also saves generated result with log saver")
    private boolean mUseLogSaver = false;

    private CompatibilityBuildHelper mBuildHelper;
    private File mResultDir = null;
    private File mLogDir = null;
    private ResultUploader mUploader;
    private String mReferenceUrl;
    private ILogSaver mLogSaver;
    private int invocationEndedCount = 0;

    private IInvocationResult mResult = new InvocationResult();
    private IModuleResult mCurrentModuleResult;
    private ICaseResult mCurrentCaseResult;
    private ITestResult mCurrentResult;
    private String mDeviceSerial = UNKNOWN_DEVICE;
    private Set<String> mMasterDeviceSerials = new HashSet<>();
    private Set<IBuildInfo> mMasterBuildInfos = new HashSet<>();

    // mCurrentTestNum and mTotalTestsInModule track the progress within the module
    // Note that this count is not necessarily equal to the count of tests contained
    // in mCurrentModuleResult because of how special cases like ignored tests are reported.
    private int mCurrentTestNum;
    private int mTotalTestsInModule;

    // Nullable. If null, "this" is considered the master and must handle
    // result aggregation and reporting. When not null, it should forward events
    // to the master.
    private final ResultReporter mMasterResultReporter;

    /**
     * Default constructor.
     */
    public ResultReporter() {
        this(null);
    }

    /**
     * Construct a shard ResultReporter that forwards module results to the
     * masterResultReporter.
     */
    public ResultReporter(ResultReporter masterResultReporter) {
        mMasterResultReporter = masterResultReporter;
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void invocationStarted(IBuildInfo buildInfo) {
        synchronized(this) {
            if (mBuildHelper == null) {
                mBuildHelper = new CompatibilityBuildHelper(buildInfo);
            }
            if (mDeviceSerial == null && buildInfo.getDeviceSerial() != null) {
                mDeviceSerial = buildInfo.getDeviceSerial();
            }
        }

        if (isShardResultReporter()) {
            // Shard ResultReporters forward invocationStarted to the mMasterResultReporter
            mMasterResultReporter.invocationStarted(buildInfo);
            return;
        }

        // NOTE: Everything after this line only applies to the master ResultReporter.

        synchronized(this) {
            if (buildInfo.getDeviceSerial() != null) {
                // The master ResultReporter collects all device serials being used
                // for the current implementation.
                mMasterDeviceSerials.add(buildInfo.getDeviceSerial());
            }

            // The master ResultReporter collects all buildInfos.
            mMasterBuildInfos.add(buildInfo);

            if (mResultDir == null) {
                // For the non-sharding case, invocationStarted is only called once,
                // but for the sharding case, this might be called multiple times.
                // Logic used to initialize the result directory should not be
                // invoked twice during the same invocation.
                initializeResultDirectories();
            }
        }
    }

    /**
     * Create directory structure where results and logs will be written.
     */
    private void initializeResultDirectories() {
        info("Initializing result directory");

        try {
            // Initialize the result directory. Either a new directory or reusing
            // an existing session.
            if (mRetrySessionId != null) {
                // Overwrite the mResult with the test results of the previous session
                mResult = ResultHandler.findResult(mBuildHelper.getResultsDir(), mRetrySessionId);
            }
            mResult.setStartTime(mBuildHelper.getStartTime());
            mResultDir = mBuildHelper.getResultDir();
            if (mResultDir != null) {
                mResultDir.mkdirs();
            }
        } catch (FileNotFoundException e) {
            throw new RuntimeException(e);
        }

        if (mResultDir == null) {
            throw new RuntimeException("Result Directory was not created");
        }
        if (!mResultDir.exists()) {
            throw new RuntimeException("Result Directory was not created: " +
                    mResultDir.getAbsolutePath());
        }

        info("Results Directory: " + mResultDir.getAbsolutePath());

        mUploader = new ResultUploader(mResultServer, mBuildHelper.getSuiteName());
        try {
            mLogDir = new File(mBuildHelper.getLogsDir(),
                    CompatibilityBuildHelper.getDirSuffix(mBuildHelper.getStartTime()));
        } catch (FileNotFoundException e) {
            e.printStackTrace();
        }
        if (mLogDir != null && mLogDir.mkdirs()) {
            info("Created log dir %s", mLogDir.getAbsolutePath());
        }
        if (mLogDir == null || !mLogDir.exists()) {
            throw new IllegalArgumentException(String.format("Could not create log dir %s",
                    mLogDir.getAbsolutePath()));
        }
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void testRunStarted(String id, int numTests) {
        if (mCurrentModuleResult != null && mCurrentModuleResult.getId().equals(id)) {
            // In case we get another test run of a known module, update the complete
            // status to false to indicate it is not complete. This happens in cases like host side
            // tests when each test class is executed as separate module.
            mCurrentModuleResult.setDone(false);
            mTotalTestsInModule += numTests;
        } else {
            mCurrentModuleResult = mResult.getOrCreateModule(id);
            mTotalTestsInModule = numTests;
            // Reset counters
            mCurrentTestNum = 0;
        }
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void testStarted(TestIdentifier test) {
        mCurrentCaseResult = mCurrentModuleResult.getOrCreateResult(test.getClassName());
        mCurrentResult = mCurrentCaseResult.getOrCreateResult(test.getTestName().trim());
        mCurrentResult.reset();
        mCurrentTestNum++;
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void testEnded(TestIdentifier test, Map<String, String> metrics) {
        if (mCurrentResult.getResultStatus() == TestStatus.FAIL) {
            // Test has previously failed.
            return;
        }
        // device test can have performance results in test metrics
        String perfResult = metrics.get(RESULT_KEY);
        ReportLog report = null;
        if (perfResult != null) {
            try {
                report = ReportLog.parse(perfResult);
            } catch (XmlPullParserException | IOException e) {
                e.printStackTrace();
            }
        } else {
            // host test should be checked into MetricsStore.
            report = MetricsStore.removeResult(mBuildHelper.getBuildInfo(),
                    mCurrentModuleResult.getAbi(), test.toString());
        }
        if (mCurrentResult.getResultStatus() == null) {
            // Only claim that we passed when we're certain our result was
            // not any other state.
            mCurrentResult.passed(report);
        }
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void testIgnored(TestIdentifier test) {
        // Ignored tests are not reported.
        mCurrentTestNum--;
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void testFailed(TestIdentifier test, String trace) {
        mCurrentResult.failed(trace);
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void testAssumptionFailure(TestIdentifier test, String trace) {
        mCurrentResult.skipped();
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void testRunStopped(long elapsedTime) {
        // ignore
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void testRunEnded(long elapsedTime, Map<String, String> metrics) {
        mCurrentModuleResult.addRuntime(elapsedTime);
        // Expect them to be equal, but greater than to be safe.
        mCurrentModuleResult.setDone(mCurrentTestNum >= mTotalTestsInModule);

        if (isShardResultReporter()) {
            // Forward module results to the master.
            mMasterResultReporter.mergeModuleResult(mCurrentModuleResult);
        }
    }

    /**
     * Directly add a module result. Note: this method is meant to be used by
     * a shard ResultReporter.
     */
    private void mergeModuleResult(IModuleResult moduleResult) {
        // This merges the results in moduleResult to any existing results already
        // contained in mResult. This is useful for retries and allows the final
        // report from a retry to contain all test results.
        synchronized(this) {
            mResult.mergeModuleResult(moduleResult);
        }
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void testRunFailed(String errorMessage) {
        // ignore
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public TestSummary getSummary() {
        // ignore
        return null;
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void putSummary(List<TestSummary> summaries) {
        // This is safe to be invoked on either the master or a shard ResultReporter,
        // but the value added to the report will be that of the master ResultReporter.
        if (summaries.size() > 0) {
            mReferenceUrl = summaries.get(0).getSummary().getString();
        }
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void invocationEnded(long elapsedTime) {
        if (isShardResultReporter()) {
            // Shard ResultReporters report
            mMasterResultReporter.invocationEnded(elapsedTime);
            return;
        }

        // NOTE: Everything after this line only applies to the master ResultReporter.


        synchronized(this) {
            // The master ResultReporter tracks the progress of all invocations across
            // shard ResultReporters. Writing results should not proceed until all
            // ResultReporters have completed.
            if (++invocationEndedCount < mMasterBuildInfos.size()) {
                return;
            }
            finalizeResults(elapsedTime);
        }
    }

    private void finalizeResults(long elapsedTime) {
        // Add all device serials into the result to be serialized
        for (String deviceSerial : mMasterDeviceSerials) {
            mResult.addDeviceSerial(deviceSerial);
        }

        Set<String> allExpectedModules = new HashSet<>();
        // Add all build info to the result to be serialized
        for (IBuildInfo buildInfo : mMasterBuildInfos) {
            for (Map.Entry<String, String> entry : buildInfo.getBuildAttributes().entrySet()) {
                String key = entry.getKey();
                String value = entry.getValue();
                if (key.startsWith(BUILD_INFO)) {
                    mResult.addInvocationInfo(key.substring(CTS_PREFIX.length()), value);
                }

                if (key.equals(CompatibilityBuildHelper.MODULE_IDS) && value.length() > 0) {
                    Collections.addAll(allExpectedModules, value.split(","));
                }
            }
        }

        // Include a record in the report of all expected modules ids, even if they weren't
        // executed.
        for (String moduleId : allExpectedModules) {
            mResult.getOrCreateModule(moduleId);
        }

        String moduleProgress = String.format("%d of %d",
                mResult.getModuleCompleteCount(), mResult.getModules().size());

        info("Invocation finished in %s. PASSED: %d, FAILED: %d, MODULES: %s",
                TimeUtil.formatElapsedTime(elapsedTime),
                mResult.countResults(TestStatus.PASS),
                mResult.countResults(TestStatus.FAIL),
                moduleProgress);

        long startTime = mResult.getStartTime();
        try {
            File resultFile = ResultHandler.writeResults(mBuildHelper.getSuiteName(),
                    mBuildHelper.getSuiteVersion(), mBuildHelper.getSuitePlan(),
                    mBuildHelper.getSuiteBuild(), mResult, mResultDir, startTime,
                    elapsedTime + startTime, mReferenceUrl, getLogUrl(),
                    mBuildHelper.getCommandLineArgs());
            info("Test Result: %s", resultFile.getCanonicalPath());

            // Zip the full test results directory.
            copyDynamicConfigFiles(mBuildHelper.getDynamicConfigFiles(), mResultDir);
            copyFormattingFiles(mResultDir);
            File zippedResults = zipResults(mResultDir);
            info("Full Result: %s", zippedResults.getCanonicalPath());

            saveLog(resultFile, zippedResults);

            uploadResult(resultFile);

        } catch (IOException | XmlPullParserException e) {
            CLog.e("[%s] Exception while saving result XML.", mDeviceSerial);
            CLog.e(e);
        }
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void invocationFailed(Throwable cause) {
        warn("Invocation failed: %s", cause);
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void testLog(String name, LogDataType type, InputStreamSource stream) {
        // This is safe to be invoked on either the master or a shard ResultReporter
        if (isShardResultReporter()) {
            // Shard ResultReporters forward testLog to the mMasterResultReporter
            mMasterResultReporter.testLog(name, type, stream);
            return;
        }
        try {
            LogFileSaver saver = new LogFileSaver(mLogDir);
            File logFile = saver.saveAndZipLogData(name, type, stream.createInputStream());
            info("Saved logs for %s in %s", name, logFile.getAbsolutePath());
        } catch (IOException e) {
            warn("Failed to write log for %s", name);
            e.printStackTrace();
        }
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void testLogSaved(String dataName, LogDataType dataType, InputStreamSource dataStream,
            LogFile logFile) {
        // This is safe to be invoked on either the master or a shard ResultReporter
        if (mIncludeTestLogTags && mCurrentResult != null
                && dataName.startsWith(mCurrentResult.getFullName())) {

            if (dataType == LogDataType.BUGREPORT) {
                mCurrentResult.setBugReport(logFile.getUrl());
            } else if (dataType == LogDataType.LOGCAT) {
                mCurrentResult.setLog(logFile.getUrl());
            } else if (dataType == LogDataType.PNG) {
                mCurrentResult.setScreenshot(logFile.getUrl());
            }
        }
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void setLogSaver(ILogSaver saver) {
        // This is safe to be invoked on either the master or a shard ResultReporter
        mLogSaver = saver;
    }

    /**
     * When enabled, save log data using log saver
     */
    private void saveLog(File resultFile, File zippedResults) throws IOException {
        if (!mUseLogSaver) {
            return;
        }

        FileInputStream fis = null;
        try {
            fis = new FileInputStream(resultFile);
            mLogSaver.saveLogData("log-result", LogDataType.XML, fis);
        } catch (IOException ioe) {
            CLog.e("[%s] error saving XML with log saver", mDeviceSerial);
            CLog.e(ioe);
        } finally {
            StreamUtil.close(fis);
        }
        // Save the full results folder.
        if (zippedResults != null) {
            FileInputStream zipResultStream = null;
            try {
                zipResultStream = new FileInputStream(zippedResults);
                mLogSaver.saveLogData("results", LogDataType.ZIP, zipResultStream);
            } finally {
                StreamUtil.close(zipResultStream);
            }
        }
    }

    /**
     * Return the path in which log saver persists log files or null if
     * logSaver is not enabled.
     */
    private String getLogUrl() {
        if (!mUseLogSaver || mLogSaver == null) {
            return null;
        }

        return mLogSaver.getLogReportDir().getUrl();
    }

    @Override
    public IShardableListener clone() {
        ResultReporter clone = new ResultReporter(this);
        OptionCopier.copyOptionsNoThrow(this, clone);
        return clone;
    }

    /**
     * Return true if this instance is a shard ResultReporter and should propagate
     * certain events to the master.
     */
    private boolean isShardResultReporter() {
        return mMasterResultReporter != null;
    }

    /**
     * When enabled, upload the result to a server.
     */
    private void uploadResult(File resultFile) throws IOException {
        if (mResultServer != null && !mResultServer.trim().isEmpty() && !mDisableResultPosting) {
            try {
                info("Result Server: %d", mUploader.uploadResult(resultFile, mReferenceUrl));
            } catch (IOException ioe) {
                CLog.e("[%s] IOException while uploading result.", mDeviceSerial);
                CLog.e(ioe);
            }
        }
    }

    /**
     * Copy the xml formatting files stored in this jar to the results directory
     *
     * @param resultsDir
     */
    static void copyFormattingFiles(File resultsDir) {
        for (String resultFileName : RESULT_RESOURCES) {
            InputStream configStream = ResultHandler.class.getResourceAsStream(
                    String.format("/report/%s", resultFileName));
            if (configStream != null) {
                File resultFile = new File(resultsDir, resultFileName);
                try {
                    FileUtil.writeToFile(configStream, resultFile);
                } catch (IOException e) {
                    warn("Failed to write %s to file", resultFileName);
                }
            } else {
                warn("Failed to load %s from jar", resultFileName);
            }
        }
    }

    /**
     * move the dynamic config files to the results directory
     *
     * @param configFiles
     * @param resultsDir
     */
    static void copyDynamicConfigFiles(Map<String, File> configFiles, File resultsDir) {
        if (configFiles.size() == 0) return;

        File folder = new File(resultsDir, "config");
        folder.mkdir();
        for (String moduleName : configFiles.keySet()) {
            File resultFile = new File(folder, moduleName+".dynamic");
            try {
                FileUtil.copyFile(configFiles.get(moduleName), resultFile);
                FileUtil.deleteFile(configFiles.get(moduleName));
            } catch (IOException e) {
                warn("Failed to copy config file for %s to file", moduleName);
            }
        }
    }

    /**
     * Zip the contents of the given results directory.
     *
     * @param resultsDir
     */
    private static File zipResults(File resultsDir) {
        File zipResultFile = null;
        try {
            // create a file in parent directory, with same name as resultsDir
            zipResultFile = new File(resultsDir.getParent(), String.format("%s.zip",
                    resultsDir.getName()));
            ZipUtil.createZip(resultsDir, zipResultFile);
        } catch (IOException e) {
            warn("Failed to create zip for %s", resultsDir.getName());
        }
        return zipResultFile;
    }

    /**
     *  Log info to the console.
     */
    private static void info(String format, Object... args) {
        log(LogLevel.INFO, format, args);
    }

    /**
     *  Log a warning to the console.
     */
    private static void warn(String format, Object... args) {
        log(LogLevel.WARN, format, args);
    }

    /**
     * Log a message to the console
     */
    private static void log(LogLevel level, String format, Object... args) {
        CLog.logAndDisplay(level, format, args);
    }

    /**
     * For testing
     */
    IInvocationResult getResult() {
        return mResult;
    }
}
