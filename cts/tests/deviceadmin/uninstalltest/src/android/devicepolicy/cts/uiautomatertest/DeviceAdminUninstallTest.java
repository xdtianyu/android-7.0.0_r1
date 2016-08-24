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
package android.devicepolicy.cts.uiautomatertest;

import android.app.Activity;
import android.app.Instrumentation;
import android.app.admin.DevicePolicyManager;
import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.content.pm.PackageInfo;
import android.content.pm.PackageManager;
import android.content.pm.PackageManager.NameNotFoundException;
import android.net.Uri;
import android.provider.Settings;
import android.widget.ScrollView;

import org.junit.After;
import org.junit.Assert;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;

import java.io.PrintWriter;
import java.io.StringWriter;
import java.util.List;

import android.support.test.InstrumentationRegistry;
import android.support.test.runner.AndroidJUnit4;
import android.support.test.filters.LargeTest;
import android.support.test.uiautomator.By;
import android.support.test.uiautomator.BySelector;
import android.support.test.uiautomator.UiDevice;
import android.support.test.uiautomator.UiObject;
import android.support.test.uiautomator.UiObjectNotFoundException;
import android.support.test.uiautomator.UiScrollable;
import android.support.test.uiautomator.UiSelector;
import android.support.test.uiautomator.Until;
import android.util.Log;

@RunWith(AndroidJUnit4.class)
@LargeTest
public class DeviceAdminUninstallTest {

    private static final String DEVICE_ADMIN_PACKAGE_NAME =
            "android.devicepolicy.cts.emptydeviceadmin";
    private static final String URI_PACKAGE_PREFIX = "package:";

    private static final UiSelector DEACTIVATE_AND_UNINSTALL_BUTTON_SELECTOR = new UiSelector()
            .resourceId("com.android.settings:id/action_button");
    private static final UiSelector UNINSTALL_BUTTON_SELECTOR = new UiSelector()
            .resourceId("com.android.settings:id/left_button");
    private static final BySelector UNINSTALL_BUTTON_BYSELECTOR = By
            .res("com.android.settings", "left_button");
    private static final UiSelector OK_BUTTON_SELECTOR = new UiSelector()
            .resourceId("android:id/button1");
    private static final UiSelector SCROLL_VIEW_SELECTOR =
            new UiSelector().className(ScrollView.class);

    private static final long DEACTIVATE_ADMIN_TIMEOUT = 15000;
    private static final long WAIT_FOR_ACTIVITY_TIMEOUT = 6000;
    private static final String TAG = DeviceAdminUninstallTest.class.getSimpleName();

    private UiDevice mUiDevice;
    private Instrumentation mInstrumentation;
    private Context mContext;
    private DevicePolicyManager mDpm;
    private PackageManager mPm;
    private Activity mActivity;
    private boolean mHasFeature;

    @Before
    public void setUp() throws Exception {
        mContext = InstrumentationRegistry.getTargetContext();
        mPm = mContext.getPackageManager();
        mHasFeature = mPm.hasSystemFeature(PackageManager.FEATURE_DEVICE_ADMIN);
        if (!mHasFeature) {
            // No point of initializing other stuff
            return;
        }
        mInstrumentation = InstrumentationRegistry.getInstrumentation();
        mUiDevice = UiDevice.getInstance(mInstrumentation);
        mDpm = mContext.getSystemService(DevicePolicyManager.class);
        startTestActivity();
    }

    private void startTestActivity() {
        final Intent testActivityIntent = new Intent(mContext, UiAutomaterTestActivity.class);
        testActivityIntent.setFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
        mActivity = mInstrumentation.startActivitySync(testActivityIntent);
        mInstrumentation.waitForIdleSync();
    }

    private boolean hasActiveAdmin() {
        final List<ComponentName> activeAdmins = mDpm.getActiveAdmins();
        if (activeAdmins == null) {
            return false;
        }
        for (ComponentName activeAdmin : activeAdmins) {
            if (DEVICE_ADMIN_PACKAGE_NAME.equals(activeAdmin.getPackageName())) {
                return true;
            }
        }
        return false;
    }

    private void waitTillNoActiveAdmin() {
        boolean interrupted = false;
        final long timeOut = System.currentTimeMillis() + DEACTIVATE_ADMIN_TIMEOUT;
        while (hasActiveAdmin() && System.currentTimeMillis() <= timeOut) {
            try {
                Thread.sleep(2500);
            } catch (InterruptedException err) {
                interrupted = true;
            }
        }
        if (interrupted) {
            Thread.currentThread().interrupt();
        }
        Assert.assertFalse(
                "Package " + DEVICE_ADMIN_PACKAGE_NAME + " still has an active device admin",
                hasActiveAdmin());
    }

    private boolean packageExists() {
        PackageInfo packageInfo = null;
        try {
            packageInfo = mPm.getPackageInfo(DEVICE_ADMIN_PACKAGE_NAME, 0);
        } catch (NameNotFoundException exc) {
        }
        return packageInfo != null;
    }

    private void launchApplicationDetailsActivity() {
        final Uri packageURI = Uri.parse(URI_PACKAGE_PREFIX + DEVICE_ADMIN_PACKAGE_NAME);
        final Intent displayAppDetailsIntent = new Intent(
                Settings.ACTION_APPLICATION_DETAILS_SETTINGS,
                packageURI);
        mActivity.startActivity(displayAppDetailsIntent);
    }

    private void automateUninstallThroughUi() {
        String errorMessage = "No exception in UI Automation";

        UiObject uninstallButton = mUiDevice.findObject(UNINSTALL_BUTTON_SELECTOR);
        UiScrollable scrollable = new UiScrollable(SCROLL_VIEW_SELECTOR);
        UiObject deactivateButton = mUiDevice.findObject(DEACTIVATE_AND_UNINSTALL_BUTTON_SELECTOR);
        UiObject okButton = mUiDevice.findObject(OK_BUTTON_SELECTOR);

        mUiDevice.wait(Until.hasObject(UNINSTALL_BUTTON_BYSELECTOR), WAIT_FOR_ACTIVITY_TIMEOUT);
        try {
            uninstallButton.clickAndWaitForNewWindow();
            scrollable.scrollIntoView(DEACTIVATE_AND_UNINSTALL_BUTTON_SELECTOR);
            deactivateButton.clickAndWaitForNewWindow();
            waitTillNoActiveAdmin();
            okButton.clickAndWaitForNewWindow();
        } catch (UiObjectNotFoundException exc) {
            Log.e(TAG, "Error while automating uninstall of " + DEVICE_ADMIN_PACKAGE_NAME, exc);
            errorMessage = generateStackTrace(exc);
        } finally {
            Assert.assertFalse("Package " + DEVICE_ADMIN_PACKAGE_NAME + " was not uninstalled\n"
                    + errorMessage, packageExists());
        }
    }

    private String generateStackTrace(Throwable t) {
        StringWriter errorWriter = new StringWriter();
        t.printStackTrace(new PrintWriter(errorWriter));
        return errorWriter.toString();
    }

    @Test
    public void uninstallPackageWithActiveAdmin() {
        if (!mHasFeature) {
            return;
        }
        Assert.assertTrue("Package " + DEVICE_ADMIN_PACKAGE_NAME + " was not found installed",
                packageExists());
        Assert.assertTrue("Package " + DEVICE_ADMIN_PACKAGE_NAME
                + " does not have any active admin component", hasActiveAdmin());
        launchApplicationDetailsActivity();
        automateUninstallThroughUi();
    }

    @After
    public void tearDown() {
        if (!mHasFeature) {
            return;
        }
        mActivity.finish();
        mUiDevice.pressHome();
    }
}
