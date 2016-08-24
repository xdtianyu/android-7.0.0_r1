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
package com.android.tv.tests.ui;

import static com.android.tv.testing.uihelper.UiDeviceAsserts.assertWaitForCondition;

import android.os.SystemClock;
import android.support.test.uiautomator.Until;
import android.test.suitebuilder.annotation.LargeTest;
import android.util.Log;

import com.android.tv.R;
import com.android.tv.testing.uihelper.Constants;

/**
 * Tests for {@link com.android.tv.MainActivity}.
 */
@LargeTest
public class ProgramGuidePerformanceTest extends LiveChannelsTestCase {
    private static final String TAG = "ProgramGuidePerformance";

    public static final int SHOW_MENU_MAX_DURATION_MS = 1500;
    public void testShowMenu() {
        mLiveChannelsHelper.assertAppStarted();
        mMenuHelper.showMenu();
        mMenuHelper.assertNavigateToMenuItem(R.string.menu_title_channels,
                R.string.channels_item_program_guide);
        //TODO: build a simple performance framework like JankTest
        long start = SystemClock.elapsedRealtime();
        Log.v(TAG, "start " + start + " milliSeconds");
        mDevice.pressDPadCenter();
        assertWaitForCondition(mDevice, Until.hasObject(Constants.PROGRAM_GUIDE));
        long end = SystemClock.elapsedRealtime();
        Log.v(TAG, "end " + end + " milliSeconds");
        long duration = end - start;
        assertDuration("ShowMenu", SHOW_MENU_MAX_DURATION_MS, duration);
        mDevice.pressBack();
    }

    private void assertDuration(String msg, long expectedMaxMilliSeconds, long actualMilliSeconds) {
        Log.d(TAG, msg + " duration " + actualMilliSeconds + " milliSeconds");
        assertTrue(msg + " duration expected to be <= " + expectedMaxMilliSeconds
                + " milliSeconds but was " + actualMilliSeconds + " milliSeconds.",
                actualMilliSeconds <= expectedMaxMilliSeconds);
    }
}
