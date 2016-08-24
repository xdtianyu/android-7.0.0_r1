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

import android.app.UiAutomation;
import android.content.Context;
import android.os.Environment;
import android.support.test.InstrumentationRegistry;
import android.support.test.uiautomator.UiDevice;
import android.test.suitebuilder.annotation.LargeTest;
import android.util.Log;

import junit.framework.TestCase;

import java.io.File;
import java.io.IOException;
import java.util.List;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

public class SysUIMultiUserTests extends TestCase {
    private UiAutomation mUiAutomation = null;
    private UiDevice mDevice;
    private Context mContext = null;
    private AndroidBvtHelper mABvtHelper = null;

    @Override
    public void setUp() throws Exception {
        super.setUp();
        mDevice = UiDevice.getInstance(InstrumentationRegistry.getInstrumentation());
        mDevice.setOrientationNatural();
        mContext = InstrumentationRegistry.getTargetContext();
        mUiAutomation = InstrumentationRegistry.getInstrumentation().getUiAutomation();
        mABvtHelper = AndroidBvtHelper.getInstance(mDevice, mContext, mUiAutomation);
    }

    @Override
    public void tearDown() throws Exception {
        mDevice.unfreezeRotation();
        super.tearDown();
    }

    /**
     * Following test creates a second user and verifies user created Also ensures owner has no
     * access to second user's dir
     */
    @LargeTest
    public void testMultiUserCreate() throws InterruptedException, IOException {
        int secondUserId = -1;
        List<String> cmdOut;
        try {
            // Ensure there are exactly 1 user
            cmdOut = mABvtHelper.executeShellCommand("pm list users");
            assertTrue("There aren't exatcly 1 user", (cmdOut.size() - 1) == 1);

            // Create user
            cmdOut = mABvtHelper.executeShellCommand("pm create-user test");
            assertTrue("Output should have 1 line", cmdOut.size() == 1);
            // output format : Success: created user id 10
            // Find user id from output above
            Pattern pattern = Pattern.compile(
                    "(.*)(:)(.*?)(\\d+)");
            Matcher matcher = pattern.matcher(cmdOut.get(0));
            if (matcher.find()) {
                Log.i(mABvtHelper.TEST_TAG, String.format("User Name:%s UserId:%d",
                        matcher.group(1), Integer.parseInt(matcher.group(4))));
                secondUserId = Integer.parseInt(matcher.group(4));
            }
            Thread.sleep(mABvtHelper.SHORT_TIMEOUT);

            // Verify second user id is created
            cmdOut = mABvtHelper.executeShellCommand("pm list users");
            // Sample output of "pm list users"
            // Users:
            // UserInfo{0:Owner:13} running
            // UserInfo{18:test:0}
            assertTrue("There aren't exatcly 2 users", (cmdOut.size() - 1) == 2);
            // Get Second user id from 'list users' cmd
            // Ensure that matches with previously created user id
            pattern = Pattern.compile(
                    "(.*\\{)(\\d+)(:)(.*?)(:)(\\d+)(\\}.*)"); // 2 = id 6 = flag
            matcher = pattern.matcher(cmdOut.get(2));
            if (matcher.find()) {
                assertTrue("Second User id doesn't match",
                        Integer.parseInt(matcher.group(2)) == secondUserId);
            }
            Thread.sleep(mABvtHelper.SHORT_TIMEOUT);

            // Ensure owner has no access to second user's directory
            final File myPath = Environment.getExternalStorageDirectory();
            final int myId = android.os.Process.myUid() / 100000;
            assertEquals(String.valueOf(myId), myPath.getName());

            Log.i(mABvtHelper.TEST_TAG, "My path is " + myPath);
            final File basePath = myPath.getParentFile();
            for (int i = 0; i < 128; i++) {
                if (i == myId) {
                    continue;
                }

                final File otherPath = new File(basePath, String.valueOf(i));
                assertNull("Owner have access to other user's resources!", otherPath.list());
                assertFalse("Owner can read other user's content!", otherPath.canRead());
            }
        } finally {
            cmdOut = mABvtHelper.executeShellCommand("pm remove-user " + secondUserId);
            Log.i(mABvtHelper.TEST_TAG,
                    String.format("Second user has been removed? %s. CmdOut = %s",
                            cmdOut.get(0).equals("Success: removed user"), cmdOut.get(0)));
        }
    }
}
