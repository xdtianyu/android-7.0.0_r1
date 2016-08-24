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

package com.android.androidbvt;

import android.app.KeyguardManager;
import android.content.Context;
import android.content.Intent;
import android.os.RemoteException;
import android.provider.Settings;
import android.support.test.InstrumentationRegistry;
import android.support.test.uiautomator.By;
import android.support.test.uiautomator.Direction;
import android.support.test.uiautomator.UiDevice;
import android.support.test.uiautomator.UiObject2;
import android.support.test.uiautomator.Until;
import android.test.suitebuilder.annotation.LargeTest;

import junit.framework.Assert;
import junit.framework.TestCase;

public class SysUILockScreenTests extends TestCase {
    private static final String LAUNCHER_PACKAGE = "com.google.android.googlequicksearchbox";
    private static final String SYSTEMUI_PACKAGE = "com.android.systemui";
    private static final String EDIT_TEXT_CLASS_NAME = "android.widget.EditText";
    private static final int SHORT_TIMEOUT = 200;
    private static final int LONG_TIMEOUT = 2000;
    private static final int PIN = 1234;
    private static final String PASSWORD = "aaaa";
    private AndroidBvtHelper mABvtHelper = null;
    private UiDevice mDevice = null;
    private Context mContext;

    @Override
    public void setUp() throws Exception {
        super.setUp();
        mDevice = UiDevice.getInstance(InstrumentationRegistry.getInstrumentation());
        mDevice.freezeRotation();
        mContext = InstrumentationRegistry.getTargetContext();
        mABvtHelper = AndroidBvtHelper.getInstance(mDevice, mContext,
                InstrumentationRegistry.getInstrumentation().getUiAutomation());
        mDevice.wakeUp();
        mDevice.pressHome();
    }

    @Override
    public void tearDown() throws Exception {
        mDevice.pressHome();
        mDevice.unfreezeRotation();
        mDevice.waitForIdle();
        super.tearDown();
    }

    /**
     * Following test will add PIN for Lock Screen, and remove PIN
     * @throws Exception
     */
    @LargeTest
    public void testLockScreenPIN() throws Exception {
        setScreenLock(Integer.toString(PIN), "PIN");
        sleepAndWakeUpDevice();
        unlockScreen(Integer.toString(PIN));
        removeScreenLock(Integer.toString(PIN));
        Thread.sleep(mABvtHelper.LONG_TIMEOUT);
        Assert.assertFalse("Lock Screen is still enabled", isLockScreenEnabled());
    }

    /**
     * Following test will add password for Lock Screen, and remove Password
     * @throws Exception
     */
    @LargeTest
    public void testLockScreenPwd() throws Exception {
        setScreenLock(PASSWORD, "Password");
        sleepAndWakeUpDevice();
        unlockScreen(PASSWORD);
        removeScreenLock(PASSWORD);
        Thread.sleep(mABvtHelper.LONG_TIMEOUT);
        Assert.assertFalse("Lock Screen is still enabled", isLockScreenEnabled());
    }

    /**
     * Following test will add password for Lock Screen, check Emergency Call Page existence, and
     * remove password for Lock Screen
     * @throws Exception
     */
    @LargeTest
    public void testEmergencyCall() throws Exception {
        if (!mABvtHelper.isTablet()) {
            setScreenLock(PASSWORD, "Password");
            sleepAndWakeUpDevice();
            checkEmergencyCall();
            unlockScreen(PASSWORD);
            removeScreenLock(PASSWORD);
            Thread.sleep(mABvtHelper.LONG_TIMEOUT);
            Assert.assertFalse("Lock Screen is still enabled", isLockScreenEnabled());
        }
    }

    /**
     * Just lock the screen and slide up to unlock
     */
    @LargeTest
    public void testSlideUnlock() throws Exception {
        sleepAndWakeUpDevice();
        mDevice.wait(Until.findObject(
                By.res(SYSTEMUI_PACKAGE, "notification_stack_scroller")), 2000)
                .swipe(Direction.UP, 1.0f);
        int counter = 6;
        UiObject2 workspace = mDevice.findObject(By.res(LAUNCHER_PACKAGE, "workspace"));
        while (counter-- > 0 && workspace == null) {
            workspace = mDevice.findObject(By.res(LAUNCHER_PACKAGE, "workspace"));
            Thread.sleep(500);
        }
        assertNotNull("Workspace wasn't found", workspace);
    }

    /**
     * Sets the screen lock pin or password
     * @param pwd text of Password or Pin for lockscreen
     * @param mode indicate if its password or PIN
     */
    private void setScreenLock(String pwd, String mode) throws Exception {
        navigateToScreenLock();
        mDevice.wait(Until.findObject(By.text(mode)), mABvtHelper.LONG_TIMEOUT).click();
        // set up Secure start-up page
        mDevice.wait(Until.findObject(By.text("No thanks")), mABvtHelper.LONG_TIMEOUT).click();
        UiObject2 pinField = mDevice.wait(Until.findObject(By.clazz(EDIT_TEXT_CLASS_NAME)),
                mABvtHelper.LONG_TIMEOUT);
        pinField.setText(pwd);
        // enter and verify password
        mDevice.pressEnter();
        pinField.setText(pwd);
        mDevice.pressEnter();
        mDevice.wait(Until.findObject(By.text("DONE")), mABvtHelper.LONG_TIMEOUT).click();
    }

    /**
     * check if Emergency Call page exists
     */
    private void checkEmergencyCall() throws Exception {
        mDevice.pressMenu();
        mDevice.wait(Until.findObject(By.text("EMERGENCY")), mABvtHelper.LONG_TIMEOUT).click();
        Thread.sleep(mABvtHelper.LONG_TIMEOUT);
        UiObject2 dialButton = mDevice.wait(Until.findObject(By.desc("dial")),
                mABvtHelper.LONG_TIMEOUT);
        Assert.assertNotNull("Can't reach emergency call page", dialButton);
        mDevice.pressBack();
        Thread.sleep(mABvtHelper.LONG_TIMEOUT);
    }

    private void removeScreenLock(String pwd) throws Exception {
        navigateToScreenLock();
        UiObject2 pinField = mDevice.wait(Until.findObject(By.clazz(EDIT_TEXT_CLASS_NAME)),
                mABvtHelper.LONG_TIMEOUT);
        pinField.setText(pwd);
        mDevice.pressEnter();
        mDevice.wait(Until.findObject(By.text("Swipe")), mABvtHelper.LONG_TIMEOUT).click();
        mDevice.wait(Until.findObject(By.text("YES, REMOVE")), mABvtHelper.LONG_TIMEOUT).click();
    }

    private void unlockScreen(String pwd) throws Exception {
        swipeUp();
        Thread.sleep(mABvtHelper.SHORT_TIMEOUT);
        // enter password to unlock screen
        String command = String.format(" %s %s %s", "input", "text", pwd);
        mDevice.executeShellCommand(command);
        mDevice.waitForIdle();
        Thread.sleep(mABvtHelper.SHORT_TIMEOUT);
        mDevice.pressEnter();
    }

    private void navigateToScreenLock() throws Exception {
        launchSettingsPage(mContext, Settings.ACTION_SECURITY_SETTINGS);
        mDevice.wait(Until.findObject(By.text("Screen lock")), mABvtHelper.LONG_TIMEOUT).click();
    }

    private void launchSettingsPage(Context ctx, String pageName) throws Exception {
        Intent intent = new Intent(pageName);
        intent.setFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
        ctx.startActivity(intent);
        Thread.sleep(mABvtHelper.LONG_TIMEOUT * 2);
    }

    private void sleepAndWakeUpDevice() throws RemoteException, InterruptedException {
        mDevice.sleep();
        Thread.sleep(mABvtHelper.LONG_TIMEOUT);
        mDevice.wakeUp();
    }

    private void swipeUp() throws Exception {
        mDevice.swipe(mDevice.getDisplayWidth() / 2, mDevice.getDisplayHeight(),
                mDevice.getDisplayWidth() / 2, 0, 30);
        Thread.sleep(mABvtHelper.SHORT_TIMEOUT);
    }

    private boolean isLockScreenEnabled() {
        KeyguardManager km = (KeyguardManager) mContext.getSystemService(Context.KEYGUARD_SERVICE);
        return km.isKeyguardSecure();
    }
}

