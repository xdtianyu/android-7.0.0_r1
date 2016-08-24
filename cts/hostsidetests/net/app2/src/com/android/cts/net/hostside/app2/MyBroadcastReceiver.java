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

package com.android.cts.net.hostside.app2;

import static android.net.ConnectivityManager.ACTION_RESTRICT_BACKGROUND_CHANGED;

import static com.android.cts.net.hostside.app2.Common.ACTION_CHECK_NETWORK;
import static com.android.cts.net.hostside.app2.Common.ACTION_GET_COUNTERS;
import static com.android.cts.net.hostside.app2.Common.ACTION_GET_RESTRICT_BACKGROUND_STATUS;
import static com.android.cts.net.hostside.app2.Common.ACTION_RECEIVER_READY;
import static com.android.cts.net.hostside.app2.Common.ACTION_SEND_NOTIFICATION;
import static com.android.cts.net.hostside.app2.Common.EXTRA_ACTION;
import static com.android.cts.net.hostside.app2.Common.EXTRA_NOTIFICATION_ID;
import static com.android.cts.net.hostside.app2.Common.EXTRA_RECEIVER_NAME;
import static com.android.cts.net.hostside.app2.Common.MANIFEST_RECEIVER;
import static com.android.cts.net.hostside.app2.Common.TAG;
import static com.android.cts.net.hostside.app2.Common.getUid;

import android.app.Notification;
import android.app.Notification.Action;
import android.app.NotificationManager;
import android.app.PendingIntent;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.SharedPreferences;
import android.net.ConnectivityManager;
import android.net.NetworkInfo;
import android.os.Bundle;
import android.util.Log;

import java.net.HttpURLConnection;
import java.net.URL;
import java.util.concurrent.LinkedBlockingQueue;
import java.util.concurrent.TimeUnit;

/**
 * Receiver used to:
 * <ol>
 * <li>Stored received RESTRICT_BACKGROUND_CHANGED broadcasts in a shared preference.
 * <li>Returned the number of RESTRICT_BACKGROUND_CHANGED broadcasts in an ordered broadcast.
 * </ol>
 */
public class MyBroadcastReceiver extends BroadcastReceiver {

    private static final int NETWORK_TIMEOUT_MS = 15 * 1000;

    private final String mName;

    public MyBroadcastReceiver() {
        this(MANIFEST_RECEIVER);
    }

    MyBroadcastReceiver(String name) {
        Log.d(TAG, "Constructing MyBroadcastReceiver named " + name);
        mName = name;
    }

    @Override
    public void onReceive(Context context, Intent intent) {
        Log.d(TAG, "onReceive() for " + mName + ": " + intent);
        final String action = intent.getAction();
        switch (action) {
            case ACTION_RESTRICT_BACKGROUND_CHANGED:
                increaseCounter(context, action);
                break;
            case ACTION_GET_COUNTERS:
                setResultDataFromCounter(context, intent);
                break;
            case ACTION_GET_RESTRICT_BACKGROUND_STATUS:
                getRestrictBackgroundStatus(context, intent);
                break;
            case ACTION_CHECK_NETWORK:
                checkNetwork(context, intent);
                break;
            case ACTION_RECEIVER_READY:
                final String message = mName + " is ready to rumble";
                Log.d(TAG, message);
                setResultData(message);
                break;
            case ACTION_SEND_NOTIFICATION:
                sendNotification(context, intent);
                break;
            default:
                Log.e(TAG, "received unexpected action: " + action);
        }
    }

    private void increaseCounter(Context context, String action) {
        final SharedPreferences prefs = context.getSharedPreferences(mName, Context.MODE_PRIVATE);
        final int value = prefs.getInt(action, 0) + 1;
        Log.d(TAG, "increaseCounter('" + action + "'): setting '" + mName + "' to " + value);
        prefs.edit().putInt(action, value).apply();
    }

    private int getCounter(Context context, String action, String receiverName) {
        final SharedPreferences prefs = context.getSharedPreferences(receiverName,
                Context.MODE_PRIVATE);
        final int value = prefs.getInt(action, 0);
        Log.d(TAG, "getCounter('" + action + "', '" + receiverName + "'): " + value);
        return value;
    }

    private void getRestrictBackgroundStatus(Context context, Intent intent) {
        final ConnectivityManager cm = (ConnectivityManager) context
                .getSystemService(Context.CONNECTIVITY_SERVICE);
        final int apiStatus = cm.getRestrictBackgroundStatus();
        Log.d(TAG, "getRestrictBackgroundStatus: returning " + apiStatus);
        setResultData(Integer.toString(apiStatus));
    }

    private void checkNetwork(final Context context, Intent intent) {
        final ConnectivityManager cm = (ConnectivityManager) context
                .getSystemService(Context.CONNECTIVITY_SERVICE);

        String netStatus = null;
        try {
            netStatus = checkNetworkStatus(context, cm);
        } catch (InterruptedException e) {
            Log.e(TAG, "Timeout checking network status");
        }
        Log.d(TAG, "checkNetwork(): returning " + netStatus);
        setResultData(netStatus);
    }


    private static final String NETWORK_STATUS_TEMPLATE = "%s|%s|%s|%s|%s";
    /**
     * Checks whether the network is available and return a string which can then be send as a
     * result data for the ordered broadcast.
     *
     * <p>
     * The string has the following format:
     *
     * <p><pre><code>
     * NetinfoState|NetinfoDetailedState|RealConnectionCheck|RealConnectionCheckDetails|Netinfo
     * </code></pre>
     *
     * <p>Where:
     *
     * <ul>
     * <li>{@code NetinfoState}: enum value of {@link NetworkInfo.State}.
     * <li>{@code NetinfoDetailedState}: enum value of {@link NetworkInfo.DetailedState}.
     * <li>{@code RealConnectionCheck}: boolean value of a real connection check (i.e., an attempt
     *     to access an external website.
     * <li>{@code RealConnectionCheckDetails}: if HTTP output core or exception string of the real
     *     connection attempt
     * <li>{@code Netinfo}: string representation of the {@link NetworkInfo}.
     * </ul>
     *
     * For example, if the connection was established fine, the result would be something like:
     * <p><pre><code>
     * CONNECTED|CONNECTED|true|200|[type: WIFI[], state: CONNECTED/CONNECTED, reason: ...]
     * </code></pre>
     *
     */
    private String checkNetworkStatus(final Context context, final ConnectivityManager cm)
            throws InterruptedException {
        final LinkedBlockingQueue<String> result = new LinkedBlockingQueue<>(1);
        new Thread(new Runnable() {

            @Override
            public void run() {
                // TODO: connect to a hostside server instead
                final String address = "http://example.com";
                final NetworkInfo networkInfo = cm.getActiveNetworkInfo();
                Log.d(TAG, "Running checkNetworkStatus() on thread "
                        + Thread.currentThread().getName() + " for UID " + getUid(context)
                        + "\n\tactiveNetworkInfo: " + networkInfo + "\n\tURL: " + address);
                boolean checkStatus = false;
                String checkDetails = "N/A";
                try {
                    final URL url = new URL(address);
                    final HttpURLConnection conn = (HttpURLConnection) url.openConnection();
                    conn.setReadTimeout(NETWORK_TIMEOUT_MS);
                    conn.setConnectTimeout(NETWORK_TIMEOUT_MS / 2);
                    conn.setRequestMethod("GET");
                    conn.setDoInput(true);
                    conn.connect();
                    final int response = conn.getResponseCode();
                    checkStatus = true;
                    checkDetails = "HTTP response for " + address + ": " + response;
                } catch (Exception e) {
                    checkStatus = false;
                    checkDetails = "Exception getting " + address + ": " + e;
                }
                Log.d(TAG, checkDetails);
                final String status = String.format(NETWORK_STATUS_TEMPLATE,
                        networkInfo.getState().name(), networkInfo.getDetailedState().name(),
                        Boolean.toString(checkStatus), checkDetails, networkInfo);
                Log.d(TAG, "Offering " + status);
                result.offer(status);
            }
        }, mName).start();
        return result.poll(NETWORK_TIMEOUT_MS * 2, TimeUnit.MILLISECONDS);
    }

    private void setResultDataFromCounter(Context context, Intent intent) {
        final String action = intent.getStringExtra(EXTRA_ACTION);
        if (action == null) {
            Log.e(TAG, "Missing extra '" + EXTRA_ACTION + "' on " + intent);
            return;
        }
        final String receiverName = intent.getStringExtra(EXTRA_RECEIVER_NAME);
        if (receiverName == null) {
            Log.e(TAG, "Missing extra '" + EXTRA_RECEIVER_NAME + "' on " + intent);
            return;
        }
        final int counter = getCounter(context, action, receiverName);
        setResultData(String.valueOf(counter));
    }

    /**
     * Sends a system notification containing actions with pending intents to launch the app's
     * main activitiy or service.
     */
    private void sendNotification(Context context, Intent intent) {
        final int notificationId = intent.getIntExtra(EXTRA_NOTIFICATION_ID, -1);
        final Intent serviceIntent = new Intent(context, MyService.class);
        final PendingIntent pendingIntent = PendingIntent.getService(context, 0, serviceIntent, 0);
        final Bundle badBundle = new Bundle();
        badBundle.putCharSequence("parcelable", "I am not");
        final Action action = new Action.Builder(
                R.drawable.ic_notification, "ACTION", pendingIntent)
                .addExtras(badBundle)
                .build();

        final Notification notification = new Notification.Builder(context)
                .setSmallIcon(R.drawable.ic_notification)
                .setContentTitle("Light, Cameras...")
                .setContentIntent(pendingIntent)
                .addAction(action)
                .build();
        ((NotificationManager) context.getSystemService(Context.NOTIFICATION_SERVICE))
            .notify(notificationId, notification);
    }
}
