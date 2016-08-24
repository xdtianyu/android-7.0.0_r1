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

package android.assist.cts;

import android.assist.cts.TestStartActivity;
import android.assist.common.Utils;

import android.app.Activity;
import android.app.assist.AssistContent;
import android.app.assist.AssistStructure;
import android.content.BroadcastReceiver;
import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.cts.util.SystemUtil;
import android.os.Bundle;
import android.provider.Settings;
import android.test.ActivityInstrumentationTestCase2;
import android.util.Log;

import java.lang.Override;
import java.util.concurrent.CountDownLatch;
import java.util.concurrent.TimeUnit;

/** Test we receive proper assist data when context is disabled or enabled */

public class LifecycleTest extends AssistTestBase {
    private static final String TAG = "LifecycleTest";
    private static final String action_hasResumed = Utils.LIFECYCLE_HASRESUMED;
    private static final String action_onPause = Utils.LIFECYCLE_ONPAUSE;
    private static final String action_onStop = Utils.LIFECYCLE_ONSTOP;
    private static final String action_onDestroy = Utils.LIFECYCLE_ONDESTROY;

    private static final String TEST_CASE_TYPE = Utils.LIFECYCLE;

    private BroadcastReceiver mLifecycleTestBroadcastReceiver;
    private CountDownLatch mHasResumedLatch = new CountDownLatch(1);
    private CountDownLatch mActivityLifecycleLatch = new CountDownLatch(1);
    private CountDownLatch mReadyLatch = new CountDownLatch(1);

    @Override
    public void setUp() throws Exception {
        super.setUp();
        setUpAndRegisterReceiver();
        startTestActivity(TEST_CASE_TYPE);
    }

    @Override
    public void tearDown() throws Exception {
        super.tearDown();
        if (mLifecycleTestBroadcastReceiver != null) {
            mContext.unregisterReceiver(mLifecycleTestBroadcastReceiver);
            mLifecycleTestBroadcastReceiver = null;
        }
    }

    private void setUpAndRegisterReceiver() {
        if (mLifecycleTestBroadcastReceiver != null) {
            mContext.unregisterReceiver(mLifecycleTestBroadcastReceiver);
        }
        mLifecycleTestBroadcastReceiver = new LifecycleTestReceiver();
        IntentFilter filter = new IntentFilter();
        filter.addAction(action_hasResumed);
        filter.addAction(action_onPause);
        filter.addAction(action_onStop);
        filter.addAction(action_onDestroy);
        filter.addAction(Utils.ASSIST_RECEIVER_REGISTERED);
        mContext.registerReceiver(mLifecycleTestBroadcastReceiver, filter);

    }

    private void waitForOnResume() throws Exception {
        Log.i(TAG, "waiting for onResume() before continuing");
        if (!mHasResumedLatch.await(Utils.ACTIVITY_ONRESUME_TIMEOUT_MS, TimeUnit.MILLISECONDS)) {
            fail("Activity failed to resume in " + Utils.ACTIVITY_ONRESUME_TIMEOUT_MS + "msec");
        }
    }

    private void waitAndSeeIfLifecycleMethodsAreTriggered() throws Exception {
        if (mActivityLifecycleLatch.await(Utils.TIMEOUT_MS, TimeUnit.MILLISECONDS)) {
            fail("One or more lifecycle methods were called after triggering assist");
        }
    }

    public void testLayerDoesNotTriggerLifecycleMethods() throws Exception {
        if (mActivityManager.isLowRamDevice()) {
            Log.d(TAG, "Not running assist tests on low-RAM device.");
            return;
        }
        mTestActivity.startTest(Utils.LIFECYCLE);
        waitForAssistantToBeReady(mReadyLatch);
        mTestActivity.start3pApp(Utils.LIFECYCLE);
        waitForOnResume();
        startSession();
        waitForContext();
        waitAndSeeIfLifecycleMethodsAreTriggered();
    }

    private class LifecycleTestReceiver extends BroadcastReceiver {
        @Override
        public void onReceive(Context context, Intent intent) {
            String action = intent.getAction();
            if (action.equals(action_hasResumed) && mHasResumedLatch != null) {
                mHasResumedLatch.countDown();
            } else if (action.equals(action_onPause) && mActivityLifecycleLatch != null) {
                mActivityLifecycleLatch.countDown();
            } else if (action.equals(action_onStop) && mActivityLifecycleLatch != null) {
                mActivityLifecycleLatch.countDown();
            } else if (action.equals(action_onDestroy) && mActivityLifecycleLatch != null) {
                mActivityLifecycleLatch.countDown();
            } else if (action.equals(Utils.ASSIST_RECEIVER_REGISTERED)) {
                if (mReadyLatch != null) {
                    mReadyLatch.countDown();
                }
            }
        }
    }
}
