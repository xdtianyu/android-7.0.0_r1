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

package com.android.bluetooth.opp;

import javax.obex.ObexTransport;

import com.android.bluetooth.BluetoothObexTransport;

import android.app.NotificationManager;
import android.bluetooth.BluetoothAdapter;
import android.bluetooth.BluetoothDevice;
import android.bluetooth.BluetoothSocket;
import android.bluetooth.BluetoothUuid;
import android.os.ParcelUuid;
import android.content.BroadcastReceiver;
import android.content.ContentValues;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.net.Uri;
import android.os.Handler;
import android.os.HandlerThread;
import android.os.Looper;
import android.os.Message;
import android.os.Parcelable;
import android.os.PowerManager;
import android.os.Process;
import android.util.Log;

import java.io.File;
import java.io.IOException;
import java.net.InetSocketAddress;
import java.net.Socket;
import java.net.UnknownHostException;

/**
 * This class run an actual Opp transfer session (from connect target device to
 * disconnect)
 */
public class BluetoothOppTransfer implements BluetoothOppBatch.BluetoothOppBatchListener {
    private static final String TAG = "BtOppTransfer";

    private static final boolean D = Constants.DEBUG;

    private static final boolean V = Constants.VERBOSE;

    private static final int RFCOMM_ERROR = 10;

    private static final int RFCOMM_CONNECTED = 11;

    private static final int SOCKET_ERROR_RETRY = 13;

    private static final int CONNECT_WAIT_TIMEOUT = 45000;

    private static final int CONNECT_RETRY_TIME = 100;

    private static final short OPUSH_UUID16 = 0x1105;

    private static final String SOCKET_LINK_KEY_ERROR = "Invalid exchange";

    private Context mContext;

    private BluetoothAdapter mAdapter;

    private BluetoothOppBatch mBatch;

    private BluetoothOppObexSession mSession;

    private BluetoothOppShareInfo mCurrentShare;

    private ObexTransport mTransport;

    private HandlerThread mHandlerThread;

    private EventHandler mSessionHandler;

    private long mTimestamp;

    public BluetoothOppTransfer(Context context, PowerManager powerManager,
            BluetoothOppBatch batch, BluetoothOppObexSession session) {

        mContext = context;
        mBatch = batch;
        mSession = session;

        mBatch.registerListern(this);
        mAdapter = BluetoothAdapter.getDefaultAdapter();

    }

    public BluetoothOppTransfer(Context context, PowerManager powerManager, BluetoothOppBatch batch) {
        this(context, powerManager, batch, null);
    }

    public int getBatchId() {
        return mBatch.mId;
    }

    /*
     * Receives events from mConnectThread & mSession back in the main thread.
     */
    private class EventHandler extends Handler {
        public EventHandler(Looper looper) {
            super(looper);
        }

        @Override
        public void handleMessage(Message msg) {
            switch (msg.what) {
                case SOCKET_ERROR_RETRY:
                    mConnectThread = new
                        SocketConnectThread((BluetoothDevice)msg.obj, true);

                    mConnectThread.start();
                    break;
                case RFCOMM_ERROR:
                    /*
                    * RFCOMM connect fail is for outbound share only! Mark batch
                    * failed, and all shares in batch failed
                    */
                    if (V) Log.v(TAG, "receive RFCOMM_ERROR msg");
                    mConnectThread = null;
                    markBatchFailed(BluetoothShare.STATUS_CONNECTION_ERROR);
                    mBatch.mStatus = Constants.BATCH_STATUS_FAILED;

                    break;
                case RFCOMM_CONNECTED:
                    /*
                    * RFCOMM connected is for outbound share only! Create
                    * BluetoothOppObexClientSession and start it
                    */
                    if (V) Log.v(TAG, "Transfer receive RFCOMM_CONNECTED msg");
                    mConnectThread = null;
                    mTransport = (ObexTransport)msg.obj;
                    startObexSession();

                    break;
                case BluetoothOppObexSession.MSG_SHARE_COMPLETE:
                    /*
                    * Put next share if available,or finish the transfer.
                    * For outbound session, call session.addShare() to send next file,
                    * or call session.stop().
                    * For inbounds session, do nothing. If there is next file to receive,it
                    * will be notified through onShareAdded()
                    */
                    BluetoothOppShareInfo info = (BluetoothOppShareInfo)msg.obj;
                    if (V) Log.v(TAG, "receive MSG_SHARE_COMPLETE for info " + info.mId);
                    if (mBatch.mDirection == BluetoothShare.DIRECTION_OUTBOUND) {
                        mCurrentShare = mBatch.getPendingShare();

                        if (mCurrentShare != null) {
                            /* we have additional share to process */
                            if (V) Log.v(TAG, "continue session for info " + mCurrentShare.mId +
                                    " from batch " + mBatch.mId);
                            processCurrentShare();
                        } else {
                            /* for outbound transfer, all shares are processed */
                            if (V) Log.v(TAG, "Batch " + mBatch.mId + " is done");
                            mSession.stop();
                        }
                    }
                    break;
                case BluetoothOppObexSession.MSG_SESSION_COMPLETE:
                    /*
                    * Handle session completed status Set batch status to
                    * finished
                    */
                    BluetoothOppShareInfo info1 = (BluetoothOppShareInfo)msg.obj;
                    if (V) Log.v(TAG, "receive MSG_SESSION_COMPLETE for batch " + mBatch.mId);
                    mBatch.mStatus = Constants.BATCH_STATUS_FINISHED;
                    /*
                     * trigger content provider again to know batch status change
                     */
                    tickShareStatus(info1);
                    break;

                case BluetoothOppObexSession.MSG_SESSION_ERROR:
                    /* Handle the error state of an Obex session */
                    if (V) Log.v(TAG, "receive MSG_SESSION_ERROR for batch " + mBatch.mId);
                    BluetoothOppShareInfo info2 = (BluetoothOppShareInfo)msg.obj;
                    mSession.stop();
                    mBatch.mStatus = Constants.BATCH_STATUS_FAILED;
                    markBatchFailed(info2.mStatus);
                    tickShareStatus(mCurrentShare);
                    break;

                case BluetoothOppObexSession.MSG_SHARE_INTERRUPTED:
                    if (V) Log.v(TAG, "receive MSG_SHARE_INTERRUPTED for batch " + mBatch.mId);
                    BluetoothOppShareInfo info3 = (BluetoothOppShareInfo)msg.obj;
                    if (mBatch.mDirection == BluetoothShare.DIRECTION_OUTBOUND) {
                        try {
                            if (mTransport == null) {
                                Log.v(TAG, "receive MSG_SHARE_INTERRUPTED but mTransport = null");
                            } else {
                                mTransport.close();
                            }
                        } catch (IOException e) {
                            Log.e(TAG, "failed to close mTransport");
                        }
                        if (V) Log.v(TAG, "mTransport closed ");
                        mBatch.mStatus = Constants.BATCH_STATUS_FAILED;
                        if (info3 != null) {
                            markBatchFailed(info3.mStatus);
                        } else {
                            markBatchFailed();
                        }
                        tickShareStatus(mCurrentShare);
                    }
                    break;

                case BluetoothOppObexSession.MSG_CONNECT_TIMEOUT:
                    if (V) Log.v(TAG, "receive MSG_CONNECT_TIMEOUT for batch " + mBatch.mId);
                    /* for outbound transfer, the block point is BluetoothSocket.write()
                     * The only way to unblock is to tear down lower transport
                     * */
                    if (mBatch.mDirection == BluetoothShare.DIRECTION_OUTBOUND) {
                        try {
                            if (mTransport == null) {
                                Log.v(TAG, "receive MSG_SHARE_INTERRUPTED but mTransport = null");
                            } else {
                                mTransport.close();
                            }
                        } catch (IOException e) {
                            Log.e(TAG, "failed to close mTransport");
                        }
                        if (V) Log.v(TAG, "mTransport closed ");
                    } else {
                        /*
                         * For inbound transfer, the block point is waiting for
                         * user confirmation we can interrupt it nicely
                         */

                        // Remove incoming file confirm notification
                        NotificationManager nm = (NotificationManager)mContext
                                .getSystemService(Context.NOTIFICATION_SERVICE);
                        nm.cancel(mCurrentShare.mId);
                        // Send intent to UI for timeout handling
                        Intent in = new Intent(BluetoothShare.USER_CONFIRMATION_TIMEOUT_ACTION);
                        mContext.sendBroadcast(in);

                        markShareTimeout(mCurrentShare);
                    }
                    break;
            }
        }
    }

    private void markShareTimeout(BluetoothOppShareInfo share) {
        Uri contentUri = Uri.parse(BluetoothShare.CONTENT_URI + "/" + share.mId);
        ContentValues updateValues = new ContentValues();
        updateValues
                .put(BluetoothShare.USER_CONFIRMATION, BluetoothShare.USER_CONFIRMATION_TIMEOUT);
        mContext.getContentResolver().update(contentUri, updateValues, null, null);
    }

    private void markBatchFailed(int failReason) {
        synchronized (this) {
            try {
                wait(1000);
            } catch (InterruptedException e) {
                if (V) Log.v(TAG, "Interrupted waiting for markBatchFailed");
            }
        }

        if (D) Log.d(TAG, "Mark all ShareInfo in the batch as failed");
        if (mCurrentShare != null) {
            if (V) Log.v(TAG, "Current share has status " + mCurrentShare.mStatus);
            if (BluetoothShare.isStatusError(mCurrentShare.mStatus)) {
                failReason = mCurrentShare.mStatus;
            }
            if (mCurrentShare.mDirection == BluetoothShare.DIRECTION_INBOUND
                    && mCurrentShare.mFilename != null) {
                new File(mCurrentShare.mFilename).delete();
            }
        }

        BluetoothOppShareInfo info = null;
        if (mBatch == null) {
            return;
        }
        info = mBatch.getPendingShare();
        while (info != null) {
            if (info.mStatus < 200) {
                info.mStatus = failReason;
                Uri contentUri = Uri.parse(BluetoothShare.CONTENT_URI + "/" + info.mId);
                ContentValues updateValues = new ContentValues();
                updateValues.put(BluetoothShare.STATUS, info.mStatus);
                /* Update un-processed outbound transfer to show some info */
                if (info.mDirection == BluetoothShare.DIRECTION_OUTBOUND) {
                    BluetoothOppSendFileInfo fileInfo
                            = BluetoothOppUtility.getSendFileInfo(info.mUri);
                    BluetoothOppUtility.closeSendFileInfo(info.mUri);
                    if (fileInfo.mFileName != null) {
                        updateValues.put(BluetoothShare.FILENAME_HINT, fileInfo.mFileName);
                        updateValues.put(BluetoothShare.TOTAL_BYTES, fileInfo.mLength);
                        updateValues.put(BluetoothShare.MIMETYPE, fileInfo.mMimetype);
                    }
                } else {
                    if (info.mStatus < 200 && info.mFilename != null) {
                        new File(info.mFilename).delete();
                    }
                }
                mContext.getContentResolver().update(contentUri, updateValues, null, null);
                Constants.sendIntentIfCompleted(mContext, contentUri, info.mStatus);
            }
            info = mBatch.getPendingShare();
        }

    }

    private void markBatchFailed() {
        markBatchFailed(BluetoothShare.STATUS_UNKNOWN_ERROR);
    }

    /*
     * NOTE
     * For outbound transfer
     * 1) Check Bluetooth status
     * 2) Start handler thread
     * 3) new a thread to connect to target device
     * 3.1) Try a few times to do SDP query for target device OPUSH channel
     * 3.2) Try a few seconds to connect to target socket
     * 4) After BluetoothSocket is connected,create an instance of RfcommTransport
     * 5) Create an instance of BluetoothOppClientSession
     * 6) Start the session and process the first share in batch
     * For inbound transfer
     * The transfer already has session and transport setup, just start it
     * 1) Check Bluetooth status
     * 2) Start handler thread
     * 3) Start the session and process the first share in batch
     */
    /**
     * Start the transfer
     */
    public void start() {
        /* check Bluetooth enable status */
        /*
         * normally it's impossible to reach here if BT is disabled. Just check
         * for safety
         */
        if (!mAdapter.isEnabled()) {
            Log.e(TAG, "Can't start transfer when Bluetooth is disabled for " + mBatch.mId);
            markBatchFailed();
            mBatch.mStatus = Constants.BATCH_STATUS_FAILED;
            return;
        }

        if (mHandlerThread == null) {
            if (V) Log.v(TAG, "Create handler thread for batch " + mBatch.mId);
            mHandlerThread = new HandlerThread("BtOpp Transfer Handler",
                    Process.THREAD_PRIORITY_BACKGROUND);
            mHandlerThread.start();
            mSessionHandler = new EventHandler(mHandlerThread.getLooper());

            if (mBatch.mDirection == BluetoothShare.DIRECTION_OUTBOUND) {
                /* for outbound transfer, we do connect first */
                startConnectSession();
            } else if (mBatch.mDirection == BluetoothShare.DIRECTION_INBOUND) {
                /*
                 * for inbound transfer, it's already connected, so we start
                 * OBEX session directly
                 */
                startObexSession();
            }
        }
    }

    /**
     * Stop the transfer
     */
    public void stop() {
        if (V) Log.v(TAG, "stop");
        if (mConnectThread != null) {
            try {
                mConnectThread.interrupt();
                if (V) Log.v(TAG, "waiting for connect thread to terminate");
                mConnectThread.join();
            } catch (InterruptedException e) {
                if (V) Log.v(TAG, "Interrupted waiting for connect thread to join");
            }
            mConnectThread = null;
        }
        if (mSession != null) {
            if (V) Log.v(TAG, "Stop mSession");
            mSession.stop();
        }
        if (mHandlerThread != null) {
            mHandlerThread.getLooper().quit();
            mHandlerThread.interrupt();
            mHandlerThread = null;
        }
    }

    private void startObexSession() {

        mBatch.mStatus = Constants.BATCH_STATUS_RUNNING;

        mCurrentShare = mBatch.getPendingShare();
        if (mCurrentShare == null) {
            /*
             * TODO catch this error
             */
            Log.e(TAG, "Unexpected error happened !");
            return;
        }
        if (V) Log.v(TAG, "Start session for info " + mCurrentShare.mId + " for batch " +
                mBatch.mId);

        if (mBatch.mDirection == BluetoothShare.DIRECTION_OUTBOUND) {
            if (V) Log.v(TAG, "Create Client session with transport " + mTransport.toString());
            mSession = new BluetoothOppObexClientSession(mContext, mTransport);
        } else if (mBatch.mDirection == BluetoothShare.DIRECTION_INBOUND) {
            /*
             * For inbounds transfer, a server session should already exists
             * before BluetoothOppTransfer is initialized. We should pass in a
             * mSession instance.
             */
            if (mSession == null) {
                /** set current share as error */
                Log.e(TAG, "Unexpected error happened !");
                markBatchFailed();
                mBatch.mStatus = Constants.BATCH_STATUS_FAILED;
                return;
            }
            if (V) Log.v(TAG, "Transfer has Server session" + mSession.toString());
        }

        mSession.start(mSessionHandler, mBatch.getNumShares());
        processCurrentShare();
    }

    private void processCurrentShare() {
        /* This transfer need user confirm */
        if (V) Log.v(TAG, "processCurrentShare" + mCurrentShare.mId);
        mSession.addShare(mCurrentShare);
        if (mCurrentShare.mConfirm == BluetoothShare.USER_CONFIRMATION_HANDOVER_CONFIRMED) {
            confirmStatusChanged();
        }
    }

    /**
     * Set transfer confirmed status. It should only be called for inbound
     * transfer
     */
    public void confirmStatusChanged() {
        /* unblock server session */
        final Thread notifyThread = new Thread("Server Unblock thread") {
            public void run() {
                synchronized (mSession) {
                    mSession.unblock();
                    mSession.notify();
                }
            }
        };
        if (V) Log.v(TAG, "confirmStatusChanged to unblock mSession" + mSession.toString());
        notifyThread.start();
    }

    private void startConnectSession() {

        if (Constants.USE_TCP_DEBUG) {
            mConnectThread = new SocketConnectThread("localhost", Constants.TCP_DEBUG_PORT, 0);
            mConnectThread.start();
        } else {
            mConnectThread = new SocketConnectThread(mBatch.mDestination,false);
            mConnectThread.start();
        }
    }

    private SocketConnectThread mConnectThread;

    private class SocketConnectThread extends Thread {
        private final String host;

        private final BluetoothDevice device;

        private final int channel;

        private boolean isConnected;

        private long timestamp;

        private BluetoothSocket btSocket = null;

        private boolean mRetry = false;

        /* create a TCP socket */
        public SocketConnectThread(String host, int port, int dummy) {
            super("Socket Connect Thread");
            this.host = host;
            this.channel = port;
            this.device = null;
            isConnected = false;
        }

        /* create a Rfcomm Socket */
        public SocketConnectThread(BluetoothDevice device, int channel, boolean
                retry) {
            super("Socket Connect Thread");
            this.device = device;
            this.host = null;
            this.channel = channel;
            isConnected = false;
            mRetry = retry;
        }

        /* create a Rfcomm Socket */
        public SocketConnectThread(BluetoothDevice device, boolean
                retry) {
            super("Socket Connect Thread");
            this.device = device;
            this.host = null;
            this.channel = -1;
            isConnected = false;
            mRetry = retry;
        }

        public void interrupt() {
            if (!Constants.USE_TCP_DEBUG) {
                if (btSocket != null) {
                    try {
                        btSocket.close();
                    } catch (IOException e) {
                        Log.v(TAG, "Error when close socket");
                    }
                }
            }
        }

        @Override
        public void run() {

            timestamp = System.currentTimeMillis();

            if (Constants.USE_TCP_DEBUG) {
                /* Use TCP socket to connect */
                Socket s = new Socket();

                // Try to connect for 50 seconds
                int result = 0;
                for (int i = 0; i < CONNECT_RETRY_TIME && result == 0; i++) {
                    try {
                        s.connect(new InetSocketAddress(host, channel), CONNECT_WAIT_TIMEOUT);
                    } catch (UnknownHostException e) {
                        Log.e(TAG, "TCP socket connect unknown host ");
                    } catch (IOException e) {
                        Log.e(TAG, "TCP socket connect failed ");
                    }
                    if (s.isConnected()) {
                        if (D) Log.d(TAG, "TCP socket connected ");
                        isConnected = true;
                        break;
                    }
                    if (isInterrupted()) {
                        Log.e(TAG, "TCP socket connect interrupted ");
                        markConnectionFailed(s);
                        return;
                    }
                }
                if (!isConnected) {
                    Log.e(TAG, "TCP socket connect failed after 20 seconds");
                    markConnectionFailed(s);
                    return;
                }

                if (V) Log.v(TAG, "TCP Socket connection attempt took " +
                        (System.currentTimeMillis() - timestamp) + " ms");

                TestTcpTransport transport;
                transport = new TestTcpTransport(s);

                if (isInterrupted()) {
                    isConnected = false;
                    markConnectionFailed(s);
                    transport = null;
                    return;
                }
                if (!isConnected) {
                    transport = null;
                    Log.e(TAG, "TCP connect session error: ");
                    markConnectionFailed(s);
                    return;
                } else {
                    if (D) Log.d(TAG, "Send transport message " + transport.toString());
                    mSessionHandler.obtainMessage(RFCOMM_CONNECTED, transport).sendToTarget();
                }
            } else {

                /* Use BluetoothSocket to connect */

                try {
                    btSocket = device.createInsecureRfcommSocketToServiceRecord(BluetoothUuid.ObexObjectPush.getUuid());
                } catch (IOException e1) {
                    Log.e(TAG, "Rfcomm socket create error",e1);
                    markConnectionFailed(btSocket);
                    return;
                }
                try {
                    btSocket.connect();

                    if (V) Log.v(TAG, "Rfcomm socket connection attempt took " +
                            (System.currentTimeMillis() - timestamp) + " ms");
                    BluetoothObexTransport transport;
                    transport = new BluetoothObexTransport(btSocket);

                    BluetoothOppPreference.getInstance(mContext).setName(device, device.getName());

                    if (V) Log.v(TAG, "Send transport message " + transport.toString());

                    mSessionHandler.obtainMessage(RFCOMM_CONNECTED, transport).sendToTarget();
                } catch (IOException e) {
                    Log.e(TAG, "Rfcomm socket connect exception",e);
                    // If the devices were paired before, but unpaired on the
                    // remote end, it will return an error for the auth request
                    // for the socket connection. Link keys will get exchanged
                    // again, but we need to retry. There is no good way to
                    // inform this socket asking it to retry apart from a blind
                    // delayed retry.
                    if (!mRetry && e.getMessage().equals(SOCKET_LINK_KEY_ERROR)) {
                        Message msg = mSessionHandler.obtainMessage(SOCKET_ERROR_RETRY,-1,-1,device);
                        mSessionHandler.sendMessageDelayed(msg, 1500);
                    } else {
                        markConnectionFailed(btSocket);
                    }
                }
            }
        }

        private void markConnectionFailed(Socket s) {
            try {
                s.close();
            } catch (IOException e) {
                Log.e(TAG, "TCP socket close error");
            }
            mSessionHandler.obtainMessage(RFCOMM_ERROR).sendToTarget();
        }

        private void markConnectionFailed(BluetoothSocket s) {
            try {
                s.close();
            } catch (IOException e) {
                if (V) Log.e(TAG, "Error when close socket");
            }
            mSessionHandler.obtainMessage(RFCOMM_ERROR).sendToTarget();
            return;
        }
    };

    /* update a trivial field of a share to notify Provider the batch status change */
    private void tickShareStatus(BluetoothOppShareInfo share) {
        if (share == null) {
            Log.d(TAG,"Share is null");
            return;
        }
        Uri contentUri = Uri.parse(BluetoothShare.CONTENT_URI + "/" + share.mId);
        ContentValues updateValues = new ContentValues();
        updateValues.put(BluetoothShare.DIRECTION, share.mDirection);
        mContext.getContentResolver().update(contentUri, updateValues, null, null);
    }

    /*
     * Note: For outbound transfer We don't implement this method now. If later
     * we want to support merging a later added share into an existing session,
     * we could implement here For inbounds transfer add share means it's
     * multiple receive in the same session, we should handle it to fill it into
     * mSession
     */
    /**
     * Process when a share is added to current transfer
     */
    public void onShareAdded(int id) {
        BluetoothOppShareInfo info = mBatch.getPendingShare();
        if (info.mDirection == BluetoothShare.DIRECTION_INBOUND) {
            mCurrentShare = mBatch.getPendingShare();
            /*
             * TODO what if it's not auto confirmed?
             */
            if (mCurrentShare != null &&
                    (mCurrentShare.mConfirm == BluetoothShare.USER_CONFIRMATION_AUTO_CONFIRMED ||
                     mCurrentShare.mConfirm ==
                     BluetoothShare.USER_CONFIRMATION_HANDOVER_CONFIRMED)) {
                /* have additional auto confirmed share to process */
                if (V) Log.v(TAG, "Transfer continue session for info " + mCurrentShare.mId +
                        " from batch " + mBatch.mId);
                processCurrentShare();
                confirmStatusChanged();
            }
        }
    }

    /*
     * NOTE We don't implement this method now. Now delete a single share from
     * the batch means the whole batch should be canceled. If later we want to
     * support single cancel, we could implement here For outbound transfer, if
     * the share is currently in transfer, cancel it For inbounds transfer,
     * delete share means the current receiving file should be canceled.
     */
    /**
     * Process when a share is deleted from current transfer
     */
    public void onShareDeleted(int id) {

    }

    /**
     * Process when current transfer is canceled
     */
    public void onBatchCanceled() {
        if (V) Log.v(TAG, "Transfer on Batch canceled");

        this.stop();
        mBatch.mStatus = Constants.BATCH_STATUS_FINISHED;
    }
}
