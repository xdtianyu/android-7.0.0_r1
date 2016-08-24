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
 * limitations under the License
 */

package android.backup.cts;

import android.app.Instrumentation;
import android.os.ParcelFileDescriptor;
import android.test.InstrumentationTestCase;
import android.util.Log;

import java.io.BufferedReader;
import java.io.FileInputStream;
import java.io.InputStream;
import java.io.InputStreamReader;
import java.nio.charset.StandardCharsets;
import java.util.concurrent.TimeUnit;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

/**
 * Verifies receiving quotaExceeded() callback on full backup.
 *
 * Uses test app that creates large file and receives the callback.
 * {@link com.android.internal.backup.LocalTransport} is used, it has size quota 25MB.
 */
public class BackupQuotaTest extends InstrumentationTestCase {
    private static final String APP_LOG_TAG = "BackupCTSApp";

    private static final String LOCAL_TRANSPORT =
            "android/com.android.internal.backup.LocalTransport";
    private static final int LOCAL_TRANSPORT_EXCEEDING_FILE_SIZE = 30 * 1024 * 1024;
    private static final String BACKUP_APP_NAME = "android.backup.app";

    private static final int SMALL_LOGCAT_DELAY = 1000;

    private boolean wasBackupEnabled;
    private String oldTransport;

    @Override
    protected void setUp() throws Exception {
        super.setUp();
        // Enable backup and select local backup transport
        assertTrue("LocalTransport should be available.", hasBackupTransport(LOCAL_TRANSPORT));
        wasBackupEnabled = enableBackup(true);
        oldTransport = setBackupTransport(LOCAL_TRANSPORT);
    }

    @Override
    protected void tearDown() throws Exception {
        // Return old transport
        setBackupTransport(oldTransport);
        enableBackup(wasBackupEnabled);
        super.tearDown();
    }

    public void testQuotaExceeded() throws Exception {
        exec("logcat --clear");
        exec("setprop log.tag." + APP_LOG_TAG +" VERBOSE");
        // Launch test app and create file exceeding limit for local transport
        exec("am start -W -a android.intent.action.MAIN " +
                "-c android.intent.category.LAUNCHER " +
                "-n " + BACKUP_APP_NAME + "/" + BACKUP_APP_NAME +".MainActivity " +
                "-e file_size " + LOCAL_TRANSPORT_EXCEEDING_FILE_SIZE);
        assertTrue("File was not created", waitForLogcat("File created!", 30));

        // Request backup and wait for quota exceeded event in logcat
        exec("bmgr backupnow " + BACKUP_APP_NAME);
        assertTrue("Quota exceeded event is not received", waitForLogcat("Quota exceeded!", 10));
    }

    private boolean enableBackup(boolean enable) throws Exception {
        boolean previouslyEnabled;
        String output = exec("bmgr enabled");
        Pattern pattern = Pattern.compile("^Backup Manager currently (enabled|disabled)$");
        Matcher matcher = pattern.matcher(output.trim());
        if (matcher.find()) {
            previouslyEnabled = "enabled".equals(matcher.group(1));
        } else {
            throw new RuntimeException("non-parsable output setting bmgr enabled: " + output);
        }

        exec("bmgr enable " + enable);
        return previouslyEnabled;
    }

    private String setBackupTransport(String transport) throws Exception {
        String output = exec("bmgr transport " + transport);
        Pattern pattern = Pattern.compile("\\(formerly (.*)\\)$");
        Matcher matcher = pattern.matcher(output);
        if (matcher.find()) {
            return matcher.group(1);
        } else {
            throw new RuntimeException("non-parsable output setting bmgr transport: " + output);
        }
    }

    private boolean hasBackupTransport(String transport) throws Exception {
        String output = exec("bmgr list transports");
        for (String t : output.split(" ")) {
            if (transport.equals(t)) {
                return true;
            }
        }
        return false;
    }

    private boolean waitForLogcat(String logcatString, int maxTimeoutInSeconds) throws Exception {
        long timeout = System.currentTimeMillis() + TimeUnit.SECONDS.toMillis(maxTimeoutInSeconds);
        while (timeout >= System.currentTimeMillis()) {
            FileInputStream fis = executeStreamedShellCommand(getInstrumentation(),
                    "logcat -v brief -d " + APP_LOG_TAG + ":* *:S");
            BufferedReader log = new BufferedReader(new InputStreamReader(fis));
            String line;
            while ((line = log.readLine()) != null) {
                if (line.contains(logcatString)) {
                    return true;
                }
            }
            closeQuietly(log);
            // In case the key has not been found, wait for the log to update before
            // performing the next search.
            Thread.sleep(SMALL_LOGCAT_DELAY);
        }
        return false;
    }

    private String exec(String command) throws Exception {
        BufferedReader br = null;
        try (InputStream in = executeStreamedShellCommand(getInstrumentation(), command)) {
            br = new BufferedReader(new InputStreamReader(in, StandardCharsets.UTF_8));
            String str = null;
            StringBuilder out = new StringBuilder();
            while ((str = br.readLine()) != null) {
                out.append(str);
            }
            return out.toString();
        } finally {
            if (br != null) {
                closeQuietly(br);
            }
        }
    }

    private static FileInputStream executeStreamedShellCommand(Instrumentation instrumentation,
                                                               String command) throws Exception {
        final ParcelFileDescriptor pfd =
                instrumentation.getUiAutomation().executeShellCommand(command);
        return new FileInputStream(pfd.getFileDescriptor());
    }

    private static void closeQuietly(AutoCloseable closeable) {
        if (closeable != null) {
            try {
                closeable.close();
            } catch (RuntimeException rethrown) {
                throw rethrown;
            } catch (Exception ignored) {
            }
        }
    }
}
