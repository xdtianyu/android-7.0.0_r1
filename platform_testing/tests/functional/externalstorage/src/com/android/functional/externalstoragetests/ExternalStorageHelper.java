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

import android.app.Instrumentation;
import android.app.UiAutomation;
import android.content.Context;
import android.content.Intent;
import android.content.pm.PackageInfo;
import android.content.pm.PackageManager;
import android.content.pm.PackageManager.NameNotFoundException;
import android.os.ParcelFileDescriptor;
import android.os.StatFs;
import android.os.SystemClock;
import android.provider.Settings;
import android.support.test.uiautomator.By;
import android.support.test.uiautomator.UiDevice;
import android.support.test.uiautomator.UiObject2;
import android.support.test.uiautomator.Until;
import android.util.Log;

import android.platform.test.helpers.PlayStoreHelperImpl;

import junit.framework.Assert;

import java.io.BufferedReader;
import java.io.File;
import java.io.FileInputStream;
import java.io.IOException;
import java.io.InputStreamReader;
import java.util.HashMap;
import java.util.Hashtable;
import java.util.List;
import java.util.Map;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

public class ExternalStorageHelper {
    public static final String TEST_TAG = "StorageFunctionalTest";
    public final String SETTINGS_PKG = "com.android.settings";
    public final String PLAYSTORE_PKG = "com.android.vending";
    public final String DOCUMENTS_PKG = "com.android.documentsui";
    public static final Map<String, String> APPLIST = new HashMap<String, String>();
    static {
        APPLIST.put("w35location1", "com.test.w35location1");
        APPLIST.put("w35location2", "com.test.w35location2");
        APPLIST.put("w35location3", "com.test.w35location3");
    }
    public final int TIMEOUT = 2000;
    public static ExternalStorageHelper mInstance = null;
    public UiDevice mDevice;
    public Context mContext;
    public static UiAutomation mUiAutomation;
    public static Instrumentation mInstrumentation;
    public static Hashtable<String, List<String>> mPermissionGroupInfo = null;

    public ExternalStorageHelper(UiDevice device, Context context, UiAutomation uiAutomation,
            Instrumentation instrumentation) {
        mDevice = device;
        mContext = context;
        mUiAutomation = uiAutomation;
        mInstrumentation = instrumentation;
    }

    public static ExternalStorageHelper getInstance(UiDevice device, Context context,
            UiAutomation uiAutomation, Instrumentation instrumentation) {
        if (mInstance == null) {
            mInstance = new ExternalStorageHelper(device, context, uiAutomation, instrumentation);
        }
        return mInstance;
    }

    /**
     * Opens SD card setup notification from homescreen
     */
    public UiObject2 openSdCardSetUpNotification() throws InterruptedException {
        boolean success = mDevice.openNotification();
        Thread.sleep(TIMEOUT);
        UiObject2 sdCardDetected = mDevice
                .wait(Until.findObject(By.textContains("SD card detected")), TIMEOUT);
        Assert.assertNotNull(sdCardDetected);
        return sdCardDetected;
    }

    /**
     * Open Storage settings, then SD Card
     */
    public void openStorageSettings() throws InterruptedException {
        Intent intent = new Intent(Settings.ACTION_INTERNAL_STORAGE_SETTINGS);
        intent.setFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
        mContext.startActivity(intent);
        Thread.sleep(TIMEOUT * 2);
    }

    /**
     * Open Storage settings, then SD Card
     */
    public void openSDCard() throws InterruptedException {
        openStorageSettings();
        mDevice.wait(Until.findObject(By.textContains("SD card")), TIMEOUT)
                .clickAndWait(Until.newWindow(), TIMEOUT);
    }

    public String executeShellCommand(String command) {
        ParcelFileDescriptor pfd = mUiAutomation.executeShellCommand(command);
        try (BufferedReader reader = new BufferedReader(
                new InputStreamReader(new FileInputStream(pfd.getFileDescriptor())))) {
            String str = reader.readLine();
            Log.d(TEST_TAG, String.format("Executing command: %s", command));
            return str;
        } catch (IOException e) {
            Log.e(TEST_TAG, e.getMessage());
        }

        return null;
    }

    /**
     * Create # of files in a given dir
     */
    public void createFiles(int numberOfFiles, String dir) {
        for (int i = 0; i < numberOfFiles; ++i) {
            if (!new File(String.format("%s/Test_%d", dir, i)).exists()) {
                fillInStorage(dir, String.format("Test_%d", i), 1);
            }
        }
    }

    public void fillInStorage(String location, String filename, int sizeInKb) {
        executeShellCommand(String.format("dd if=/dev/zero of=%s/%s bs=1024 count=%d",
                location, filename, sizeInKb));
    }

    public int getFreeSpaceSize(File path) {
        StatFs stat = new StatFs(path.getPath());
        long blockSize = stat.getBlockSize();
        long availableBlocks = stat.getAvailableBlocks();
        return (int) ((availableBlocks * blockSize) / (1024 * 1024));
    }

    public boolean hasAdoptable() {
        return Boolean.parseBoolean(executeShellCommand("sm has-adoptable").trim());
    }

    public String getAdoptionDisk() throws InterruptedException {
        int counter = 10;
        String disks = null;
        while (((disks == null || disks.length() == 0)) && counter > 0) {
            disks = executeShellCommand("sm list-disks adoptable");
            Thread.sleep(TIMEOUT);
            --counter;
        }
        if (counter == 0) {
            throw new AssertionError("Devices must have adoptable media inserted");
        }
        return disks.split("\n")[0].trim();
    }

    public Boolean hasPublicVolume() {
        return (null != executeShellCommand("sm list-volumes public"));
    }

    public String getAdoptionVolumeId(String volType) throws InterruptedException {
        return getAdoptionVolumeInfo(volType).volId;
    }

    public String getAdoptionVolumeUuid(String volType) throws InterruptedException {
        return getAdoptionVolumeInfo(volType).uuid;
    }

    public LocalVolumeInfo getAdoptionVolumeInfo(String volType) throws InterruptedException {
        String[] lines = null;
        int attempt = 0;
        while (attempt++ < 5) {
            if (null != (lines = executeShellCommand("sm list-volumes " + volType).split("\n"))) {
                for (String line : lines) {
                    final LocalVolumeInfo info = new LocalVolumeInfo(line.trim());
                    if (info.volId.startsWith(volType) && "mounted".equals(info.state)) {
                        return info;
                    }
                }
                Thread.sleep(TIMEOUT);
            }
        }
        return null;
    }

    public void partitionDisk(String type) throws InterruptedException {
        if (type.equals("private")) {
            executeShellCommand(String.format("sm partition %s %s", getAdoptionDisk(), type));
            Thread.sleep(2 * TIMEOUT);
        } else if (!hasPublicVolume() && type.equals("public")) {
            executeShellCommand("sm forget all");
            executeShellCommand(String.format("sm partition %s %s", getAdoptionDisk(), type));
            Thread.sleep(2 * TIMEOUT);
            setupAsPortableUiFlow();
        }
    }

    public void setupAsPortableUiFlow() throws InterruptedException {
        openSdCardSetUpNotification();
        Thread.sleep(TIMEOUT);
        Pattern pattern = Pattern.compile("Set up", Pattern.CASE_INSENSITIVE);
        UiObject2 adoptFlowUi = mDevice.wait(Until.findObject(By.desc(pattern)), TIMEOUT);
        adoptFlowUi.clickAndWait(Until.newWindow(), TIMEOUT);
        adoptFlowUi = mDevice.wait(Until.findObject(
                By.res(SETTINGS_PKG, "storage_wizard_init_external_title")),
                TIMEOUT);
        adoptFlowUi.click();
        pattern = Pattern.compile("Next", Pattern.CASE_INSENSITIVE);
        adoptFlowUi = mDevice.wait(Until.findObject(By.text(pattern)),
                TIMEOUT);
        adoptFlowUi.clickAndWait(Until.newWindow(), TIMEOUT);
        pattern = Pattern.compile("Done", Pattern.CASE_INSENSITIVE);
        mDevice.wait(Until.findObject(By.text(pattern)), TIMEOUT).clickAndWait(
                Until.newWindow(), TIMEOUT);
        hasPublicVolume();

    }

    public void installFromPlayStore(String appName) {
        PlayStoreHelperImpl mHelper = new PlayStoreHelperImpl(mInstrumentation);
        mHelper.open();
        mHelper.doSearch(appName);
        mHelper.selectFirstResult();
        mDevice.wait(Until.findObject(By.res(PLAYSTORE_PKG, "buy_button").text("INSTALL")),
                TIMEOUT).clickAndWait(Until.newWindow(), 2 * TIMEOUT);
        SystemClock.sleep(2 * TIMEOUT);
        mDevice.wait(Until.findObject(By.res(PLAYSTORE_PKG, "launch_button").text("OPEN")),
                5 * TIMEOUT);
    }

    public PackageInfo getPackageInfo(String packageName) throws NameNotFoundException {
        return mContext.getPackageManager().getPackageInfo(packageName, 0);
    }

    public Boolean doesPackageExist(String packageName) throws NameNotFoundException {
        try {
            mContext.getPackageManager().getPackageInfo(packageName, 0);
        } catch (NameNotFoundException nex) {
            throw nex;
        }

        return Boolean.TRUE;
    }

    public String getInstalledLocation(String packageName) throws NameNotFoundException {
        Assert.assertTrue(String.format("%s doesn't exist!", packageName),
                doesPackageExist(packageName));
        return getPackageInfo(packageName).applicationInfo.dataDir;
    }

    public void settingsUiCleanUp() {
        executeShellCommand("pm clear " + SETTINGS_PKG);
        executeShellCommand("pm clear " + DOCUMENTS_PKG);
    }

    private static class LocalVolumeInfo {
        public String volId;
        public String state;
        public String uuid;

        public LocalVolumeInfo(String line) {
            final String[] split = line.split(" ");
            volId = split[0];
            state = split[1];
            uuid = split[2];
        }
    }

    public PackageManager getPackageManager() {
        return mContext.getPackageManager();
    }
}
