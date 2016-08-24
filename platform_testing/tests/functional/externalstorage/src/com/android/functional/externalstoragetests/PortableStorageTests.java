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

package com.android.functional.externalstoragetests;

import android.app.UiAutomation;
import android.content.Context;
import android.content.Intent;
import android.provider.Settings;
import android.support.test.uiautomator.By;
import android.support.test.uiautomator.UiDevice;
import android.support.test.uiautomator.UiObject2;
import android.support.test.uiautomator.Until;
import android.test.InstrumentationTestCase;
import android.test.suitebuilder.annotation.LargeTest;
import java.util.regex.Matcher;
import java.util.regex.Pattern;
import junit.framework.Assert;

public class PortableStorageTests extends InstrumentationTestCase {
    private UiDevice mDevice = null;
    private Context mContext = null;
    private UiAutomation mUiAutomation = null;
    private ExternalStorageHelper storageHelper;

    @Override
    protected void setUp() throws Exception {
        super.setUp();
        mDevice = UiDevice.getInstance(getInstrumentation());
        mContext = getInstrumentation().getContext();
        mUiAutomation = getInstrumentation().getUiAutomation();
        storageHelper = ExternalStorageHelper.getInstance(mDevice, mContext, mUiAutomation,
                getInstrumentation());
        mDevice.setOrientationNatural();
    }

    /**
     * Test to ensure sd card can be adopted as portable storage
     */
    @LargeTest
    public void testAdoptAsPortableViaUI() throws InterruptedException {
        // ensure notification
        storageHelper.executeShellCommand(String.format(
                "sm partition %s %s", storageHelper.getAdoptionDisk(), "public"));
        Thread.sleep(storageHelper.TIMEOUT);
        storageHelper.setupAsPortableUiFlow();
        storageHelper.executeShellCommand(String.format("sm forget all"));
        Thread.sleep(storageHelper.TIMEOUT);
    }

    /**
     * tests to ensure copy option is visible for items on portable storage
     */
    @LargeTest
    public void testCopyFromPortable() throws InterruptedException {
        ensureHasPortable();
        storageHelper.createFiles(2,
                String.format("/storage/%s", storageHelper.getAdoptionVolumeUuid("public")));
        storageHelper.openSDCard();
        mDevice.wait(Until.findObject(By.res("android:id/title").text("Test_0")),
                storageHelper.TIMEOUT).click(storageHelper.TIMEOUT);
        mDevice.wait(Until.findObject(By.desc(Pattern.compile("More options",
                Pattern.CASE_INSENSITIVE))), storageHelper.TIMEOUT).click();
        assertNotNull(mDevice.wait(Until.findObject(By.res("android:id/title").text("Copy to…")),
                2 * storageHelper.TIMEOUT));
        mDevice.wait(Until.findObject(By.res("android:id/title").text("Copy to…")),
                storageHelper.TIMEOUT).clickAndWait(Until.newWindow(), storageHelper.TIMEOUT);
        mDevice.pressBack();
    }

    /**
     * tests to ensure that resources on portable storage can be deleted via UI
     */
    @LargeTest
    public void testDeleteFromPortable() throws InterruptedException {
        ensureHasPortable();
        storageHelper.createFiles(2,
                String.format("/storage/%s", storageHelper.getAdoptionVolumeUuid("public")));
        storageHelper.openSDCard();
        mDevice.wait(Until.findObject(By.res("android:id/title").text("Test_0")),
                storageHelper.TIMEOUT).click(storageHelper.TIMEOUT);
        mDevice.wait(Until.findObject(By.res("com.android.documentsui:id/menu_sort")),
                storageHelper.TIMEOUT).clickAndWait(Until.newWindow(), storageHelper.TIMEOUT);
        assertNull(mDevice.wait(Until.findObject(By.res("android:id/title").text("Test_0")),
                2 * storageHelper.TIMEOUT));
    }

    /**
     * tests to ensure that external storage is explorable via UI
     */
    @LargeTest
    public void testExplorePortable() throws InterruptedException {
        ensureHasPortable();
        // Create 2 random files on SDCard
        storageHelper.createFiles(2,
                String.format("/storage/%s", storageHelper.getAdoptionVolumeUuid("public")));
        Intent intent = new Intent(Settings.ACTION_INTERNAL_STORAGE_SETTINGS);
        intent.setFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
        mContext.startActivity(intent);
        Thread.sleep(storageHelper.TIMEOUT * 2);
        mDevice.wait(Until.findObject(By.textContains("SD card")), storageHelper.TIMEOUT)
                .clickAndWait(Until.newWindow(), storageHelper.TIMEOUT);
        for (int i = 0; i < 2; ++i) {
            Assert.assertTrue(mDevice.wait(Until.hasObject(By.res("android:id/title")
                    .text(String.format("Test_%d", i))), storageHelper.TIMEOUT));
        }
    }

    /**
     * tests to ensure that resources on portable storage can be shared via UI
     */
    @LargeTest
    public void testShareableFromPortable() throws InterruptedException {
        ensureHasPortable();
        storageHelper.createFiles(2,
                String.format("/storage/%s", storageHelper.getAdoptionVolumeUuid("public")));
        storageHelper.openSDCard();
        mDevice.wait(Until.findObject(By.res("android:id/title").text("Test_0")),
                storageHelper.TIMEOUT).click(storageHelper.TIMEOUT);
        mDevice.wait(Until.findObject(By.res("com.android.documentsui:id/menu_list")),
                storageHelper.TIMEOUT).click();
        assertNotNull(mDevice.wait(Until.findObject(By.res("android:id/resolver_list")),
                storageHelper.TIMEOUT));
        // click and ensure intent is sent to share? or actual share?
        mDevice.pressBack();
    }

    /**
     * tests to ensure that portable overflow menu contain all setting options
     */
    @LargeTest
    public void testPortableOverflowSettings() throws InterruptedException {
        ensureHasPortable();
        storageHelper.createFiles(2,
                String.format("/storage/%s", storageHelper.getAdoptionVolumeUuid("public")));
        storageHelper.openSDCard();
        mDevice.wait(Until.findObject(By.res("android:id/title").text("Test_0")),
                storageHelper.TIMEOUT).click(storageHelper.TIMEOUT);
        assertTrue(mDevice.wait(Until.hasObject(By.res(storageHelper.DOCUMENTS_PKG, "menu_search")),
                storageHelper.TIMEOUT));
        assertTrue(mDevice.wait(Until.hasObject(By.res(storageHelper.DOCUMENTS_PKG, "menu_sort")),
                storageHelper.TIMEOUT));
        assertTrue(mDevice.wait(Until.hasObject(By.text("1 selected")), storageHelper.TIMEOUT));
    }

    /**
     * tests to ensure that portable storage has setting options format, format as internal, eject
     */
    @LargeTest
    public void testPortableSettings() throws InterruptedException {
        ensureHasPortable();
        storageHelper.openSDCard();
        Pattern pattern = Pattern.compile("More options", Pattern.CASE_INSENSITIVE);
        mDevice.wait(Until.findObject(By.desc(pattern)), storageHelper.TIMEOUT).click();
        pattern = Pattern.compile("Storage settings", Pattern.CASE_INSENSITIVE);
        mDevice.wait(Until.findObject(By.text(pattern)), storageHelper.TIMEOUT)
                .clickAndWait(
                        Until.newWindow(), storageHelper.TIMEOUT);
        pattern = Pattern.compile("Eject", Pattern.CASE_INSENSITIVE);
        assertTrue(mDevice.wait(Until.hasObject(By.text(pattern)), storageHelper.TIMEOUT));
        pattern = Pattern.compile("Format", Pattern.CASE_INSENSITIVE);
        assertTrue(mDevice.wait(Until.hasObject(By.text(pattern)), storageHelper.TIMEOUT));
        pattern = Pattern.compile("Format as internal", Pattern.CASE_INSENSITIVE);
        assertTrue(mDevice.wait(Until.hasObject(By.text(pattern)),
                storageHelper.TIMEOUT));
    }

    /**
     * tests to ensure that portable storage can be ejected from settings
     */
    @LargeTest
    public void testEjectPortable() throws InterruptedException {
        ensureHasPortable();
        storageHelper.openStorageSettings();
        mDevice.wait(Until.findObject(By.res(storageHelper.SETTINGS_PKG, "unmount")),
                storageHelper.TIMEOUT).click();
        assertTrue(mDevice.wait(Until.hasObject(By.res("android:id/summary").text("Ejected")),
                4 * storageHelper.TIMEOUT));
        mDevice.wait(Until.findObject(By.textContains("SD card")), 2 * storageHelper.TIMEOUT)
                .click();
        Pattern pattern = Pattern.compile("Mount", Pattern.CASE_INSENSITIVE);
        mDevice.wait(Until.findObject(By.res("android:id/button1").text(pattern)),
                2 * storageHelper.TIMEOUT).clickAndWait(Until.newWindow(), storageHelper.TIMEOUT);
        ;
    }

    /**
     * tests to ensure that portable storage can be erased and formated from settings
     */
    @LargeTest
    public void testFormatPortable() throws InterruptedException {
        ensureHasPortable();
        storageHelper.openSDCard();
        Pattern pattern = Pattern.compile("More options", Pattern.CASE_INSENSITIVE);
        mDevice.wait(Until.findObject(By.desc(pattern)), storageHelper.TIMEOUT).click();
        pattern = Pattern.compile("Storage settings", Pattern.CASE_INSENSITIVE);
        mDevice.wait(Until.findObject(By.text(pattern)), storageHelper.TIMEOUT)
                .clickAndWait(
                        Until.newWindow(), storageHelper.TIMEOUT);
        UiObject2 format = mDevice.wait(Until.findObject(By.text("Format")), storageHelper.TIMEOUT);
        format.clickAndWait(Until.newWindow(), storageHelper.TIMEOUT);
        pattern = Pattern.compile("Erase & Format", Pattern.CASE_INSENSITIVE);
        mDevice.wait(Until.findObject(By.text(pattern)), storageHelper.TIMEOUT).click();
        pattern = Pattern.compile("Done", Pattern.CASE_INSENSITIVE);
        mDevice.wait(Until.findObject(By.text(pattern)), 20 * storageHelper.TIMEOUT).click();
    }

    /**
     * tests to ensure that portable storage can be erased and formated as internal from settings
     */
    @LargeTest
    public void testFormatPortableAsAdoptable() throws InterruptedException {
        try {
            ensureHasPortable();
            storageHelper.openSDCard();
            Pattern pattern = Pattern.compile("More options", Pattern.CASE_INSENSITIVE);
            mDevice.wait(Until.findObject(By.desc(pattern)), storageHelper.TIMEOUT).click();
            pattern = Pattern.compile("Storage settings", Pattern.CASE_INSENSITIVE);
            mDevice.wait(Until.findObject(By.text(pattern)), storageHelper.TIMEOUT)
                    .clickAndWait(
                            Until.newWindow(), storageHelper.TIMEOUT);
            pattern = Pattern.compile("Format", Pattern.CASE_INSENSITIVE);
            assertTrue(mDevice.wait(Until.hasObject(By.text(pattern)), storageHelper.TIMEOUT));
            pattern = Pattern.compile("Format as internal", Pattern.CASE_INSENSITIVE);
            assertTrue(mDevice.wait(Until.hasObject(By.text(pattern)),
                    storageHelper.TIMEOUT));
            // Next flow is same as adoption, so no need to test
        } finally {
            storageHelper.partitionDisk("public");
        }
    }

    private void ensureHasPortable() throws InterruptedException {
        storageHelper.partitionDisk("public");
        storageHelper.settingsUiCleanUp();
    }

    @Override
    protected void tearDown() throws Exception {
        mDevice.unfreezeRotation();
        mDevice.pressBack();
        mDevice.pressHome();
        super.tearDown();
    }
}
