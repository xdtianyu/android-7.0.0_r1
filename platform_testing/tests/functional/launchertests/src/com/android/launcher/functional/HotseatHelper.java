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
import android.content.Context;
import android.content.Intent;
import android.content.pm.PackageManager;
import android.os.Environment;
import android.os.RemoteException;
import android.os.SystemClock;
import android.support.test.uiautomator.By;
import android.support.test.uiautomator.UiDevice;
import android.support.test.uiautomator.UiObject2;
import android.support.test.uiautomator.UiObjectNotFoundException;
import android.support.test.uiautomator.Until;

import junit.framework.Assert;


public class HotseatHelper {

    private static final int TIMEOUT = 3000;
    private UiDevice mDevice;
    private PackageManager pm;
    private Context mContext;
    public static HotseatHelper mInstance = null;

    private HotseatHelper(UiDevice device, Context context) {
        mDevice = device;
        mContext = context;
    }

    public static HotseatHelper getInstance(UiDevice device, Context context) {
        if (mInstance == null) {
            mInstance = new HotseatHelper(device, context);
        }
        return mInstance;
    }

    public void launchAppFromHotseat(String textAppName, String appPackage) {
        mDevice.pressHome();
        UiObject2 appOnHotseat = mDevice.findObject(By.clazz("android.widget.TextView")
                .desc(textAppName));
        Assert.assertNotNull(textAppName + " app couldn't be found on hotseat", appOnHotseat);
        appOnHotseat.click();
        UiObject2 appLoaded = mDevice.wait(Until.findObject(By.pkg(appPackage)), TIMEOUT*2);
        Assert.assertNotNull(textAppName + "app did not load on tapping from hotseat", appLoaded);
    }
}
