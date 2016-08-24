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

package android.support.test.aupt;

import android.app.Instrumentation;
import android.app.Service;
import android.content.Context;
import android.content.ContextWrapper;
import android.content.Intent;
import android.content.IntentFilter;
import android.os.Bundle;
import android.os.Environment;
import android.os.IBinder;
import android.support.test.uiautomator.UiDevice;
import android.test.AndroidTestRunner;
import android.test.InstrumentationTestCase;
import android.test.InstrumentationTestRunner;
import android.util.Log;

import dalvik.system.BaseDexClassLoader;

import junit.framework.AssertionFailedError;
import junit.framework.Test;
import junit.framework.TestCase;
import junit.framework.TestListener;
import junit.framework.TestResult;

import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Collections;
import java.util.Comparator;
import java.util.concurrent.TimeUnit;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

/**
 * Test runner to use when running AUPT tests.
 * <p>
 * Adds support for multiple iterations, randomizing the order of the tests,
 * terminating tests after UI errors, detecting when processes are getting killed,
 * collecting bugreports and procrank data while the test is running.
 */
public class AuptTestRunner extends InstrumentationTestRunner {
    private static final String DEFAULT_JAR_PATH = "/data/local/tmp/";

    private static final String LOG_TAG = "AuptTestRunner";
    private static final String DEX_OPT_PATH = "aupt-opt";
    private static final String PARAM_JARS = "jars";
    private Bundle mParams;

    private long mIterations;
    private boolean mShuffle;
    private boolean mGenerateAnr;
    private long mTestCaseTimeout = TimeUnit.MINUTES.toMillis(10);
    private DataCollector mDataCollector;
    private File mResultsDirectory;

    private boolean mDeleteOldFiles;
    private long mFileRetainCount;

    private AuptPrivateTestRunner mRunner = new AuptPrivateTestRunner();
    private ClassLoader mLoader = null;
    private Context mTargetContextWrapper;

    private IProcessStatusTracker mProcessTracker;

    private Map<String, List<AuptTestCase.MemHealthRecord>> mMemHealthRecords;

    private boolean mTrackJank;
    private GraphicsStatsMonitor mGraphicsStatsMonitor;

    /**
     * {@inheritDoc}
     */
    @Override
    public void onCreate(Bundle params) {

        mParams = params;

        mMemHealthRecords = new HashMap<String, List<AuptTestCase.MemHealthRecord>>();

        mIterations = parseLongParam("iterations", 1);
        mShuffle = parseBooleanParam("shuffle", false);
        // set to 'generateANR to 'true' when more info required for debugging on test timeout'
        mGenerateAnr = parseBooleanParam("generateANR", false);
        if (parseBooleanParam("quitOnError", false)) {
            mRunner.addTestListener(new QuitOnErrorListener());
        }
        if (parseBooleanParam("checkBattery", false)) {
            mRunner.addTestListener(new BatteryChecker());
        }
        mTestCaseTimeout = parseLongParam("testCaseTimeout", mTestCaseTimeout);

        // Option: -e detectKill com.pkg1,...,com.pkg8
        String processes = parseStringParam("detectKill", null);
        if (processes != null) {
            mProcessTracker = new ProcessStatusTracker(processes.split(","));
        } else {
            mProcessTracker = new ProcessStatusTracker(null);
        }

        // Option: -e trackJank boolean
        mTrackJank = parseBooleanParam("trackJank", false);
        if (mTrackJank) {
            mGraphicsStatsMonitor = new GraphicsStatsMonitor();

            // Option: -e jankInterval long
            long interval = parseLongParam("jankInterval",
                    GraphicsStatsMonitor.DEFAULT_INTERVAL_RATE);
            mGraphicsStatsMonitor.setIntervalRate(interval);
        }
        mRunner.addTestListener(new PidChecker());
        mResultsDirectory = new File(Environment.getExternalStorageDirectory(),
                parseStringParam("outputLocation", "aupt_results"));
        if (!mResultsDirectory.exists() && !mResultsDirectory.mkdirs()) {
            Log.w(LOG_TAG, "Did not create output directory");
        }

        mFileRetainCount = parseLongParam("fileRetainCount", -1);
        if (mFileRetainCount == -1) {
            mDeleteOldFiles = false;
        } else {
            mDeleteOldFiles = true;
        }

        mDataCollector = new DataCollector(
                TimeUnit.MINUTES.toMillis(parseLongParam("bugreportInterval", 0)),
                TimeUnit.MINUTES.toMillis(parseLongParam("meminfoInterval", 0)),
                TimeUnit.MINUTES.toMillis(parseLongParam("cpuinfoInterval", 0)),
                TimeUnit.MINUTES.toMillis(parseLongParam("fragmentationInterval", 0)),
                TimeUnit.MINUTES.toMillis(parseLongParam("ionInterval", 0)),
                TimeUnit.MINUTES.toMillis(parseLongParam("pagetypeinfoInterval", 0)),
                TimeUnit.MINUTES.toMillis(parseLongParam("traceInterval", 0)),
                mResultsDirectory, this);
        String jars = params.getString(PARAM_JARS);
        if (jars != null) {
            loadDexJars(jars);
        }
        mTargetContextWrapper = new ClassLoaderContextWrapper();
        super.onCreate(params);
    }

    private void loadDexJars(String jars) {
        // scan provided jar paths, translate relative to absolute paths, and check for existence
        String[] jarsArray = jars.split(":");
        StringBuilder jarFiles = new StringBuilder();
        for (int i = 0; i < jarsArray.length; i++) {
            String jar = jarsArray[i];
            if (!jar.startsWith("/")) {
                jar = DEFAULT_JAR_PATH + jar;
            }
            File jarFile = new File(jar);
            if (!jarFile.exists() || !jarFile.canRead()) {
                throw new IllegalArgumentException("Jar file does not exist or not accessible: "
                        + jar);
            }
            if (i != 0) {
                jarFiles.append(File.pathSeparator);
            }
            jarFiles.append(jarFile.getAbsolutePath());
        }
        // now load them
        File optDir = new File(getContext().getCacheDir(), DEX_OPT_PATH);
        if (!optDir.exists() && !optDir.mkdirs()) {
            throw new RuntimeException(
                    "Failed to create dex optimize directory: " + optDir.getAbsolutePath());
        }
        mLoader = new BaseDexClassLoader(jarFiles.toString(), optDir, null,
                super.getTargetContext().getClassLoader());

    }

    private long parseLongParam(String key, long alternative) throws NumberFormatException {
        if (mParams.containsKey(key)) {
            return Long.parseLong(
                    mParams.getString(key));
        } else {
            return alternative;
        }
    }

    private boolean parseBooleanParam(String key, boolean alternative)
            throws NumberFormatException {
        if (mParams.containsKey(key)) {
            return Boolean.parseBoolean(mParams.getString(key));
        } else {
            return alternative;
        }
    }

    private String parseStringParam(String key, String alternative) {
        if (mParams.containsKey(key)) {
            return mParams.getString(key);
        } else {
            return alternative;
        }
    }

    private void writeProgressMessage(String msg) {
        writeMessage("progress.txt", msg);
    }
    
    private void writeGraphicsMessage(String msg) {
        writeMessage("graphics.txt", msg);
    }

    private void writeMessage(String filename, String msg) {
        try {
            FileOutputStream fos = new FileOutputStream(
                    new File(mResultsDirectory, filename));
            fos.write(msg.getBytes());
            fos.flush();
            fos.close();
        } catch (IOException ioe) {
            Log.e(LOG_TAG, "error saving progress file", ioe);
        }
    }

    /**
     * Provide a wrapped context so that we can provide an alternative class loader
     * @return
     */
    @Override
    public Context getTargetContext() {
        return mTargetContextWrapper;
    }

    /**
     * {@inheritDoc}
     */
    @Override
    protected AndroidTestRunner getAndroidTestRunner() {
        // AndroidTestRunner is what determines which tests get to run.
        // Unfortunately there is no hooks into it, so most of
        // the functionality has to be duplicated
        return mRunner;
    }

    /**
     * Sets up and starts monitoring jank metrics by clearing the currently existing data.
     */
    private void startJankMonitoring () {
        if (mTrackJank) {
            mGraphicsStatsMonitor.setUiAutomation(getUiAutomation());
            mGraphicsStatsMonitor.startMonitoring();

            // TODO: Clear graphics.txt file if extant
        }
    }

    /**
     * Stops future monitoring of jank metrics, but preserves current metrics intact.
     */
    private void stopJankMonitoring () {
        if (mTrackJank) {
            mGraphicsStatsMonitor.stopMonitoring();
        }
    }

    /**
     * Aggregates and merges jank metrics and writes them to the graphics file.
     */
    private void writeJankMetrics () {
        if (mTrackJank) {
            List<JankStat> mergedStats = mGraphicsStatsMonitor.aggregateStatsImages();
            String mergedStatsString = JankStat.statsListToString(mergedStats);

            Log.d(LOG_TAG, "Writing jank metrics to the graphics file");
            writeGraphicsMessage(mergedStatsString);
        }
    }

    /**
     * Determines which tests to run, configures the test class and then runs the test.
     */
    private class AuptPrivateTestRunner extends AndroidTestRunner {

        private List<TestCase> mTestCases;
        private List<TestListener> mTestListeners = new ArrayList<>();
        private Instrumentation mInstrumentation;
        private TestResult mTestResult;

        @Override
        public List<TestCase> getTestCases() {
            if (mTestCases != null) {
                return mTestCases;
            }

            List<TestCase> testCases = new ArrayList<TestCase>(super.getTestCases());
            List<TestCase> completeList = new ArrayList<TestCase>();

            for (int i = 0; i < mIterations; i++) {
                if (mShuffle) {
                    Collections.shuffle(testCases);
                }
                completeList.addAll(testCases);
            }
            mTestCases = completeList;
            return mTestCases;
        }

        @Override
        public void runTest(TestResult testResult) {
            mTestResult = testResult;

            ((ProcessStatusTracker)mProcessTracker).setUiAutomation(getUiAutomation());

            mDataCollector.start();
            startJankMonitoring();

            for (TestListener testListener : mTestListeners) {
                mTestResult.addListener(testListener);
            }

            Runnable timeBomb = new Runnable() {
                @Override
                public void run() {
                    try {
                        Thread.sleep(mTestCaseTimeout);
                    } catch (InterruptedException e) {
                        return;
                    }
                    // if we ever wake up, a timeout has occurred, set off the bomb,
                    // but trigger a service ANR first
                    if (mGenerateAnr) {
                        Context ctx = getTargetContext();
                        Log.d(LOG_TAG, "About to induce artificial ANR for debugging");
                        ctx.startService(new Intent(ctx, BadService.class));
                        // intentional delay to allow the service ANR to happen then resolve
                        try {
                            Thread.sleep(BadService.DELAY + BadService.DELAY / 4);
                        } catch (InterruptedException e) {
                            // ignore
                            Log.d(LOG_TAG, "interrupted in wait on BadService");
                            return;
                        }
                    } else {
                        Log.d("THREAD_DUMP", getStackTraces());
                    }
                    throw new RuntimeException(String.format("max testcase timeout exceeded: %s ms",
                            mTestCaseTimeout));
                }
            };

            try {
                // Try to run all TestCases, but ensure the finally block is reached
                for (TestCase testCase : mTestCases) {
                    setInstrumentationIfInstrumentationTestCase(testCase, mInstrumentation);
                    setupAuptIfAuptTestCase(testCase);

                    // Remove device storage as necessary
                    removeOldImagesFromDcimCameraFolder();

                    Thread timeBombThread = null;
                    if (mTestCaseTimeout > 0) {
                        timeBombThread = new Thread(timeBomb);
                        timeBombThread.setName("Boom!");
                        timeBombThread.setDaemon(true);
                        timeBombThread.start();
                    }

                    try {
                        testCase.run(mTestResult);
                    } catch (AuptTerminator ex) {
                        // Write to progress.txt to pass the exception message to the dashboard
                        writeProgressMessage("Exception: " + ex);
                        // Throw the exception, because we want to discontinue running tests
                        throw ex;
                    }

                    if (mTestCaseTimeout > 0) {
                        timeBombThread.interrupt();
                        try {
                            timeBombThread.join();
                        } catch (InterruptedException e) {
                            // ignore
                        }
                    }
                }
            } finally {
                // Ensure the DataCollector ends all dangling Threads
                mDataCollector.stop();
                // Ensure the Timer in GraphicsStatsMonitor is canceled
                stopJankMonitoring(); // However, it is daemon
                // Ensure jank metrics are written to the graphics file
                writeJankMetrics();
            }
        }

        /**
         * Gets all thread stack traces.
         *
         * @return string of all thread stack traces
         */
        private String getStackTraces() {
            StringBuilder sb = new StringBuilder();
            Map<Thread, StackTraceElement[]> stacks = Thread.getAllStackTraces();
            for (Thread t : stacks.keySet()) {
                sb.append(t.toString()).append('\n');
                for (StackTraceElement ste : t.getStackTrace()) {
                    sb.append("\tat ").append(ste.toString()).append('\n');
                }
                sb.append('\n');
            }
            return sb.toString();
        }

        private void setInstrumentationIfInstrumentationTestCase(
                Test test, Instrumentation instrumentation) {
            if (InstrumentationTestCase.class.isAssignableFrom(test.getClass())) {
                ((InstrumentationTestCase) test).injectInstrumentation(instrumentation);
            }
        }

        // Aupt specific set up.
        private void setupAuptIfAuptTestCase(Test test) {
            if (test instanceof AuptTestCase){
                ((AuptTestCase)test).setProcessStatusTracker(mProcessTracker);
                ((AuptTestCase)test).setMemHealthRecords(mMemHealthRecords);
                ((AuptTestCase)test).setDataCollector(mDataCollector);
            }
        }

        private void removeOldImagesFromDcimCameraFolder() {
            if (!mDeleteOldFiles) {
                return;
            }

            File dcimFolder = new File(Environment.getExternalStorageDirectory(), "DCIM");
            if (dcimFolder != null) {
                File cameraFolder = new File(dcimFolder, "Camera");
                if (cameraFolder != null) {
                    File[] allMediaFiles = cameraFolder.listFiles();
                    Arrays.sort(allMediaFiles, new Comparator<File> () {
                        public int compare(File f1, File f2) {
                            return Long.valueOf(f1.lastModified()).compareTo(f2.lastModified());
                        }
                    });
                    for (int i = 0; i < allMediaFiles.length - mFileRetainCount; i++) {
                        allMediaFiles[i].delete();
                    }
                } else {
                    Log.w(LOG_TAG, "No Camera folder found to delete from.");
                }
            } else {
                Log.w(LOG_TAG, "No DCIM folder found to delete from.");
            }
        }

        @Override
        public void clearTestListeners() {
            mTestListeners.clear();
        }

        @Override
        public void addTestListener(TestListener testListener) {
            if (testListener != null) {
                mTestListeners.add(testListener);
            }
        }

        @Override
        public void setInstrumentation(Instrumentation instrumentation) {
            mInstrumentation = instrumentation;
        }

        @Override
        public TestResult getTestResult() {
            return mTestResult;
        }

        @Override
        protected TestResult createTestResult() {
            return new TestResult();
        }
    }

    /**
     * Test listener that monitors the AUPT tests for any errors. If the option is set it will
     * terminate the whole test run if it encounters an exception.
     */
    private class QuitOnErrorListener implements TestListener {

        @Override
        public void addError(Test test, Throwable t) {
            Log.e(LOG_TAG, "Caught exception from a test", t);
            if ((t instanceof AuptTerminator)) {
                throw (AuptTerminator)t;
            } else {
                // check that if the UI exception is caused by process getting killed
                if (test instanceof AuptTestCase) {
                    ((AuptTestCase)test).getProcessStatusTracker().verifyRunningProcess();
                }
                // if previous line did not throw an exception, we are interested to know what
                // caused the UI exception
                Log.v(LOG_TAG, "Dumping UI hierarchy");
                try {
                    UiDevice.getInstance(AuptTestRunner.this).dumpWindowHierarchy(
                            new File("/data/local/tmp/error_dump.xml"));
                } catch (IOException e) {
                    Log.w(LOG_TAG, "Failed to create UI hierarchy dump for UI error", e);
                }
            }

            throw new AuptTerminator(t.getMessage(), t);
        }

        @Override
        public void addFailure(Test test, AssertionFailedError t) {
            throw new AuptTerminator(t.getMessage(), t);
        }

        @Override
        public void endTest(Test test) {
            // skip
        }

        @Override
        public void startTest(Test test) {
            // skip
        }
    }

    /**
     * A listener that checks that none of the monitored processes died during the test.
     * If a process dies it will  terminate the test early.
     */
    private class PidChecker implements TestListener {

        @Override
        public void addError(Test test, Throwable t) {
            // no-op
        }

        @Override
        public void addFailure(Test test, AssertionFailedError t) {
            // no-op
        }

        @Override
        public void endTest(Test test) {
            mProcessTracker.verifyRunningProcess();
        }

        @Override
        public void startTest(Test test) {
            mProcessTracker.verifyRunningProcess();
        }
    }

    private class BatteryChecker implements TestListener {
        private static final double BATTERY_THRESHOLD = 0.05;

        private void checkBattery() {
            Intent batteryIntent = getContext().registerReceiver(null,
                    new IntentFilter(Intent.ACTION_BATTERY_CHANGED));
            int rawLevel = batteryIntent.getIntExtra("level", -1);
            int scale = batteryIntent.getIntExtra("scale", -1);

            if (rawLevel < 0 || scale <= 0) {
                return;
            }

            double level = (double) rawLevel / (double) scale;
            if (level < BATTERY_THRESHOLD) {
                throw new AuptTerminator(String.format("Current battery level %f lower than %f",
                        level,
                        BATTERY_THRESHOLD));
            }
        }

        @Override
        public void addError(Test test, Throwable t) {
            // skip
        }

        @Override
        public void addFailure(Test test, AssertionFailedError afe) {
            // skip
        }

        @Override
        public void endTest(Test test) {
            // skip
        }

        @Override
        public void startTest(Test test) {
            checkBattery();
        }
    }

    /**
     * A {@link ContextWrapper} that overrides {@link Context#getClassLoader()}
     */
    class ClassLoaderContextWrapper extends ContextWrapper {

        public ClassLoaderContextWrapper() {
            super(AuptTestRunner.super.getTargetContext());
        }

        /**
         * Alternatively returns a custom class loader with classes loaded from additional jars
         */
        @Override
        public ClassLoader getClassLoader() {
            if (mLoader != null) {
                return mLoader;
            } else {
                return super.getClassLoader();
            }
        }
    }

    public static class BadService extends Service {
        public static final long DELAY = 30000;
        @Override
        public IBinder onBind(Intent intent) {
            return null;
        }

        @Override
        public int onStartCommand(Intent intent, int flags, int id) {
            Log.i(LOG_TAG, "in service start -- about to hang");
            try { Thread.sleep(DELAY); } catch (InterruptedException e) { Log.wtf(LOG_TAG, e); }
            Log.i(LOG_TAG, "service hang finished -- stopping and returning");
            stopSelf();
            return START_NOT_STICKY;
        }
    }
}
