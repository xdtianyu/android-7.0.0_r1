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
package com.android.cts.deviceowner;

import android.app.Activity;
import android.app.ActivityManager;
import android.content.BroadcastReceiver;
import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.provider.Settings;
import android.os.Bundle;

// This is not a standard test of an android activity (such as
// ActivityInstrumentationTestCase2) as it is attempting to test the actual
// life cycle and how it is affected by lock task, rather than mock intents
// and setup.
public class LockTaskTest extends BaseDeviceOwnerTest {

    private static final String TEST_PACKAGE = "com.google.android.example.somepackage";

    private static final String UTILITY_ACTIVITY
            = "com.android.cts.deviceowner.LockTaskUtilityActivity";
    private static final String UTILITY_ACTIVITY_IF_WHITELISTED
            = "com.android.cts.deviceowner.LockTaskUtilityActivityIfWhitelisted";

    private static final String RECEIVING_ACTIVITY_PACKAGE_NAME
            = "com.android.cts.intent.receiver";
    private static final String RECEIVING_ACTIVITY_NAME
            = "com.android.cts.intent.receiver.IntentReceiverActivity";
    private static final String ACTION_JUST_CREATE =
            "com.android.cts.action.JUST_CREATE";

    private static final int ACTIVITY_RESUMED_TIMEOUT_MILLIS = 20000;  // 20 seconds
    private static final int ACTIVITY_RUNNING_TIMEOUT_MILLIS = 10000;  // 10 seconds
    private static final int ACTIVITY_DESTROYED_TIMEOUT_MILLIS = 60000;  // 60 seconds
    private static final int UPDATE_LOCK_TASK_TIMEOUT_MILLIS = 1000; // 1 second
    public static final String RECEIVING_ACTIVITY_CREATED_ACTION
            = "com.android.cts.deviceowner.RECEIVING_ACTIVITY_CREATED_ACTION";
    /**
     * The tests below need to keep detailed track of the state of the activity
     * that is started and stopped frequently.  To do this it sends a number of
     * broadcasts that are caught here and translated into booleans (as well as
     * notify some locks in case we are waiting).  There is also an action used
     * to specify that the activity has finished handling the current command
     * (INTENT_ACTION).
     */
    private final BroadcastReceiver mReceiver = new BroadcastReceiver() {
        @Override
        public void onReceive(Context context, Intent intent) {
            String action = intent.getAction();
            if (LockTaskUtilityActivity.CREATE_ACTION.equals(action)) {
                synchronized (mActivityRunningLock) {
                    mIsActivityRunning = true;
                    mActivityRunningLock.notify();
                }
            } else if (LockTaskUtilityActivity.DESTROY_ACTION.equals(action)) {
                synchronized (mActivityRunningLock) {
                    mIsActivityRunning = false;
                    mActivityRunningLock.notify();
                }
            } else if (LockTaskUtilityActivity.RESUME_ACTION.equals(action)) {
                synchronized (mActivityResumedLock) {
                    mIsActivityResumed = true;
                    mActivityResumedLock.notify();
                }
            } else if (LockTaskUtilityActivity.PAUSE_ACTION.equals(action)) {
                synchronized (mActivityResumedLock) {
                    mIsActivityResumed = false;
                    mActivityResumedLock.notify();
                }
            } else if (LockTaskUtilityActivity.INTENT_ACTION.equals(action)) {
                // Notify that intent has been handled.
                synchronized (LockTaskTest.this) {
                    mIntentHandled = true;
                    LockTaskTest.this.notify();
                }
            } else if (RECEIVING_ACTIVITY_CREATED_ACTION.equals(action)) {
                synchronized(mReceivingActivityCreatedLock) {
                    mReceivingActivityWasCreated = true;
                    mReceivingActivityCreatedLock.notify();
                }
            }
        }
    };

    public static class IntentReceivingActivity extends Activity {
        @Override
        public void onCreate(Bundle savedInstanceState) {
            super.onCreate(savedInstanceState);
            sendBroadcast(new Intent(RECEIVING_ACTIVITY_CREATED_ACTION));
            finish();
        }
    }

    private boolean mIsActivityRunning;
    private boolean mIsActivityResumed;
    private boolean mReceivingActivityWasCreated;
    private final Object mActivityRunningLock = new Object();
    private final Object mActivityResumedLock = new Object();
    private final Object mReceivingActivityCreatedLock = new Object();
    private Boolean mIntentHandled;

    private ActivityManager mActivityManager;

    @Override
    protected void setUp() throws Exception {
        super.setUp();
        mDevicePolicyManager.setLockTaskPackages(getWho(), new String[0]);
        mActivityManager = (ActivityManager) mContext.getSystemService(Context.ACTIVITY_SERVICE);
        IntentFilter filter = new IntentFilter();
        filter.addAction(LockTaskUtilityActivity.CREATE_ACTION);
        filter.addAction(LockTaskUtilityActivity.DESTROY_ACTION);
        filter.addAction(LockTaskUtilityActivity.INTENT_ACTION);
        filter.addAction(LockTaskUtilityActivity.RESUME_ACTION);
        filter.addAction(LockTaskUtilityActivity.PAUSE_ACTION);
        filter.addAction(RECEIVING_ACTIVITY_CREATED_ACTION);
        mContext.registerReceiver(mReceiver, filter);
    }

    @Override
    protected void tearDown() throws Exception {
        mDevicePolicyManager.setLockTaskPackages(getWho(), new String[0]);
        mContext.unregisterReceiver(mReceiver);
        super.tearDown();
    }

    // Setting and unsetting the lock task packages.
    public void testSetLockTaskPackages() {
        mDevicePolicyManager.setLockTaskPackages(getWho(), new String[] { TEST_PACKAGE });
        assertTrue(mDevicePolicyManager.isLockTaskPermitted(TEST_PACKAGE));

        mDevicePolicyManager.setLockTaskPackages(getWho(), new String[0]);
        assertFalse(mDevicePolicyManager.isLockTaskPermitted(TEST_PACKAGE));
    }

    // Start lock task, verify that ActivityManager knows thats what is going on.
    public void testStartLockTask() {
        mDevicePolicyManager.setLockTaskPackages(getWho(), new String[] { PACKAGE_NAME });
        startLockTask(UTILITY_ACTIVITY);
        waitForResume();

        // Verify that activity open and activity manager is in lock task.
        assertLockTaskModeActive();
        assertTrue(mIsActivityRunning);
        assertTrue(mIsActivityResumed);

        stopAndFinish(UTILITY_ACTIVITY);
    }

    // Verifies that the act of finishing is blocked by ActivityManager in lock task.
    // This results in onDestroy not being called until stopLockTask is called before finish.
    public void testCannotFinish() {
        mDevicePolicyManager.setLockTaskPackages(getWho(), new String[] { PACKAGE_NAME });
        startLockTask(UTILITY_ACTIVITY);

        // If lock task has not exited then the activity shouldn't actually receive onDestroy.
        finishAndWait(UTILITY_ACTIVITY);
        assertLockTaskModeActive();
        assertTrue(mIsActivityRunning);

        stopAndFinish(UTILITY_ACTIVITY);
    }

    // Verifies that updating the whitelisting during lock task mode finishes the locked task.
    public void testUpdateWhitelisting() {
        mDevicePolicyManager.setLockTaskPackages(getWho(), new String[] { PACKAGE_NAME });
        startLockTask(UTILITY_ACTIVITY);

        mDevicePolicyManager.setLockTaskPackages(getWho(), new String[0]);

        synchronized (mActivityRunningLock) {
            try {
                mActivityRunningLock.wait(UPDATE_LOCK_TASK_TIMEOUT_MILLIS);
            } catch (InterruptedException e) {
            }
        }

        assertLockTaskModeInactive();
        assertFalse(mIsActivityRunning);
        assertFalse(mIsActivityResumed);
    }

    // This launches an activity that is in the current task.
    // This should always be permitted as a part of lock task (since it isn't a new task).
    public void testStartActivity_withinTask() {
        mDevicePolicyManager.setLockTaskPackages(getWho(), new String[] { PACKAGE_NAME });
        startLockTask(UTILITY_ACTIVITY);
        waitForResume();

        mReceivingActivityWasCreated = false;
        Intent launchIntent = getIntentReceivingActivityIntent(0);
        Intent lockTaskUtility = getLockTaskUtility(UTILITY_ACTIVITY);
        lockTaskUtility.putExtra(LockTaskUtilityActivity.START_ACTIVITY, launchIntent);
        mContext.startActivity(lockTaskUtility);

        synchronized (mReceivingActivityCreatedLock) {
            try {
                mReceivingActivityCreatedLock.wait(ACTIVITY_RESUMED_TIMEOUT_MILLIS);
            } catch (InterruptedException e) {
            }
            assertTrue(mReceivingActivityWasCreated);
        }
        stopAndFinish(UTILITY_ACTIVITY);
    }

    // This launches a whitelisted activity that is not part of the current task.
    // This should be permitted as a part of lock task.
    public void testStartActivity_outsideTaskWhitelisted() {
        mDevicePolicyManager.setLockTaskPackages(getWho(), new String[] { PACKAGE_NAME,
                RECEIVING_ACTIVITY_PACKAGE_NAME });
        startLockTask(UTILITY_ACTIVITY);
        waitForResume();

        mReceivingActivityWasCreated = false;
        Intent launchIntent = getIntentReceivingActivityIntent(0);
        launchIntent.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
        mContext.startActivity(launchIntent);
        synchronized (mReceivingActivityCreatedLock) {
            try {
                mReceivingActivityCreatedLock.wait(ACTIVITY_RESUMED_TIMEOUT_MILLIS);
            } catch (InterruptedException e) {
            }
            assertTrue(mReceivingActivityWasCreated);
        }
        stopAndFinish(UTILITY_ACTIVITY);
    }

    // This launches a non-whitelisted activity that is not part of the current task.
    // This should be blocked.
    public void testStartActivity_outsideTaskNonWhitelisted() {
        mDevicePolicyManager.setLockTaskPackages(getWho(), new String[] { PACKAGE_NAME });
        startLockTask(UTILITY_ACTIVITY);
        waitForResume();

        Intent launchIntent = getIntentReceivingActivityIntent(Intent.FLAG_ACTIVITY_NEW_TASK);
        mContext.startActivity(launchIntent);
        synchronized (mActivityResumedLock) {
            try {
                mActivityResumedLock.wait(ACTIVITY_RESUMED_TIMEOUT_MILLIS);
            } catch (InterruptedException e) {
            }
            assertFalse(mReceivingActivityWasCreated);
        }
        stopAndFinish(UTILITY_ACTIVITY);
    }

    // Test the lockTaskMode flag for an activity declaring if_whitelisted.
    // Whitelist the activity and verify that lock task mode is started.
    public void testManifestArgument_whitelisted() {
        mDevicePolicyManager.setLockTaskPackages(getWho(), new String[] { PACKAGE_NAME });
        startAndWait(getLockTaskUtility(UTILITY_ACTIVITY_IF_WHITELISTED));
        waitForResume();

        assertLockTaskModeActive();
        assertTrue(mIsActivityRunning);
        assertTrue(mIsActivityResumed);

        stopAndFinish(UTILITY_ACTIVITY_IF_WHITELISTED);
    }

    // Test the lockTaskMode flag for an activity declaring if_whitelisted.
    // Don't whitelist the activity and verify that lock task mode is not started.
    public void testManifestArgument_nonWhitelisted() {
        startAndWait(getLockTaskUtility(UTILITY_ACTIVITY_IF_WHITELISTED));
        waitForResume();

        assertLockTaskModeInactive();
        assertTrue(mIsActivityRunning);
        assertTrue(mIsActivityResumed);

        stopAndFinish(UTILITY_ACTIVITY_IF_WHITELISTED);
    }

    // Test the lockTaskMode flag for an activity declaring if_whitelisted.
    // An activity locked via manifest argument cannot finish without calling stopLockTask.
    public void testManifestArgument_cannotFinish() {
        mDevicePolicyManager.setLockTaskPackages(getWho(), new String[] { PACKAGE_NAME });
        startAndWait(getLockTaskUtility(UTILITY_ACTIVITY_IF_WHITELISTED));
        waitForResume();

        // If lock task has not exited then the activity shouldn't actually receive onDestroy.
        finishAndWait(UTILITY_ACTIVITY_IF_WHITELISTED);
        assertLockTaskModeActive();
        assertTrue(mIsActivityRunning);

        stopAndFinish(UTILITY_ACTIVITY_IF_WHITELISTED);
    }

    // Test the lockTaskMode flag for an activity declaring if_whitelisted.
    // Verifies that updating the whitelisting during lock task mode finishes the locked task.
    public void testManifestArgument_updateWhitelisting() {
        mDevicePolicyManager.setLockTaskPackages(getWho(), new String[] { PACKAGE_NAME });
        startAndWait(getLockTaskUtility(UTILITY_ACTIVITY_IF_WHITELISTED));
        waitForResume();

        mDevicePolicyManager.setLockTaskPackages(getWho(), new String[0]);

        synchronized (mActivityRunningLock) {
            try {
                mActivityRunningLock.wait(UPDATE_LOCK_TASK_TIMEOUT_MILLIS);
            } catch (InterruptedException e) {
            }
        }

        assertLockTaskModeInactive();
        assertFalse(mIsActivityRunning);
        assertFalse(mIsActivityResumed);
    }

    /**
     * Checks that lock task mode is active and fails the test if it isn't.
     */
    private void assertLockTaskModeActive() {
        assertTrue(mActivityManager.isInLockTaskMode());
        assertEquals(ActivityManager.LOCK_TASK_MODE_LOCKED,
                mActivityManager.getLockTaskModeState());
    }

    /**
     * Checks that lock task mode is not active and fails the test if it is.
     */
    private void assertLockTaskModeInactive() {
        assertFalse(mActivityManager.isInLockTaskMode());
        assertEquals(ActivityManager.LOCK_TASK_MODE_NONE, mActivityManager.getLockTaskModeState());
    }

    /**
     * Call stopLockTask and finish on the LockTaskUtilityActivity.
     *
     * Verify that the activity is no longer running.
     *
     * If activityManager is not null then verify that the ActivityManager
     * is no longer in lock task mode.
     */
    private void stopAndFinish(String className) {
        stopLockTask(className);
        finishAndWait(className);
        assertLockTaskModeInactive();
        assertFalse(mIsActivityRunning);
    }

    /**
     * Call finish on the LockTaskUtilityActivity and wait for
     * onDestroy to be called.
     */
    private void finishAndWait(String className) {
        synchronized (mActivityRunningLock) {
            finish(className);
            if (mIsActivityRunning) {
                try {
                    mActivityRunningLock.wait(ACTIVITY_DESTROYED_TIMEOUT_MILLIS);
                } catch (InterruptedException e) {
                }
            }
        }
    }

    /**
     * Wait for onResume to be called on the LockTaskUtilityActivity.
     */
    private void waitForResume() {
        // It may take a moment for the resume to come in.
        synchronized (mActivityResumedLock) {
            if (!mIsActivityResumed) {
                try {
                    mActivityResumedLock.wait(ACTIVITY_RESUMED_TIMEOUT_MILLIS);
                } catch (InterruptedException e) {
                }
            }
        }
    }

    /**
     * Calls startLockTask on the LockTaskUtilityActivity
     */
    private void startLockTask(String className) {
        Intent intent = getLockTaskUtility(className);
        intent.putExtra(LockTaskUtilityActivity.START_LOCK_TASK, true);
        startAndWait(intent);
    }

    /**
     * Calls stopLockTask on the LockTaskUtilityActivity
     */
    private void stopLockTask(String className) {
        Intent intent = getLockTaskUtility(className);
        intent.putExtra(LockTaskUtilityActivity.STOP_LOCK_TASK, true);
        startAndWait(intent);
    }

    /**
     * Calls finish on the LockTaskUtilityActivity
     */
    private void finish(String className) {
        Intent intent = getLockTaskUtility(className);
        intent.putExtra(LockTaskUtilityActivity.FINISH, true);
        startAndWait(intent);
    }

    /**
     * Sends a command intent to the LockTaskUtilityActivity and waits
     * to receive the broadcast back confirming it has finished processing
     * the command.
     */
    private void startAndWait(Intent intent) {
        mIntentHandled = false;
        synchronized (this) {
            mContext.startActivity(intent);
            // Give 20 secs to finish.
            try {
                wait(ACTIVITY_RUNNING_TIMEOUT_MILLIS);
            } catch (InterruptedException e) {
            }
            assertTrue(mIntentHandled);
        }
    }

    /**
     * Get basic intent that points at the LockTaskUtilityActivity.
     *
     * This intent includes the flags to make it act as single top.
     */
    private Intent getLockTaskUtility(String className) {
        Intent intent = new Intent();
        intent.setClassName(PACKAGE_NAME, className);
        intent.addFlags(Intent.FLAG_ACTIVITY_SINGLE_TOP | Intent.FLAG_ACTIVITY_NEW_TASK);
        return intent;
    }

    private Intent getIntentReceivingActivityIntent(int flags) {
        Intent intent = new Intent();
        intent.setComponent(
                new ComponentName(RECEIVING_ACTIVITY_PACKAGE_NAME, RECEIVING_ACTIVITY_NAME));
        intent.setAction(ACTION_JUST_CREATE);
        intent.setFlags(flags);
        return intent;
    }
}
