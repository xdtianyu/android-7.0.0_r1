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
import android.database.Cursor;
import android.os.Bundle;
import android.os.Environment;
import android.os.RemoteException;
import android.os.SystemClock;
import android.provider.ContactsContract;
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
import android.widget.TextView;
import junit.framework.Assert;
import android.support.test.timeresulthelper.TimeResultLogger;

/**
 * Jank test for Contacts
 * open contact list and fling
 */

public class ContactsJankTests extends JankTestBase {
    private static final int TIMEOUT = 5000;
    private static final int SHORT_TIMEOUT = 1000;
    private static final int INNER_LOOP = 5;
    private static final int EXPECTED_FRAMES = 100;
    private static final int MIN_CONTACT_COUNT = 75;
    private static final String PACKAGE_NAME = "com.google.android.contacts";
    private static final String RES_PACKAGE_NAME = "com.android.contacts";
    private static final String PM_PACKAGE_NAME = "com.android.packageinstaller";
    private UiDevice mDevice;
    private static final File TIMESTAMP_FILE = new File(Environment.getExternalStorageDirectory()
            .getAbsolutePath(), "autotester.log");
    private static final File RESULTS_FILE = new File(Environment.getExternalStorageDirectory()
            .getAbsolutePath(), "results.log");

    @Override
    public void setUp() throws Exception {
        super.setUp();
        mDevice = UiDevice.getInstance(getInstrumentation());
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

    public void launchContacts () throws UiObjectNotFoundException, IOException {
        launchApp(PACKAGE_NAME);
        mDevice.waitForIdle();
        // To infer that test is ready to be executed
        Assert.assertNotNull("'All Contacts' not selected",
                mDevice.wait(Until.findObject(By.clazz(TextView.class).selected(true)), TIMEOUT));
        Assert.assertNotNull("Contacts list is not populated", mDevice.wait(Until.findObject(
                By.res(RES_PACKAGE_NAME, "pinned_header_list_layout")), TIMEOUT));
        Cursor cursor =  getInstrumentation().getContext().getContentResolver().query(
                ContactsContract.CommonDataKinds.Phone.CONTENT_URI, null, null, null, null);
        Assert.assertTrue("There are not enough contacts", cursor.getCount() > MIN_CONTACT_COUNT);
        TimeResultLogger.writeTimeStampLogStart(String.format("%s-%s",
                getClass().getSimpleName(), getName()), TIMESTAMP_FILE);
    }

    public void afterTestAllContactsFling(Bundle metrics) throws IOException {
        TimeResultLogger.writeTimeStampLogEnd(String.format("%s-%s",
                getClass().getSimpleName(), getName()), TIMESTAMP_FILE);
        TimeResultLogger.writeResultToFile(String.format("%s-%s",
                getClass().getSimpleName(), getName()), RESULTS_FILE, metrics);
        super.afterTest(metrics);
    }

    // Measures jank while flinging contacts list
    @JankTest(beforeTest="launchContacts", expectedFrames=EXPECTED_FRAMES,
            afterTest="afterTestAllContactsFling")
    @GfxMonitor(processName=PACKAGE_NAME)
    public void testAllContactsFling() {
        UiObject2 contactList = null;
        for (int i = 0; i < INNER_LOOP; i++) {
            contactList = mDevice.wait(Until.findObject(
                    By.res(RES_PACKAGE_NAME, "pinned_header_list_layout")), TIMEOUT);
            contactList.fling(Direction.DOWN);
            SystemClock.sleep(100);
            contactList.fling(Direction.UP);
            SystemClock.sleep(100);
        }
    }
}
