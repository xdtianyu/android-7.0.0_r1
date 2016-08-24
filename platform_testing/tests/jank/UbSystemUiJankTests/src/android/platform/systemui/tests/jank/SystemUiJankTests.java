/*
 * Copyright (C) 2015 The Android Open Source Project
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

package android.platform.systemui.tests.jank;

import android.app.Notification.Builder;
import android.app.NotificationManager;
import android.content.Context;
import android.content.Intent;
import android.content.pm.PackageInfo;
import android.content.pm.PackageManager;
import android.graphics.Rect;
import android.os.Bundle;
import android.os.RemoteException;
import android.os.SystemClock;
import android.support.test.jank.GfxMonitor;
import android.support.test.jank.JankTest;
import android.support.test.jank.JankTestBase;
import android.support.test.uiautomator.By;
import android.support.test.uiautomator.BySelector;
import android.support.test.uiautomator.UiDevice;
import android.support.test.uiautomator.UiObject2;
import android.support.test.uiautomator.Until;
import android.support.test.timeresulthelper.TimeResultLogger;
import android.util.Log;
import android.os.Environment;

import java.io.File;
import java.io.IOException;
import java.util.ArrayList;
import java.util.List;

public class SystemUiJankTests extends JankTestBase {

    private static final String SYSTEMUI_PACKAGE = "com.android.systemui";
    private static final BySelector RECENTS = By.res(SYSTEMUI_PACKAGE, "recents_view");
    private static final String LOG_TAG = SystemUiJankTests.class.getSimpleName();
    private static final int SWIPE_MARGIN = 5;
    private static final int DEFAULT_FLING_STEPS = 5;
    private static final int DEFAULT_SCROLL_STEPS = 15;
    // short transitions should be repeated within the test function, otherwise frame stats
    // captured are not really meaningful in a statistical sense
    private static final int INNER_LOOP = 3;
    private static final int[] ICONS = new int[] {
            android.R.drawable.stat_notify_call_mute,
            android.R.drawable.stat_notify_chat,
            android.R.drawable.stat_notify_error,
            android.R.drawable.stat_notify_missed_call,
            android.R.drawable.stat_notify_more,
            android.R.drawable.stat_notify_sdcard,
            android.R.drawable.stat_notify_sdcard_prepare,
            android.R.drawable.stat_notify_sdcard_usb,
            android.R.drawable.stat_notify_sync,
            android.R.drawable.stat_notify_sync_noanim,
            android.R.drawable.stat_notify_voicemail,
    };
    private static final String NOTIFICATION_TEXT = "Lorem ipsum dolor sit amet";
    private static final File TIMESTAMP_FILE = new File(Environment.getExternalStorageDirectory()
            .getAbsolutePath(), "autotester.log");
    private static final File RESULTS_FILE = new File(Environment.getExternalStorageDirectory()
            .getAbsolutePath(), "results.log");

    private UiDevice mDevice;
    private List<String> mLaunchedPackages = new ArrayList<>();

    public void setUp() {
        mDevice = UiDevice.getInstance(getInstrumentation());
        try {
            mDevice.setOrientationNatural();
        } catch (RemoteException e) {
            throw new RuntimeException("failed to freeze device orientaion", e);
        }
    }

    public void goHome() {
        mDevice.pressHome();
        mDevice.waitForIdle();
    }

    @Override
    protected void tearDown() throws Exception {
        mDevice.unfreezeRotation();
        super.tearDown();
    }

    public void populateRecentApps() throws IOException {
        PackageManager pm = getInstrumentation().getContext().getPackageManager();
        List<PackageInfo> packages = pm.getInstalledPackages(0);
        mLaunchedPackages.clear();
        for (PackageInfo pkg : packages) {
            if (pkg.packageName.equals(getInstrumentation().getTargetContext().getPackageName())) {
                continue;
            }
            Intent intent = pm.getLaunchIntentForPackage(pkg.packageName);
            if (intent == null) {
                continue;
            }
            intent.addCategory(Intent.CATEGORY_LAUNCHER);
            intent.setFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
            getInstrumentation().getTargetContext().startActivity(intent);
            SystemClock.sleep(5000);
            mLaunchedPackages.add(pkg.packageName);
        }

        // Close any crash dialogs
        while (mDevice.hasObject(By.textContains("has stopped"))) {
            mDevice.findObject(By.text("Close")).clickAndWait(Until.newWindow(), 2000);
        }
        TimeResultLogger.writeTimeStampLogStart(String.format("%s-%s",
                getClass().getSimpleName(), getName()), TIMESTAMP_FILE);
    }

    public void forceStopPackages(Bundle metrics) throws IOException {
        TimeResultLogger.writeTimeStampLogEnd(String.format("%s-%s",
                getClass().getSimpleName(), getName()), TIMESTAMP_FILE);
        for (String pkg : mLaunchedPackages) {
            try {
                mDevice.executeShellCommand("am force-stop " + pkg);
            } catch (IOException e) {
                Log.w(LOG_TAG, "exeception while force stopping package " + pkg, e);
            }
        }
        goHome();
        TimeResultLogger.writeResultToFile(String.format("%s-%s",
                getClass().getSimpleName(), getName()), RESULTS_FILE, metrics);
        super.afterTest(metrics);
    }

    public void resetRecentsToBottom() {
        // Rather than trying to scroll back to the bottom, just re-open the recents list
        mDevice.pressHome();
        mDevice.waitForIdle();
        try {
            mDevice.pressRecentApps();
        } catch (RemoteException e) {
            throw new RuntimeException(e);
        }
        // use a long timeout to wait until recents populated
        mDevice.wait(Until.findObject(RECENTS), 10000);
        mDevice.waitForIdle();
    }

    public void prepareNotifications() throws IOException {
        goHome();
        Builder builder = new Builder(getInstrumentation().getTargetContext())
                .setContentTitle(NOTIFICATION_TEXT);
        NotificationManager nm = (NotificationManager) getInstrumentation().getTargetContext()
                .getSystemService(Context.NOTIFICATION_SERVICE);
        for (int icon : ICONS) {
            builder.setContentText(Integer.toHexString(icon))
                    .setSmallIcon(icon);
            nm.notify(icon, builder.build());
            SystemClock.sleep(100);
        }
        mDevice.waitForIdle();
        TimeResultLogger.writeTimeStampLogStart(String.format("%s-%s",
                getClass().getSimpleName(), getName()), TIMESTAMP_FILE);
    }

    public void cancelNotifications(Bundle metrics) throws IOException {
        TimeResultLogger.writeTimeStampLogEnd(String.format("%s-%s",
                getClass().getSimpleName(), getName()), TIMESTAMP_FILE);
        NotificationManager nm = (NotificationManager) getInstrumentation().getTargetContext()
                .getSystemService(Context.NOTIFICATION_SERVICE);
        nm.cancelAll();
        TimeResultLogger.writeResultToFile(String.format("%s-%s",
                getClass().getSimpleName(), getName()), RESULTS_FILE, metrics);
        super.afterTest(metrics);
    }

    /** Starts from the bottom of the recent apps list and measures jank while flinging up. */
    @JankTest(beforeTest = "populateRecentApps", beforeLoop = "resetRecentsToBottom",
            afterTest = "forceStopPackages", expectedFrames = 100)
    @GfxMonitor(processName = SYSTEMUI_PACKAGE)
    public void testRecentAppsFling() {
        UiObject2 recents = mDevice.findObject(RECENTS);
        Rect r = recents.getVisibleBounds();
        // decide the top & bottom edges for scroll gesture
        int top = r.top + r.height() / 4; // top edge = top + 25% height
        int bottom = r.bottom - 200; // bottom edge = bottom & shift up 200px
        for (int i = 0; i < INNER_LOOP; i++) {
            mDevice.swipe(r.width() / 2, top, r.width() / 2, bottom, DEFAULT_FLING_STEPS);
            mDevice.waitForIdle();
            mDevice.swipe(r.width() / 2, bottom, r.width() / 2, top, DEFAULT_FLING_STEPS);
            mDevice.waitForIdle();
        }
    }

    private void openNotification() {
        mDevice.swipe(mDevice.getDisplayWidth() / 2,
                SWIPE_MARGIN, mDevice.getDisplayWidth() / 2,
                mDevice.getDisplayHeight() - SWIPE_MARGIN,
                DEFAULT_SCROLL_STEPS);
    }

    private void closeNotification() {
        mDevice.swipe(mDevice.getDisplayWidth() / 2,
                mDevice.getDisplayHeight() - SWIPE_MARGIN,
                mDevice.getDisplayWidth() / 2,
                SWIPE_MARGIN,
                DEFAULT_SCROLL_STEPS);
    }

    /** Measures jank while pulling down the notification list */
    @JankTest(expectedFrames = 100,
            beforeTest = "prepareNotifications", afterTest = "cancelNotifications")
    @GfxMonitor(processName = SYSTEMUI_PACKAGE)
    public void testNotificationListPull() {
        for (int i = 0; i < INNER_LOOP; i++) {
            openNotification();
            mDevice.waitForIdle();
            closeNotification();
            mDevice.waitForIdle();
        }
    }
}

