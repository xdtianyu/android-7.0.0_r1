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

import android.os.Handler;
import android.os.HandlerThread;
import android.os.Looper;
import android.os.Message;
import android.util.Log;

import java.io.IOException;
import java.lang.ref.WeakReference;

import javax.obex.ClientSession;
import javax.obex.HeaderSet;
import javax.obex.ObexTransport;
import javax.obex.ResponseCodes;

final class BluetoothPbapObexSession {
    private static final boolean DBG = true;
    private static final String TAG = "BluetoothPbapObexSession";

    private static final byte[] PBAP_TARGET = new byte[] {
            0x79, 0x61, 0x35, (byte) 0xf0, (byte) 0xf0, (byte) 0xc5, 0x11, (byte) 0xd8, 0x09, 0x66,
            0x08, 0x00, 0x20, 0x0c, (byte) 0x9a, 0x66
    };

    final static int OBEX_SESSION_CONNECTED = 100;
    final static int OBEX_SESSION_FAILED = 101;
    final static int OBEX_SESSION_DISCONNECTED = 102;
    final static int OBEX_SESSION_REQUEST_COMPLETED = 103;
    final static int OBEX_SESSION_REQUEST_FAILED = 104;
    final static int OBEX_SESSION_AUTHENTICATION_REQUEST = 105;
    final static int OBEX_SESSION_AUTHENTICATION_TIMEOUT = 106;

    final static int MSG_CONNECT = 0;
    final static int MSG_REQUEST = 1;

    final static int CONNECTED = 0;
    final static int CONNECTING = 1;
    final static int DISCONNECTED = 2;

    private Handler mSessionHandler;
    private final ObexTransport mTransport;
    // private ObexClientThread mObexClientThread;
    private BluetoothPbapObexAuthenticator mAuth = null;
    private HandlerThread mThread;
    private Handler mHandler;
    private ClientSession mClientSession;

    private int mState = DISCONNECTED;

    public BluetoothPbapObexSession(ObexTransport transport) {
        mTransport = transport;
    }

    public synchronized boolean start(Handler handler) {
        Log.d(TAG, "start");

        if (mState == CONNECTED || mState == CONNECTING) {
            return false;
        }
        mState = CONNECTING;
        mSessionHandler = handler;

        mAuth = new BluetoothPbapObexAuthenticator(mSessionHandler);

        // Start the thread to process requests (see {@link schedule()}.
        mThread = new HandlerThread("BluetoothPbapObexSessionThread");
        mThread.start();
        mHandler = new ObexClientHandler(mThread.getLooper(), this);

        // Make connect call non blocking.
        boolean status = mHandler.sendMessage(mHandler.obtainMessage(MSG_CONNECT));
        if (!status) {
            mState = DISCONNECTED;
            return false;
        } else {
            return true;
        }
    }

    public void stop() {
        if (DBG) {
            Log.d(TAG, "stop");
        }

        // This will essentially stop the handler and ignore any inflight requests.
        mThread.quit();

        // We clean up the state here.
        disconnect(false /* no callback */);
    }

    public void abort() {
        stop();
    }

    public boolean schedule(BluetoothPbapRequest request) {
        if (DBG) {
            Log.d(TAG, "schedule() called with: " + request);
        }

        boolean status = mHandler.sendMessage(mHandler.obtainMessage(MSG_REQUEST, request));
        if (!status) {
            Log.e(TAG, "Adding messages failed, obex must be disconnected.");
            return false;
        }
        return true;
    }

    public int isConnected() {
        return mState;
    }

    private void connect() {
       if (DBG) {
          Log.d(TAG, "connect()");
       }

       boolean success = true;
       try {
          mClientSession = new ClientSession(mTransport);
          mClientSession.setAuthenticator(mAuth);
       } catch (IOException e) {
          Log.d(TAG, "connect() exception: " + e);
          success = false;
       }

       HeaderSet hs = new HeaderSet();
       hs.setHeader(HeaderSet.TARGET, PBAP_TARGET);
       try {
          hs = mClientSession.connect(hs);

          if (hs.getResponseCode() != ResponseCodes.OBEX_HTTP_OK) {
              disconnect(true  /* callback */);
              success = false;
          }
       } catch (IOException e) {
          success = false;
       }

       synchronized (this) {
           if (success) {
              mSessionHandler.obtainMessage(OBEX_SESSION_CONNECTED).sendToTarget();
              mState = CONNECTED;
           } else {
              mSessionHandler.obtainMessage(OBEX_SESSION_DISCONNECTED).sendToTarget();
              mState = DISCONNECTED;
           }
       }
    }

    private synchronized void disconnect(boolean callback) {
        if (DBG) {
            Log.d(TAG, "disconnect()");
        }

        if (mState != DISCONNECTED) {
            return;
        }

        if (mClientSession != null) {
            try {
                mClientSession.disconnect(null);
                mClientSession.close();
            } catch (IOException e) {
            }
        }

        if (callback) {
            mSessionHandler.obtainMessage(OBEX_SESSION_DISCONNECTED).sendToTarget();
        }

        mState = DISCONNECTED;
    }

    private void executeRequest(BluetoothPbapRequest req) {
        try {
            req.execute(mClientSession);
        } catch (IOException e) {
            Log.e(TAG, "Error during executeRequest " + e);
            disconnect(true);
        }

        if (req.isSuccess()) {
            mSessionHandler.obtainMessage(OBEX_SESSION_REQUEST_COMPLETED, req).sendToTarget();
        } else {
            mSessionHandler.obtainMessage(OBEX_SESSION_REQUEST_FAILED, req).sendToTarget();
        }
    }

    public boolean setAuthReply(String key) {
        Log.d(TAG, "setAuthReply key=" + key);

        if (mAuth == null) {
            return false;
        }

        mAuth.setReply(key);

        return true;
    }

    private static class ObexClientHandler extends Handler {
        WeakReference<BluetoothPbapObexSession> mInst;

        ObexClientHandler(Looper looper, BluetoothPbapObexSession inst) {
            super(looper);
            mInst = new WeakReference<BluetoothPbapObexSession>(inst);
        }

        @Override
        public void handleMessage(Message msg) {
            BluetoothPbapObexSession inst = mInst.get();
            if (inst == null) {
                Log.e(TAG, "The instance class is no longer around; terminating.");
                return;
            }

            if (inst.isConnected() != CONNECTED && msg.what != MSG_CONNECT) {
                Log.w(TAG, "Cannot execute " + msg + " when not CONNECTED.");
                return;
            }

            switch (msg.what) {
                case MSG_CONNECT:
                    inst.connect();
                    break;
                case MSG_REQUEST:
                    inst.executeRequest((BluetoothPbapRequest) msg.obj);
                    break;
                default:
                    Log.e(TAG, "Unknwown message type: " + msg.what);
            }
        }
    }
}

