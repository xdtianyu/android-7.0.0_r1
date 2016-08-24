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
import android.support.test.uiautomator.Direction;
import android.support.test.uiautomator.UiDevice;
import android.support.test.uiautomator.UiObject2;
import android.support.test.uiautomator.UiObjectNotFoundException;
import android.support.test.uiautomator.Until;
import android.widget.Button;
import android.widget.ProgressBar;
import junit.framework.Assert;
import android.support.test.timeresulthelper.TimeResultLogger;

/**
 * Jank test for Books app recommendation page fling
 */

public class BooksJankTests extends JankTestBase {
    private static final int LONG_TIMEOUT = 1000;
    private static final int SHORT_TIMEOUT = 1000;
    private static final int INNER_LOOP = 5;
    private static final int EXPECTED_FRAMES = 100;
    private static final String PACKAGE_NAME = "com.google.android.apps.books";
    private UiDevice mDevice;
    private static final File TIMESTAMP_FILE = new File(Environment.getExternalStorageDirectory()
            .getAbsolutePath(), "autotester.log");
    private static final File RESULTS_FILE = new File(Environment.getExternalStorageDirectory()
            .getAbsolutePath(), "results.log");

    @Override
    public void setUp() throws Exception {
        super.setUp();
        mDevice = UiDevice.getInstance(getInstrumentation());
        try {
            mDevice.setOrientationNatural();
        } catch (RemoteException e) {
            throw new RuntimeException("failed to freeze device orientaion", e);
        }
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

    public void launchBooks () throws UiObjectNotFoundException, IOException {
        launchApp(PACKAGE_NAME);
        dismissClings();
        openMyLibrary();
        Assert.assertTrue("Books haven't loaded yet", getNumberOfVisibleBooks() > 3);
        TimeResultLogger.writeTimeStampLogStart(String.format("%s-%s",
                getClass().getSimpleName(), getName()), TIMESTAMP_FILE);
    }

    public void afterTestBooksRecommendationPageFling(Bundle metrics) throws IOException {
        TimeResultLogger.writeTimeStampLogEnd(String.format("%s-%s",
                getClass().getSimpleName(), getName()), TIMESTAMP_FILE);
        TimeResultLogger.writeResultToFile(String.format("%s-%s",
                getClass().getSimpleName(), getName()), RESULTS_FILE, metrics);
        super.afterTest(metrics);
    }

    // Measures jank while fling books mylibrary
    @JankTest(beforeTest="launchBooks", expectedFrames=EXPECTED_FRAMES,
            afterTest="afterTestBooksRecommendationPageFling")
    @GfxMonitor(processName=PACKAGE_NAME)
    // Books is not a system app anymore
    public void doNotRun_BooksRecommendationPageFling() {
        UiObject2 container = mDevice.wait(Until.findObject(
                By.res(PACKAGE_NAME, "content_container")), LONG_TIMEOUT);
        for (int i = 0; i < INNER_LOOP; i++) {
          container.scroll(Direction.DOWN, 1.0f);
          SystemClock.sleep(SHORT_TIMEOUT);
          container.scroll(Direction.UP, 1.0f);
        }
    }

    // All helper methods are at bottom
    // with the assumptions is that these will have their own library
    private void dismissClings() {
        // Dismiss confidentiality warning. It's okay to timeout here.
        UiObject2 warning = mDevice.wait(
                Until.findObject(By.clazz(".Button").text("OK")), LONG_TIMEOUT);
        if (warning != null) {
            warning. click();
        }
        // Close the drawer.
        UiObject2 close = mDevice.wait(
                Until.findObject(By.desc("Hide navigation drawer")), LONG_TIMEOUT);
        if (close != null) {
            close.click();
        }
        // Turn sync off
        UiObject2 syncoff = mDevice.wait(Until.findObject(
                By.clazz(Button.class).text("Keep sync off")), LONG_TIMEOUT);
        if (syncoff != null) {
            syncoff.click();
        }
    }

    public void openNavigationDrawer() {
      if (!mDevice.hasObject(By.res(PACKAGE_NAME, "play_drawer_container"))) {
          mDevice.findObject(By.desc("Show navigation drawer")).click();
          Assert.assertTrue("Failed to open navigation drawer", mDevice.wait(
              Until.hasObject(By.res(PACKAGE_NAME, "play_drawer_list")), LONG_TIMEOUT));

          // Extra sleep to wait for the drawer to finish sliding in
          SystemClock.sleep(500);
      }
  }

    public void openMyLibrary() {
        openNavigationDrawer();
        UiObject2 library = mDevice.wait(
            Until.findObject(By.text("My Library").res("")), LONG_TIMEOUT);
        Assert.assertNotNull("Could not find 'My Library' button", library);
        library.click();
    }

    public int getNumberOfVisibleBooks() {
      UiObject2 list = mDevice.wait(
              Until.findObject(By.res(PACKAGE_NAME, "cards_grid")), LONG_TIMEOUT);
      Assert.assertNotNull("Failed to locate 'cards_grid'", list);
      return list.getChildCount();
    }
}
