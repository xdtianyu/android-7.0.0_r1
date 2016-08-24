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
 * limitations under the License.
 */

package com.android.notification.functional;

import android.app.NotificationManager;
import android.content.Context;
import android.service.notification.StatusBarNotification;
import android.support.test.uiautomator.By;
import android.support.test.uiautomator.Direction;
import android.support.test.uiautomator.UiDevice;
import android.support.test.uiautomator.UiObject2;
import android.support.test.uiautomator.Until;
import android.test.InstrumentationTestCase;
import android.test.suitebuilder.annotation.MediumTest;
import android.util.Log;

import java.util.HashMap;
import java.util.Map;

public class NotificationInteractionTests extends InstrumentationTestCase {
    private static final String LOG_TAG = NotificationInteractionTests.class.getSimpleName();
    private static final int LONG_TIMEOUT = 2000;
    private final boolean DEBUG = false;
    private NotificationManager mNotificationManager;
    private UiDevice mDevice = null;
    private Context mContext;
    private NotificationHelper mHelper;
    private static final int CUSTOM_NOTIFICATION_ID = 1;
    private static final int NOTIFICATIONS_COUNT = 3;

    @Override
    public void setUp() throws Exception {
        super.setUp();
        mDevice = UiDevice.getInstance(getInstrumentation());
        mContext = getInstrumentation().getContext();
        mNotificationManager = (NotificationManager) mContext
                .getSystemService(Context.NOTIFICATION_SERVICE);
        mHelper = new NotificationHelper(mDevice, getInstrumentation(), mNotificationManager);
        mDevice.setOrientationNatural();
        mNotificationManager.cancelAll();
    }

    @Override
    public void tearDown() throws Exception {
        super.tearDown();
        mDevice.unfreezeRotation();
        mDevice.pressHome();
        mNotificationManager.cancelAll();
    }

    @MediumTest
    public void testNonDismissNotification() throws Exception {
        String text = "USB debugging connected";
        mDevice.openNotification();
        Thread.sleep(LONG_TIMEOUT);
        UiObject2 obj = findByText(text);
        assertNotNull(String.format("Couldn't find %s notification", text), obj);
        obj.swipe(Direction.LEFT, 1.0f);
        Thread.sleep(LONG_TIMEOUT);
        obj = mDevice.wait(Until.findObject(By.text(text)),
                LONG_TIMEOUT);
        assertNotNull("USB debugging notification has been dismissed", obj);
    }

    /** send out multiple notifications in order to test CLEAR ALL function */
    @MediumTest
    public void testDismissAll() throws Exception {
        String text = "CLEAR ALL";
        Map<Integer, String> lists = new HashMap<Integer, String>();
        StatusBarNotification[] sbns = mNotificationManager.getActiveNotifications();
        int currentSbns = sbns.length;
        for (int i = 0; i < NOTIFICATIONS_COUNT; i++) {
            lists.put(CUSTOM_NOTIFICATION_ID + i, Integer.toString(CUSTOM_NOTIFICATION_ID + i));
        }
        mHelper.sendNotifications(lists);
        if (DEBUG) {
            Log.d(LOG_TAG,
                    String.format("posted %s notifications, here they are: ", NOTIFICATIONS_COUNT));
            sbns = mNotificationManager.getActiveNotifications();
            for (StatusBarNotification sbn : sbns) {
                Log.d(LOG_TAG, "  " + sbn);
            }
        }
        if (mDevice.openNotification()) {
            Thread.sleep(LONG_TIMEOUT);
            UiObject2 clearAll = findByText(text);
            clearAll.click();
        }
        Thread.sleep(LONG_TIMEOUT);
        sbns = mNotificationManager.getActiveNotifications();
        assertTrue(String.format("%s notifications have not been cleared", sbns.length),
                sbns.length == currentSbns);
    }

    private UiObject2 findByText(String text) throws Exception {
        int maxAttempt = 5;
        UiObject2 item = null;
        while (maxAttempt-- > 0) {
            item = mDevice.wait(Until.findObject(By.text(text)), LONG_TIMEOUT);
            if (item == null) {
                mDevice.swipe(mDevice.getDisplayWidth() / 2, mDevice.getDisplayHeight() / 2,
                        mDevice.getDisplayWidth() / 2, 0, 30);
            } else {
                return item;
            }
        }
        return null;
    }
}
