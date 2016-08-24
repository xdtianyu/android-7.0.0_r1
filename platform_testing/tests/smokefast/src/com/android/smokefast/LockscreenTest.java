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

import android.os.SystemClock;
import android.support.test.uiautomator.By;
import android.support.test.uiautomator.Direction;
import android.support.test.uiautomator.UiDevice;
import android.support.test.uiautomator.UiObject2;
import android.support.test.uiautomator.Until;
import android.test.InstrumentationTestCase;
import android.test.suitebuilder.annotation.LargeTest;

public class LockscreenTest extends InstrumentationTestCase {
    private static final String LAUNCHER_PACKAGE = "com.google.android.googlequicksearchbox";
    private static final String SYSTEMUI_PACKAGE = "com.android.systemui";
    private UiDevice mDevice;

    @Override
    public void setUp() throws Exception {
        super.setUp();
        mDevice = UiDevice.getInstance(getInstrumentation());
        mDevice.freezeRotation();
    }

    @Override
    public void tearDown() throws Exception {
        mDevice.wakeUp();
        mDevice.pressMenu();
        mDevice.unfreezeRotation();
        super.tearDown();
    }

    @LargeTest
    public void testSlideUnlock() throws Exception {
        sleepAndWakeUpDevice();
        mDevice.wait(Until.findObject(
                By.res(SYSTEMUI_PACKAGE, "notification_stack_scroller")), 2000)
                .swipe(Direction.UP, 1.0f);
        int counter = 6;
        UiObject2 workspace =  mDevice.findObject(By.res(LAUNCHER_PACKAGE, "workspace"));
        while (counter-- > 0 && workspace == null) {
            workspace =  mDevice.findObject(By.res(LAUNCHER_PACKAGE, "workspace"));
            SystemClock.sleep(500);
        }
        assertNotNull("Workspace wasn't found", workspace);
    }

    private void sleepAndWakeUpDevice() throws Exception {
        mDevice.sleep();
        SystemClock.sleep(1000);
        mDevice.wakeUp();
    }
}
