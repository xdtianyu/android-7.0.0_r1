/*
 * Copyright (C) 2013 The Android Open Source Project
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

package com.android.cts.verifier.notifications;

import android.annotation.SuppressLint;
import android.app.Activity;
import android.app.Notification;
import android.provider.Settings.Secure;
import android.util.Log;
import android.view.View;
import android.view.ViewGroup;

import com.android.cts.verifier.R;

import org.json.JSONException;
import org.json.JSONObject;

import java.util.ArrayList;
import java.util.HashSet;
import java.util.List;
import java.util.Set;
import java.util.UUID;

import static com.android.cts.verifier.notifications.MockListener.*;

public class NotificationListenerVerifierActivity extends InteractiveVerifierActivity
        implements Runnable {
    private static final String TAG = "NoListenerVerifier";

    private String mTag1;
    private String mTag2;
    private String mTag3;
    private int mIcon1;
    private int mIcon2;
    private int mIcon3;
    private int mId1;
    private int mId2;
    private int mId3;
    private long mWhen1;
    private long mWhen2;
    private long mWhen3;
    private int mFlag1;
    private int mFlag2;
    private int mFlag3;

    @Override
    int getTitleResource() {
        return R.string.nls_test;
    }

    @Override
    int getInstructionsResource() {
        return R.string.nls_info;
    }

    // Test Setup

    @Override
    protected List<InteractiveTestCase> createTestItems() {
        List<InteractiveTestCase> tests = new ArrayList<>(9);
        tests.add(new IsEnabledTest());
        tests.add(new ServiceStartedTest());
        tests.add(new NotificationRecievedTest());
        tests.add(new DataIntactTest());
        tests.add(new DismissOneTest());
        tests.add(new DismissAllTest());
        tests.add(new IsDisabledTest());
        tests.add(new ServiceStoppedTest());
        tests.add(new NotificationNotReceivedTest());
        return tests;
    }

    @SuppressLint("NewApi")
    private void sendNotifications() {
        mTag1 = UUID.randomUUID().toString();
        mTag2 = UUID.randomUUID().toString();
        mTag3 = UUID.randomUUID().toString();

        mNm.cancelAll();

        mWhen1 = System.currentTimeMillis() + 1;
        mWhen2 = System.currentTimeMillis() + 2;
        mWhen3 = System.currentTimeMillis() + 3;

        mIcon1 = R.drawable.ic_stat_alice;
        mIcon2 = R.drawable.ic_stat_bob;
        mIcon3 = R.drawable.ic_stat_charlie;

        mId1 = NOTIFICATION_ID + 1;
        mId2 = NOTIFICATION_ID + 2;
        mId3 = NOTIFICATION_ID + 3;

        mPackageString = "com.android.cts.verifier";

        Notification n1 = new Notification.Builder(mContext)
                .setContentTitle("ClearTest 1")
                .setContentText(mTag1.toString())
                .setPriority(Notification.PRIORITY_LOW)
                .setSmallIcon(mIcon1)
                .setWhen(mWhen1)
                .setDeleteIntent(makeIntent(1, mTag1))
                .setOnlyAlertOnce(true)
                .build();
        mNm.notify(mTag1, mId1, n1);
        mFlag1 = Notification.FLAG_ONLY_ALERT_ONCE;

        Notification n2 = new Notification.Builder(mContext)
                .setContentTitle("ClearTest 2")
                .setContentText(mTag2.toString())
                .setPriority(Notification.PRIORITY_HIGH)
                .setSmallIcon(mIcon2)
                .setWhen(mWhen2)
                .setDeleteIntent(makeIntent(2, mTag2))
                .setAutoCancel(true)
                .build();
        mNm.notify(mTag2, mId2, n2);
        mFlag2 = Notification.FLAG_AUTO_CANCEL;

        Notification n3 = new Notification.Builder(mContext)
                .setContentTitle("ClearTest 3")
                .setContentText(mTag3.toString())
                .setPriority(Notification.PRIORITY_LOW)
                .setSmallIcon(mIcon3)
                .setWhen(mWhen3)
                .setDeleteIntent(makeIntent(3, mTag3))
                .setAutoCancel(true)
                .setOnlyAlertOnce(true)
                .build();
        mNm.notify(mTag3, mId3, n3);
        mFlag3 = Notification.FLAG_ONLY_ALERT_ONCE | Notification.FLAG_AUTO_CANCEL;
    }

    // Tests

    private class NotificationRecievedTest extends InteractiveTestCase {
        @Override
        View inflate(ViewGroup parent) {
            return createAutoItem(parent, R.string.nls_note_received);

        }

        @Override
        void setUp() {
            sendNotifications();
            status = READY;
            // wait for notifications to move through the system
            delay();
        }

        @Override
        void test() {
            MockListener.probeListenerPosted(mContext,
                    new MockListener.StringListResultCatcher() {
                        @Override
                        public void accept(List<String> result) {
                            if (result != null && result.size() > 0 && result.contains(mTag1)) {
                                status = PASS;
                            } else {
                                logFail();
                                status = FAIL;
                            }
                            next();
                        }
                    });
            delay();  // in case the catcher never returns
        }
    }

    private class DataIntactTest extends InteractiveTestCase {
        @Override
        View inflate(ViewGroup parent) {
            return createAutoItem(parent, R.string.nls_payload_intact);
        }

        @Override
        void test() {
            MockListener.probeListenerPayloads(mContext,
                    new MockListener.StringListResultCatcher() {
                        @Override
                        public void accept(List<String> result) {
                            Set<String> found = new HashSet<String>();
                            if (result == null || result.size() == 0) {
                                status = FAIL;
                                return;
                            }
                            boolean pass = true;
                            for (String payloadData : result) {
                                try {
                                    JSONObject payload = new JSONObject(payloadData);
                                    pass &= checkEquals(mPackageString,
                                            payload.getString(JSON_PACKAGE),
                                            "data integrity test: notification package (%s, %s)");
                                    String tag = payload.getString(JSON_TAG);
                                    if (mTag1.equals(tag)) {
                                        found.add(mTag1);
                                        pass &= checkEquals(mIcon1, payload.getInt(JSON_ICON),
                                                "data integrity test: notification icon (%d, %d)");
                                        pass &= checkFlagSet(mFlag1, payload.getInt(JSON_FLAGS),
                                                "data integrity test: notification flags (%d, %d)");
                                        pass &= checkEquals(mId1, payload.getInt(JSON_ID),
                                                "data integrity test: notification ID (%d, %d)");
                                        pass &= checkEquals(mWhen1, payload.getLong(JSON_WHEN),
                                                "data integrity test: notification when (%d, %d)");
                                    } else if (mTag2.equals(tag)) {
                                        found.add(mTag2);
                                        pass &= checkEquals(mIcon2, payload.getInt(JSON_ICON),
                                                "data integrity test: notification icon (%d, %d)");
                                        pass &= checkFlagSet(mFlag2, payload.getInt(JSON_FLAGS),
                                                "data integrity test: notification flags (%d, %d)");
                                        pass &= checkEquals(mId2, payload.getInt(JSON_ID),
                                                "data integrity test: notification ID (%d, %d)");
                                        pass &= checkEquals(mWhen2, payload.getLong(JSON_WHEN),
                                                "data integrity test: notification when (%d, %d)");
                                    } else if (mTag3.equals(tag)) {
                                        found.add(mTag3);
                                        pass &= checkEquals(mIcon3, payload.getInt(JSON_ICON),
                                                "data integrity test: notification icon (%d, %d)");
                                        pass &= checkFlagSet(mFlag3, payload.getInt(JSON_FLAGS),
                                                "data integrity test: notification flags (%d, %d)");
                                        pass &= checkEquals(mId3, payload.getInt(JSON_ID),
                                                "data integrity test: notification ID (%d, %d)");
                                        pass &= checkEquals(mWhen3, payload.getLong(JSON_WHEN),
                                                "data integrity test: notification when (%d, %d)");
                                    } else {
                                        pass = false;
                                        logFail("unexpected notification tag: " + tag);
                                    }
                                } catch (JSONException e) {
                                    pass = false;
                                    Log.e(TAG, "failed to unpack data from mocklistener", e);
                                }
                            }

                            pass &= found.size() == 3;
                            status = pass ? PASS : FAIL;
                            next();
                        }
                    });
            delay();  // in case the catcher never returns
        }

        @Override
        void tearDown() {
            mNm.cancelAll();
            MockListener.resetListenerData(mContext);
            delay();
        }
    }

    private class DismissOneTest extends InteractiveTestCase {
        @Override
        View inflate(ViewGroup parent) {
            return createAutoItem(parent, R.string.nls_clear_one);
        }

        @Override
        void setUp() {
            sendNotifications();
            status = READY;
            delay();
        }

        @Override
        void test() {
            if (status == READY) {
                MockListener.clearOne(mContext, mTag1, mId1);
                status = RETEST;
            } else {
                MockListener.probeListenerRemoved(mContext,
                        new MockListener.StringListResultCatcher() {
                            @Override
                            public void accept(List<String> result) {
                                if (result != null && result.size() != 0
                                        && result.contains(mTag1)
                                        && !result.contains(mTag2)
                                        && !result.contains(mTag3)) {
                                    status = PASS;
                                } else {
                                    logFail();
                                    status = FAIL;
                                }
                                next();
                            }
                        });
            }
            delay();
        }

        @Override
        void tearDown() {
            mNm.cancelAll();
            MockListener.resetListenerData(mContext);
            delay();
        }
    }

    private class DismissAllTest extends InteractiveTestCase {
        @Override
        View inflate(ViewGroup parent) {
            return createAutoItem(parent, R.string.nls_clear_all);
        }

        @Override
        void setUp() {
            sendNotifications();
            status = READY;
            delay();
        }

        @Override
        void test() {
            if (status == READY) {
                MockListener.clearAll(mContext);
                status = RETEST;
            } else {
                MockListener.probeListenerRemoved(mContext,
                        new MockListener.StringListResultCatcher() {
                            @Override
                            public void accept(List<String> result) {
                                if (result != null && result.size() != 0
                                        && result.contains(mTag1)
                                        && result.contains(mTag2)
                                        && result.contains(mTag3)) {
                                    status = PASS;
                                } else {
                                    logFail();
                                    status = FAIL;
                                }
                                next();
                            }
                        });
            }
            delay();  // in case the catcher never returns
        }

        @Override
        void tearDown() {
            mNm.cancelAll();
            MockListener.resetListenerData(mContext);
            delay();
        }
    }

    private class IsDisabledTest extends InteractiveTestCase {
        @Override
        View inflate(ViewGroup parent) {
            return createNlsSettingsItem(parent, R.string.nls_disable_service);
        }

        @Override
        boolean autoStart() {
            return true;
        }

        @Override
        void test() {
            String listeners = Secure.getString(getContentResolver(),
                    ENABLED_NOTIFICATION_LISTENERS);
            if (listeners == null || !listeners.contains(LISTENER_PATH)) {
                status = PASS;
            } else {
                status = WAIT_FOR_USER;
            }
            next();
        }

        @Override
        void tearDown() {
            MockListener.resetListenerData(mContext);
            delay();
        }
    }

    private class ServiceStoppedTest extends InteractiveTestCase {
        @Override
        View inflate(ViewGroup parent) {
            return createAutoItem(parent, R.string.nls_service_stopped);
        }

        @Override
        void test() {
            MockListener.probeListenerStatus(mContext,
                    new MockListener.StatusCatcher() {
                        @Override
                        public void accept(int result) {
                            if (result == Activity.RESULT_OK) {
                                logFail();
                                status = FAIL;
                            } else {
                                status = PASS;
                            }
                            next();
                        }
                    });
            delay();  // in case the catcher never returns
        }

        @Override
        void tearDown() {
            // wait for intent to move through the system
            delay();
        }
    }

    private class NotificationNotReceivedTest extends InteractiveTestCase {
        @Override
        View inflate(ViewGroup parent) {
            return createAutoItem(parent, R.string.nls_note_missed);

        }

        @Override
        void setUp() {
            sendNotifications();
            status = READY;
            delay();
        }

        @Override
        void test() {
            MockListener.probeListenerPosted(mContext,
                    new MockListener.StringListResultCatcher() {
                        @Override
                        public void accept(List<String> result) {
                            if (result == null || result.size() == 0) {
                                status = PASS;
                            } else {
                                logFail();
                                status = FAIL;
                            }
                            next();
                        }
                    });
            delay();  // in case the catcher never returns
        }

        @Override
        void tearDown() {
            mNm.cancelAll();
            MockListener.resetListenerData(mContext);
            delay();
        }
    }
}
