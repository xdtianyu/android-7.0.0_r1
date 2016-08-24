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

import java.io.IOException;
import java.net.ServerSocket;
import java.net.Socket;

import com.android.bluetooth.BluetoothObexTransport;

import android.bluetooth.BluetoothAdapter;
import android.bluetooth.BluetoothServerSocket;
import android.bluetooth.BluetoothSocket;
import android.bluetooth.BluetoothUuid;
import android.os.Handler;
import android.os.Message;
import android.util.Log;

/**
 * This class listens on OPUSH channel for incoming connection
 */
public class BluetoothOppRfcommListener {
    private static final String TAG = "BtOppRfcommListener";

    private static final boolean V = Constants.VERBOSE;

    public static final int MSG_INCOMING_BTOPP_CONNECTION = 100;

    private volatile boolean mInterrupted;

    private Thread mSocketAcceptThread;

    private Handler mCallback;

    private static final int CREATE_RETRY_TIME = 10;

    private final BluetoothAdapter mAdapter;

    private BluetoothServerSocket mBtServerSocket = null;

    private ServerSocket mTcpServerSocket = null;

    public BluetoothOppRfcommListener(BluetoothAdapter adapter) {
        mAdapter = adapter;
    }


    public synchronized boolean start(Handler callback) {
        if (mSocketAcceptThread == null) {
            mCallback = callback;

            mSocketAcceptThread = new Thread(TAG) {

                public void run() {
                    if (Constants.USE_TCP_DEBUG) {
                        try {
                            if (V) Log.v(TAG, "Create TCP ServerSocket");
                            mTcpServerSocket = new ServerSocket(Constants.TCP_DEBUG_PORT, 1);
                        } catch (IOException e) {
                            Log.e(TAG, "Error listing on port" + Constants.TCP_DEBUG_PORT);
                            mInterrupted = true;
                        }
                        while (!mInterrupted) {
                            try {
                                Socket clientSocket = mTcpServerSocket.accept();

                                if (V) Log.v(TAG, "Socket connected!");
                                TestTcpTransport transport = new TestTcpTransport(clientSocket);
                                Message msg = Message.obtain();
                                msg.setTarget(mCallback);
                                msg.what = MSG_INCOMING_BTOPP_CONNECTION;
                                msg.obj = transport;
                                msg.sendToTarget();

                            } catch (IOException e) {
                                Log.e(TAG, "Error accept connection " + e);
                            }
                        }
                        if (V) Log.v(TAG, "TCP listen thread finished");
                    } else {
                        boolean serverOK = true;

                        /*
                         * it's possible that create will fail in some cases.
                         * retry for 10 times
                         */
                        for (int i = 0; i < CREATE_RETRY_TIME && !mInterrupted; i++) {
                            try {
                                if (V) Log.v(TAG, "Starting RFCOMM listener....");
                                mBtServerSocket = mAdapter.listenUsingInsecureRfcommWithServiceRecord("OBEX Object Push", BluetoothUuid.ObexObjectPush.getUuid());
                                if (V) Log.v(TAG, "Started RFCOMM listener....");
                            } catch (IOException e1) {
                                Log.e(TAG, "Error create RfcommServerSocket " + e1);
                                serverOK = false;
                            }

                            if (!serverOK) {
                                synchronized (this) {
                                    try {
                                        if (V) Log.v(TAG, "Wait 300 ms");
                                        Thread.sleep(300);
                                    } catch (InterruptedException e) {
                                        Log.e(TAG, "socketAcceptThread thread was interrupted (3)");
                                        mInterrupted = true;
                                    }
                                }
                            } else {
                                break;
                            }
                        }
                        if (!serverOK) {
                            Log.e(TAG, "Error start listening after " + CREATE_RETRY_TIME + " try");
                            mInterrupted = true;
                        }
                        if (!mInterrupted) {
                            Log.i(TAG, "Accept thread started.");
                        }
                        BluetoothSocket clientSocket;
                        while (!mInterrupted) {
                            try {
                                if (V) Log.v(TAG, "Accepting connection...");
                                if (mBtServerSocket == null) {

                                }
                                BluetoothServerSocket sSocket = mBtServerSocket;
                                if (sSocket ==null) {
                                    mInterrupted = true;

                                } else {
                                    clientSocket = sSocket.accept();
                                    if (V) Log.v(TAG, "Accepted connection from "
                                        + clientSocket.getRemoteDevice());
                                    BluetoothObexTransport transport = new BluetoothObexTransport(
                                        clientSocket);
                                    Message msg = Message.obtain();
                                    msg.setTarget(mCallback);
                                    msg.what = MSG_INCOMING_BTOPP_CONNECTION;
                                    msg.obj = transport;
                                    msg.sendToTarget();
                                }
                            } catch (IOException e) {
                                Log.e(TAG, "Error accept connection " + e);
                                try {
                                    Thread.sleep(500);
                                } catch (InterruptedException ie) {}
                            }
                        }
                        Log.i(TAG, "BluetoothSocket listen thread finished");
                    }
                }
            };
            mInterrupted = false;
            if(!Constants.USE_TCP_SIMPLE_SERVER) {
                mSocketAcceptThread.start();
            }
        }
        return true;
    }

    public synchronized void stop() {
        if (mSocketAcceptThread != null) {
            Log.i(TAG, "stopping Accept Thread");

            mInterrupted = true;
             if (Constants.USE_TCP_DEBUG) {
                if (V) Log.v(TAG, "close mTcpServerSocket");
                if (mTcpServerSocket != null) {
                    try {
                        mTcpServerSocket.close();
                        mTcpServerSocket = null;
                    } catch (IOException e) {
                        Log.e(TAG, "Error close mTcpServerSocket");
                    }
                }
            } else {
                if (V) Log.v(TAG, "close mBtServerSocket");

                if (mBtServerSocket != null) {
                    try {
                        mBtServerSocket.close();
                        mBtServerSocket = null;
                    } catch (IOException e) {
                        Log.e(TAG, "Error close mBtServerSocket");
                    }
                }
            }
            try {
                mSocketAcceptThread.interrupt();
                if (V) Log.v(TAG, "waiting for thread to terminate");
                //mSocketAcceptThread.join(JOIN_TIMEOUT_MS);
                mSocketAcceptThread.join();
                if (V) Log.v(TAG, "done waiting for thread to terminate");
                mSocketAcceptThread = null;
                mCallback = null;
            } catch (InterruptedException e) {
                if (V) Log.v(TAG, "Interrupted waiting for Accept Thread to join");
            }
        }
    }
}
