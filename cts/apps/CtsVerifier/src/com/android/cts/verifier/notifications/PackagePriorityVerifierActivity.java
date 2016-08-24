/*
 * Copyright (C) 2014 The Android Open Source Project
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

import android.app.Notification;
import android.content.Intent;
import android.content.pm.PackageManager;
import android.util.Log;
import android.view.View;
import android.view.ViewGroup;
import com.android.cts.verifier.R;

import java.util.ArrayList;
import java.util.List;

/**
 * Tests that the notification ranker honors user preferences about package priority.
 * Users can, in Settings, specify a package as being high priority. This should
 * result in the notificaitons from that package being ranked higher than those from
 * other packages.
 */
public class PackagePriorityVerifierActivity
        extends InteractiveVerifierActivity {
    private static final String ACTION_POST = "com.android.cts.robot.ACTION_POST";
    private static final String ACTION_CANCEL = "com.android.cts.robot.ACTION_CANCEL";
    private static final String EXTRA_ID = "ID";
    private static final String EXTRA_NOTIFICATION = "NOTIFICATION";
    private static final String NOTIFICATION_BOT_PACKAGE = "com.android.cts.robot";
    private CharSequence mAppLabel;

    @Override
    int getTitleResource() {
        return R.string.package_priority_test;
    }

    @Override
    int getInstructionsResource() {
        return R.string.package_priority_info;
    }

    // Test Setup

    @Override
    protected List<InteractiveTestCase> createTestItems() {
        mAppLabel = getString(R.string.app_name);
        List<InteractiveTestCase> tests = new ArrayList<>(17);
        tests.add(new CheckForBot());
        tests.add(new IsEnabledTest());
        tests.add(new ServiceStartedTest());
        tests.add(new WaitForSetPriorityDefault());
        tests.add(new DefaultOrderTest());
        tests.add(new WaitForSetPriorityHigh());
        tests.add(new PackagePriorityOrderTest());
        return tests;
    }

    // Tests

    /** Make sure the helper package is installed. */
    protected class CheckForBot extends InteractiveTestCase {
        @Override
        View inflate(ViewGroup parent) {
            return createAutoItem(parent, R.string.package_priority_bot);
        }

        @Override
        void test() {
            PackageManager pm = mContext.getPackageManager();
            try {
                pm.getPackageInfo(NOTIFICATION_BOT_PACKAGE, 0);
                status = PASS;
            } catch (PackageManager.NameNotFoundException e) {
                status = FAIL;
                logFail("You must install the CTS Robot helper, aka " + NOTIFICATION_BOT_PACKAGE);
            }
            next();
        }
    }

    /** Wait for the user to set the target package priority to default. */
    protected class WaitForSetPriorityDefault extends InteractiveTestCase {
        @Override
        View inflate(ViewGroup parent) {
            return createRetryItem(parent, R.string.package_priority_default, mAppLabel);
        }

        @Override
        void setUp() {
            Log.i("WaitForSetPriorityDefault", "waiting for user");
            status = WAIT_FOR_USER;
        }

        @Override
        void test() {
            status = PASS;
            next();
        }

        @Override
        void tearDown() {
            MockListener.resetListenerData(mContext);
            delay();
        }
    }

    /** Wait for the user to set the target package priority to high. */
    protected class WaitForSetPriorityHigh extends InteractiveTestCase {
        @Override
        View inflate(ViewGroup parent) {
            return createRetryItem(parent, R.string.package_priority_high, mAppLabel);
        }

        @Override
        void setUp() {
            Log.i("WaitForSetPriorityHigh", "waiting for user");
            status = WAIT_FOR_USER;
        }

        @Override
        void test() {
            status = PASS;
            next();
        }

        @Override
        void tearDown() {
            MockListener.resetListenerData(mContext);
            delay();
        }
    }

    /**
     * With default priority, the notifcations should be reverse-ordered by time.
     * A is before B, and therefor should B should rank before A.
     */
    protected class DefaultOrderTest extends InteractiveTestCase {
        @Override
        View inflate(ViewGroup parent) {
            return createAutoItem(parent, R.string.attention_default_order);
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
            MockListener.probeListenerOrder(mContext,
                    new MockListener.StringListResultCatcher() {
                        @Override
                        public void accept(List<String> orderedKeys) {
                            int rankA = indexOfPackageInKeys(orderedKeys, getPackageName());
                            int rankB = indexOfPackageInKeys(orderedKeys, NOTIFICATION_BOT_PACKAGE);
                            if (rankB != -1 && rankB < rankA) {
                                status = PASS;
                            } else {
                                logFail("expected rankA (" + rankA + ") > rankB (" + rankB + ")");
                                status = FAIL;
                            }
                            next();
                        }
                    });
            delay();  // in case the catcher never returns
        }

        @Override
        void tearDown() {
            cancelNotifications();
            MockListener.resetListenerData(mContext);
            delay();
        }
    }

    /**
     * With higher package priority, A should rank above B.
     */
    protected class PackagePriorityOrderTest extends InteractiveTestCase {
        @Override
        View inflate(ViewGroup parent) {
            return createAutoItem(parent, R.string.package_priority_user_order);
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
            MockListener.probeListenerOrder(mContext,
                    new MockListener.StringListResultCatcher() {
                        @Override
                        public void accept(List<String> orderedKeys) {
                            int rankA = indexOfPackageInKeys(orderedKeys, getPackageName());
                            int rankB = indexOfPackageInKeys(orderedKeys, NOTIFICATION_BOT_PACKAGE);
                            if (rankA != -1 && rankA < rankB) {
                                status = PASS;
                            } else {
                                logFail("expected rankA (" + rankA + ") < rankB (" + rankB + ")");
                                status = FAIL;
                            }
                            next();
                        }
                    });
            delay();  // in case the catcher never returns
        }

        @Override
        void tearDown() {
            cancelNotifications();
            MockListener.resetListenerData(mContext);
            delay();
        }
    }
    // Utilities

    private void sendNotifications() {
        // post ours first, with an explicit time in the past to avoid any races.
        Notification.Builder alice = new Notification.Builder(mContext)
                .setContentTitle("alice title")
                .setContentText("alice content")
                .setSmallIcon(R.drawable.ic_stat_alice)
                .setWhen(System.currentTimeMillis() - 10000L)
                .setPriority(Notification.PRIORITY_DEFAULT);
        mNm.notify(0, alice.build());

        // then post theirs, so it should be higher by default due to recency
        Notification.Builder bob = new Notification.Builder(mContext)
                .setContentTitle("bob title")
                .setContentText("bob content")
                .setSmallIcon(android.R.drawable.stat_notify_error) // must be global resource
                .setWhen(System.currentTimeMillis())
                .setPriority(Notification.PRIORITY_DEFAULT);
        Intent postIntent = new Intent(ACTION_POST);
        postIntent.setPackage(NOTIFICATION_BOT_PACKAGE);
        postIntent.putExtra(EXTRA_ID, 0);
        postIntent.putExtra(EXTRA_NOTIFICATION, bob.build());
        postIntent.addFlags(Intent.FLAG_INCLUDE_STOPPED_PACKAGES);
        sendBroadcast(postIntent);
    }

    private void cancelNotifications() {
        //cancel ours
        mNm.cancelAll();
        //cancel theirs
        Intent cancelIntent = new Intent(ACTION_CANCEL);
        cancelIntent.setPackage(NOTIFICATION_BOT_PACKAGE);
        cancelIntent.putExtra(EXTRA_ID, 0);
        cancelIntent.addFlags(Intent.FLAG_INCLUDE_STOPPED_PACKAGES);
        sendBroadcast(cancelIntent);
    }

    /** Search a list of notification keys for a given packageName. */
    private int indexOfPackageInKeys(List<String> orderedKeys, String packageName) {
        for (int i = 0; i < orderedKeys.size(); i++) {
            if (orderedKeys.get(i).contains(packageName)) {
                return i;
            }
        }
        return -1;
    }
}
