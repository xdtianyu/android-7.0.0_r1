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

package com.android.smokefast;

import android.content.Intent;
import android.net.Uri;
import android.os.Environment;
import android.provider.MediaStore;
import android.support.test.uiautomator.By;
import android.support.test.uiautomator.UiDevice;
import android.support.test.uiautomator.UiObject2;
import android.support.test.uiautomator.Until;
import android.test.InstrumentationTestCase;
import android.test.suitebuilder.annotation.LargeTest;

import java.io.File;
import java.util.regex.Pattern;

/**
 * Basic tests for the Camera app.
 */
public class MediaCaptureTest extends InstrumentationTestCase {
    private static final int CAPTURE_TIMEOUT = 6000;
    private static final String DESC_BTN_CAPTURE_PHOTO = "Capture photo";
    private static final String DESC_BTN_CAPTURE_VIDEO = "Capture video";
    private static final String DESC_BTN_DONE = "Done";
    private static final String DESC_BTN_PHOTO_MODE = "Open photo mode";
    private static final String DESC_BTN_VIDEO_MODE = "Open video mode";
    private static final int FILE_CHECK_ATTEMPTS = 5;
    private static final int VIDEO_LENGTH = 2000;

    private UiDevice mDevice;

    @Override
    public void setUp() throws Exception {
        super.setUp();
        mDevice = UiDevice.getInstance(getInstrumentation());
        mDevice.freezeRotation();
        // if there are any dialogues that pop up, dismiss them
        UiObject2 maybeOkButton = mDevice.wait(Until.findObject(By.res("android:id/ok_button")),
                CAPTURE_TIMEOUT);
        if (maybeOkButton != null) {
            maybeOkButton.click();
        }
    }

    @Override
    public void tearDown() throws Exception {
        mDevice.unfreezeRotation();
        super.tearDown();
    }

    /**
     * Test that the device can capture a photo.
     */
    @LargeTest
    public void testPhotoCapture() {
        runCaptureTest(new Intent(MediaStore.ACTION_IMAGE_CAPTURE), "smoke.jpg", false);
    }

    /**
     * Test that the device can capture a video.
     */
    @LargeTest
    public void testVideoCapture() {
        runCaptureTest(new Intent(MediaStore.ACTION_VIDEO_CAPTURE), "smoke.avi", true);
    }

    private void runCaptureTest(Intent intent, String tmpName, boolean isVideo) {
        intent.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
        if (intent.resolveActivity(
                    getInstrumentation().getContext().getPackageManager()) != null) {
            File outputFile = null;
            try {
                outputFile = new File(Environment
                        .getExternalStoragePublicDirectory(Environment.DIRECTORY_DCIM), tmpName);
                intent.putExtra(MediaStore.EXTRA_OUTPUT, Uri.fromFile(outputFile));
                getInstrumentation().getContext().startActivity(intent);
                switchCaptureMode(isVideo);
                pressCaptureButton(isVideo);
                if (isVideo) {
                    Thread.sleep(VIDEO_LENGTH);
                    pressCaptureButton(isVideo);
                }
                Thread.sleep(1000);
                pushButton(DESC_BTN_DONE);
                long fileLength = outputFile.length();
                for (int i=0; i<FILE_CHECK_ATTEMPTS; i++) {
                    if ((fileLength = outputFile.length()) > 0) {
                        break;
                    }
                    Thread.sleep(1000);
                }
                assertTrue(fileLength > 0);
            } catch (InterruptedException e) {
                fail(e.getLocalizedMessage());
            } finally {
                if (outputFile != null) {
                    outputFile.delete();
                }
            }
        }
    }

    private void switchCaptureMode(boolean isVideo) {
        if (isVideo) {
            pushButton(DESC_BTN_VIDEO_MODE);
        } else {
            pushButton(DESC_BTN_PHOTO_MODE);
        }
    }

    private void pressCaptureButton(boolean isVideo) {
        if (isVideo) {
            pushButton(DESC_BTN_CAPTURE_VIDEO);
        } else {
            pushButton(DESC_BTN_CAPTURE_PHOTO);
        }
    }

    private void pushButton(String desc) {
        Pattern pattern = Pattern.compile(desc, Pattern.CASE_INSENSITIVE);
        UiObject2 doneBtn = mDevice.wait(Until.findObject(By.desc(pattern)), CAPTURE_TIMEOUT);
        if (null != doneBtn) {
            doneBtn.clickAndWait(Until.newWindow(), 500);
        }
    }
}
