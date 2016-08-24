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

package com.android.uibench.janktests;

import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.os.SystemClock;
import android.support.test.uiautomator.By;
import android.support.test.uiautomator.Direction;
import android.support.test.uiautomator.UiDevice;
import android.support.test.uiautomator.UiObject2;
import android.support.test.uiautomator.Until;
import junit.framework.Assert;
/**
 * Jank benchmark tests helper for UiBench app
 */

public class UiBenchJankTestsHelper {
    public static final int LONG_TIMEOUT = 5000;
    public static final int TIMEOUT = 250;
    public static final int SHORT_TIMEOUT = 2000;
    public static final int EXPECTED_FRAMES = 100;

    public static final String PACKAGE_NAME = "com.android.test.uibench";

    private static UiBenchJankTestsHelper mInstance;
    private static UiDevice mDevice;
    private Context mContext;
    protected UiObject2 mContents;

    private UiBenchJankTestsHelper(Context context, UiDevice device) {
        mContext = context;
        mDevice = device;
    }

    public static UiBenchJankTestsHelper getInstance(Context context, UiDevice device) {
        if (mInstance == null) {
            mInstance = new UiBenchJankTestsHelper(context, device);
        }
        return mInstance;
    }

    /**
     * Launch activity using intent
     * @param activityName
     * @param verifyText
     */
    public void launchActivity(String activityName, String verifyText) {
        ComponentName cn = new ComponentName(PACKAGE_NAME,
                String.format("%s.%s", PACKAGE_NAME, activityName));
        Intent intent = new Intent(Intent.ACTION_MAIN);
        intent.setFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
        intent.addFlags(Intent.FLAG_ACTIVITY_CLEAR_TASK);
        intent.setComponent(cn);
        // Launch the activity
        mContext.startActivity(intent);
        UiObject2 expectedTextCmp = mDevice.wait(Until.findObject(
                By.text(verifyText)), LONG_TIMEOUT);
        Assert.assertNotNull(String.format("Issue in opening %s", activityName),
                expectedTextCmp);
    }

    /**
     * To perform the fling down and up on given content for flingCount number
     * of times
     * @param content
     * @param timeout
     * @param flingCount
     */
    public void flingUpDown(UiObject2 content, long timeout, int flingCount) {
        for (int count = 0; count < flingCount; count++) {
            SystemClock.sleep(timeout);
            content.fling(Direction.DOWN);
            SystemClock.sleep(timeout);
            content.fling(Direction.UP);
        }
    }

}
