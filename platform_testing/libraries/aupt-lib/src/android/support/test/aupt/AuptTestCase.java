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

import java.io.BufferedReader;
import java.io.BufferedWriter;
import java.io.ByteArrayOutputStream;
import java.io.File;
import java.io.FileInputStream;
import java.io.FileNotFoundException;
import java.io.FileOutputStream;
import java.io.FileWriter;
import java.io.IOException;
import java.io.InputStreamReader;
import java.io.PrintWriter;
import java.util.ArrayList;
import java.util.Collection;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

import android.accounts.Account;
import android.accounts.AccountManager;
import android.content.Context;
import android.content.Intent;
import android.content.pm.PackageInfo;
import android.content.pm.PackageManager;
import android.content.pm.PackageManager.NameNotFoundException;
import android.os.Bundle;
import android.os.Environment;
import android.os.SystemClock;
import android.support.test.uiautomator.By;
import android.support.test.uiautomator.UiDevice;
import android.support.test.uiautomator.UiWatcher;
import android.test.InstrumentationTestCase;
import android.test.InstrumentationTestRunner;
import android.util.Log;

import junit.framework.Assert;

/**
 * Base class for AuptTests.
 */
public class AuptTestCase extends InstrumentationTestCase {
    private static final String SDCARD =
            Environment.getExternalStorageDirectory().getAbsolutePath();
    private static final String RECORD_MEMINFO_PARAM = "record_meminfo";
    private static final long DEFAULT_SHORT_SLEEP = 5 * 1000;
    private static final long DEFAULT_LONG_SLEEP = 30 * 1000;
    protected static final int STEPS_BACK = 10;
    protected static final int RECOVERY_SLEEP = 2000;
    private static final String TAG = AuptTestCase.class.getSimpleName();

    private boolean mRecordMeminfo = false;
    private UiWatchers mWatchers;
    private IProcessStatusTracker mProcessStatusTracker;
    private DataCollector mDataCollector;

    // We want to periodically collect dumpheap output if the process grows to large to proactivelly
    // help with catching memory leaks, but don't want to do it too often so it does not disturb the
    // test. We are going to limit the total number of dumpheap commands per proccess to 5 by
    // default, rate limit it to one per hour be default, and only do it for monitored processes
    // which grow larger than a certain size (200MB by default).
    private boolean mDumpheapEnabled = false;
    private long mDumpheapThreshold;
    private long mDumpheapInterval ;
    private long mMaxDumpheaps;
    private Map<String, Long> mLastDumpheap = new HashMap<String, Long>();
    private Map<String, Long> mDumpheapCount = new HashMap<String, Long>();

    private UiDevice mDevice;

    public static class MemHealthRecord {
        public enum Context { FOREGROUND, BACKGROUND };

        private final long mTimeMs;
        private final long mDalvikHeap;
        private final long mNativeHeap;
        private final long mPss;
        private final Context mContext;

        public MemHealthRecord(long timeMs, long dalvikHeap, long nativeHeap, long pss,
                Context context) {
            mTimeMs = timeMs;
            mDalvikHeap = dalvikHeap;
            mNativeHeap = nativeHeap;
            mPss = pss;
            mContext = context;
        }

        public static List<Long> getForegroundDalvikHeap(Collection<MemHealthRecord> samples) {
            List<Long> ret = new ArrayList<>(samples.size());
            for (MemHealthRecord sample : samples) {
                if (Context.FOREGROUND.equals(sample.mContext)) {
                    ret.add(sample.mDalvikHeap);
                }
            }
            return ret;
        }

        public static List<Long> getBackgroundDalvikHeap(Collection<MemHealthRecord> samples) {
            List<Long> ret = new ArrayList<>(samples.size());
            for (MemHealthRecord sample : samples) {
                if (Context.BACKGROUND.equals(sample.mContext)) {
                    ret.add(sample.mDalvikHeap);
                }
            }
            return ret;
        }

        public static List<Long> getForegroundNativeHeap(Collection<MemHealthRecord> samples) {
            List<Long> ret = new ArrayList<>(samples.size());
            for (MemHealthRecord sample : samples) {
                if (Context.FOREGROUND.equals(sample.mContext)) {
                    ret.add(sample.mNativeHeap);
                }
            }
            return ret;
        }

        public static List<Long> getBackgroundNativeHeap(Collection<MemHealthRecord> samples) {
            List<Long> ret = new ArrayList<>(samples.size());
            for (MemHealthRecord sample : samples) {
                if (Context.BACKGROUND.equals(sample.mContext)) {
                    ret.add(sample.mNativeHeap);
                }
            }
            return ret;
        }

        public static List<Long> getForegroundPss(Collection<MemHealthRecord> samples) {
            List<Long> ret = new ArrayList<>(samples.size());
            for (MemHealthRecord sample : samples) {
                if (Context.FOREGROUND.equals(sample.mContext)) {
                    ret.add(sample.mPss);
                }
            }
            return ret;
        }

        public static List<Long> getBackgroundPss(Collection<MemHealthRecord> samples) {
            List<Long> ret = new ArrayList<>(samples.size());
            for (MemHealthRecord sample : samples) {
                if (Context.BACKGROUND.equals(sample.mContext)) {
                    ret.add(sample.mPss);
                }
            }
            return ret;
        }

        private static Long getMax(Collection<Long> samples) {
            Long max = null;
            for (Long sample : samples) {
                if (max == null || sample > max) {
                    max = sample;
                }
            }
            return max;
        }

        private static Long getAverage(Collection<Long> samples) {
            if (samples.size() == 0) {
                return null;
            }

            double sum = 0;
            for (Long sample : samples) {
                sum += sample;
            }
            return (long) (sum / samples.size());
        }
    }

    private Map<String, List<MemHealthRecord>> mMemHealthRecords;
    private String[] mProcsToTrack;
    private String mResultsDirectory;

    public void setMemHealthRecords(Map<String, List<MemHealthRecord>> records) {
        mMemHealthRecords = records;
    }

    @Override
    protected void setUp() throws Exception {
        super.setUp();

        mDevice = UiDevice.getInstance(getInstrumentation());
        mWatchers = new UiWatchers();
        mWatchers.registerAnrAndCrashWatchers(getInstrumentation());
        mDevice.registerWatcher("LockScreenWatcher", new LockScreenWatcher());
        mRecordMeminfo = "true".equals(getParams().getString(RECORD_MEMINFO_PARAM, "false"));

        mDevice.setOrientationNatural();

        mResultsDirectory = SDCARD + "/" + getParams().getString(
                "outputLocation", "aupt_results");

        String processes = getParams().getString("trackMemory", null);
        if (processes != null) {
            mProcsToTrack = processes.split(",");
        } else {
            readProcessesFromFile();
        }

        mDumpheapEnabled = "true".equals(getParams().getString("enableDumpheap"));
        if (mDumpheapEnabled) {
            mDumpheapThreshold = getLongParam("dumpheapThreshold", 200 * 1024 * 1024); // 200MB
            mDumpheapInterval = getLongParam("dumpheapInterval", 60 * 60 * 1000); // one hour
            mMaxDumpheaps = getLongParam("maxDumpheaps", 5);
        }
    }

    private void readProcessesFromFile() {
        File trackFile = new File(SDCARD + "/track_memory.txt");
        if (trackFile.exists()) {
            BufferedReader in = null;
            try {
                in = new BufferedReader(new InputStreamReader(new FileInputStream(trackFile)));
                String processes = in.readLine();
                in.close();

                if (!"".equals(processes)) {
                    mProcsToTrack = processes.split(",");
                }
            } catch (FileNotFoundException e) {
                Log.e(TAG, "Error opening track file", e);
            } catch (IOException e) {
                Log.e(TAG, "Error opening track file", e);
            }
        }
    }

    /**
     * {@inheritDoc}
     */
    @Override
    protected void tearDown() throws Exception {
        mDevice.removeWatcher("LockScreenWatcher");
        mDevice.unfreezeRotation();

        saveMemoryStats();

        super.tearDown();
    }

    private class LockScreenWatcher implements UiWatcher {

        @Override
        public boolean checkForCondition() {
            if (mDevice.hasObject(By.desc("Slide area."))) {
                mDevice.pressMenu();
                return true;
            }
            return false;
        }
    }

    /**
     * Looks up a parameter or returns a default value if parameter is not
     * present.
     * @param key
     * @param defaultValue
     * @return passed in parameter or default value if parameter is not found.
     */
    public long getLongParam(String key, long defaultValue) throws NumberFormatException {
        if (getParams().containsKey(key)) {
            return Long.parseLong(getParams().getString(key));
        } else {
            return defaultValue;
        }
    }

    /**
     * Returns the timeout for short sleep. Can be set with shortSleep command
     * line option. Default is 5 seconds.
     * @return time in milliseconds
     */
    public long getShortSleep() {
        return getLongParam("shortSleep", DEFAULT_SHORT_SLEEP);
    }

    /**
     * Returns the timeout for long sleep. Can be set with longSleep command
     * line option. Default is 30 seconds
     * @return time in milliseconds.
     */
    public long getLongSleep() {
        return getLongParam("longSleep", DEFAULT_LONG_SLEEP);
    }

    /**
     * Press back button repeatedly in order to attempt to bring the app back to home screen.
     * This is intended so that an app can recover if the previous session left an app in a weird
     * state.
     */
    public void navigateToHome() {
        int iterations = 0;
        String launcherPkg = mDevice.getLauncherPackageName();
        while (!launcherPkg.equals(mDevice.getCurrentPackageName())
                && iterations < STEPS_BACK) {
            mDevice.pressBack();
            SystemClock.sleep(RECOVERY_SLEEP);
            iterations++;
        }
    }

    /**
     * Writes out condensed memory data about the running processes.
     * @param notes about when the dump was taken.
     */
    public void dumpMemInfo(String notes) {
        if (mRecordMeminfo) {
            mDevice.waitForIdle();
            mDataCollector.dumpMeminfo(notes);
        }
        if (mProcsToTrack != null) {
            recordMemoryUsage();
        }
    }

    private void saveMemoryStats() {
        if (mProcsToTrack == null) {
            return;
        }
        try {
            PrintWriter out = new PrintWriter(new BufferedWriter(
                    new FileWriter(mResultsDirectory + "/memory-health.txt")));
            out.println("Foreground");
            for (Map.Entry<String, List<MemHealthRecord>> record : mMemHealthRecords.entrySet()) {
                List<Long> nativeHeap = MemHealthRecord.getForegroundNativeHeap(record.getValue());
                List<Long> dalvikHeap = MemHealthRecord.getForegroundDalvikHeap(record.getValue());
                List<Long> pss = MemHealthRecord.getForegroundPss(record.getValue());

                // nativeHeap, dalvikHeap, and pss all have the same size, just use one
                if (nativeHeap.size() == 0) {
                    continue;
                }

                out.println(record.getKey());
                out.printf("Average Native Heap: %d\n", MemHealthRecord.getAverage(nativeHeap));
                out.printf("Average Dalvik Heap: %d\n", MemHealthRecord.getAverage(dalvikHeap));
                out.printf("Average PSS: %d\n", MemHealthRecord.getAverage(pss));
                out.printf("Peak Native Heap: %d\n", MemHealthRecord.getMax(nativeHeap));
                out.printf("Peak Dalvik Heap: %d\n", MemHealthRecord.getMax(dalvikHeap));
                out.printf("Peak PSS: %d\n", MemHealthRecord.getMax(pss));
                out.printf("Count %d\n", nativeHeap.size());
            }
            out.println("Background");
            for (Map.Entry<String, List<MemHealthRecord>> record : mMemHealthRecords.entrySet()) {
                List<Long> nativeHeap = MemHealthRecord.getBackgroundNativeHeap(record.getValue());
                List<Long> dalvikHeap = MemHealthRecord.getBackgroundDalvikHeap(record.getValue());
                List<Long> pss = MemHealthRecord.getBackgroundPss(record.getValue());

                // nativeHeap, dalvikHeap, and pss all have the same size, just use one
                if (nativeHeap.size() == 0) {
                    continue;
                }

                out.println(record.getKey());
                out.printf("Average Native Heap: %d\n", MemHealthRecord.getAverage(nativeHeap));
                out.printf("Average Dalvik Heap: %d\n", MemHealthRecord.getAverage(dalvikHeap));
                out.printf("Average PSS: %d\n", MemHealthRecord.getAverage(pss));
                out.printf("Peak Native Heap: %d\n", MemHealthRecord.getMax(nativeHeap));
                out.printf("Peak Dalvik Heap: %d\n", MemHealthRecord.getMax(dalvikHeap));
                out.printf("Peak PSS: %d\n", MemHealthRecord.getMax(pss));
                out.printf("Count %d\n", nativeHeap.size());
            }
            out.close();
        } catch (IOException e) {
            Log.e(TAG, "Error while saving memory stats", e);
        }

        // Temporary hack to write full logs
        try {
            PrintWriter out = new PrintWriter(new BufferedWriter(new FileWriter(
                    mResultsDirectory + "/memory-health-details.txt")));
            for (Map.Entry<String, List<MemHealthRecord>> record : mMemHealthRecords.entrySet()) {
                out.println(record.getKey());
                out.printf("time,native_heap,dalvik_heap,pss,context\n");
                for (MemHealthRecord sample : record.getValue()) {
                    out.printf("%d,%d,%d,%s\n", sample.mTimeMs, sample.mNativeHeap,
                            sample.mDalvikHeap, sample.mContext.toString().toLowerCase());
                }
            }
            out.close();
        } catch (IOException e) {
            Log.e(TAG, "Error while saving memory stat details", e);
        }
    }

    private void recordMemoryUsage() {
        if (mProcsToTrack == null) {
            return;
        }
        long timeMs = System.currentTimeMillis();
        List<String> foregroundProcs = getForegroundProc();
        for (String proc : mProcsToTrack) {
            recordMemoryUsage(proc, timeMs, foregroundProcs);
        }
    }

    private void recordMemoryUsage(String proc, long timeMs, List<String> foregroundProcs) {
        try {
            String meminfo = getMeminfoOutput(proc);
            int nativeHeap = parseMeminfoLine(meminfo, "Native Heap\\s+\\d+\\s+(\\d+)");
            int dalvikHeap = parseMeminfoLine(meminfo, "Dalvik Heap\\s+\\d+\\s+(\\d+)");
            int pss = parseMeminfoLine(meminfo, "TOTAL\\s+(\\d+)");

            if (nativeHeap < 0 || dalvikHeap < 0 || pss < 0) {
                return;
            }
            MemHealthRecord.Context context = foregroundProcs.contains(proc) ?
                    MemHealthRecord.Context.FOREGROUND : MemHealthRecord.Context.BACKGROUND;
            if (!mMemHealthRecords.containsKey(proc)) {
                mMemHealthRecords.put(proc, new ArrayList<MemHealthRecord>());
            }
            mMemHealthRecords.get(proc).add(
                    new MemHealthRecord(timeMs, dalvikHeap, nativeHeap, pss, context));
            recordDumpheap(proc, pss);
        } catch (IOException e) {
            Log.e(TAG, "exception while memory stats", e);
        }
    }

    private int parseMeminfoLine(String meminfo, String pattern)
    {
        Pattern p = Pattern.compile(pattern);
        Matcher m = p.matcher(meminfo);
        if (m.find()) {
            return Integer.parseInt(m.group(1));
        } else {
            return -1;
        }
    }

    private String getMeminfoOutput(String processName) throws IOException {
        return getProcessOutput("dumpsys meminfo " + processName);
    }

    private void recordDumpheap(String proc, long pss) throws IOException {
        if (!mDumpheapEnabled) {
            return;
        }
        Long count = mDumpheapCount.get(proc);
        if (count == null) {
            count = 0L;
        }
        Long lastDumpheap = mLastDumpheap.get(proc);
        if (lastDumpheap == null) {
            lastDumpheap = 0L;
        }
        long currentTime = SystemClock.uptimeMillis();
        if (pss > mDumpheapThreshold && count < mMaxDumpheaps &&
                currentTime - lastDumpheap > mDumpheapInterval) {
            recordDumpheap(proc);
            mDumpheapCount.put(proc, count + 1);
            mLastDumpheap.put(proc, currentTime);
        }
    }

    private void recordDumpheap(String proc) throws IOException {
        // Turns out getting dumpheap output is non-trivial. The command runs as shell user, and
        // only has access to /data/local/tmp directory to write files to. The test does not have
        // access to the output file by default because of the permissions dumpheap sets. So we need
        // to run dumpheap, change permissions on the output file and copy it to where test harness
        // can pick it up.
        Long count = mDumpheapCount.get(proc);
        if (count == null) {
            count = 0L;
        }
        String filename = String.format("dumpheap-%s-%d", proc, count);
        String tempFilename = "/data/local/tmp/" + filename;
        String finalFilename = mResultsDirectory +"/" + filename;
        String command = String.format("am dumpheap %s %s", proc, tempFilename);
        getProcessOutput(command);
        SystemClock.sleep(3000);
        getProcessOutput(String.format("cp %s %s", tempFilename, finalFilename));
    }

    public String getProcessOutput(String command) throws IOException {
        ByteArrayOutputStream baos = new ByteArrayOutputStream();
        mDataCollector.saveProcessOutput(command, baos);
        baos.close();
        return baos.toString();
    }

    private List<String> getForegroundProc() {
        List<String> foregroundProcs = new ArrayList<String>();
        try {
            String compactMeminfo = getProcessOutput("dumpsys meminfo -c");
            for (String line : compactMeminfo.split("\\r?\\n")) {
                if (line.contains("proc,fore")) {
                    String proc = line.split(",")[2];
                    foregroundProcs.add(proc);
                }
            }
        } catch (IOException e) {
            Log.e(TAG, "Error while getting foreground process", e);
        } finally {
            return foregroundProcs;
        }
    }

    public void setProcessStatusTracker(IProcessStatusTracker processStatusTracker) {
        mProcessStatusTracker = processStatusTracker;
    }

    public IProcessStatusTracker getProcessStatusTracker() {
        return mProcessStatusTracker;
    }

    public void launchIntent(Intent intent) {
        getInstrumentation().getContext().startActivity(intent);
    }

    protected Bundle getParams() {
        return ((InstrumentationTestRunner)getInstrumentation()).getArguments();
    }

    protected UiDevice getUiDevice() {
        return mDevice;
    }

    public void setDataCollector(DataCollector collector) {
        mDataCollector = collector;
    }

    public String getPackageVersion(String packageName) throws NameNotFoundException {
        if (null == packageName || packageName.isEmpty()) {
              throw new RuntimeException("Package name can't be null or empty");
        }
        PackageManager pm = getInstrumentation().getContext().getPackageManager();
        PackageInfo pInfo = pm.getPackageInfo(packageName, 0);
        String version = pInfo.versionName;
        if (null == version || version.isEmpty()) {
              throw new RuntimeException(
                      String.format("Version isn't found for package = %s", packageName));
        }

        return version;
    }

    /**
     * Get registered accounts
     * Ensures there is at least one account registered
     * returns the google account name
     */
    public String getRegisteredEmailAccount() {
        Account[] accounts = AccountManager.get(getInstrumentation().getContext()).getAccounts();
        Assert.assertTrue("Device doesn't have any account registered", accounts.length >= 1);
        for(int i =0; i < accounts.length; ++i) {
            if(accounts[i].type.equals("com.google")) {
                return accounts[i].name;
            }
        }

        throw new RuntimeException("The device is not registered with a google account");
    }
}
