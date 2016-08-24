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
import android.os.Environment;
import android.os.ParcelFileDescriptor;
import android.os.SystemClock;
import android.util.Log;

import java.io.ByteArrayOutputStream;
import java.io.File;
import java.io.FileNotFoundException;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.text.SimpleDateFormat;
import java.util.Date;
import java.util.Locale;

public class DataCollector {
    private static final String TAG = "AuptDataCollector";
    private long mBugreportInterval, mMeminfoInterval, mCpuinfoInterval, mFragmentationInterval,
            mIonHeapInterval, mPageTypeInfoInterval, mTraceInterval;
    private File mResultsDirectory;

    private Thread mLoggerThread;
    private Logger mLogger;
    private Instrumentation mInstrumentation;

    public DataCollector(long bugreportInterval, long meminfoInterval, long cpuinfoInterval,
            long fragmentationInterval, long ionHeapInterval, long pagetypeinfoInterval,
            long traceInterval, File outputLocation, Instrumentation intrumentation) {
        mBugreportInterval = bugreportInterval;
        mMeminfoInterval = meminfoInterval;
        mCpuinfoInterval = cpuinfoInterval;
        mFragmentationInterval = fragmentationInterval;
        mIonHeapInterval = ionHeapInterval;
        mPageTypeInfoInterval = pagetypeinfoInterval;
        mResultsDirectory = outputLocation;
        mTraceInterval = traceInterval;
        mInstrumentation = intrumentation;
    }

    public void start() {
        mLogger = new Logger();
        mLoggerThread = new Thread(mLogger);
        mLoggerThread.start();
    }

    public void stop() {
        mLogger.stop();
        try {
            mLoggerThread.join();
        } catch (InterruptedException e) {
            // ignore
        }
    }

    private class Logger implements Runnable {
        private final long mIntervals[] = {
                mBugreportInterval, mMeminfoInterval, mCpuinfoInterval, mFragmentationInterval,
                mIonHeapInterval, mPageTypeInfoInterval, mTraceInterval
        };
        private final LogGenerator mLoggers[] = {
                new BugreportGenerator(), new CompactMemInfoGenerator(), new CpuInfoGenerator(),
                new FragmentationGenerator(), new IonHeapGenerator(), new PageTypeInfoGenerator(),
                new TraceGenerator()
        };

        private final long mLastUpdate[] = new long[mLoggers.length];
        private final long mSleepInterval;

        private boolean mStopped = false;

        public Logger() {
            for (int i = 0; i < mIntervals.length; i++) {
                if (mIntervals[i] > 0) {
                    try {
                        mLoggers[i].createLog();
                    } catch (InterruptedException e) {
                        // ignore
                    }
                    mLastUpdate[i] = SystemClock.uptimeMillis();
                }
            }

            mSleepInterval = gcd(mIntervals);
        }

        public void stop() {
            synchronized(this) {
                mStopped = true;
                notifyAll();
            }
        }

        private long gcd(long values[]) {
            if (values.length < 2)
                return 0;

            long gcdSoFar = values[0];
            for (int i = 1; i < values.length; i++) {
                gcdSoFar = gcd(gcdSoFar, values[i]);
            }

            return gcdSoFar;
        }

        private long gcd(long a, long b) {
            if (a == 0)
                return b;
            if (b == 0)
                return a;
            if (a > b)
                return gcd(b, a % b);
            else
                return gcd(a, b % a);
        }

        @Override
        public void run() {
            if (mSleepInterval <= 0)
                return;

            synchronized(this) {
                while (!mStopped) {
                    try {
                        for (int i = 0; i < mIntervals.length; i++) {
                            if (mIntervals[i] > 0 &&
                                    SystemClock.uptimeMillis() - mLastUpdate[i] > mIntervals[i]) {
                                mLoggers[i].createLog();
                                mLastUpdate[i] = SystemClock.uptimeMillis();
                            }
                        }
                        wait(mSleepInterval);
                    } catch (InterruptedException e) {
                        // Ignore.
                    }
                }
            }
        }
    }

    private interface LogGenerator {
        public void createLog() throws InterruptedException;
    }

    private class CompactMemInfoGenerator implements LogGenerator {
        @Override
        public void createLog() throws InterruptedException {
            try {
                saveCompactMeminfo(mResultsDirectory + "/compact-meminfo-%s.txt");
            } catch (IOException ioe) {
                Log.w(TAG, "Error while saving dumpsys meminfo -c: " + ioe.getMessage());
            }
        }
    }

    private class CpuInfoGenerator implements LogGenerator {
        @Override
        public void createLog() throws InterruptedException {
            try {
                saveCpuinfo(mResultsDirectory + "/cpuinfo-%s.txt");
            } catch (IOException ioe) {
                Log.w(TAG, "Error while saving dumpsys cpuinfo : " + ioe.getMessage());
            }
        }
    }

    private class BugreportGenerator implements LogGenerator {
        @Override
        public void createLog() throws InterruptedException {
            try {
                saveBugreport(mResultsDirectory + "/bugreport-%s.txt");
            } catch (IOException e) {
                Log.w(TAG, String.format("Failed to take bugreport: %s", e.getMessage()));
            }
        }
    }

    private class FragmentationGenerator implements LogGenerator {
        @Override
        public void createLog() throws InterruptedException {
            try {
                saveFragmentation(mResultsDirectory + "/unusable-index-%s.txt");
            } catch (IOException e) {
                Log.w(TAG, String.format("Failed to save buddyinfo: %s", e.getMessage()));
            }
        }
    }

    private class IonHeapGenerator implements LogGenerator {
        @Override
        public void createLog() throws InterruptedException {
            try {
                saveIonHeap("audio", mResultsDirectory + "/ion-audio-%s.txt");
                saveIonHeap("system", mResultsDirectory + "/ion-system-%s.txt");
            } catch (IOException e) {
                Log.w(TAG, String.format("Failed to save ION heap: %s", e.getMessage()));
            }
        }
    }

    private class PageTypeInfoGenerator implements LogGenerator {
        @Override
        public void createLog() throws InterruptedException {
            try {
                savePageTypeInfo(mResultsDirectory + "/pagetypeinfo-%s.txt");
            } catch (IOException e) {
                Log.w(TAG, String.format("Failed to save pagetypeinfo: %s", e.getMessage()));
            }
        }
    }

    private class TraceGenerator implements LogGenerator {
        @Override
        public void createLog() throws InterruptedException {
            try {
                saveTrace(mResultsDirectory + "/trace-%s.txt");
            } catch (IOException e) {
                Log.w(TAG, String.format("Failed to save trace: %s", e.getMessage()));
            }
        }
    }

    public void saveCompactMeminfo(String filename)
            throws FileNotFoundException, IOException, InterruptedException {
        saveProcessOutput("dumpsys meminfo -c -S", filename);
    }

    public void saveCpuinfo(String filename)
            throws FileNotFoundException, IOException, InterruptedException {
        saveProcessOutput("dumpsys cpuinfo", filename);
    }

    public void saveFragmentation(String filename)
            throws FileNotFoundException, IOException, InterruptedException {
        saveProcessOutput("cat /d/extfrag/unusable_index", filename);
    }

    public void saveIonHeap(String type, String filename)
            throws FileNotFoundException, IOException, InterruptedException {
        saveProcessOutput(String.format("cat /d/ion/heaps/%s", type), filename);
    }

    public void savePageTypeInfo(String filename)
            throws FileNotFoundException, IOException, InterruptedException {
        saveProcessOutput("cat /proc/pagetypeinfo", filename);
    }

    public void saveTrace(String filename)
            throws FileNotFoundException, IOException, InterruptedException {
        saveProcessOutput("cat /sys/kernel/debug/tracing/trace", filename);
    }

    public void saveBugreport(String filename)
            throws IOException, InterruptedException {
        ByteArrayOutputStream baos = new ByteArrayOutputStream();
        // Spaces matter in the following command line. Make sure there are no spaces
        // in the filename and around the '>' sign.
        String cmdline = String.format("/system/bin/sh -c /system/bin/bugreport>%s",
                templateToFilename(filename));
        saveProcessOutput(cmdline, baos);
        baos.close();
    }

    public void dumpMeminfo(String notes) {
        long epochSeconds = System.currentTimeMillis() / 1000;
        File outputDir = new File(Environment.getExternalStorageDirectory(), "meminfo");
        Log.i(TAG, outputDir.toString());
        if (!outputDir.exists()) {
            boolean yes  = outputDir.mkdirs();
            Log.i(TAG, yes ? "created" : "not created");
        }
        File outputFile = new File(outputDir, String.format("%d.txt", epochSeconds));
        Log.i(TAG, outputFile.toString());
        FileOutputStream fos = null;

        try {
            fos = new FileOutputStream(outputFile);
            fos.write(String.format("notes: %s\n\n", notes).getBytes());

            saveProcessOutput("dumpsys meminfo -c", fos);
            fos.close();
        } catch (FileNotFoundException e) {
            Log.e(TAG, "exception while dumping meminfo", e);
        } catch (IOException e) {
            Log.e(TAG, "exception while dumping meminfo", e);
        }
    }

    private void saveProcessOutput(String command, String filenameTemplate)
            throws IOException, FileNotFoundException {
        String outFilename = templateToFilename(filenameTemplate);
        File file = new File(outFilename);
        Log.d(TAG, String.format("Saving command \"%s\" output into file %s",
                command, file.getAbsolutePath()));

        OutputStream out = new FileOutputStream(file);
        saveProcessOutput(command, out);
        out.close();
    }

    private String templateToFilename(String filenameTemplate) {
        SimpleDateFormat sdf = new SimpleDateFormat("yyyy-MM-dd-HH-mm-ss", Locale.US);
        return String.format(filenameTemplate, sdf.format(new Date()));
    }

    public void saveProcessOutput(String command, OutputStream out) throws IOException {
        InputStream in = null;
        try {
            ParcelFileDescriptor pfd =
                    mInstrumentation.getUiAutomation().executeShellCommand(command);
            in = new ParcelFileDescriptor.AutoCloseInputStream(pfd);
            byte[] buffer = new byte[4096];  //4K buffer
            int bytesRead = -1;
            while (true) {
                bytesRead = in.read(buffer);
                if (bytesRead == -1) {
                    break;
                }
                out.write(buffer, 0, bytesRead);
            }
        } finally {
            if (in != null) {
                in.close();
            }
            if (out != null) {
                out.flush();
            }
        }
    }
}
