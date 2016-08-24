/*
 * Copyright (C) 2015 The Android Open Source Project
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

package com.google.android.auto.mapservice;

import android.bluetooth.BluetoothDevice;
import android.bluetooth.BluetoothAdapter;
import android.content.BroadcastReceiver;
import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.content.ServiceConnection;
import android.os.Handler;
import android.os.IBinder;
import android.os.Looper;
import android.os.RemoteException;
import android.util.Log;

import java.lang.ref.WeakReference;
import java.util.List;

public final class BluetoothMapManager {
    public static final String CALLBACK_MISMATCH = "CALLBACK_MISMATCH";

    /**
     * Connection State(s) for the service bound to this Manager.
     * The connection state manage if the Manager is connected to Service:
     * DISCONNECTED: Manager cannot execute calls on service currently because the client either
     * never called connect() on the manager OR the device is disconnected. In the later case the
     * client will have to eventually call connect() again.
     * CONNECTING: This state persists from calling onBind on the service, until we have
     * successfully received onConnect from the service.
     * CONNECTED: The service is successfully connected and ready to receive commands.
     */
    private static final int DISCONNECTED = 0;
    private static final int SUSPENDED = 1;
    private static final int CONNECTING = 2;
    private static final int CONNECTED = 3;

    // Error codes returned via the onError call.

    // On connection suspended the Manager will callback with onConnect when the service is
    // restarted and connected by android binder.
    public static final int ERROR_CONNECT_SUSPENDED = 0;
    // Connection between the service (backed by this Manager) and the remote device has failed.
    // It can be due to variety of reasons such as obex transport failure or the device going out
    // of range. The client will need to either call connect() again. In cases where the device
    // goes out of range, calling connect agian will lead to this error being throw again but if
    // it was a transient failure due to obex transport or other binder issue, then this call will
    // succeed.
    public static final int ERROR_CONNECT_FAILED = 1;

    // String representation of operations.
    private static final String OP_NONE = "";
    private static final String OP_PUSH_MESSAGE = "pushMessage";
    private static final String OP_GET_MESSAGE = "getMessage";
    private static final String OP_GET_MESSAGES_LISTING = "getMessagesListing";
    private static final String OP_ENABLE_NOTIFICATIONS = "enableNotifications";

    private static final boolean DBG = true;
    private static final String TAG = "BluetoothMapManager";
    private final Context mContext;
    private final ConnectionCallbacks mCallbacks;
    private final BluetoothDevice mDevice;
    // We have a handler to make sure that all code modifying/accessing the final non-final
    // objects are serialized. This is done by ensuring the following:
    // a) All calls done by the client (using this manager) should be on the main thread.
    // b) All calls done by the manager not-on main thread (binder threads) should be posted back
    // to main thread using this handler.
    private final Handler mHandler = new Handler();

    private IBluetoothMapService mServiceBinder;
    private IBluetoothMapServiceCallbacks mServiceCallbacks;
    private BluetoothMapServiceConnection mServiceConnection;
    private int mConnectionState = DISCONNECTED;
    private String mOpInflight = OP_NONE;

    public BluetoothMapManager(
        Context context, BluetoothDevice device, ConnectionCallbacks callbacks) {
        if (device == null) {
          throw new IllegalArgumentException("Device cannot be null.");
        }
        if (callbacks == null) {
            throw new IllegalArgumentException(TAG + ": Callbacks cannot be null!");
        }

        if (Looper.getMainLooper().getThread() != Thread.currentThread()) {
            throw new IllegalStateException(
                "Client needs to call the manager from the main UI thread.");
        }

        mDevice = device;
        mContext = context;
        mCallbacks = callbacks;
    }

    /**
     * Defines the callback interface that clients using the Manager should implement in order to
     * receive callbacks and notification for changes happening to either the binder connection
     * between the Manager and Service or MAP profile changes.
     */
    public static abstract class ConnectionCallbacks {
        /**
         * Called when connection has been established successfully with the service and the
         * service itself is successfully connected to MAP profile.
         * See connect().
         */
        public abstract void onConnected();

        /**
         * Called when the Manager is no longer able to execute commands on the service.
         * Client who holds this manager should consider all in-flight commands sent till now as
         * cancelled. For permanent failures such as device going out of range and not coming back
         * the client should call connect() again too continue working otherwise
         * when the Manager does connect back it will call onConnected() (see above).
         */
        public abstract void onError(int errorCode);

        /**
         * Callen when notification status has been adjusted.
         * See enableNotifications().
         */
        public abstract void onEnableNotifications();

        /**
         * Called when the message has been queued for sending.
         * The argument always contains a "valid" handle.
         * See pushMessage().
         */
        public abstract void onPushMessage(String handle);

        /**
         * Called when the message is fetched.
         * See getMessage().
         */
        public abstract void onGetMessage(BluetoothMapMessage msg);

        /**
         * Called when the messages listing is retrieved.
         * See getMessagesListing().
         */
        public abstract void onGetMessagesListing(List<BluetoothMapMessagesListing> msgsListing);

        /**
         * Called when an event has occured.
         * See BluetoothMapEventReport for a description of what an event may look like.
         */
        public abstract void onEvent(BluetoothMapEventReport eventReport);

    }

    /**
     * Bind to the service and register the callback passed in the constructor.
     */
    public boolean connect() {
        checkMainThread();

        if (mConnectionState != DISCONNECTED && mConnectionState != SUSPENDED) {
            Log.w(TAG, "Not in disconnected state, connection will eventually resume: " +
                mConnectionState);
            return true;
        }
        mConnectionState = CONNECTING;
        mServiceConnection = new BluetoothMapServiceConnection();

        boolean bound = false;
        ComponentName cName =
            new ComponentName(
                "com.google.android.auto.mapservice",
                "com.google.android.auto.mapservice.BluetoothMapService");
        final Intent intent = new Intent();
        intent.setComponent(cName);
        try {
            bound = mContext.bindService(intent, mServiceConnection, Context.BIND_AUTO_CREATE);
        } catch (Exception ex) {
            Log.e(TAG, "Failed binding to service." + ex);
            dumpState();
            return false;
        }

        if (!bound) {
            forceCloseConnection();
            dumpState();
            return false;
        }

        dumpState();
        return true;
    }

    /**
     * Unregister the callbacks and unbind from the service.
     */
    public void disconnect() {
        checkMainThread();

        if (DBG) {
            Log.d(TAG, "Calling IBluetoothMapService.disconnect ...");
        }

        // In case the manager is already disconnected, we don't need to do anything more here.
        if (mServiceBinder != null) {
            try {
                mServiceBinder.disconnect(mServiceCallbacks);
            } catch (RemoteException ex) {
                Log.w(TAG, "RemoteException during disconnect for " + mServiceConnection);
            } catch (IllegalStateException ex) {
                sendError(ex);
            }
        }
        forceCloseConnection();
        dumpState();
    }

    /**
     * Enable notifications.
     */
    public void enableNotifications(boolean status) {
        checkMainThread();

        if (DBG) {
            Log.d(TAG, "Calling IBluetoothMapService.enableNotifications ..." + status);
        }

        if (mConnectionState != CONNECTED) {
            if (DBG) {
                Log.d(TAG, "Not connected to service.");
            }
            throw new IllegalStateException(
                "Service is not connected, either connect() is not called or a disconnect " +
                "event is not handled correctly.");
        }

        if (!mOpInflight.equals(OP_NONE)) {
            throw new IllegalStateException(
                TAG + "Operation already in flight: " + mOpInflight +
                ". Please wait for an appropriate callback from your previous operation.");
        }

        try {
            mServiceBinder.enableNotifications(mServiceCallbacks, status);
        } catch (RemoteException ex) {
            // If we have disconnected then the client will get a ERROR_CONNECT_SUSPENDED.
            Log.e(TAG, "", ex);
            return;
        } catch (IllegalStateException ex) {
            sendError(ex);
        }
        mOpInflight = OP_ENABLE_NOTIFICATIONS;
    }

    /**
     * Push a message.
     */
    public void pushMessage(BluetoothMapMessage msg) {
        checkMainThread();

        if (mConnectionState != CONNECTED) {
            throw new IllegalStateException(
                "Service is not connected, either connect() is not called or a disconnect " +
                "event is not handled correctly.");
        }

        if (!mOpInflight.equals(OP_NONE)) {
            throw new IllegalStateException(
                TAG + "Operation already in flight: " + mOpInflight +
                ". Please wait for an appropriate callback from your previous operation.");
        }

        try {
            mServiceBinder.pushMessage(mServiceCallbacks, msg);
        } catch (RemoteException ex) {
            // If we have disconnected then the client will get a ERROR_CONNECT_SUSPENDED.
            Log.e(TAG, "", ex);
            return;
        } catch (IllegalStateException ex) {
            sendError(ex);
        }
        mOpInflight = OP_PUSH_MESSAGE;
    }

    /**
     * Get a message by its handle.
     */
    public void getMessage(String handle) {
        checkMainThread();

        if (mConnectionState != CONNECTED) {
            throw new IllegalStateException(
                "Service is not connected, either connect() is not called or a disconnect " +
                "event is not handled correctly.");
        }

        if (!mOpInflight.equals(OP_NONE)) {
            throw new IllegalStateException(
                TAG + "Operation already in flight: " + mOpInflight +
                ". Please wait for an appropriate callback from your previous operation.");
        }

        try {
            mServiceBinder.getMessage(mServiceCallbacks, handle);
        } catch (RemoteException ex) {
            // If we have disconnected then the client will get a ERROR_CONNECT_SUSPENDED.
            Log.e(TAG, "", ex);
            return;
        } catch (IllegalStateException ex) {
            sendError(ex);
        }
        mOpInflight = OP_GET_MESSAGE;
    }

    public void getMessagesListing(String folder) {
        // If count is not specified the cap is put by the Bluetooth spec, we pass a large number
        // to use the cap provided by spec implementation on MAS server.
        getMessagesListing(folder, 65535, 0);
    }
    public void getMessagesListing(String folder, int count) {
        getMessagesListing(folder, count, 0);
    }
    public void getMessagesListing(String folder, int count, int offset) {
        checkMainThread();

        if (mConnectionState != CONNECTED) {
            throw new IllegalStateException(
                "Service is not connected, either connect() is not called or a disconnect " +
                "event is not handled correctly.");
        }

        if (!mOpInflight.equals(OP_NONE)) {
            throw new IllegalStateException(
                TAG + "Operation already in flight: " + mOpInflight +
                ". Please wait for an appropriate callback from your previous operation.");
        }

        try {
            mServiceBinder.getMessagesListing(mServiceCallbacks, folder, count, offset);
        } catch (RemoteException ex) {
            // If we have disconnected then the client will get a ERROR_CONNECT_SUSPENDED.
            Log.e(TAG, "", ex);
            return;
        } catch (IllegalStateException ex) {
            sendError(ex);
        }
        mOpInflight = OP_GET_MESSAGES_LISTING;
    }

    /**
     * Checks if the current thread is main thread.
     *
     * Throws an IllegalStateException otherwise.
     */
    void checkMainThread() {
        if (Looper.getMainLooper().getThread() != Thread.currentThread()) {
            throw new IllegalStateException(
                "Manager APIs should be called only from main thread.");
        }
    }

    /**
     * Implements the callbacks when changes in the binder connection of Manager (this) and the
     * BluetoothMapService occur.
     * For a list of possible states that the binder connection can exist, see DISCONNECTED,
     * CONNECTING and CONNECTED states above.
     */
    private class BluetoothMapServiceConnection implements ServiceConnection {
        /**
         * Called when the Manager connects to service via the binder */
        @Override
        public void onServiceConnected(ComponentName name, IBinder binder) {
            // onServiceConnected can be called either because we called onBind and in which case
            // connection state should already be CONNECTING, or it could be called because the
            // service came back up in which case we need to set it to CONNECTING (from
            // DISCONNECTED).
            if (mConnectionState == CONNECTED) {
                throw new IllegalStateException(
                    "Cannot be in connected state while (re)connecting.");
            }
            // We may either be in CONNECTING or DISCONNECTED state here. Its safe to set to
            // CONNECTING in any scenario.
            mConnectionState = CONNECTING;

            if (DBG) {
                Log.d(TAG, "BluetoothMapServiceConnection.onServiceConnected name=" +
                    name + " binder=" + binder);
            }

            // Save the binder for future calls to service.
            mServiceBinder = IBluetoothMapService.Stub.asInterface(binder);

            // Register the callbacks to the service.
            mServiceCallbacks = new ServiceCallbacks(BluetoothMapManager.this);

            try {
                if (DBG) {
                  Log.d(TAG, "ServiceCallbacks.connect ...");
                }
                boolean status = mServiceBinder.connect(mServiceCallbacks, mDevice);
                if (DBG && !status) {
                    Log.d(TAG, "Failed to connect to service after binding.");
                }
            } catch (RemoteException ex) {
                Log.d(TAG, "connect failed with RemoteException.");
            }
        }

        /**
         * Called when the service is disconnected from the manager due to binder failure.
         */
        @Override
        public void onServiceDisconnected(ComponentName name) {
            if (DBG) {
                Log.d(TAG, "BluetoothMapServiceConnection.onServiceDisconnected name=" + name
                        + " this=" + this + "mServiceConnection=" + mServiceConnection);
            }

            mConnectionState = SUSPENDED;

            mServiceBinder = null;
            mServiceCallbacks = null;
            mOpInflight = OP_NONE;
            mCallbacks.onError(ERROR_CONNECT_SUSPENDED);
        }
    }

    /**
     * Implements the AIDL interface which is called by BluetoothMapService either in reply to any
     * of the commands issued to it via the service binder or when there's a new notification that
     * service has to push to Manager.
     */
    private static class ServiceCallbacks extends IBluetoothMapServiceCallbacks.Stub {
        private WeakReference<BluetoothMapManager> mMngr;

        public ServiceCallbacks(BluetoothMapManager manager) {
            mMngr = new WeakReference<BluetoothMapManager>(manager);
        }

        /**
         * Called when the service is successfully connected to a remote device and is capable to
         * execute the MAP profile.
         */
        @Override
        public void onConnect() {
            if (DBG) {
                Log.d(TAG, "IBluetoothMapServiceCallbacks.onConnect() called.");
            }

            // Client may clean up the manager before the service responds, we may have a race
            // if the manager instance has disappeared, in which case we can return silently.
            BluetoothMapManager mgr = mMngr.get();
            if (mgr == null) return;

            mgr.onServiceConnected();
        }

        /**
         * Called when the service is not connected to a remote device and cannot execute the MAP
         * profile.
         */
        @Override
        public void onConnectFailed() {
            if (DBG) {
               Log.d(TAG, "IBluetoothMapServiceCallbacks.onConnectionFailed() called.");
            }

            // Client may clean up the manager before the service responds, we may have a race if
            // the manager instance has disappeared, in which case we can return silently.
            BluetoothMapManager mgr = mMngr.get();
            if (mgr == null) return;

           mgr.onServiceConnectionFailed();
        }

        @Override
        public void onEnableNotifications() {
            if (DBG) {
                Log.d(TAG, "IBluetoothMapServiceCallbacks.onEnableNotifications() called.");
            }

            // Client may clean up the manager before the service responds, we may have a race if
            // the manager instance has disappeared, in which case we can return silently.
            BluetoothMapManager mgr = mMngr.get();
            if (mgr == null) return;

            mgr.onEnableNotifications();
        }

        @Override
        public void onPushMessage(String handle) {
            if (DBG) {
                Log.d(TAG, "IBluetoothMapServiceCallbacks.onPushMessage() called with " + handle);
            }

            // Client may clean up the manager before the service responds, we may have a race if
            // the manager instance has disappeared, in which case we can return silently.
            BluetoothMapManager mgr = mMngr.get();
            if (mgr == null) return;

            mgr.onPushMessage(handle);
        }

        @Override
        public void onGetMessage(BluetoothMapMessage msg) {
            if (DBG) {
                Log.d(TAG, "IBluetoothMapServiceCallbacks.onGetMessage() called with " + msg);
            }

            // Client may clean up the manager before the service responds, we may have a race if
            // the manager instance has disappeared, in which case we can return silently.
            BluetoothMapManager mgr = mMngr.get();
            if (mgr == null) return;

            mgr.onGetMessage(msg);
        }

        @Override
        public void onGetMessagesListing(List<BluetoothMapMessagesListing> msgsListing) {
            if (DBG) {
                Log.d(TAG, "IBluetoothMapServiceCallbacks.onGetMessagesListing() called with " +
                    msgsListing);
            }

            // Client may clean up the manager before the service responds, we may have a race if
            // the manager instance has disappeared, in which case we can return silently.
            BluetoothMapManager mgr = mMngr.get();
            if (mgr == null) return;

            mgr.onGetMessagesListing(msgsListing);
        }

        @Override
        public void onEvent(BluetoothMapEventReport eventReport) {
             if (DBG) {
                Log.d(TAG, "IBluetoothMapServiceCallbacks.onEvent() called with " + eventReport);
             }

             BluetoothMapManager mngr = mMngr.get();
            // Client may clean up the manager before the service responds, we may have a race if
            // the manager instance has disappeared, in which case we can return silently.
            BluetoothMapManager mgr = mMngr.get();
            if (mgr == null) return;

            mgr.onEvent(eventReport);
        }
    };

    private void onServiceConnected() {
        mHandler.post(new Runnable() {
            @Override
            public void run() {
                mConnectionState = CONNECTED;
                mCallbacks.onConnected();
                dumpState();
            }
        });
    }

    private void onServiceConnectionFailed() {
        mHandler.post(new Runnable() {
            @Override
            public void run() {
                forceCloseConnection();
                mCallbacks.onError(ERROR_CONNECT_FAILED);
                dumpState();
            }
        });
    }

    private void onEnableNotifications() {
        mHandler.post(new Runnable() {
            @Override
            public void run() {
                if (!mOpInflight.equals(OP_ENABLE_NOTIFICATIONS)) {
                    throw new IllegalStateException(
                        TAG + " Expected Inflight op: " + OP_ENABLE_NOTIFICATIONS +
                        " actual op: " + mOpInflight);
                }
                mOpInflight = OP_NONE;
                mCallbacks.onEnableNotifications();
            }
        });
    }

    private void onPushMessage(final String handle) {
        mHandler.post(new Runnable() {
            @Override
            public void run() {
                if (!mOpInflight.equals(OP_PUSH_MESSAGE)) {
                    throw new IllegalStateException(
                        TAG + " Expected Inflight op: " + OP_PUSH_MESSAGE +
                        " actual op: " + mOpInflight);
                }

                if (handle == null || handle.equals("")) {
                    Log.e(TAG, "Empty handle, the service may have been disconnected.");
                }
                mOpInflight = OP_NONE;
                mCallbacks.onPushMessage(handle);
            }
        });
    }

    private void onGetMessage(final BluetoothMapMessage msg) {
        mHandler.post(new Runnable() {
            @Override
            public void run() {
                if (!mOpInflight.equals(OP_GET_MESSAGE)) {
                    throw new IllegalStateException(
                        TAG + " Expected inflight op: " + OP_GET_MESSAGE +
                        " actual op: " + mOpInflight);
                }
                mOpInflight = OP_NONE;
                mCallbacks.onGetMessage(msg);
            }
        });
    }

    private void onGetMessagesListing(final List<BluetoothMapMessagesListing> msgsListing) {
        mHandler.post(new Runnable() {
            @Override
            public void run() {
                if (!mOpInflight.equals(OP_GET_MESSAGES_LISTING)) {
                    throw new IllegalStateException(
                        TAG + " Expected inflight op: " + OP_GET_MESSAGES_LISTING +
                        " actual op: " + mOpInflight);
                }
                mOpInflight = OP_NONE;
                mCallbacks.onGetMessagesListing(msgsListing);
            }
        });
    }

    private void onEvent(final BluetoothMapEventReport eventReport) {
        mHandler.post(new Runnable() {
            @Override
            public void run() {
                mCallbacks.onEvent(eventReport);
            }
        });
    }

    private void forceCloseConnection() {
        if (mConnectionState == DISCONNECTED) {
            Log.e(TAG, "Connection already closed.");
            return;
        }
        mConnectionState = DISCONNECTED;

        if (mServiceConnection != null) {
            mContext.unbindService(mServiceConnection);
        }
        mServiceConnection = null;
        mServiceCallbacks = null;
        mServiceBinder = null;
        // Even if there is an inflight message, we will never hear from the callback now.
        mOpInflight = OP_NONE;
    }

    private void sendError(IllegalStateException ex) {
        if (ex.getMessage().equals(CALLBACK_MISMATCH)) {
            throw new IllegalStateException(
                "Client tried to call with an unregistered callback. This can happen if either " +
                "client never called connect() or if it got disconnected and GCed by service but " +
                "forgot to call connect(). Check your connection state and reconnect.");
        } else {
            throw new IllegalArgumentException(TAG + " unknown exception: " + ex.toString());
        }
    }

    // Log the state of Manager. Useful for debugging.
    private void dumpState() {
        if (!DBG) {
            return;
        }
        Log.d(TAG, "dumpState(). Connection State: " + mConnectionState);
    }
}

