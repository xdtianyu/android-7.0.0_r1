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

package android.launcher.functional;

import java.io.IOException;
import android.content.Intent;
import android.os.RemoteException;
import android.os.SystemClock;
import android.support.test.uiautomator.By;
import android.support.test.uiautomator.UiDevice;
import android.support.test.uiautomator.UiObject2;
import android.support.test.uiautomator.UiObjectNotFoundException;
import android.support.test.uiautomator.Until;
import android.test.InstrumentationTestCase;
import android.test.suitebuilder.annotation.MediumTest;
import android.test.suitebuilder.annotation.Suppress;

public class TabletHotseatTests extends InstrumentationTestCase {

    private static final int TIMEOUT = 3000;
    private static final String HOTSEAT = "hotseat";
    private UiDevice mDevice;
    private HotseatHelper hotseatHelper = null;

    @Override
    public void setUp() throws Exception {
        super.setUp();
        mDevice = UiDevice.getInstance(getInstrumentation());
        try {
            mDevice.setOrientationNatural();
        } catch (RemoteException e) {
            throw new RuntimeException("failed to freeze device orientaion", e);
        }
        hotseatHelper = HotseatHelper.getInstance(mDevice, getInstrumentation().getContext());
    }

    @Override
    protected void tearDown() throws Exception {
        mDevice.pressHome();
        super.tearDown();
    }

    public String getLauncherPackage() {
        return mDevice.getLauncherPackageName();
    }

    @Suppress
    @MediumTest
    public void testOpenChromeFromHotseat() {
        hotseatHelper.launchAppFromHotseat("Chrome", "com.android.chrome");
    }

    @Suppress
    @MediumTest
    public void testOpenCameraFromHotseat() {
        hotseatHelper.launchAppFromHotseat("Camera", "com.google.android.GoogleCamera");
    }

    @Suppress
    @MediumTest
    public void testOpenGMailFromHotseat() {
        hotseatHelper.launchAppFromHotseat("Gmail", "com.google.android.gm");
    }

    @Suppress
    @MediumTest
    public void testOpenHangoutsFromHotseat() {
        hotseatHelper.launchAppFromHotseat("Hangouts", "com.google.android.gms");
    }

    @Suppress
    @MediumTest
    public void testOpenPhotosFromHotseat() {
        hotseatHelper.launchAppFromHotseat("Photos", "com.google.android.apps.photos");
    }

    @Suppress
    @MediumTest
    public void testOpenYoutubeFromHotseat() {
        hotseatHelper.launchAppFromHotseat("YouTube", "com.google.android.youtube");
    }

    @Suppress
    @MediumTest
    public void testHomeToAllAppsNavigation() {
        hotseatHelper.launchAppFromHotseat("Apps", getLauncherPackage());
        assertNotNull("All apps page not found when navigating from hotseat",
                mDevice.wait(Until.hasObject(By.res(getLauncherPackage(), "apps_view")), TIMEOUT));
    }
}
