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

import android.accounts.Account;
import android.accounts.AccountManager;
import android.app.Service;
import android.bluetooth.BluetoothAdapter;
import android.bluetooth.BluetoothDevice;
import android.bluetooth.BluetoothProfile;
import android.content.ContentResolver;
import android.content.Context;
import android.content.Intent;
import android.os.AsyncTask;
import android.os.Bundle;
import android.os.Handler;
import android.os.HandlerThread;
import android.os.IBinder;
import android.os.Looper;
import android.os.Message;
import android.os.Process;
import android.net.Uri;
import android.provider.CallLog;
import android.provider.ContactsContract;
import android.util.Log;
import android.util.Pair;

import com.android.vcard.VCardEntry;
import com.android.bluetooth.btservice.ProfileService;
import com.android.bluetooth.R;

import java.util.ArrayDeque;
import java.util.ArrayList;
import java.util.List;
import java.util.Queue;
import java.lang.InterruptedException;
import java.lang.Thread;
/**
 * These are the possible paths that can be pulled:
 *       BluetoothPbapClient.PB_PATH;
 *       BluetoothPbapClient.SIM_PB_PATH;
 *       BluetoothPbapClient.ICH_PATH;
 *       BluetoothPbapClient.SIM_ICH_PATH;
 *       BluetoothPbapClient.OCH_PATH;
 *       BluetoothPbapClient.SIM_OCH_PATH;
 *       BluetoothPbapClient.MCH_PATH;
 *       BluetoothPbapClient.SIM_MCH_PATH;
 */
public class PbapPCEClient  implements PbapHandler.PbapListener {
    private static final String TAG = "PbapPCEClient";
    private static final boolean DBG = false;
    private final Queue<PullRequest> mPendingRequests = new ArrayDeque<PullRequest>();
    private BluetoothDevice mDevice;
    private BluetoothPbapClient mClient;
    private boolean mClientConnected = false;
    private PbapHandler mHandler;
    private ConnectionHandler mConnectionHandler;
    private PullRequest mLastPull;
    private HandlerThread mContactHandlerThread;
    private Handler mContactHandler;
    private Account mAccount = null;
    private Context mContext = null;
    private AccountManager mAccountManager;

    PbapPCEClient(Context context) {
        mContext = context;
        mConnectionHandler = new ConnectionHandler(mContext.getMainLooper());
        mHandler = new PbapHandler(this);
        mAccountManager = AccountManager.get(mContext);
        mContactHandlerThread = new HandlerThread("PBAP contact handler",
                Process.THREAD_PRIORITY_BACKGROUND);
        mContactHandlerThread.start();
        mContactHandler = new ContactHandler(mContactHandlerThread.getLooper());
    }

    public int getConnectionState() {
        if (mDevice == null) {
            return BluetoothProfile.STATE_DISCONNECTED;
        }
        BluetoothPbapClient.ConnectionState currentState = mClient.getState();
        int bluetoothConnectionState;
        switch(currentState) {
          case DISCONNECTED:
              bluetoothConnectionState = BluetoothProfile.STATE_DISCONNECTED;
              break;
          case CONNECTING:
              bluetoothConnectionState = BluetoothProfile.STATE_CONNECTING;
              break;
          case CONNECTED:
              bluetoothConnectionState = BluetoothProfile.STATE_CONNECTED;
              break;
          case DISCONNECTING:
              bluetoothConnectionState = BluetoothProfile.STATE_DISCONNECTING;
              break;
          default:
              bluetoothConnectionState = BluetoothProfile.STATE_DISCONNECTED;
        }
        return bluetoothConnectionState;
    }

    public BluetoothDevice getDevice() {
        return mDevice;
    }

    private boolean processNextRequest() {
        if (DBG) {
            Log.d(TAG,"processNextRequest()");
        }
        if (mPendingRequests.isEmpty()) {
            return false;
        }
        if (mClient != null  && mClient.getState() ==
                BluetoothPbapClient.ConnectionState.CONNECTED) {
            mLastPull = mPendingRequests.remove();
            if (DBG) {
                Log.d(TAG, "Pulling phone book from: " + mLastPull.path);
            }
            return mClient.pullPhoneBook(mLastPull.path);
        }
        return false;
    }


    @Override
    public void onPhoneBookPullDone(List<VCardEntry> entries) {
        mLastPull.setResults(entries);
        mContactHandler.obtainMessage(ContactHandler.EVENT_ADD_CONTACTS,mLastPull).sendToTarget();
        processNextRequest();
    }

    @Override
    public void onPhoneBookError() {
        if (DBG) {
            Log.d(TAG, "Error, mLastPull = "  + mLastPull);
        }
        processNextRequest();
    }

    @Override
    public synchronized void onPbapClientConnected(boolean status) {
        mClientConnected = status;
        if (mClientConnected == false) {
            // If we are disconnected then whatever the current device is we should simply clean up.
            onConnectionStateChanged(mDevice, BluetoothProfile.STATE_CONNECTING,
                    BluetoothProfile.STATE_DISCONNECTED);
            disconnect(null);
        }
        if (mClientConnected == true) {
            onConnectionStateChanged(mDevice, BluetoothProfile.STATE_CONNECTING,
                    BluetoothProfile.STATE_CONNECTED);
            processNextRequest();
        }
    }

    private class ConnectionHandler extends Handler {
        public static final int EVENT_CONNECT = 1;
        public static final int EVENT_DISCONNECT = 2;

        public ConnectionHandler(Looper looper) {
            super(looper);
        }

        @Override
        public void handleMessage(Message msg) {
            if (DBG) {
                Log.d(TAG, "Connection Handler Message " + msg.what + " with " + msg.obj);
            }
            switch (msg.what) {
                case EVENT_CONNECT:
                    if (msg.obj instanceof BluetoothDevice) {
                        BluetoothDevice device = (BluetoothDevice) msg.obj;
                        int oldState = getConnectionState();
                        if (oldState != BluetoothProfile.STATE_DISCONNECTED) {
                            return;
                        }
                        handleConnect(device);
                    } else {
                        Log.e(TAG, "Invalid instance in Connection Handler:Connect");
                    }
                    break;

                case EVENT_DISCONNECT:
                    if (mDevice == null) {
                        return;
                    }
                    if (msg.obj == null || msg.obj instanceof BluetoothDevice) {
                        BluetoothDevice device = (BluetoothDevice) msg.obj;
                        if (!mDevice.equals(device)) {
                            return;
                        }
                        int oldState = getConnectionState();
                        handleDisconnect(device);
                        int newState = getConnectionState();
                        if (device != null) {
                            onConnectionStateChanged(device, oldState, newState);
                        }
                    } else {
                        Log.e(TAG, "Invalid instance in Connection Handler:Disconnect");
                    }
                    break;

                default:
                    Log.e(TAG, "Unknown Request to Connection Handler");
                    break;
            }
        }

        private void handleConnect(BluetoothDevice device) {
          Log.d(TAG,"HANDLECONNECT" + device);
            if (device == null) {
                throw new IllegalStateException(TAG + ":Connect with null device!");
            } else if (mDevice != null && !mDevice.equals(device)) {
                // Check that we are not already connected to an existing different device.
                // Since the device can be connected to multiple external devices -- we use the honor
                // protocol and only accept the first connecting device.
                Log.e(TAG, ":Got a connected event when connected to a different device. " +
                      "existing = " + mDevice + " new = " + device);
                return;
            } else if (device.equals(mDevice)) {
                Log.w(TAG, "Got a connected event for the same device. Ignoring!");
                return;
            }
            // Update the device.
            mDevice = device;
            onConnectionStateChanged(mDevice,BluetoothProfile.STATE_DISCONNECTED,
                    BluetoothProfile.STATE_CONNECTING);
            // Add the account. This should give us a place to stash the data.
            mAccount = new Account(device.getAddress(),
                    mContext.getString(R.string.pbap_account_type));
            mContactHandler.obtainMessage(ContactHandler.EVENT_ADD_ACCOUNT, mAccount)
                    .sendToTarget();
            mClient = new BluetoothPbapClient(mDevice, mAccount, mHandler);
            downloadPhoneBook();
            downloadCallLogs();
            mClient.connect();
        }

        private void handleDisconnect(BluetoothDevice device) {
            Log.w(TAG, "pbap disconnecting from = " + device);

            if (device == null) {
                // If we have a null device then disconnect the current device.
                device = mDevice;
            } else if (mDevice == null) {
                Log.w(TAG, "No existing device connected to service - ignoring device = " + device);
                return;
            } else if (!mDevice.equals(device)) {
                Log.w(TAG, "Existing device different from disconnected device. existing = " + mDevice +
                           " disconnecting device = " + device);
                return;
            }
            resetState();
        }
    }

    private void onConnectionStateChanged(BluetoothDevice device, int prevState, int state) {
        Intent intent = new Intent(android.bluetooth.BluetoothPbapClient.ACTION_CONNECTION_STATE_CHANGED);
        intent.putExtra(BluetoothProfile.EXTRA_PREVIOUS_STATE, prevState);
        intent.putExtra(BluetoothProfile.EXTRA_STATE, state);
        intent.putExtra(BluetoothDevice.EXTRA_DEVICE, device);
        intent.addFlags(Intent.FLAG_RECEIVER_REGISTERED_ONLY_BEFORE_BOOT);
        mContext.sendBroadcast(intent, ProfileService.BLUETOOTH_PERM);
        Log.d(TAG,"Connection state " + device + ": " + prevState + "->" + state);
    }

    public void connect(BluetoothDevice device) {
        mConnectionHandler.obtainMessage(ConnectionHandler.EVENT_CONNECT,device).sendToTarget();
    }

    public void disconnect(BluetoothDevice device) {
        mConnectionHandler.obtainMessage(ConnectionHandler.EVENT_DISCONNECT,device).sendToTarget();
    }

    public void start() {
        if (mDevice != null) {
            // We are already connected -Ignore.
            Log.w(TAG, "Already started, ignoring request to start again.");
            return;
        }
        // Device is NULL, we go on remove any unclean shutdown accounts.
        mContactHandler.obtainMessage(ContactHandler.EVENT_CLEANUP).sendToTarget();
    }

    private void resetState() {
        if (DBG) {
            Log.d(TAG,"resetState()");
        }
        if (mClient != null) {
            // This should abort any inflight messages.
            mClient.disconnect();
        }
        mClient = null;
        mClientConnected = false;

        mContactHandler.removeCallbacksAndMessages(null);
        mContactHandlerThread.interrupt();
        mContactHandler.obtainMessage(ContactHandler.EVENT_CLEANUP).sendToTarget();

        mDevice = null;
        mAccount = null;
        mPendingRequests.clear();
        if (DBG) {
            Log.d(TAG,"resetState Complete");
        }

    }

    private void downloadCallLogs() {
        // Download Incoming Call Logs.
        CallLogPullRequest ichCallLog =
                new CallLogPullRequest(mContext, BluetoothPbapClient.ICH_PATH);
        addPullRequest(ichCallLog);

        // Downoad Outgoing Call Logs.
        CallLogPullRequest ochCallLog =
                new CallLogPullRequest(mContext, BluetoothPbapClient.OCH_PATH);
        addPullRequest(ochCallLog);

        // Downoad Missed Call Logs.
        CallLogPullRequest mchCallLog =
                new CallLogPullRequest(mContext, BluetoothPbapClient.MCH_PATH);
        addPullRequest(mchCallLog);
    }

    private void downloadPhoneBook() {
        // Download the phone book.
        PhonebookPullRequest pb = new PhonebookPullRequest(mContext, mAccount);
        addPullRequest(pb);
    }

    private void addPullRequest(PullRequest r) {
        if (DBG) {
            Log.d(TAG, "pull request mClient=" + mClient + " connected= " +
                    mClientConnected + " mDevice=" + mDevice + " path= " + r.path);
        }
        if (mClient == null || mDevice == null) {
            // It seems we want to pull but the bt connection isn't up, fail it
            // immediately.
            Log.w(TAG, "aborting pull request.");
            return;
        }
        mPendingRequests.add(r);
    }

    private class ContactHandler extends Handler {
        public static final int EVENT_ADD_ACCOUNT = 1;
        public static final int EVENT_ADD_CONTACTS = 2;
        public static final int EVENT_CLEANUP = 3;

        public ContactHandler(Looper looper) {
          super(looper);
        }

        @Override
        public void handleMessage(Message msg) {
            if (DBG) {
                Log.d(TAG, "Contact Handler Message " + msg.what + " with " + msg.obj);
            }
            switch (msg.what) {
                case EVENT_ADD_ACCOUNT:
                    if (msg.obj instanceof Account) {
                        Account account = (Account) msg.obj;
                        addAccount(account);
                    } else {
                        Log.e(TAG, "invalid Instance in Contact Handler: Add Account");
                    }
                    break;

                case EVENT_ADD_CONTACTS:
                    if (msg.obj instanceof PullRequest) {
                        PullRequest req = (PullRequest) msg.obj;
                        req.onPullComplete();
                    } else {
                        Log.e(TAG, "invalid Instance in Contact Handler: Add Contacts");
                    }
                    break;

                case EVENT_CLEANUP:
                    Thread.currentThread().interrupted();  //clear state of interrupt.
                    removeUncleanAccounts();
                    mContext.getContentResolver().delete(CallLog.Calls.CONTENT_URI, null, null);
                    if (DBG) {
                        Log.d(TAG, "Call logs deleted.");
                    }
                    break;

                default:
                    Log.e(TAG, "Unknown Request to Contact Handler");
                    break;
            }
        }

        private void removeUncleanAccounts() {
            // Find all accounts that match the type "pbap" and delete them. This section is
            // executed only if the device was shut down in an unclean state and contacts persisted.
            Account[] accounts =
                mAccountManager.getAccountsByType(mContext.getString(R.string.pbap_account_type));
            Log.w(TAG, "Found " + accounts.length + " unclean accounts");
            for (Account acc : accounts) {
                Log.w(TAG, "Deleting " + acc);
                // The device ID is the name of the account.
                removeAccount(acc);
            }
        }

        private boolean addAccount(Account account) {
            if (mAccountManager.addAccountExplicitly(account, null, null)) {
                if (DBG) {
                    Log.d(TAG, "Added account " + mAccount);
                }
                return true;
            }
            return false;
        }

        private boolean removeAccount(Account acc) {
            if (mAccountManager.removeAccountExplicitly(acc)) {
                if (DBG) {
                    Log.d(TAG, "Removed account " + acc);
                }
                return true;
            }
            Log.e(TAG, "Failed to remove account " + mAccount);
            return false;
        }
   }
}
