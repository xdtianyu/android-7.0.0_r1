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

package com.android.sysapp.janktests;

import java.io.File;
import java.io.IOException;

import android.content.Intent;
import android.content.pm.PackageManager;
import android.os.Bundle;
import android.os.Environment;
import android.os.RemoteException;
import android.os.SystemClock;
import android.support.test.jank.GfxMonitor;
import android.support.test.jank.JankTest;
import android.support.test.jank.JankTestBase;
import android.support.test.uiautomator.By;
import android.support.test.uiautomator.BySelector;
import android.support.test.uiautomator.Direction;
import android.support.test.uiautomator.StaleObjectException;
import android.support.test.uiautomator.UiDevice;
import android.support.test.uiautomator.UiObject2;
import android.support.test.uiautomator.UiObjectNotFoundException;
import android.support.test.uiautomator.Until;
import android.widget.ImageButton;
import junit.framework.Assert;
import android.platform.test.helpers.GmailHelperImpl;
import android.support.test.timeresulthelper.TimeResultLogger;
import java.util.regex.Pattern;

/**
 * Jank test for scrolling gmail inbox mails
 */

public class GMailJankTests extends JankTestBase {
    private static final int SHORT_TIMEOUT = 1000;
    private static final int LONG_TIMEOUT = 5000;
    private static final int INNER_LOOP = 5;
    private static final int EXPECTED_FRAMES = 100;
    private static final int TAB_MIN_WIDTH = 600;
    private static final String PACKAGE_NAME = "com.google.android.gm";
    private static final String RES_PACKAGE_NAME = "android";
    private UiDevice mDevice;
    private GmailHelperImpl mGmailHelper;
    private static final File TIMESTAMP_FILE = new File(Environment.getExternalStorageDirectory()
            .getAbsolutePath(), "autotester.log");
    private static final File RESULTS_FILE = new File(Environment.getExternalStorageDirectory()
            .getAbsolutePath(), "results.log");

    @Override
    public void setUp() throws Exception {
        super.setUp();
        mDevice = UiDevice.getInstance(getInstrumentation());
        mGmailHelper = new GmailHelperImpl(getInstrumentation());
        mDevice.setOrientationNatural();
    }

    @Override
    protected void tearDown() throws Exception {
        mDevice.unfreezeRotation();
        super.tearDown();
    }

    public void launchApp(String packageName) throws UiObjectNotFoundException{
        PackageManager pm = getInstrumentation().getContext().getPackageManager();
        Intent appIntent = pm.getLaunchIntentForPackage(packageName);
        appIntent.setFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
        getInstrumentation().getContext().startActivity(appIntent);
        SystemClock.sleep(SHORT_TIMEOUT);
    }

    public void launchGMail () throws UiObjectNotFoundException {
        launchApp(PACKAGE_NAME);
        mGmailHelper.dismissInitialDialogs();
    }

    public void prepGMailInboxFling() throws UiObjectNotFoundException, IOException {
      launchGMail();
      // Ensure test is ready to be executed
      UiObject2 list = mDevice.wait(
              Until.findObject(By.res(PACKAGE_NAME, "conversation_list_view")), SHORT_TIMEOUT);
      Assert.assertNotNull("Failed to locate 'conversation_list_view'", list);
      TimeResultLogger.writeTimeStampLogStart(String.format("%s-%s",
              getClass().getSimpleName(), getName()), TIMESTAMP_FILE);
    }

    public void afterTestGMailInboxFling(Bundle metrics) throws IOException {
        TimeResultLogger.writeTimeStampLogEnd(String.format("%s-%s",
                getClass().getSimpleName(), getName()), TIMESTAMP_FILE);
        TimeResultLogger.writeResultToFile(String.format("%s-%s",
                getClass().getSimpleName(), getName()), RESULTS_FILE, metrics);
        super.afterTest(metrics);
    }

    // Measures jank while scrolling gmail inbox
    @JankTest(beforeTest="prepGMailInboxFling", expectedFrames=EXPECTED_FRAMES,
            afterTest="afterTestGMailInboxFling")
    @GfxMonitor(processName=PACKAGE_NAME)
    public void testGMailInboxFling() {
        UiObject2 list = mDevice.wait(
                Until.findObject(By.res(PACKAGE_NAME, "conversation_list_view")), LONG_TIMEOUT);
        Assert.assertNotNull("Failed to locate 'conversation_list_view'", list);
        for (int i = 0; i < INNER_LOOP; i++) {
            list.scroll(Direction.DOWN, 1.0f);
            SystemClock.sleep(SHORT_TIMEOUT);
            list.scroll(Direction.UP, 1.0f);
            SystemClock.sleep(SHORT_TIMEOUT);
        }
    }

    public void prepOpenNavDrawer() throws UiObjectNotFoundException, IOException {
      launchGMail();
      // Ensure test is ready to be executed
      Assert.assertNotNull("Failed to locate Nav Drawer Openner", openNavigationDrawer());
      TimeResultLogger.writeTimeStampLogStart(String.format("%s-%s",
              getClass().getSimpleName(), getName()), TIMESTAMP_FILE);
    }

    public void afterTestOpenNavDrawer(Bundle metrics) throws IOException {
        TimeResultLogger.writeTimeStampLogEnd(String.format("%s-%s",
                getClass().getSimpleName(), getName()), TIMESTAMP_FILE);
        TimeResultLogger.writeResultToFile(String.format("%s-%s",
                getClass().getSimpleName(), getName()), RESULTS_FILE, metrics);
        super.afterTest(metrics);
    }

    // Measures jank while opening Navigation Drawer
    @JankTest(beforeTest="prepOpenNavDrawer", expectedFrames=EXPECTED_FRAMES,
            afterTest="afterTestOpenNavDrawer")
    @GfxMonitor(processName=PACKAGE_NAME)
    public void testOpenNavDrawer() {
        UiObject2 navDrawer = openNavigationDrawer();
        for (int i = 0; i < INNER_LOOP; i++) {
            navDrawer.click();
            SystemClock.sleep(SHORT_TIMEOUT);
            mDevice.pressBack();
            SystemClock.sleep(SHORT_TIMEOUT);
        }
    }

    public void prepFlingNavDrawer() throws UiObjectNotFoundException, IOException{
        launchGMail();
        UiObject2 navDrawer = openNavigationDrawer();
        Assert.assertNotNull("Failed to locate Nav Drawer Openner", navDrawer);
        navDrawer.click();
        // Ensure test is ready to be executed
        UiObject2 acctListBtn = mDevice.wait(
                Until.findObject(By.res(PACKAGE_NAME, "account_list_button")),
                SHORT_TIMEOUT);
        Assert.assertNotNull("Failed to locate Nav drawer ", acctListBtn);
        TimeResultLogger.writeTimeStampLogStart(String.format("%s-%s",
                getClass().getSimpleName(), getName()), TIMESTAMP_FILE);
    }

    public void afterTestFlingNavDrawer(Bundle metrics) throws IOException {
        if (!mGmailHelper.closeNavigationDrawer()) {
            UiObject2 container = getNavigationDrawerContainer();
            if (container != null) {
                container.fling(Direction.RIGHT);
            }
        }
        TimeResultLogger.writeTimeStampLogEnd(String.format("%s-%s",
                getClass().getSimpleName(), getName()), TIMESTAMP_FILE);
        TimeResultLogger.writeResultToFile(String.format("%s-%s",
                getClass().getSimpleName(), getName()), RESULTS_FILE, metrics);
        super.afterTest(metrics);
    }

    // Measures jank while flinging Navigation Drawer
    @JankTest(beforeTest="prepFlingNavDrawer", expectedFrames=EXPECTED_FRAMES,
            afterTest="afterTestFlingNavDrawer")
    @GfxMonitor(processName=PACKAGE_NAME)
    public void testFlingNavDrawer() {
        UiObject2 container = getNavigationDrawerContainer();
        for (int i = 0; i < INNER_LOOP; i++) {
            container.fling(Direction.DOWN);
            SystemClock.sleep(SHORT_TIMEOUT);
            container.fling(Direction.UP);
            SystemClock.sleep(SHORT_TIMEOUT);
        }
    }


    public UiObject2 openNavigationDrawer() {
        UiObject2 nav = mDevice.findObject(By.desc(Pattern.compile(
                "(Open navigation drawer)|(Navigate up)")));
        if (nav == null) {
            throw new IllegalStateException("Could not find navigation drawer");
        }
        return nav;
    }

    public UiObject2 getNavigationDrawerContainer() {
        UiObject2 container = null;
        if (mDevice.getDisplaySizeDp().x < TAB_MIN_WIDTH) {
            container = mDevice.wait(
                    Until.findObject(By.res(PACKAGE_NAME, "content_pane")), SHORT_TIMEOUT);
        } else {
            container = mDevice.wait(
                    Until.findObject(By.res(RES_PACKAGE_NAME, "list")), SHORT_TIMEOUT);
        }
        return container;
    }
}
