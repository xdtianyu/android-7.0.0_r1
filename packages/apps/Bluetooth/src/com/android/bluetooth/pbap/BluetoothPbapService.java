/*
 * Copyright (c) 2008-2009, Motorola, Inc.
 *
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * - Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 *
 * - Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 *
 * - Neither the name of the Motorola, Inc. nor the names of its contributors
 * may be used to endorse or promote products derived from this software
 * without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

package com.android.bluetooth.pbap;

import android.app.Notification;
import android.app.NotificationManager;
import android.app.PendingIntent;
import android.app.Service;
import android.bluetooth.BluetoothAdapter;
import android.bluetooth.BluetoothDevice;
import android.bluetooth.BluetoothPbap;
import android.bluetooth.BluetoothProfile;
import android.bluetooth.BluetoothServerSocket;
import android.bluetooth.BluetoothSocket;
import android.bluetooth.IBluetooth;
import android.bluetooth.IBluetoothPbap;
import android.bluetooth.BluetoothUuid;
import android.content.Context;
import android.content.Intent;
import android.os.Handler;
import android.os.IBinder;
import android.os.Message;
import android.os.PowerManager;
import android.os.RemoteException;
import android.os.ServiceManager;
import android.telephony.TelephonyManager;
import android.text.TextUtils;
import android.util.Log;

import com.android.bluetooth.BluetoothObexTransport;
import com.android.bluetooth.Utils;


import com.android.bluetooth.R;
import com.android.bluetooth.btservice.AdapterService;

import java.io.IOException;

import javax.obex.ServerSession;

public class BluetoothPbapService extends Service {
    private static final String TAG = "BluetoothPbapService";

    /**
     * To enable PBAP DEBUG/VERBOSE logging - run below cmd in adb shell, and
     * restart com.android.bluetooth process. only enable DEBUG log:
     * "setprop log.tag.BluetoothPbapService DEBUG"; enable both VERBOSE and
     * DEBUG log: "setprop log.tag.BluetoothPbapService VERBOSE"
     */

    public static final boolean DEBUG = true;

    public static final boolean VERBOSE = false;

    /**
     * Intent indicating incoming obex authentication request which is from
     * PCE(Carkit)
     */
    public static final String AUTH_CHALL_ACTION = "com.android.bluetooth.pbap.authchall";

    /**
     * Intent indicating obex session key input complete by user which is sent
     * from BluetoothPbapActivity
     */
    public static final String AUTH_RESPONSE_ACTION = "com.android.bluetooth.pbap.authresponse";

    /**
     * Intent indicating user canceled obex authentication session key input
     * which is sent from BluetoothPbapActivity
     */
    public static final String AUTH_CANCELLED_ACTION = "com.android.bluetooth.pbap.authcancelled";

    /**
     * Intent indicating timeout for user confirmation, which is sent to
     * BluetoothPbapActivity
     */
    public static final String USER_CONFIRM_TIMEOUT_ACTION =
            "com.android.bluetooth.pbap.userconfirmtimeout";

    /**
     * Intent Extra name indicating session key which is sent from
     * BluetoothPbapActivity
     */
    public static final String EXTRA_SESSION_KEY = "com.android.bluetooth.pbap.sessionkey";

    public static final String THIS_PACKAGE_NAME = "com.android.bluetooth";

    public static final int MSG_SERVERSESSION_CLOSE = 5000;

    public static final int MSG_SESSION_ESTABLISHED = 5001;

    public static final int MSG_SESSION_DISCONNECTED = 5002;

    public static final int MSG_OBEX_AUTH_CHALL = 5003;

    public static final int MSG_ACQUIRE_WAKE_LOCK = 5004;

    public static final int MSG_RELEASE_WAKE_LOCK = 5005;

    private static final String BLUETOOTH_PERM = android.Manifest.permission.BLUETOOTH;

    private static final String BLUETOOTH_ADMIN_PERM = android.Manifest.permission.BLUETOOTH_ADMIN;

    private static final int START_LISTENER = 1;

    private static final int USER_TIMEOUT = 2;

    private static final int AUTH_TIMEOUT = 3;


    private static final int USER_CONFIRM_TIMEOUT_VALUE = 30000;

    private static final int RELEASE_WAKE_LOCK_DELAY = 10000;

    // Ensure not conflict with Opp notification ID
    private static final int NOTIFICATION_ID_ACCESS = -1000001;

    private static final int NOTIFICATION_ID_AUTH = -1000002;

    private PowerManager.WakeLock mWakeLock = null;

    private BluetoothAdapter mAdapter;

    private SocketAcceptThread mAcceptThread = null;

    private BluetoothPbapAuthenticator mAuth = null;

    private BluetoothPbapObexServer mPbapServer;

    private ServerSession mServerSession = null;

    private BluetoothServerSocket mServerSocket = null;

    private BluetoothSocket mConnSocket = null;

    private BluetoothDevice mRemoteDevice = null;

    private static String sLocalPhoneNum = null;

    private static String sLocalPhoneName = null;

    private static String sRemoteDeviceName = null;

    private boolean mHasStarted = false;

    private volatile boolean mInterrupted;

    private int mState;

    private int mStartId = -1;

    //private IBluetooth mBluetoothService;

    private boolean mIsWaitingAuthorization = false;

    // package and class name to which we send intent to check phone book access permission
    private static final String ACCESS_AUTHORITY_PACKAGE = "com.android.settings";
    private static final String ACCESS_AUTHORITY_CLASS =
        "com.android.settings.bluetooth.BluetoothPermissionRequest";

    public BluetoothPbapService() {
        mState = BluetoothPbap.STATE_DISCONNECTED;
    }

    @Override
    public void onCreate() {
        super.onCreate();
        if (VERBOSE) Log.v(TAG, "Pbap Service onCreate");

        mInterrupted = false;
        mAdapter = BluetoothAdapter.getDefaultAdapter();

        if (!mHasStarted) {
            mHasStarted = true;
            if (VERBOSE) Log.v(TAG, "Starting PBAP service");
            BluetoothPbapConfig.init(this);
            int state = mAdapter.getState();
            if (state == BluetoothAdapter.STATE_ON) {
                mSessionStatusHandler.sendMessage(mSessionStatusHandler
                        .obtainMessage(START_LISTENER));
            }
        }
    }

    @Override
    public int onStartCommand(Intent intent, int flags, int startId) {
        //int retCode = super.onStartCommand(intent, flags, startId);
        //if (retCode == START_STICKY) {
            mStartId = startId;
            if (mAdapter == null) {
                Log.w(TAG, "Stopping BluetoothPbapService: "
                        + "device does not have BT or device is not ready");
                // Release all resources
                closeService();
            } else {
                // No need to handle the null intent case, because we have
                // all restart work done in onCreate()
                if (intent != null) {
                    parseIntent(intent);
                }
            }
        //}
        return START_NOT_STICKY;
    }

    // process the intent from receiver
    private void parseIntent(final Intent intent) {
        String action = intent.getStringExtra("action");
        if (action == null) return;             // Nothing to do
        if (VERBOSE) Log.v(TAG, "action: " + action);

        int state = intent.getIntExtra(BluetoothAdapter.EXTRA_STATE, BluetoothAdapter.ERROR);
        if (VERBOSE) Log.v(TAG, "state: " + state);

        boolean removeTimeoutMsg = true;
        if (action.equals(BluetoothAdapter.ACTION_STATE_CHANGED)) {
            if (state == BluetoothAdapter.STATE_TURNING_OFF) {
                // Send any pending timeout now, as this service will be destroyed.
                if (mSessionStatusHandler.hasMessages(USER_TIMEOUT)) {
                    Intent timeoutIntent =
                        new Intent(BluetoothDevice.ACTION_CONNECTION_ACCESS_CANCEL);
                    timeoutIntent.setClassName(ACCESS_AUTHORITY_PACKAGE, ACCESS_AUTHORITY_CLASS);
                    timeoutIntent.putExtra(BluetoothDevice.EXTRA_ACCESS_REQUEST_TYPE,
                                     BluetoothDevice.REQUEST_TYPE_PHONEBOOK_ACCESS);
                    sendBroadcast(timeoutIntent, BLUETOOTH_ADMIN_PERM);
                }
                // Release all resources
                closeService();
            } else {
                removeTimeoutMsg = false;
            }
        } else if (action.equals(BluetoothDevice.ACTION_ACL_DISCONNECTED)
                && mIsWaitingAuthorization) {
            BluetoothDevice device = intent.getParcelableExtra(BluetoothDevice.EXTRA_DEVICE);

            if (mRemoteDevice == null || device == null) {
                Log.e(TAG, "Unexpected error!");
                return;
            }

            if (DEBUG) Log.d(TAG,"ACL disconnected for "+ device);

            if (mRemoteDevice.equals(device)) {
                Intent cancelIntent = new Intent(BluetoothDevice.ACTION_CONNECTION_ACCESS_CANCEL);
                cancelIntent.putExtra(BluetoothDevice.EXTRA_DEVICE, device);
                cancelIntent.putExtra(BluetoothDevice.EXTRA_ACCESS_REQUEST_TYPE,
                                      BluetoothDevice.REQUEST_TYPE_PHONEBOOK_ACCESS);
                sendBroadcast(cancelIntent);
                mIsWaitingAuthorization = false;
                stopObexServerSession();
            }
        } else if (action.equals(BluetoothDevice.ACTION_CONNECTION_ACCESS_REPLY)) {
            int requestType = intent.getIntExtra(BluetoothDevice.EXTRA_ACCESS_REQUEST_TYPE,
                                           BluetoothDevice.REQUEST_TYPE_PHONEBOOK_ACCESS);

            if ((!mIsWaitingAuthorization)
                    || (requestType != BluetoothDevice.REQUEST_TYPE_PHONEBOOK_ACCESS)) {
                // this reply is not for us
                return;
            }

            mIsWaitingAuthorization = false;

            if (intent.getIntExtra(BluetoothDevice.EXTRA_CONNECTION_ACCESS_RESULT,
                                   BluetoothDevice.CONNECTION_ACCESS_NO)
                    == BluetoothDevice.CONNECTION_ACCESS_YES) {
                if (intent.getBooleanExtra(BluetoothDevice.EXTRA_ALWAYS_ALLOWED, false)) {
                    boolean result = mRemoteDevice.setPhonebookAccessPermission(
                            BluetoothDevice.ACCESS_ALLOWED);
                    if (VERBOSE) {
                        Log.v(TAG, "setPhonebookAccessPermission(ACCESS_ALLOWED) result=" + result);
                    }
                }
                try {
                    if (mConnSocket != null) {
                        startObexServerSession();
                    } else {
                        stopObexServerSession();
                    }
                } catch (IOException ex) {
                    Log.e(TAG, "Caught the error: " + ex.toString());
                }
            } else {
                if (intent.getBooleanExtra(BluetoothDevice.EXTRA_ALWAYS_ALLOWED, false)) {
                    boolean result = mRemoteDevice.setPhonebookAccessPermission(
                            BluetoothDevice.ACCESS_REJECTED);
                    if (VERBOSE) {
                        Log.v(TAG, "setPhonebookAccessPermission(ACCESS_REJECTED) result="
                                + result);
                    }
                }
                stopObexServerSession();
            }
        } else if (action.equals(AUTH_RESPONSE_ACTION)) {
            String sessionkey = intent.getStringExtra(EXTRA_SESSION_KEY);
            notifyAuthKeyInput(sessionkey);
        } else if (action.equals(AUTH_CANCELLED_ACTION)) {
            notifyAuthCancelled();
        } else {
            removeTimeoutMsg = false;
        }

        if (removeTimeoutMsg) {
            mSessionStatusHandler.removeMessages(USER_TIMEOUT);
        }
    }

    @Override
    public void onDestroy() {
        if (VERBOSE) Log.v(TAG, "Pbap Service onDestroy");

        super.onDestroy();
        setState(BluetoothPbap.STATE_DISCONNECTED, BluetoothPbap.RESULT_CANCELED);
        closeService();
        if(mSessionStatusHandler != null) {
            mSessionStatusHandler.removeCallbacksAndMessages(null);
        }
    }

    @Override
    public IBinder onBind(Intent intent) {
        if (VERBOSE) Log.v(TAG, "Pbap Service onBind");
        return mBinder;
    }

    private void startRfcommSocketListener() {
        if (VERBOSE) Log.v(TAG, "Pbap Service startRfcommSocketListener");

        if (mAcceptThread == null) {
            mAcceptThread = new SocketAcceptThread();
            mAcceptThread.setName("BluetoothPbapAcceptThread");
            mAcceptThread.start();
        }
    }

    private final boolean initSocket() {
        if (VERBOSE) Log.v(TAG, "Pbap Service initSocket");

        boolean initSocketOK = false;
        final int CREATE_RETRY_TIME = 10;

        // It's possible that create will fail in some cases. retry for 10 times
        for (int i = 0; i < CREATE_RETRY_TIME && !mInterrupted; i++) {
            initSocketOK = true;
            try {
                // It is mandatory for PSE to support initiation of bonding and
                // encryption.
                mServerSocket = mAdapter.listenUsingEncryptedRfcommWithServiceRecord
                    ("OBEX Phonebook Access Server", BluetoothUuid.PBAP_PSE.getUuid());

            } catch (IOException e) {
                Log.e(TAG, "Error create RfcommServerSocket " + e.toString());
                initSocketOK = false;
            }
            if (!initSocketOK) {
                // Need to break out of this loop if BT is being turned off.
                if (mAdapter == null) break;
                int state = mAdapter.getState();
                if ((state != BluetoothAdapter.STATE_TURNING_ON) &&
                    (state != BluetoothAdapter.STATE_ON)) {
                    Log.w(TAG, "initServerSocket failed as BT is (being) turned off");
                    break;
                }
                try {
                    if (VERBOSE) Log.v(TAG, "wait 300 ms");
                    Thread.sleep(300);
                } catch (InterruptedException e) {
                    Log.e(TAG, "socketAcceptThread thread was interrupted (3)");
                    break;
                }
            } else {
                break;
            }
        }

        if (mInterrupted) {
            initSocketOK = false;
            // close server socket to avoid resource leakage
            closeServerSocket();
        }

        if (initSocketOK) {
            if (VERBOSE) Log.v(TAG, "Succeed to create listening socket ");

        } else {
            Log.e(TAG, "Error to create listening socket after " + CREATE_RETRY_TIME + " try");
        }
        return initSocketOK;
    }

    private final synchronized void closeServerSocket() {
        // exit SocketAcceptThread early
        if (mServerSocket != null) {
            try {
                // this will cause mServerSocket.accept() return early with IOException
                mServerSocket.close();
                mServerSocket = null;
            } catch (IOException ex) {
                Log.e(TAG, "Close Server Socket error: " + ex);
            }
        }
    }

    private final synchronized void closeConnectionSocket() {
        if (mConnSocket != null) {
            try {
                mConnSocket.close();
                mConnSocket = null;
            } catch (IOException e) {
                Log.e(TAG, "Close Connection Socket error: " + e.toString());
            }
        }
    }

    private final void closeService() {
        if (VERBOSE) Log.v(TAG, "Pbap Service closeService in");

        // exit initSocket early
        mInterrupted = true;
        closeServerSocket();

        if (mAcceptThread != null) {
            try {
                mAcceptThread.shutdown();
                mAcceptThread.join();
                mAcceptThread = null;
            } catch (InterruptedException ex) {
                Log.w(TAG, "mAcceptThread close error" + ex);
            }
        }

        if (mWakeLock != null) {
            mWakeLock.release();
            mWakeLock = null;
        }

        if (mServerSession != null) {
            mServerSession.close();
            mServerSession = null;
        }

        closeConnectionSocket();

        mHasStarted = false;
        if (mStartId != -1 && stopSelfResult(mStartId)) {
            if (VERBOSE) Log.v(TAG, "successfully stopped pbap service");
            mStartId = -1;
        }
        if (VERBOSE) Log.v(TAG, "Pbap Service closeService out");
    }

    private final void startObexServerSession() throws IOException {
        if (VERBOSE) Log.v(TAG, "Pbap Service startObexServerSession");

        // acquire the wakeLock before start Obex transaction thread
        if (mWakeLock == null) {
            PowerManager pm = (PowerManager)getSystemService(Context.POWER_SERVICE);
            mWakeLock = pm.newWakeLock(PowerManager.PARTIAL_WAKE_LOCK,
                    "StartingObexPbapTransaction");
            mWakeLock.setReferenceCounted(false);
            mWakeLock.acquire();
        }
        TelephonyManager tm = (TelephonyManager)getSystemService(Context.TELEPHONY_SERVICE);
        if (tm != null) {
            sLocalPhoneNum = tm.getLine1Number();
            sLocalPhoneName = tm.getLine1AlphaTag();
            if (TextUtils.isEmpty(sLocalPhoneName)) {
                sLocalPhoneName = this.getString(R.string.localPhoneName);
            }
        }

        mPbapServer = new BluetoothPbapObexServer(mSessionStatusHandler, this);
        synchronized (this) {
            mAuth = new BluetoothPbapAuthenticator(mSessionStatusHandler);
            mAuth.setChallenged(false);
            mAuth.setCancelled(false);
        }
        BluetoothObexTransport transport = new BluetoothObexTransport(mConnSocket);
        mServerSession = new ServerSession(transport, mPbapServer, mAuth);
        setState(BluetoothPbap.STATE_CONNECTED);

        mSessionStatusHandler.removeMessages(MSG_RELEASE_WAKE_LOCK);
        mSessionStatusHandler.sendMessageDelayed(mSessionStatusHandler
            .obtainMessage(MSG_RELEASE_WAKE_LOCK), RELEASE_WAKE_LOCK_DELAY);

        if (VERBOSE) {
            Log.v(TAG, "startObexServerSession() success!");
        }
    }

    private void stopObexServerSession() {
        if (VERBOSE) Log.v(TAG, "Pbap Service stopObexServerSession");

        mSessionStatusHandler.removeMessages(MSG_ACQUIRE_WAKE_LOCK);
        mSessionStatusHandler.removeMessages(MSG_RELEASE_WAKE_LOCK);
        // Release the wake lock if obex transaction is over
        if (mWakeLock != null) {
            mWakeLock.release();
            mWakeLock = null;
        }

        if (mServerSession != null) {
            mServerSession.close();
            mServerSession = null;
        }

        mAcceptThread = null;

        closeConnectionSocket();

        // Last obex transaction is finished, we start to listen for incoming
        // connection again
        if (mAdapter.isEnabled()) {
            startRfcommSocketListener();
        }
        setState(BluetoothPbap.STATE_DISCONNECTED);
    }

    private void notifyAuthKeyInput(final String key) {
        synchronized (mAuth) {
            if (key != null) {
                mAuth.setSessionKey(key);
            }
            mAuth.setChallenged(true);
            mAuth.notify();
        }
    }

    private void notifyAuthCancelled() {
        synchronized (mAuth) {
            mAuth.setCancelled(true);
            mAuth.notify();
        }
    }

    /**
     * A thread that runs in the background waiting for remote rfcomm
     * connect.Once a remote socket connected, this thread shall be
     * shutdown.When the remote disconnect,this thread shall run again waiting
     * for next request.
     */
    private class SocketAcceptThread extends Thread {

        private boolean stopped = false;

        @Override
        public void run() {
            BluetoothServerSocket serverSocket;
            if (mServerSocket == null) {
                if (!initSocket()) {
                    return;
                }
            }

            while (!stopped) {
                try {
                    if (VERBOSE) Log.v(TAG, "Accepting socket connection...");
                    serverSocket = mServerSocket;
                    if (serverSocket == null) {
                        Log.w(TAG, "mServerSocket is null");
                        break;
                    }
                    mConnSocket = serverSocket.accept();
                    if (VERBOSE) Log.v(TAG, "Accepted socket connection...");

                    synchronized (BluetoothPbapService.this) {
                        if (mConnSocket == null) {
                            Log.w(TAG, "mConnSocket is null");
                            break;
                        }
                        mRemoteDevice = mConnSocket.getRemoteDevice();
                    }
                    if (mRemoteDevice == null) {
                        Log.i(TAG, "getRemoteDevice() = null");
                        break;
                    }
                    sRemoteDeviceName = mRemoteDevice.getName();
                    // In case getRemoteName failed and return null
                    if (TextUtils.isEmpty(sRemoteDeviceName)) {
                        sRemoteDeviceName = getString(R.string.defaultname);
                    }
                    int permission = mRemoteDevice.getPhonebookAccessPermission();
                    if (VERBOSE) Log.v(TAG, "getPhonebookAccessPermission() = " + permission);

                    if (permission == BluetoothDevice.ACCESS_ALLOWED) {
                        try {
                            if (VERBOSE) {
                                Log.v(TAG, "incoming connection accepted from: " + sRemoteDeviceName
                                        + " automatically as already allowed device");
                            }
                            startObexServerSession();
                        } catch (IOException ex) {
                            Log.e(TAG, "Caught exception starting obex server session"
                                    + ex.toString());
                        }
                    } else if (permission == BluetoothDevice.ACCESS_REJECTED) {
                        if (VERBOSE) {
                            Log.v(TAG, "incoming connection rejected from: " + sRemoteDeviceName
                                    + " automatically as already rejected device");
                        }
                        stopObexServerSession();
                    } else {  // permission == BluetoothDevice.ACCESS_UNKNOWN
                        // Send an Intent to Settings app to ask user preference.
                        Intent intent =
                                new Intent(BluetoothDevice.ACTION_CONNECTION_ACCESS_REQUEST);
                        intent.setClassName(ACCESS_AUTHORITY_PACKAGE, ACCESS_AUTHORITY_CLASS);
                        intent.putExtra(BluetoothDevice.EXTRA_ACCESS_REQUEST_TYPE,
                                        BluetoothDevice.REQUEST_TYPE_PHONEBOOK_ACCESS);
                        intent.putExtra(BluetoothDevice.EXTRA_DEVICE, mRemoteDevice);
                        intent.putExtra(BluetoothDevice.EXTRA_PACKAGE_NAME, getPackageName());
                        intent.putExtra(BluetoothDevice.EXTRA_CLASS_NAME,
                                        BluetoothPbapReceiver.class.getName());

                        mIsWaitingAuthorization = true;
                        sendOrderedBroadcast(intent, BLUETOOTH_ADMIN_PERM);

                        if (VERBOSE) Log.v(TAG, "waiting for authorization for connection from: "
                                + sRemoteDeviceName);

                        // In case car kit time out and try to use HFP for
                        // phonebook
                        // access, while UI still there waiting for user to
                        // confirm
                        mSessionStatusHandler.sendMessageDelayed(mSessionStatusHandler
                                .obtainMessage(USER_TIMEOUT), USER_CONFIRM_TIMEOUT_VALUE);
                        // We will continue the process when we receive
                        // BluetoothDevice.ACTION_CONNECTION_ACCESS_REPLY from Settings app.
                    }
                    stopped = true; // job done ,close this thread;
                } catch (IOException ex) {
                    stopped=true;
                    /*
                    if (stopped) {
                        break;
                    }
                    */
                    if (VERBOSE) Log.v(TAG, "Accept exception: " + ex.toString());
                }
            }
        }

        void shutdown() {
            stopped = true;
            interrupt();
        }
    }

    private final Handler mSessionStatusHandler = new Handler() {
        @Override
        public void handleMessage(Message msg) {
            if (VERBOSE) Log.v(TAG, "Handler(): got msg=" + msg.what);

            switch (msg.what) {
                case START_LISTENER:
                    if (mAdapter.isEnabled()) {
                        startRfcommSocketListener();
                    } else {
                        closeService();// release all resources
                    }
                    break;
                case USER_TIMEOUT:
                    Intent intent = new Intent(BluetoothDevice.ACTION_CONNECTION_ACCESS_CANCEL);
                    intent.putExtra(BluetoothDevice.EXTRA_DEVICE, mRemoteDevice);
                    intent.putExtra(BluetoothDevice.EXTRA_ACCESS_REQUEST_TYPE,
                                    BluetoothDevice.REQUEST_TYPE_PHONEBOOK_ACCESS);
                    sendBroadcast(intent);
                    mIsWaitingAuthorization = false;
                    stopObexServerSession();
                    break;
                case AUTH_TIMEOUT:
                    Intent i = new Intent(USER_CONFIRM_TIMEOUT_ACTION);
                    sendBroadcast(i);
                    removePbapNotification(NOTIFICATION_ID_AUTH);
                    notifyAuthCancelled();
                    break;
                case MSG_SERVERSESSION_CLOSE:
                    stopObexServerSession();
                    break;
                case MSG_SESSION_ESTABLISHED:
                    break;
                case MSG_SESSION_DISCONNECTED:
                    // case MSG_SERVERSESSION_CLOSE will handle ,so just skip
                    break;
                case MSG_OBEX_AUTH_CHALL:
                    createPbapNotification(AUTH_CHALL_ACTION);
                    mSessionStatusHandler.sendMessageDelayed(mSessionStatusHandler
                            .obtainMessage(AUTH_TIMEOUT), USER_CONFIRM_TIMEOUT_VALUE);
                    break;
                case MSG_ACQUIRE_WAKE_LOCK:
                    if (mWakeLock == null) {
                        PowerManager pm = (PowerManager)getSystemService(
                                          Context.POWER_SERVICE);
                        mWakeLock = pm.newWakeLock(PowerManager.PARTIAL_WAKE_LOCK,
                                    "StartingObexPbapTransaction");
                        mWakeLock.setReferenceCounted(false);
                        mWakeLock.acquire();
                        Log.w(TAG, "Acquire Wake Lock");
                    }
                    mSessionStatusHandler.removeMessages(MSG_RELEASE_WAKE_LOCK);
                    mSessionStatusHandler.sendMessageDelayed(mSessionStatusHandler
                      .obtainMessage(MSG_RELEASE_WAKE_LOCK), RELEASE_WAKE_LOCK_DELAY);
                    break;
                case MSG_RELEASE_WAKE_LOCK:
                    if (mWakeLock != null) {
                        mWakeLock.release();
                        mWakeLock = null;
                        Log.w(TAG, "Release Wake Lock");
                    }
                    break;
                default:
                    break;
            }
        }
    };

    private void setState(int state) {
        setState(state, BluetoothPbap.RESULT_SUCCESS);
    }

    private synchronized void setState(int state, int result) {
        if (state != mState) {
            if (DEBUG) Log.d(TAG, "Pbap state " + mState + " -> " + state + ", result = "
                    + result);
            int prevState = mState;
            mState = state;
            Intent intent = new Intent(BluetoothPbap.PBAP_STATE_CHANGED_ACTION);
            intent.putExtra(BluetoothPbap.PBAP_PREVIOUS_STATE, prevState);
            intent.putExtra(BluetoothPbap.PBAP_STATE, mState);
            intent.putExtra(BluetoothDevice.EXTRA_DEVICE, mRemoteDevice);
            sendBroadcast(intent, BLUETOOTH_PERM);
            AdapterService s = AdapterService.getAdapterService();
            if (s != null) {
                s.onProfileConnectionStateChanged(mRemoteDevice, BluetoothProfile.PBAP,
                        mState, prevState);
            }
        }
    }

    private void createPbapNotification(String action) {

        NotificationManager nm = (NotificationManager)
            getSystemService(Context.NOTIFICATION_SERVICE);

        // Create an intent triggered by clicking on the status icon.
        Intent clickIntent = new Intent();
        clickIntent.setClass(this, BluetoothPbapActivity.class);
        clickIntent.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
        clickIntent.setAction(action);

        // Create an intent triggered by clicking on the
        // "Clear All Notifications" button
        Intent deleteIntent = new Intent();
        deleteIntent.setClass(this, BluetoothPbapReceiver.class);

        Notification notification = null;
        String name = getRemoteDeviceName();

        if (action.equals(AUTH_CHALL_ACTION)) {
            deleteIntent.setAction(AUTH_CANCELLED_ACTION);
            notification = new Notification(android.R.drawable.stat_sys_data_bluetooth,
                getString(R.string.auth_notif_ticker), System.currentTimeMillis());
            notification.color = getResources().getColor(
                    com.android.internal.R.color.system_notification_accent_color);
            notification.setLatestEventInfo(this, getString(R.string.auth_notif_title),
                    getString(R.string.auth_notif_message, name), PendingIntent
                            .getActivity(this, 0, clickIntent, 0));

            notification.flags |= Notification.FLAG_AUTO_CANCEL;
            notification.flags |= Notification.FLAG_ONLY_ALERT_ONCE;
            notification.defaults = Notification.DEFAULT_SOUND;
            notification.deleteIntent = PendingIntent.getBroadcast(this, 0, deleteIntent, 0);
            nm.notify(NOTIFICATION_ID_AUTH, notification);
        }
    }

    private void removePbapNotification(int id) {
        NotificationManager nm = (NotificationManager)
            getSystemService(Context.NOTIFICATION_SERVICE);
        nm.cancel(id);
    }

    public static String getLocalPhoneNum() {
        return sLocalPhoneNum;
    }

    public static String getLocalPhoneName() {
        return sLocalPhoneName;
    }

    public static String getRemoteDeviceName() {
        return sRemoteDeviceName;
    }

    /**
     * Handlers for incoming service calls
     */
    private final IBluetoothPbap.Stub mBinder = new IBluetoothPbap.Stub() {
        public int getState() {
            if (DEBUG) Log.d(TAG, "getState " + mState);

            if (!Utils.checkCaller()) {
                Log.w(TAG,"getState(): not allowed for non-active user");
                return BluetoothPbap.STATE_DISCONNECTED;
            }

            enforceCallingOrSelfPermission(BLUETOOTH_PERM, "Need BLUETOOTH permission");
            return mState;
        }

        public BluetoothDevice getClient() {
            if (DEBUG) Log.d(TAG, "getClient" + mRemoteDevice);

            if (!Utils.checkCaller()) {
                Log.w(TAG,"getClient(): not allowed for non-active user");
                return null;
            }

            enforceCallingOrSelfPermission(BLUETOOTH_PERM, "Need BLUETOOTH permission");
            if (mState == BluetoothPbap.STATE_DISCONNECTED) {
                return null;
            }
            return mRemoteDevice;
        }

        public boolean isConnected(BluetoothDevice device) {
            if (!Utils.checkCaller()) {
                Log.w(TAG,"isConnected(): not allowed for non-active user");
                return false;
            }

            enforceCallingOrSelfPermission(BLUETOOTH_PERM, "Need BLUETOOTH permission");
            return mState == BluetoothPbap.STATE_CONNECTED && mRemoteDevice.equals(device);
        }

        public boolean connect(BluetoothDevice device) {
            if (!Utils.checkCaller()) {
                Log.w(TAG,"connect(): not allowed for non-active user");
                return false;
            }

            enforceCallingOrSelfPermission(BLUETOOTH_ADMIN_PERM,
                    "Need BLUETOOTH_ADMIN permission");
            return false;
        }

        public void disconnect() {
            if (DEBUG) Log.d(TAG, "disconnect");

            if (!Utils.checkCaller()) {
                Log.w(TAG,"disconnect(): not allowed for non-active user");
                return;
            }

            enforceCallingOrSelfPermission(BLUETOOTH_ADMIN_PERM,
                    "Need BLUETOOTH_ADMIN permission");
            synchronized (BluetoothPbapService.this) {
                switch (mState) {
                    case BluetoothPbap.STATE_CONNECTED:
                        if (mServerSession != null) {
                            mServerSession.close();
                            mServerSession = null;
                        }

                        closeConnectionSocket();

                        setState(BluetoothPbap.STATE_DISCONNECTED, BluetoothPbap.RESULT_CANCELED);
                        break;
                    default:
                        break;
                }
            }
        }
    };
}
