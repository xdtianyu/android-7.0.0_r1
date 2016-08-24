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

import android.app.Activity;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.os.Bundle;
import android.service.notification.NotificationListenerService;
import android.service.notification.StatusBarNotification;
import android.util.ArrayMap;
import android.util.Log;

import org.json.JSONException;
import org.json.JSONObject;

import java.lang.reflect.InvocationTargetException;
import java.util.ArrayList;
import java.util.HashSet;
import java.util.List;
import java.util.Set;

public class MockListener extends NotificationListenerService {
    static final String TAG = "MockListener";

    static final String SERVICE_BASE = "android.service.notification.cts.";
    static final String SERVICE_CHECK = SERVICE_BASE + "SERVICE_CHECK";
    static final String SERVICE_POSTED = SERVICE_BASE + "SERVICE_POSTED";
    static final String SERVICE_PAYLOADS = SERVICE_BASE + "SERVICE_PAYLOADS";
    static final String SERVICE_REMOVED = SERVICE_BASE + "SERVICE_REMOVED";
    static final String SERVICE_RESET = SERVICE_BASE + "SERVICE_RESET";
    static final String SERVICE_CLEAR_ONE = SERVICE_BASE + "SERVICE_CLEAR_ONE";
    static final String SERVICE_CLEAR_ALL = SERVICE_BASE + "SERVICE_CLEAR_ALL";
    public static final String SERVICE_ORDER = SERVICE_BASE + "SERVICE_ORDER";
    public static final String SERVICE_DND = SERVICE_BASE + "SERVICE_DND";

    static final String EXTRA_PAYLOAD = "PAYLOAD";
    static final String EXTRA_INT = "INT";
    static final String EXTRA_TAG = "TAG";
    static final String EXTRA_CODE = "CODE";

    static final int RESULT_TIMEOUT = Activity.RESULT_FIRST_USER;
    static final int RESULT_NO_SERVER = Activity.RESULT_FIRST_USER + 1;

    public static final String JSON_FLAGS = "flag";
    public static final String JSON_ICON = "icon";
    public static final String JSON_ID = "id";
    public static final String JSON_PACKAGE = "pkg";
    public static final String JSON_WHEN = "when";
    public static final String JSON_TAG = "tag";
    public static final String JSON_RANK = "rank";
    public static final String JSON_AMBIENT = "ambient";
    public static final String JSON_MATCHES_ZEN_FILTER = "matches_zen_filter";

    private ArrayList<String> mPosted = new ArrayList<String>();
    private ArrayMap<String, JSONObject> mNotifications = new ArrayMap<>();
    private ArrayMap<String, String> mNotificationKeys = new ArrayMap<>();
    private ArrayList<String> mRemoved = new ArrayList<String>();
    private ArrayList<String> mOrder = new ArrayList<>();
    private Set<String> mTestPackages = new HashSet<>();
    private BroadcastReceiver mReceiver;
    private int mDND = -1;

    @Override
    public void onCreate() {
        super.onCreate();
        Log.d(TAG, "created");

        mTestPackages.add("com.android.cts.verifier");
        mTestPackages.add("com.android.cts.robot");

        mPosted = new ArrayList<String>();
        mRemoved = new ArrayList<String>();

        mReceiver = new BroadcastReceiver() {
            @Override
            public void onReceive(Context context, Intent intent) {
                String action = intent.getAction();
                if (SERVICE_CHECK.equals(action)) {
                    Log.d(TAG, "SERVICE_CHECK");
                    setResultCode(Activity.RESULT_OK);
                } else if (SERVICE_POSTED.equals(action)) {
                    Log.d(TAG, "SERVICE_POSTED");
                    Bundle bundle = new Bundle();
                    bundle.putStringArrayList(EXTRA_PAYLOAD, mPosted);
                    setResultExtras(bundle);
                    setResultCode(Activity.RESULT_OK);
                } else if (SERVICE_DND.equals(action)) {
                    Log.d(TAG, "SERVICE_DND");
                    Bundle bundle = new Bundle();
                    bundle.putInt(EXTRA_INT, mDND);
                    setResultExtras(bundle);
                    setResultCode(Activity.RESULT_OK);
                } else if (SERVICE_ORDER.equals(action)) {
                    Log.d(TAG, "SERVICE_ORDER");
                    Bundle bundle = new Bundle();
                    bundle.putStringArrayList(EXTRA_PAYLOAD, mOrder);
                    setResultExtras(bundle);
                    setResultCode(Activity.RESULT_OK);
                } else if (SERVICE_PAYLOADS.equals(action)) {
                    Log.d(TAG, "SERVICE_PAYLOADS");
                    Bundle bundle = new Bundle();
                    ArrayList<String> payloadData = new ArrayList<>(mNotifications.size());
                    for (JSONObject payload: mNotifications.values()) {
                        payloadData.add(payload.toString());
                    }
                    bundle.putStringArrayList(EXTRA_PAYLOAD, payloadData);
                    setResultExtras(bundle);
                    setResultCode(Activity.RESULT_OK);
                } else if (SERVICE_REMOVED.equals(action)) {
                    Log.d(TAG, "SERVICE_REMOVED");
                    Bundle bundle = new Bundle();
                    bundle.putStringArrayList(EXTRA_PAYLOAD, mRemoved);
                    setResultExtras(bundle);
                    setResultCode(Activity.RESULT_OK);
                } else if (SERVICE_CLEAR_ONE.equals(action)) {
                    Log.d(TAG, "SERVICE_CLEAR_ONE");
                    String tag = intent.getStringExtra(EXTRA_TAG);
                    String key = mNotificationKeys.get(tag);
                    if (key != null) {
                        MockListener.this.cancelNotification(key);
                    } else {
                        Log.w(TAG, "Notification does not exist: " + tag);
                    }
                } else if (SERVICE_CLEAR_ALL.equals(action)) {
                    Log.d(TAG, "SERVICE_CLEAR_ALL");
                    MockListener.this.cancelAllNotifications();
                } else if (SERVICE_RESET.equals(action)) {
                    Log.d(TAG, "SERVICE_RESET");
                    resetData();
                } else {
                    Log.w(TAG, "unknown action");
                    setResultCode(Activity.RESULT_CANCELED);
                }
            }
        };
        IntentFilter filter = new IntentFilter();
        filter.addAction(SERVICE_CHECK);
        filter.addAction(SERVICE_DND);
        filter.addAction(SERVICE_POSTED);
        filter.addAction(SERVICE_ORDER);
        filter.addAction(SERVICE_PAYLOADS);
        filter.addAction(SERVICE_REMOVED);
        filter.addAction(SERVICE_CLEAR_ONE);
        filter.addAction(SERVICE_CLEAR_ALL);
        filter.addAction(SERVICE_RESET);
        registerReceiver(mReceiver, filter);
    }

    @Override
    public void onDestroy() {
        super.onDestroy();
        unregisterReceiver(mReceiver);
        mReceiver = null;
        Log.d(TAG, "destroyed");
    }

    @Override
    public void onListenerConnected() {
        super.onListenerConnected();
        mDND = getCurrentInterruptionFilter();
        Log.d(TAG, "initial value of CurrentInterruptionFilter is " + mDND);
    }

    @Override
    public void onInterruptionFilterChanged(int interruptionFilter) {
        super.onInterruptionFilterChanged(interruptionFilter);
        mDND = interruptionFilter;
        Log.d(TAG, "value of CurrentInterruptionFilter changed to " + mDND);
    }

    public void resetData() {
        mPosted.clear();
        mNotifications.clear();
        mRemoved.clear();
        mOrder.clear();
    }

    @Override
    public void onNotificationRankingUpdate(RankingMap rankingMap) {
        String[] orderedKeys = rankingMap.getOrderedKeys();
        mOrder.clear();
        Ranking rank = new Ranking();
        for( int i = 0; i < orderedKeys.length; i++) {
            String key = orderedKeys[i];
            mOrder.add(key);
            rankingMap.getRanking(key, rank);
            JSONObject note = mNotifications.get(key);
            if (note != null) {
                try {
                    note.put(JSON_RANK, rank.getRank());
                    note.put(JSON_AMBIENT, rank.isAmbient());
                    note.put(JSON_MATCHES_ZEN_FILTER, rank.matchesInterruptionFilter());
                } catch (JSONException e) {
                    Log.e(TAG, "failed to pack up notification payload", e);
                }
            }
        }
    }

    @Override
    public void onNotificationPosted(StatusBarNotification sbn, RankingMap rankingMap) {
        if (!mTestPackages.contains(sbn.getPackageName())) { return; }
        Log.d(TAG, "posted: " + sbn.getTag());
        mPosted.add(sbn.getTag());
        JSONObject notification = new JSONObject();
        try {
            notification.put(JSON_TAG, sbn.getTag());
            notification.put(JSON_ID, sbn.getId());
            notification.put(JSON_PACKAGE, sbn.getPackageName());
            notification.put(JSON_WHEN, sbn.getNotification().when);
            notification.put(JSON_ICON, sbn.getNotification().icon);
            notification.put(JSON_FLAGS, sbn.getNotification().flags);
            mNotifications.put(sbn.getKey(), notification);
            mNotificationKeys.put(sbn.getTag(), sbn.getKey());
        } catch (JSONException e) {
            Log.e(TAG, "failed to pack up notification payload", e);
        }
        onNotificationRankingUpdate(rankingMap);
    }

    @Override
    public void onNotificationRemoved(StatusBarNotification sbn, RankingMap rankingMap) {
        Log.d(TAG, "removed: " + sbn.getTag());
        mRemoved.add(sbn.getTag());
        mNotifications.remove(sbn.getKey());
        mNotificationKeys.remove(sbn.getTag());
        onNotificationRankingUpdate(rankingMap);
    }

    public static void resetListenerData(Context context) {
        sendCommand(context, SERVICE_RESET, null, 0);
    }

    public static void probeListenerStatus(Context context, StatusCatcher catcher) {
        requestStatus(context, SERVICE_CHECK, catcher);
    }

    public static void probeFilter(Context context, IntegerResultCatcher catcher) {
        requestIntegerResult(context, SERVICE_DND, catcher);
    }

    public static void probeListenerPosted(Context context, StringListResultCatcher catcher) {
        requestStringListResult(context, SERVICE_POSTED, catcher);
    }

    public static void probeListenerOrder(Context context, StringListResultCatcher catcher) {
        requestStringListResult(context, SERVICE_ORDER, catcher);
    }

    public static void probeListenerPayloads(Context context, StringListResultCatcher catcher) {
        requestStringListResult(context, SERVICE_PAYLOADS, catcher);
    }

    public static void probeListenerRemoved(Context context, StringListResultCatcher catcher) {
        requestStringListResult(context, SERVICE_REMOVED, catcher);
    }

    public static void clearOne(Context context, String tag, int code) {
        sendCommand(context, SERVICE_CLEAR_ONE, tag, code);
    }

    public static void clearAll(Context context) {
        sendCommand(context, SERVICE_CLEAR_ALL, null, 0);
    }

    private static void sendCommand(Context context, String action, String tag, int code) {
        Intent broadcast = new Intent(action);
        if (tag != null) {
            broadcast.putExtra(EXTRA_TAG, tag);
            broadcast.putExtra(EXTRA_CODE, code);
        }
        context.sendBroadcast(broadcast);
    }

    public abstract static class StatusCatcher extends BroadcastReceiver {
        @Override
        public void onReceive(Context context, Intent intent) {
            accept(Integer.valueOf(getResultCode()));
        }

        abstract public void accept(int result);
    }

    private static void requestStatus(Context context, String action,
            StatusCatcher catcher) {
        Intent broadcast = new Intent(action);
        context.sendOrderedBroadcast(broadcast, null, catcher, null, RESULT_NO_SERVER, null, null);
    }

    public abstract static class IntegerResultCatcher extends BroadcastReceiver {
        @Override
        public void onReceive(Context context, Intent intent) {
            accept(getResultExtras(true).getInt(EXTRA_INT, -1));
        }

        abstract public void accept(int result);
    }

    private static void requestIntegerResult(Context context, String action,
            IntegerResultCatcher catcher) {
        Intent broadcast = new Intent(action);
        context.sendOrderedBroadcast(broadcast, null, catcher, null, RESULT_NO_SERVER, null, null);
    }

    public abstract static class StringListResultCatcher extends BroadcastReceiver {
        @Override
        public void onReceive(Context context, Intent intent) {
            accept(getResultExtras(true).getStringArrayList(EXTRA_PAYLOAD));
        }

        abstract public void accept(List<String> result);
    }

    private static void requestStringListResult(Context context, String action,
            StringListResultCatcher catcher) {
        Intent broadcast = new Intent(action);
        context.sendOrderedBroadcast(broadcast, null, catcher, null, RESULT_NO_SERVER, null, null);
    }
}
