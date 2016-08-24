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
import android.content.pm.PackageManager.NameNotFoundException;
import android.provider.Settings;
import android.support.test.uiautomator.By;
import android.support.test.uiautomator.UiDevice;
import android.support.test.uiautomator.UiObject2;
import android.support.test.uiautomator.Until;
import android.test.InstrumentationTestCase;
import android.test.suitebuilder.annotation.LargeTest;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

public class AdoptableStorageTests extends InstrumentationTestCase {
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
     * Tests external storage adoption and move data later flow via UI
     */
    @LargeTest
    public void testAdoptAsAdoptableMoveDataLaterUIFlow() throws InterruptedException {
        // ensure there is a storage to be adopted
        storageHelper.partitionDisk("public");
        initiateAdoption();
        Pattern pattern = Pattern.compile("Move later", Pattern.CASE_INSENSITIVE);
        mDevice.wait(Until.findObject(By.text(pattern)), storageHelper.TIMEOUT).click();
        pattern = Pattern.compile("Next", Pattern.CASE_INSENSITIVE);
        mDevice.wait(Until.findObject(By.text(pattern)), storageHelper.TIMEOUT).clickAndWait(
                Until.newWindow(), storageHelper.TIMEOUT);
        pattern = Pattern.compile("Done", Pattern.CASE_INSENSITIVE);
        mDevice.wait(Until.findObject(By.text(pattern)), storageHelper.TIMEOUT).clickAndWait(
                Until.newWindow(), storageHelper.TIMEOUT);
        assertNotNull(storageHelper.getAdoptionVolumeId("private"));
        // ensure data dirs have not moved
        Intent intent = new Intent(Settings.ACTION_INTERNAL_STORAGE_SETTINGS);
        intent.setFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
        mContext.startActivity(intent);
        mDevice.wait(Until.findObject(By.textContains("SD card")), 2 * storageHelper.TIMEOUT)
                .clickAndWait(Until.newWindow(), storageHelper.TIMEOUT);
        assertTrue(mDevice.wait(Until.hasObject(By.res("android:id/title").text("Apps")),
                storageHelper.TIMEOUT));
    }

    // Adoptable storage settings
    /**
     * tests to ensure that adoptable storage has setting options rename, eject, format as portable
     */
    @LargeTest
    public void testAdoptableOverflowSettings() throws InterruptedException {
        storageHelper.partitionDisk("private");
        storageHelper.openSDCard();
        Pattern pattern = Pattern.compile("More options", Pattern.CASE_INSENSITIVE);
        UiObject2 moreOptions = mDevice.wait(Until.findObject(By.desc(pattern)),
                storageHelper.TIMEOUT);
        assertNotNull("Over flow menu options shouldn't be null", moreOptions);
        moreOptions.click();
        pattern = Pattern.compile("Rename", Pattern.CASE_INSENSITIVE);
        assertTrue(mDevice.wait(Until.hasObject(By.text(pattern)), storageHelper.TIMEOUT));
        pattern = Pattern.compile("Eject", Pattern.CASE_INSENSITIVE);
        assertTrue(mDevice.wait(Until.hasObject(By.text(pattern)), storageHelper.TIMEOUT));
        pattern = Pattern.compile("Format as portable", Pattern.CASE_INSENSITIVE);
        assertTrue(mDevice.wait(Until.hasObject(By.text(pattern)), storageHelper.TIMEOUT));
    }

    /**
     * tests to ensure that adoptable storage can be renamed
     */
    @LargeTest
    public void testRenameAdoptable() throws InterruptedException {
        storageHelper.partitionDisk("private");
        storageHelper.openSDCard();
        Pattern pattern = Pattern.compile("More options", Pattern.CASE_INSENSITIVE);
        mDevice.wait(Until.findObject(By.desc(pattern)), storageHelper.TIMEOUT).click();
        pattern = Pattern.compile("Rename", Pattern.CASE_INSENSITIVE);
        mDevice.wait(Until.findObject(By.text(pattern)), storageHelper.TIMEOUT).click();
        mDevice.wait(Until.findObject(By.res(storageHelper.SETTINGS_PKG, "edittext")),
                storageHelper.TIMEOUT).setText("My SD card");
        pattern = Pattern.compile("Save", Pattern.CASE_INSENSITIVE);
        mDevice.wait(Until.findObject(By.text(pattern)), storageHelper.TIMEOUT).clickAndWait(
                Until.newWindow(), storageHelper.TIMEOUT);
        assertTrue(mDevice.wait(Until.hasObject(By.text("My SD card")), storageHelper.TIMEOUT));
    }

    /**
     * tests to ensure that adoptable storage can be ejected
     */
    @LargeTest
    public void testEjectAdoptable() throws InterruptedException {
        storageHelper.partitionDisk("private");
        storageHelper.openSDCard();
        Pattern pattern = Pattern.compile("More options", Pattern.CASE_INSENSITIVE);
        mDevice.wait(Until.findObject(By.desc(pattern)), storageHelper.TIMEOUT).click();
        pattern = Pattern.compile("Eject", Pattern.CASE_INSENSITIVE);
        mDevice.wait(Until.findObject(By.text(pattern)), storageHelper.TIMEOUT).click();
        assertTrue(mDevice.wait(Until.hasObject(By.res(storageHelper.SETTINGS_PKG, "body")),
                storageHelper.TIMEOUT));
        mDevice.wait(Until.findObject(By.res(storageHelper.SETTINGS_PKG, "confirm").text(pattern)),
                storageHelper.TIMEOUT).clickAndWait(Until.newWindow(), storageHelper.TIMEOUT);
        pattern = Pattern.compile("Ejected", Pattern.CASE_INSENSITIVE);
        assertTrue(mDevice.wait(Until.hasObject(By.res("android:id/summary").text(pattern)),
                storageHelper.TIMEOUT));
        mDevice.wait(Until.findObject(By.textContains("SD card")), storageHelper.TIMEOUT).click();
        pattern = Pattern.compile("Mount", Pattern.CASE_INSENSITIVE);
        mDevice.wait(Until.findObject(By.res("android:id/button1").text(pattern)),
                2 * storageHelper.TIMEOUT).clickAndWait(Until.newWindow(), storageHelper.TIMEOUT);
    }

    /**
     * tests to ensure that adoptable storage can be formated back as portable from settings
     */
    @LargeTest
    public void testFormatAdoptableAsPortable() throws InterruptedException {
        storageHelper.partitionDisk("private");
        storageHelper.openSDCard();
        Pattern pattern = Pattern.compile("More options", Pattern.CASE_INSENSITIVE);
        mDevice.wait(Until.findObject(By.desc(pattern)), storageHelper.TIMEOUT).click();
        pattern = Pattern.compile("Format as portable", Pattern.CASE_INSENSITIVE);
        mDevice.wait(Until.findObject(By.text(pattern)), storageHelper.TIMEOUT)
                .clickAndWait(Until.newWindow(), storageHelper.TIMEOUT);
        mDevice.wait(Until.hasObject(
                By.textContains("After formatting, you can use this")), storageHelper.TIMEOUT);
        mDevice.wait(Until.findObject(By.text("FORMAT")), 2 * storageHelper.TIMEOUT)
                .clickAndWait(Until.newWindow(), storageHelper.TIMEOUT);
        pattern = Pattern.compile("Done", Pattern.CASE_INSENSITIVE);
        mDevice.wait(Until.findObject(By.text(pattern)), 5 * storageHelper.TIMEOUT)
                .clickAndWait(Until.newWindow(), storageHelper.TIMEOUT);
    }

    public void initiateAdoption() throws InterruptedException {
        storageHelper.openSdCardSetUpNotification().clickAndWait(Until.newWindow(),
                storageHelper.TIMEOUT);
        UiObject2 adoptFlowUi = mDevice.wait(Until.findObject(
                By.res(storageHelper.SETTINGS_PKG, "storage_wizard_init_internal_title")),
                storageHelper.TIMEOUT);
        adoptFlowUi.click();
        Pattern pattern = Pattern.compile("NEXT", Pattern.CASE_INSENSITIVE);
        adoptFlowUi = mDevice.wait(Until.findObject(
                By.res(storageHelper.SETTINGS_PKG, "suw_navbar_next").text(pattern)),
                storageHelper.TIMEOUT);
        adoptFlowUi.clickAndWait(Until.newWindow(), storageHelper.TIMEOUT);
        pattern = Pattern.compile("ERASE & FORMAT", Pattern.CASE_INSENSITIVE);
        adoptFlowUi = mDevice.wait(Until.findObject(By.text(pattern)),
                storageHelper.TIMEOUT);
        adoptFlowUi.clickAndWait(Until.newWindow(), storageHelper.TIMEOUT);
        adoptFlowUi = mDevice.wait(
                Until.findObject(By.res(storageHelper.SETTINGS_PKG, "storage_wizard_progress")),
                storageHelper.TIMEOUT);
        assertNotNull(adoptFlowUi);
        if ((mDevice.wait(Until.findObject(By.res("android:id/message")),
                60 * storageHelper.TIMEOUT)) != null) {
            mDevice.wait(Until.findObject(By.text("OK")), storageHelper.TIMEOUT).clickAndWait(
                    Until.newWindow(), storageHelper.TIMEOUT);
        }
    }

    /**
     * System apps can't be moved to adopted storage
     */
    @LargeTest
    public void testTransferSystemApp() throws InterruptedException, NameNotFoundException {
        storageHelper.partitionDisk("private");
        storageHelper.executeShellCommand("pm move-package " + storageHelper.SETTINGS_PKG + " "
                + storageHelper.getAdoptionVolumeId("private"));
        assertTrue(storageHelper.getInstalledLocation(storageHelper.SETTINGS_PKG)
                .startsWith("/data/user_de/0"));
    }

    @Override
    protected void tearDown() throws Exception {
        // Convert sdcard to public
        storageHelper.executeShellCommand(String.format("sm partition %s %s",
                storageHelper.getAdoptionDisk(), "public"));
        Thread.sleep(storageHelper.TIMEOUT);
        storageHelper.executeShellCommand("sm forget all");
        Thread.sleep(storageHelper.TIMEOUT);
        // move back to homescreen
        mDevice.unfreezeRotation();
        mDevice.pressBack();
        mDevice.pressHome();
        super.tearDown();
    }
}
