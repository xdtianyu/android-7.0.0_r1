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
package com.android.bluetooth.pbapclient;

import com.android.bluetooth.pbapclient.BluetoothPbapCard;
import com.android.bluetooth.pbapclient.BluetoothPbapClient;
import android.os.Handler;
import android.os.Message;
import android.util.Log;

import com.android.vcard.VCardEntry;

import java.util.List;

public class PbapHandler extends Handler {
    private static final String TAG = "PbapHandler";

    private PbapListener mListener;

    public PbapHandler(PbapListener listener) {
        mListener = listener;
    }

    @Override
    public void handleMessage(Message msg) {
        Log.d(TAG, "handleMessage " + msg.what);
        switch (msg.what) {
            case BluetoothPbapClient.EVENT_PULL_PHONE_BOOK_DONE:
                Log.d(TAG, "EVENT_PULL_PHONE_BOOK_DONE with entries " +
                    ((List<VCardEntry>)(msg.obj)).size());
                mListener.onPhoneBookPullDone((List<VCardEntry>) msg.obj);
                break;
            case BluetoothPbapClient.EVENT_PULL_PHONE_BOOK_ERROR:
                Log.d(TAG, "EVENT_PULL_PHONE_BOOK_ERROR");
                mListener.onPhoneBookError();
                break;
            case BluetoothPbapClient.EVENT_SESSION_CONNECTED:
                Log.d(TAG, "EVENT_SESSION_CONNECTED");
                mListener.onPbapClientConnected(true);
                break;
            case BluetoothPbapClient.EVENT_SESSION_DISCONNECTED:
                Log.d(TAG, "EVENT_SESSION_DISCONNECTED");
                mListener.onPbapClientConnected(false);
                break;

            // TODO(rni): Actually handle these cases.
            case BluetoothPbapClient.EVENT_SET_PHONE_BOOK_DONE:
                Log.d(TAG, "EVENT_SET_PHONE_BOOK_DONE");
                break;
            case BluetoothPbapClient.EVENT_PULL_VCARD_LISTING_DONE:
                Log.d(TAG, "EVENT_PULL_VCARD_LISTING_DONE");
                break;
            case BluetoothPbapClient.EVENT_PULL_VCARD_ENTRY_DONE:
                Log.d(TAG, "EVENT_PULL_VCARD_ENTRY_DONE");
                break;
            case BluetoothPbapClient.EVENT_PULL_PHONE_BOOK_SIZE_DONE:
                Log.d(TAG, "EVENT_PULL_PHONE_BOOK_SIZE_DONE");
                break;
            case BluetoothPbapClient.EVENT_PULL_VCARD_LISTING_SIZE_DONE:
                Log.d(TAG, "EVENT_PULL_VCARD_LISTING_SIZE_DONE");
                break;
            case BluetoothPbapClient.EVENT_SET_PHONE_BOOK_ERROR:
                Log.d(TAG, "EVENT_SET_PHONE_BOOK_ERROR");
                break;
            case BluetoothPbapClient.EVENT_PULL_VCARD_LISTING_ERROR:
                Log.d(TAG, "EVENT_PULL_VCARD_LISTING_ERROR");
                break;
            case BluetoothPbapClient.EVENT_PULL_VCARD_ENTRY_ERROR:
                Log.d(TAG, "EVENT_PULL_VCARD_ENTRY_ERROR");
                break;
            case BluetoothPbapClient.EVENT_PULL_PHONE_BOOK_SIZE_ERROR:
                Log.d(TAG, "EVENT_PULL_PHONE_BOOK_SIZE_ERROR");
                break;
            case BluetoothPbapClient.EVENT_PULL_VCARD_LISTING_SIZE_ERROR:
                Log.d(TAG, "EVENT_PULL_VCARD_LISTING_SIZE_ERROR");
                break;
            case BluetoothPbapClient.EVENT_SESSION_AUTH_REQUESTED:
                Log.d(TAG, "EVENT_SESSION_AUTH_REQUESTED");
                break;
            case BluetoothPbapClient.EVENT_SESSION_AUTH_TIMEOUT:
                Log.d(TAG, "EVENT_SESSION_AUTH_TIMEOUT");
                break;
            default:
                Log.e(TAG, "unknown message " + msg);
                break;
        }
    }

    public interface PbapListener {
        void onPhoneBookPullDone(List<VCardEntry> entries);
        void onPhoneBookError();
        void onPbapClientConnected(boolean status);
    }
}
