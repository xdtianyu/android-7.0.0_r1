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

package com.android.wearable.sysapp.janktests;

import android.app.Instrumentation;
import android.content.ComponentName;
import android.content.Intent;
import android.os.SystemClock;
import android.support.test.uiautomator.By;
import android.support.test.uiautomator.UiDevice;
import android.support.test.uiautomator.UiObject;
import android.support.test.uiautomator.UiObject2;
import android.support.test.uiautomator.UiSelector;
import android.support.test.uiautomator.Until;
import android.util.Log;
import android.view.KeyEvent;

import junit.framework.Assert;

import java.util.concurrent.TimeoutException;

/**
 * Helper for all the system apps jank tests
 */
public class SysAppTestHelper {

    private static final String LOG_TAG = SysAppTestHelper.class.getSimpleName();
    public static final int EXPECTED_FRAMES_CARDS_TEST = 20;
    public static final int EXPECTED_FRAMES = 100;
    public static final int LONG_TIMEOUT = 5000;
    public static final int SHORT_TIMEOUT = 500;
    public static final int FLING_SPEED = 5000;
    private static final long NEW_CARD_TIMEOUT_MS = 5 * 1000; // 5s
    private static final String RELOAD_NOTIFICATION_CARD_INTENT = "com.google.android.wearable."
            + "support.wearnotificationgenerator.SHOW_NOTIFICATION";
    private static final String HOME_INDICATOR = "charging_icon";
    private static final String LAUNCHER_VIEW_NAME = "launcher_view";
    private static final String CARD_VIEW_NAME = "activity_view";
    private static final String QUICKSETTING_VIEW_NAME = "settings_icon";

    // Demo card selectors
    private static final UiSelector CARD_SELECTOR = new UiSelector()
            .resourceId("com.google.android.wearable.app:id/snippet");
    private static final UiSelector TITLE_SELECTOR = new UiSelector()
            .resourceId("com.google.android.wearable.app:id/title");
    private static final UiSelector CLOCK_SELECTOR = new UiSelector()
            .resourceId("com.google.android.wearable.app:id/clock_bar");
    private static final UiSelector ICON_SELECTOR = new UiSelector()
            .resourceId("com.google.android.wearable.app:id/icon");
    private static final UiSelector TEXT_SELECTOR = new UiSelector()
            .resourceId("com.google.android.wearable.app:id/text");
    private static final UiSelector STATUS_BAR_SELECTOR = new UiSelector()
            .resourceId("com.google.android.wearable.app:id/status_bar_icons");

    private UiDevice mDevice = null;
    private Instrumentation instrumentation = null;
    private UiObject mCard = null;
    private UiObject mTitle = null;
    private UiObject mClock = null;
    private UiObject mIcon = null;
    private UiObject mText = null;
    private UiObject mStatus = null;
    private Intent mIntent = null;
    private static SysAppTestHelper sysAppTestHelperInstance;

    /**
     * @param mDevice
     * @param instrumentation
     */
    private SysAppTestHelper(UiDevice mDevice, Instrumentation instrumentation) {
        super();
        this.mDevice = mDevice;
        this.instrumentation = instrumentation;
        mIntent = new Intent();
        mCard = mDevice.findObject(CARD_SELECTOR);
        mTitle = mDevice.findObject(TITLE_SELECTOR);
        mClock = mDevice.findObject(CLOCK_SELECTOR);
        mIcon = mDevice.findObject(ICON_SELECTOR);
        mText = mDevice.findObject(TEXT_SELECTOR);
        mStatus = mDevice.findObject(STATUS_BAR_SELECTOR);
    }

    public static SysAppTestHelper getInstance(UiDevice device, Instrumentation instrumentation) {
        if (sysAppTestHelperInstance == null) {
            sysAppTestHelperInstance = new SysAppTestHelper(device, instrumentation);
        }
        return sysAppTestHelperInstance;
    }

    public void swipeRight() {
        mDevice.swipe(50,
                mDevice.getDisplayHeight() / 2, mDevice.getDisplayWidth() - 25,
                mDevice.getDisplayHeight() / 2, 30); // slow speed
        SystemClock.sleep(SHORT_TIMEOUT);
    }

    public void swipeLeft() {
        mDevice.swipe(mDevice.getDisplayWidth() - 50, mDevice.getDisplayHeight() / 2, 50,
                mDevice.getDisplayHeight() / 2, 30); // slow speed
        SystemClock.sleep(SHORT_TIMEOUT);
    }

    public void swipeUp() {
        mDevice.swipe(mDevice.getDisplayWidth() / 2, mDevice.getDisplayHeight() / 2 + 50,
                mDevice.getDisplayWidth() / 2, 0, 30); // slow speed
        SystemClock.sleep(SHORT_TIMEOUT);
    }

    public void swipeDown() {
        mDevice.swipe(mDevice.getDisplayWidth() / 2, 0, mDevice.getDisplayWidth() / 2,
                mDevice.getDisplayHeight() / 2 + 50, 30); // slow speed
        SystemClock.sleep(SHORT_TIMEOUT);
    }

    public void flingUp() {
        mDevice.swipe(mDevice.getDisplayWidth() / 2, mDevice.getDisplayHeight() / 2 + 50,
                mDevice.getDisplayWidth() / 2, 0, 5); // fast speed
        SystemClock.sleep(SHORT_TIMEOUT);
    }

    public void flingDown() {
        mDevice.swipe(mDevice.getDisplayWidth() / 2, 0, mDevice.getDisplayWidth() / 2,
                mDevice.getDisplayHeight() / 2 + 50, 5); // fast speed
        SystemClock.sleep(SHORT_TIMEOUT);
    }

    // Helper method to go back to home screen
    public void goBackHome() {
        String launcherPackage = mDevice.getLauncherPackageName();
        UiObject2 homeScreen = mDevice.findObject(By.res(launcherPackage, HOME_INDICATOR));
        int count = 0;
        while (homeScreen == null && count < 5) {
            mDevice.pressBack();
            homeScreen = mDevice.findObject(By.res(launcherPackage, HOME_INDICATOR));
            count ++;
        }

        // TODO (yuanlang@) Delete the following hacky codes after charging icon issue fixed
        // Make sure we're not in the launcher
        homeScreen = mDevice.findObject(By.res(launcherPackage, LAUNCHER_VIEW_NAME));
        if (homeScreen != null) {
            mDevice.pressBack();
        }
        // Make sure we're not in cards view
        homeScreen = mDevice.findObject(By.res(launcherPackage, CARD_VIEW_NAME));
        if (homeScreen != null) {
            mDevice.pressBack();
        }
        // Make sure we're not in the quick settings
        homeScreen = mDevice.findObject(By.res(launcherPackage, QUICKSETTING_VIEW_NAME));
        if (homeScreen != null) {
            mDevice.pressBack();
        }
        SystemClock.sleep(LONG_TIMEOUT);
    }

    // Helper method to verify if there are any Demo cards.

    // TODO: Allow user to pass in how many cards are expected to find cause some tests may require
    // more than one card.
    public void hasDemoCards() throws Exception {
        // Device should be pre-loaded with demo cards.

        goBackHome(); // Start by going to Home.

        if (!mTitle.waitForExists(NEW_CARD_TIMEOUT_MS)) {
            Log.d(LOG_TAG, "Card previews not available, swiping up");
            swipeUp();
            // For few devices, demo card preview is hidden by default. So swipe once to bring up
            // the card.
        }

        // First card from the pre-loaded demo cards could be either in peek view
        // or in full view(e.g Dory) or no peek view(Sturgeon). Ensure to check for demo cards
        // existence in both cases.
        if (!(mCard.waitForExists(NEW_CARD_TIMEOUT_MS)
                || mTitle.waitForExists(NEW_CARD_TIMEOUT_MS)
                || mIcon.waitForExists(NEW_CARD_TIMEOUT_MS)
                || mText.waitForExists(NEW_CARD_TIMEOUT_MS))) {
            Log.d(LOG_TAG, "Demo cards not found, going to reload the cards");
            // If there are no Demo cards, reload them.
            reloadDemoCards();
            if (!mTitle.waitForExists(NEW_CARD_TIMEOUT_MS)) {
                swipeUp(); // For few devices, demo card preview is hidden by
                // default. So swipe once to bring up the card.
            }
        }
        Assert.assertTrue("no cards available for testing",
                (mTitle.waitForExists(NEW_CARD_TIMEOUT_MS)
                        || mIcon.waitForExists(NEW_CARD_TIMEOUT_MS)
                        || mText.waitForExists(NEW_CARD_TIMEOUT_MS)));
    }

    // This will ensure to reload notification cards by launching NotificationsGeneratorWear app
    // when there are insufficient cards.
    private void reloadDemoCards() {
        mIntent.setAction(RELOAD_NOTIFICATION_CARD_INTENT);
        instrumentation.getContext().sendBroadcast(mIntent);
        SystemClock.sleep(LONG_TIMEOUT);
    }

    public void launchActivity(String appPackage, String activityToLaunch) {
        mIntent.setAction("android.intent.action.MAIN");
        mIntent.setComponent(new ComponentName(appPackage, activityToLaunch));
        mIntent.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
        instrumentation.getContext().startActivity(mIntent);
    }

    // Helper method to goto app launcher and verifies you are there.
    public void gotoAppLauncher() throws TimeoutException {
        goBackHome();
        mDevice.pressKeyCode(KeyEvent.KEYCODE_BACK);
        UiObject2 appLauncher = mDevice.wait(Until.findObject(By.text("Agenda")),
                SysAppTestHelper.LONG_TIMEOUT);
        Assert.assertNotNull("App launcher not launched", appLauncher);
    }
}
