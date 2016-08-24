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
package android.platform.test.helpers;

import android.app.Instrumentation;
import android.content.pm.PackageManager.NameNotFoundException;
import android.os.RemoteException;
import android.os.SystemClock;
import android.support.test.uiautomator.By;
import android.support.test.uiautomator.BySelector;
import android.support.test.uiautomator.Direction;
import android.support.test.uiautomator.Until;
import android.support.test.uiautomator.UiDevice;
import android.support.test.uiautomator.UiObject;
import android.support.test.uiautomator.UiObject2;
import android.support.test.uiautomator.UiObjectNotFoundException;
import android.support.test.uiautomator.UiSelector;
import android.util.Log;

import java.util.regex.Pattern;

public class FlightDemoHelperImpl extends AbstractFlightDemoHelper {
    private static final String LOG_TAG = FlightDemoHelperImpl.class.getCanonicalName();
    private static final String UI_PACKAGE_NAME = "leofs.android.free";
    private static final String UI_ACTIVITY_NAME = "leofs.android.free.LeofsActivity";

    private static final int UI_RESPONSE_WAIT = 2000; // 2 secs
    private static final int MAX_MENU_SCROLL_DOWN_COUNT = 10;

    public FlightDemoHelperImpl(Instrumentation instr) {
        super(instr);
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public String getPackage() {
        return UI_PACKAGE_NAME;
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public String getLauncherName() {
        return "Leo´s RC Simulator";
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void dismissInitialDialogs() {
        // Nothing to do here.  There is no initial dialog in this app.
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void startDemo() {
        Log.v(LOG_TAG, "Starting flight simulator demo");
        selectMenuItem("Demo");
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void stopDemo() {
        Log.v(LOG_TAG, "Stopping flight simulator demo");
        selectMenuItem("Reset");
        mDevice.pressBack();
    }

    private void selectMenuItem(String item) {
        mDevice.pressMenu();
        UiObject2 container = mDevice.wait(Until.findObject(By.res("android", "list")),
                                           UI_RESPONSE_WAIT);
        if (container == null) {
            throw new IllegalStateException("Cannot find scrollable menu");
        }

        String err_msg = String.format("Cannot find menu item %s", item);
        int scroll_counter = 0;
        UiObject2 button = null;
        boolean reachedEnd = false;
        while (!reachedEnd) {
            final Pattern word = Pattern.compile(item, Pattern.CASE_INSENSITIVE);
            button = mDevice.wait(Until.findObject(By.text(word)), UI_RESPONSE_WAIT);
            if (button != null) {
                button.click();
                break;
            }

            if (!container.scroll(Direction.DOWN, 1.0f) &&
                scroll_counter >= MAX_MENU_SCROLL_DOWN_COUNT) {
                reachedEnd = true;
            }
            scroll_counter++;
        }
        if (button != null) {
            button.click();
        }
        else {
            throw new IllegalStateException(err_msg);
        }
    }
}
