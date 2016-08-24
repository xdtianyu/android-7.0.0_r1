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

import android.content.ContentValues;
import android.content.Context;
import android.content.Intent;
import android.net.Uri;
import android.os.Environment;
import android.platform.test.helpers.GoogleCameraHelperImpl;
import android.provider.MediaStore;
import android.support.test.InstrumentationRegistry;
import android.support.test.uiautomator.By;
import android.support.test.uiautomator.UiDevice;
import android.support.test.uiautomator.UiObject2;
import android.support.test.uiautomator.Until;
import android.test.InstrumentationTestCase;
import android.test.suitebuilder.annotation.LargeTest;

import junit.framework.TestCase;
import java.io.File;
import java.util.regex.Pattern;

/**
 * Basic tests for the Camera app.
 */
public class MediaCaptureTests extends TestCase {
    private static final int CAPTURE_TIMEOUT = 6000;
    private static final String DESC_BTN_DONE = "Done";
    private static final int FILE_CHECK_ATTEMPTS = 5;
    private static final long VIDEO_LENGTH = 2000;
    private static final String CAMERA_DIRECTORY = "/sdcard/DCIM/Camera";
    private Context mContext;
    private UiDevice mDevice;
    private GoogleCameraHelperImpl mCameraHelper;

    @Override
    public void setUp() throws Exception {
        super.setUp();
        mDevice = UiDevice.getInstance(InstrumentationRegistry.getInstrumentation());
        mDevice.freezeRotation();
        mContext = InstrumentationRegistry.getTargetContext();
        mCameraHelper = new GoogleCameraHelperImpl(InstrumentationRegistry.getInstrumentation());
        mCameraHelper.open();
        // if there are any dialogues that pop up, dismiss them
        mCameraHelper.dismissInitialDialogs();
    }

    @Override
    public void tearDown() throws Exception {
        mDevice.pressHome();
        mDevice.unfreezeRotation();
        super.tearDown();
    }

    /**
     * Test that the device can capture a photo.
     */
    @LargeTest
    public void testPhotoCapture() throws InterruptedException {
        int beforeCount = getValidFileCountFromFilesystem();
        mCameraHelper.goToCameraMode();
        // Capture photo using front camera
        mCameraHelper.goToFrontCamera();
        mCameraHelper.capturePhoto();
        Thread.sleep(CAPTURE_TIMEOUT);
        int afterCount = getValidFileCountFromFilesystem();
        assertTrue("Camera didnt capture picture using front camera", afterCount > beforeCount);
        // Capture photo using back camera
        beforeCount = getValidFileCountFromFilesystem();
        mCameraHelper.goToBackCamera();
        mCameraHelper.capturePhoto();
        Thread.sleep(CAPTURE_TIMEOUT);
        afterCount = getValidFileCountFromFilesystem();
        assertTrue("Camera didnt capture picture using back camera", afterCount > beforeCount);
    }

    /**
     * Test that the device can capture a video.
     */
    @LargeTest
    public void testVideoCapture() throws InterruptedException {
        int beforeCount = getValidFileCountFromFilesystem();
        mCameraHelper.goToVideoMode();
        Thread.sleep(CAPTURE_TIMEOUT);
        // Capture video using front camera
        mCameraHelper.goToFrontCamera();
        mCameraHelper.captureVideo(VIDEO_LENGTH);
        pushButton(DESC_BTN_DONE);
        Thread.sleep(CAPTURE_TIMEOUT);
        int afterCount = getValidFileCountFromFilesystem();
        assertTrue("Camera didnt capture video", afterCount > beforeCount);
        // Capture video using back camera
        beforeCount = getValidFileCountFromFilesystem();
        mCameraHelper.goToBackCamera();
        mCameraHelper.captureVideo(VIDEO_LENGTH);
        pushButton(DESC_BTN_DONE);
        Thread.sleep(CAPTURE_TIMEOUT);
        afterCount = getValidFileCountFromFilesystem();
        assertTrue("Camera didnt capture video", afterCount > beforeCount);
    }

    private void pushButton(String desc) {
        Pattern pattern = Pattern.compile(desc, Pattern.CASE_INSENSITIVE);
        UiObject2 doneBtn = mDevice.wait(Until.findObject(By.desc(pattern)), CAPTURE_TIMEOUT);
        if (null != doneBtn) {
            doneBtn.clickAndWait(Until.newWindow(), 500);
        }
    }

    private int getValidFileCountFromFilesystem() {
        int count = 0;
        File file = new File(CAMERA_DIRECTORY);
        for (File child : file.listFiles()) {
            if (validateSavedFile(child)) {
                count++;
            }
        }
        return count;
    }

    private boolean validateSavedFile(File file) {
        return (file.exists() && file.length() > 0);
    }
}
