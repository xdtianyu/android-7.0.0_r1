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

import android.app.IntentService;
import android.app.Notification;
import android.app.NotificationManager;
import android.app.PendingIntent;
import android.app.RemoteInput;
import android.content.ContentResolver;
import android.content.Context;
import android.content.Intent;
import android.os.Handler;
import android.os.RemoteException;
import android.provider.Settings;
import android.service.notification.StatusBarNotification;
import android.support.test.InstrumentationRegistry;
import android.support.test.uiautomator.By;
import android.support.test.uiautomator.Direction;
import android.support.test.uiautomator.UiDevice;
import android.support.test.uiautomator.UiObject2;
import android.support.test.uiautomator.Until;
import android.view.inputmethod.InputMethodManager;
import android.test.suitebuilder.annotation.LargeTest;
import android.test.suitebuilder.annotation.MediumTest;
import android.widget.EditText;
import android.widget.Toast;

import android.util.Log;
import junit.framework.Assert;
import junit.framework.TestCase;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;

public class SysUINotificationShadeTests extends TestCase {
    private static final String LOG_TAG = SysUINotificationShadeTests.class.getSimpleName();
    private static final int SHORT_TIMEOUT = 200;
    private static final int LONG_TIMEOUT = 2000;
    private static final int GROUP_NOTIFICATION_ID = 1;
    private static final int CHILD_NOTIFICATION_ID = 100;
    private static final int SECOND_CHILD_NOTIFICATION_ID = 101;
    private static final int NOTIFICATION_ID_2 = 2;
    private static final String KEY_QUICK_REPLY_TEXT = "quick_reply";
    private static final String INLINE_REPLY_TITLE = "INLINE REPLY TITLE";
    private static final String RECEIVER_PKG_NAME = "com.android.systemui";
    private static final String BUNDLE_GROUP_KEY = "group key ";
    private UiDevice mDevice = null;
    private Context mContext;
    private NotificationManager mNotificationManager;
    private ContentResolver mResolver;
    private AndroidBvtHelper mABvtHelper = null;

    private enum QuickSettingTiles {
        WIFI("Wi-Fi"), SIM("SIM"), DND("Do not disturb"), BATTERY("Battery"),
        FLASHLIGHT("Flashlight"), SCREEN("screen"), BLUETOOTH("Bluetooth"),
        AIRPLANE("Airplane mode"), LOCATION("Location");

        private final String name;

        private QuickSettingTiles(String name) {
            this.name = name;
        }

        public String getName() {
            return this.name;
        }
    };

    @Override
    public void setUp() throws Exception {
        super.setUp();
        mDevice = UiDevice.getInstance(InstrumentationRegistry.getInstrumentation());
        mContext = InstrumentationRegistry.getTargetContext();
        mResolver = mContext.getContentResolver();
        mABvtHelper = AndroidBvtHelper.getInstance(mDevice, mContext,
                InstrumentationRegistry.getInstrumentation().getUiAutomation());
        mDevice.setOrientationNatural();
        mNotificationManager = (NotificationManager) mContext
                .getSystemService(Context.NOTIFICATION_SERVICE);
        mDevice.pressHome();
        mNotificationManager.cancelAll();
    }

    @Override
    public void tearDown() throws Exception {
        mNotificationManager.cancelAll();
        mDevice.pressHome();
        mDevice.unfreezeRotation();
        mDevice.waitForIdle();
        super.tearDown();
    }

    /**
     * Following test will create notifications, and verify notification can be expanded and
     * redacted
     */
    @LargeTest
    public void testNotifications() throws Exception {
        // test receive notification and expand/redact notification
        verifyReceiveAndExpandRedactNotification();
        // test inline notification and dismiss notification
        verifyInlineAndDimissNotification();
    }

    /**
     * Following test will open Quick Setting shade, and verify icons in the shade
     */
    @MediumTest
    public void testQuickSettings() throws Exception {
        mDevice.openQuickSettings();
        Thread.sleep(LONG_TIMEOUT * 2);
        // Verify quick settings are displayed on the phone screen.
        for (QuickSettingTiles tile : QuickSettingTiles.values()) {
            if (!(mABvtHelper.isTablet() && tile.getName().equals("SIM"))) {
                UiObject2 quickSettingTile = mDevice.wait(
                        Until.findObject(By.descContains(tile.getName())),
                        SHORT_TIMEOUT);
                assertNotNull(String.format("%s did not load correctly", tile.getName()),
                        quickSettingTile);
            }
        }
        // Verify tapping on Settings icon in Quick settings launches Settings.
        mDevice.wait(Until.findObject(By.descContains("Open settings.")), LONG_TIMEOUT)
                .click();
        UiObject2 settingHeading = mDevice.wait(Until.findObject(By.text("Settings")),
                LONG_TIMEOUT);
        assertNotNull("Setting menu has not loaded correctly", settingHeading);
    }

    private void verifyReceiveAndExpandRedactNotification() throws Exception {
        List<Integer> lists = new ArrayList<Integer>(Arrays.asList(GROUP_NOTIFICATION_ID,
                CHILD_NOTIFICATION_ID, SECOND_CHILD_NOTIFICATION_ID));
        sendBundlingNotifications(lists, BUNDLE_GROUP_KEY);
        Thread.sleep(LONG_TIMEOUT);
        swipeDown();
        UiObject2 obj = mDevice.wait(
                Until.findObject(By.textContains(lists.get(1).toString())),
                LONG_TIMEOUT);
        int currentY = obj.getVisibleCenter().y;
        mDevice.wait(Until.findObject(By.res("android:id/expand_button")), LONG_TIMEOUT * 2)
                .click();
        obj = mDevice.wait(Until.findObject(By.textContains(lists.get(0).toString())),
                LONG_TIMEOUT);
        assertFalse("The notifications has not been bundled",
                obj.getVisibleCenter().y == currentY);
        mDevice.wait(Until.findObject(By.res("android:id/expand_button")), LONG_TIMEOUT).click();
        obj = mDevice.wait(Until.findObject(By.textContains(lists.get(1).toString())),
                LONG_TIMEOUT);
        assertTrue("The notifications can not be redacted",
                obj.getVisibleCenter().y == currentY);
        mNotificationManager.cancelAll();
    }

    private void verifyInlineAndDimissNotification() throws Exception {
        sendNotificationsWithInLineReply(NOTIFICATION_ID_2, INLINE_REPLY_TITLE);
        Thread.sleep(LONG_TIMEOUT);
        mDevice.openNotification();
        mDevice.wait(Until.findObject(By.text("REPLY")), LONG_TIMEOUT).click();
        UiObject2 replyBox = mDevice.wait(
                Until.findObject(By.res(RECEIVER_PKG_NAME, "remote_input_send")),
                LONG_TIMEOUT);
        InputMethodManager imm = (InputMethodManager) mContext
                .getSystemService(Context.INPUT_METHOD_SERVICE);
        if (!imm.isAcceptingText()) {
            assertNotNull("Keyboard for inline reply has not loaded correctly", replyBox);
        }
        UiObject2 obj = mDevice.wait(Until.findObject(By.text(INLINE_REPLY_TITLE)),
                LONG_TIMEOUT);
        obj.swipe(Direction.LEFT, 1.0f);
        Thread.sleep(LONG_TIMEOUT);
        if (checkNotificationExistence(NOTIFICATION_ID_2)) {
            fail(String.format("Notification %s has not been dismissed", NOTIFICATION_ID_2));
        }
    }

    /**
     * send out a group of notifications
     * @param lists notification list for a group of notifications which includes two child
     *            notifications and one summary notification
     * @param groupKey the group key of group notification
     */
    private void sendBundlingNotifications(List<Integer> lists, String groupKey) throws Exception {
        Notification childNotification = new Notification.Builder(mContext)
                .setContentTitle(lists.get(1).toString())
                .setSmallIcon(R.drawable.stat_notify_email)
                .setContentText("test1")
                .setWhen(System.currentTimeMillis())
                .setGroup(groupKey)
                .build();
        mNotificationManager.notify(lists.get(1),
                childNotification);
        childNotification = new Notification.Builder(mContext)
                .setContentTitle(lists.get(2).toString())
                .setContentText("test2")
                .setSmallIcon(R.drawable.stat_notify_email)
                .setWhen(System.currentTimeMillis())
                .setGroup(groupKey)
                .build();
        mNotificationManager.notify(lists.get(2),
                childNotification);
        Notification notification = new Notification.Builder(mContext)
                .setContentTitle(lists.get(0).toString())
                .setSubText(groupKey)
                .setSmallIcon(R.drawable.stat_notify_email)
                .setGroup(groupKey)
                .setGroupSummary(true)
                .build();
        mNotificationManager.notify(lists.get(0),
                notification);
    }

    /**
     * send out a notification with inline reply
     *
     * @param notificationId An identifier for this notification
     * @param title notification title
     */
    private void sendNotificationsWithInLineReply(int notificationId, String title) {
        Notification.Action action = new Notification.Action.Builder(
                R.drawable.stat_notify_email, "Reply", ToastService.getPendingIntent(mContext,
                        title))
                                .addRemoteInput(new RemoteInput.Builder(KEY_QUICK_REPLY_TEXT)
                                        .setLabel("Quick reply").build())
                                .build();
        Notification.Builder n = new Notification.Builder(mContext)
                .setContentTitle(Integer.toString(notificationId))
                .setContentText(title)
                .setWhen(System.currentTimeMillis())
                .setSmallIcon(R.drawable.stat_notify_email)
                .addAction(action)
                .setPriority(Notification.PRIORITY_HIGH)
                .setDefaults(Notification.DEFAULT_VIBRATE);
        mNotificationManager.notify(notificationId, n.build());
    }

    private boolean checkNotificationExistence(int id) throws Exception {
        boolean isFound = false;
        for (int tries = 3; tries-- > 0;) {
            isFound = false;
            StatusBarNotification[] sbns = mNotificationManager.getActiveNotifications();
            for (StatusBarNotification sbn : sbns) {
                if (sbn.getId() == id) {
                    isFound = true;
                    break;
                }
            }
            if (isFound) {
                break;
            }
            Thread.sleep(SHORT_TIMEOUT);
        }
        Log.i(LOG_TAG, "checkNotificationExistence..." + isFound);
        return isFound;
    }

    private void swipeDown() throws Exception {
        mDevice.swipe(mDevice.getDisplayWidth() / 2, 0, mDevice.getDisplayWidth() / 2,
                mDevice.getDisplayHeight() / 2 + 50, 20);
        Thread.sleep(SHORT_TIMEOUT);
    }

    public static class ToastService extends IntentService {
        private static final String TAG = "ToastService";
        private static final String ACTION_TOAST = "toast";
        private Handler handler;

        public ToastService() {
            super(TAG);
        }

        public ToastService(String name) {
            super(name);
        }

        @Override
        public int onStartCommand(Intent intent, int flags, int startId) {
            handler = new Handler();
            return super.onStartCommand(intent, flags, startId);
        }

        @Override
        protected void onHandleIntent(Intent intent) {
            if (intent.hasExtra("text")) {
                final String text = intent.getStringExtra("text");
                handler.post(new Runnable() {
                    @Override
                    public void run() {
                        Toast.makeText(ToastService.this, text, Toast.LENGTH_LONG).show();
                        Log.v(TAG, "toast " + text);
                    }
                });
            }
        }

        public static PendingIntent getPendingIntent(Context context, String text) {
            Intent toastIntent = new Intent(context, ToastService.class);
            toastIntent.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
            toastIntent.setAction(ACTION_TOAST + ":" + text); // one per toast message
            toastIntent.putExtra("text", text);
            PendingIntent pi = PendingIntent.getService(
                    context, 58, toastIntent, PendingIntent.FLAG_UPDATE_CURRENT);
            return pi;
        }
    }
}
