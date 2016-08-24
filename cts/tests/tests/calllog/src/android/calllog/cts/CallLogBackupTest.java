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

package android.calllog.cts;

import android.content.ContentResolver;
import android.content.ContentValues;
import android.content.Context;
import android.content.pm.PackageManager;
import android.database.Cursor;
import android.provider.CallLog;
import android.provider.CallLog.Calls;
import android.provider.Settings;
import android.test.InstrumentationTestCase;
import android.util.Log;

import java.util.LinkedList;
import java.util.List;
import java.util.Objects;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

/**
 * Verifies the behavior of CallLogBackup.
 *
 * This tests three import things:
 * 1. That call log gets backed up and restored using the standard BackupAgent implementation.
 *     - To test we create call-log entries back them up, clear the call log, and restore them.
 *     - We leverage the LocalTransport backup implementation to do this.
 * 2. The call log backup is implemented by the expected package: {@link #CALLLOG_BACKUP_PACKAGE}.
 *     - We always trigger the expected package for backup/restore within the tests.
 * 3. The backup format for call log is as expected so that backup works across android devices
 *    by different OEMs.
 *     - We peek into the backup files saved by LocalTransport and verify their binary output is
 *       as expected.
 */
public class CallLogBackupTest extends InstrumentationTestCase {
    private static final String TAG = "CallLogBackupTest";

    private static final String LOCAL_BACKUP_COMPONENT =
            "android/com.android.internal.backup.LocalTransport";

    private static final String CALLLOG_BACKUP_PACKAGE = "com.android.providers.calllogbackup";
    private static final String ALT_CALLLOG_BACKUP_PACKAGE = "com.android.calllogbackup";

    private static final String TEST_NUMBER = "555-1234";
    private static final int CALL_START_TIME = 0;
    private static final int CALL_DURATION = 2000;
    private static final int TIMEOUT_BACKUP = 10000;
    private static final String TEST_POST_DIAL_DIGITS = ";1234";
    private static final String TEST_VIA_NUMBER = "555-1112";

    private static final Pattern BMGR_ENABLED_PATTERN = Pattern.compile(
            "^Backup Manager currently (enabled|disabled)$");

    private static final String[] CALL_LOG_PROJECTION = new String[] {
        CallLog.Calls._ID,
        CallLog.Calls.DATE,
        CallLog.Calls.DURATION,
        CallLog.Calls.NUMBER,
        CallLog.Calls.TYPE,
        CallLog.Calls.COUNTRY_ISO,
        CallLog.Calls.GEOCODED_LOCATION,
        CallLog.Calls.NUMBER_PRESENTATION,
        CallLog.Calls.PHONE_ACCOUNT_COMPONENT_NAME,
        CallLog.Calls.PHONE_ACCOUNT_ID,
        CallLog.Calls.DATA_USAGE,
        CallLog.Calls.FEATURES,
        CallLog.Calls.POST_DIAL_DIGITS,
        CallLog.Calls.VIA_NUMBER
    };

    protected interface Condition {
        Object expected();
        Object actual();
    }

    class Call {
        int id;
        long date;
        long duration;
        String number;
        int type;
        String phoneAccountComponent;
        String phoneAccountId;
        int presentation;
        String postDialDigits;
        String viaNumber;
    }

    private String mCallLogBackupPackageName;

    @Override
    protected void setUp() throws Exception {
        super.setUp();
        PackageManager pm = getContext().getPackageManager();
        try {
            pm.getPackageInfo(CALLLOG_BACKUP_PACKAGE, 0);
            mCallLogBackupPackageName = CALLLOG_BACKUP_PACKAGE;
        } catch (PackageManager.NameNotFoundException e) {
            mCallLogBackupPackageName = ALT_CALLLOG_BACKUP_PACKAGE;
        }
    }

    /**
     * Test:
     *   1) Clear the call log
     *   2) Add a single call
     *   3) Run backup
     *   4) clear the call log
     *   5) Run restore
     *   6) Verify that we the call from step (2)
     */
    public void testSingleCallBackup() throws Exception {
        if (!getContext().getPackageManager().hasSystemFeature(PackageManager.FEATURE_TELEPHONY)) {
            Log.i(TAG, "Skipping calllog tests: no telephony feature");
            return;
        }
        // This CTS test depends on the local transport and so if it is not present,
        // skip the test with success.
        if (!hasBackupTransport(LOCAL_BACKUP_COMPONENT)) {
            Log.i(TAG, "skipping calllog tests: no local transport");
            return;
        }

        // Turn on backup and set to the local backup agent.
        boolean previouslyEnabled = enableBackup(true /* enable */);
        String oldTransport = setBackupTransport(LOCAL_BACKUP_COMPONENT);
        int previousFullDataBackupAware = Settings.Secure.getInt(getContext().getContentResolver(),
                "user_full_data_backup_aware", 0);
        enableFullDataBackupAware(1);

        // Clear the call log
        Log.i(TAG, "Clearing the call log");
        clearCallLog();
        clearBackups();

        // Add a single call and verify it exists
        Log.i(TAG, "Adding a call");
        addCall();
        verifyCall();

        // Run backup for the call log (saves the single call).
        Log.i(TAG, "Running backup");
        runBackupFor(mCallLogBackupPackageName);

        // Clear the call log and verify that it is empty
        Log.i(TAG, "Clearing the call log");
        clearCallLog();
        assertEquals(0, getCalls().size());

        // Restore from the previous backup and verify we have the new call again.
        Log.i(TAG, "Restoring the single call");
        runRestoreFor(mCallLogBackupPackageName);

        verifyCall();

        // Clean up after ourselves
        clearCallLog();

        // Reset backup manager to original state.
        Log.i(TAG, "Reseting backup");
        setBackupTransport(oldTransport);
        enableBackup(previouslyEnabled);
        enableFullDataBackupAware(previousFullDataBackupAware);
    }

    private Call verifyCall() {
        waitUntilConditionIsTrueOrTimeout(new Condition() {
            @Override
            public Object expected() {
                return 1;
            }

            @Override
            public Object actual() {
                return getCalls().size();
            }
        }, TIMEOUT_BACKUP);

        List<Call> calls = getCalls();
        Call call = calls.get(0);
        assertEquals(TEST_NUMBER, call.number);
        assertEquals(CALL_START_TIME, call.date);
        assertEquals(CALL_DURATION, call.duration);
        assertEquals(Calls.OUTGOING_TYPE, call.type);
        assertEquals(TEST_POST_DIAL_DIGITS, call.postDialDigits);
        assertEquals(TEST_VIA_NUMBER, call.viaNumber);
        return call;
    }

    private boolean enableBackup(boolean enable) throws Exception {
        // Check to see the previous state of the backup service
        boolean previouslyEnabled = false;
        String output = exec("bmgr enabled");
        Matcher matcher = BMGR_ENABLED_PATTERN.matcher(output.trim());
        if (matcher.find()) {
            previouslyEnabled = "enabled".equals(matcher.group(1));
        } else {
            throw new RuntimeException("Backup output format changed.  No longer matches"
                    + " expected regex: " + BMGR_ENABLED_PATTERN + "\nactual: '" + output + "'");
        }

        exec("bmgr enable " + enable);
        return previouslyEnabled;
    }

    private void runBackupFor(String packageName) throws Exception {
        exec("bmgr backupnow " + packageName);
    }

    private void runRestoreFor(String packageName) throws Exception {
        exec("bmgr restore " + packageName);
    }

    private void enableFullDataBackupAware(int status) throws Exception {
        exec("settings put secure user_full_data_backup_aware " + status);
    }

    private String setBackupTransport(String transport) throws Exception {
        String output = exec("bmgr transport " + transport);
        Pattern pattern = Pattern.compile("\\(formerly (.*)\\)$");
        Matcher matcher = pattern.matcher(output);
        if (matcher.find()) {
            return matcher.group(1);
        } else {
            throw new Exception("non-parsable output setting bmgr transport: " + output);
        }
    }

    /**
     * Checks the list of supported transports and verifies that the specified transport
     * is included.
     */
    private boolean hasBackupTransport(String transport) throws Exception {
        String output = exec("bmgr list transports");
        for (String t : output.split(" ")) {
            if ("*".equals(t)) {
                // skip the current selection marker.
                continue;
            } else if (Objects.equals(transport, t)) {
                return true;
            }
        }
        return false;
    }

    private void clearCallLog() {
        ContentResolver resolver = getContext().getContentResolver();
        resolver.delete(Calls.CONTENT_URI, null, null);
    }

    private void clearBackups() throws Exception {
        exec("bmgr wipe " + LOCAL_BACKUP_COMPONENT + " " + mCallLogBackupPackageName);
    }

    private void addCall() {
        ContentValues values = new ContentValues(6);
        values.put(Calls.NUMBER, TEST_NUMBER);
        values.put(Calls.NUMBER_PRESENTATION, Calls.PRESENTATION_ALLOWED);
        values.put(Calls.TYPE, Integer.valueOf(Calls.OUTGOING_TYPE));
        values.put(Calls.DATE, Long.valueOf(CALL_START_TIME));
        values.put(Calls.DURATION, Long.valueOf(CALL_DURATION));
        values.put(Calls.NEW, Integer.valueOf(1));
        values.put(Calls.POST_DIAL_DIGITS, TEST_POST_DIAL_DIGITS);
        values.put(Calls.VIA_NUMBER, TEST_VIA_NUMBER);

        getContext().getContentResolver().insert(Calls.CONTENT_URI, values);
    }

    void waitUntilConditionIsTrueOrTimeout(Condition condition, long timeout) {
        final long start = System.currentTimeMillis();
        while (!condition.expected().equals(condition.actual())
                && System.currentTimeMillis() - start < timeout) {
            try {
                Thread.sleep(100);
            } catch (InterruptedException e) {
                // ignore
            }
        }
        assertEquals(condition.expected(), condition.actual());
    }

    private List<Call> getCalls() {
        List<Call> calls = new LinkedList<>();

        ContentResolver resolver = getContext().getContentResolver();
        Cursor cursor = resolver.query(Calls.CONTENT_URI, CALL_LOG_PROJECTION, null, null, null);
        if (cursor != null) {
            try {
                while (cursor.moveToNext()) {
                    Call call = new Call();
                    call.id = cursor.getInt(cursor.getColumnIndex(Calls._ID));
                    call.number = cursor.getString(cursor.getColumnIndex(Calls.NUMBER));
                    call.date = cursor.getLong(cursor.getColumnIndex(Calls.DATE));
                    call.duration = cursor.getLong(cursor.getColumnIndex(Calls.DURATION));
                    call.type = cursor.getInt(cursor.getColumnIndex(Calls.TYPE));
                    call.phoneAccountComponent = cursor.getString(
                            cursor.getColumnIndex(Calls.PHONE_ACCOUNT_COMPONENT_NAME));
                    call.phoneAccountId = cursor.getString(
                            cursor.getColumnIndex(Calls.PHONE_ACCOUNT_ID));
                    call.presentation = cursor.getInt(
                            cursor.getColumnIndex(Calls.NUMBER_PRESENTATION));
                    call.postDialDigits = cursor.getString(
                            cursor.getColumnIndex(Calls.POST_DIAL_DIGITS));
                    call.viaNumber = cursor.getString(
                            cursor.getColumnIndex(Calls.VIA_NUMBER));
                    calls.add(call);
                }
            } finally {
                cursor.close();
            }
        }
        return calls;
    }

    private Context getContext() {
        return getInstrumentation().getContext();
    }

    private String exec(String command) throws Exception {
        return TestUtils.executeShellCommand(getInstrumentation(), command);
    }
}
