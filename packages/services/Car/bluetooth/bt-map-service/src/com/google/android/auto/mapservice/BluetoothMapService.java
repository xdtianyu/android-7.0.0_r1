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

import android.app.Service;
import android.bluetooth.BluetoothAdapter;
import android.bluetooth.BluetoothDevice;
import android.bluetooth.BluetoothUuid;
import android.bluetooth.SdpMasRecord;
import android.bluetooth.client.map.BluetoothMapBmessage;
import android.bluetooth.client.map.BluetoothMasClient;
import android.bluetooth.client.map.BluetoothMasClient.CharsetType;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.os.Binder;
import android.os.Bundle;
import android.os.IBinder;
import android.os.Handler;
import android.os.Message;
import android.os.RemoteException;
import android.util.Log;
import android.util.Pair;
import com.android.vcard.VCardEntry;
import com.android.vcard.VCardProperty;
import com.android.vcard.VCardConstants;
import com.google.android.auto.mapservice.BluetoothMapManager;
import com.google.android.auto.mapservice.BluetoothMapMessage;
import com.google.android.auto.mapservice.BluetoothMapMessagesListing;
import com.google.android.auto.mapservice.BluetoothMapEventReport;
import com.google.android.auto.mapservice.IBluetoothMapService;
import com.google.android.auto.mapservice.IBluetoothMapServiceCallbacks;

import java.lang.ref.WeakReference;
import java.util.ArrayList;
import java.util.LinkedList;
import java.util.List;
import java.util.Queue;

/**
 * Service to provide a channel for SMS interaction with remote device.
 *
 * The service can be used to send/browse text SMS messages and also recieve notifications
 * for new incoming messages or delivery notifications on sent messages.
 *
 * Connection Model
 * ----------------
 *
 *  The service only cares about one device (and one external connection) at a time. Also it is
 *  reactive in nautre, i.e. it will *not* actively look if the connection between here and remote
 *  device has been dropped. What this means is that if the connection does indeed gets dropped -
 *  service will only send a connection failure on the next command that is executed. It is assumed
 *  that the caller can then take appropriate error handling decisions. A Manager can wrap execpted
 *  cases of disconnection (such as adapters on either side being turned off/on).
 *
 * Execution Model
 * ---------------
 *  The service provides following types of commands:
 *  a) connect(): Connect will try to initiate a connection with remote device which
 *  it does not already have with. If the service is already connected it will refuse to do so. When
 *  the device is connected or connection gets failed, onConnect{Failed}() callbacks will be called.
 *  b) disconnect(): Disconnect is a no-callback command which is synchronously disconnect the
 *  remote device.
 *  c) pushMessage, browseMessage (etc): These are user commands which can happen within a connect()
 *  disconnect() session. They will be followed by a onX() callback where X is the user method. In
 *  case there is a snap of connection while executing these commands - the service will fire
 *  onConnectFailed(). The user of this service should appropriately handle those conditions.
 */
public class BluetoothMapService extends Service {
    private static final String TAG = "BluetoothMapMceService";
    private static final boolean DBG = true;

    private static final int FAIL_CALLBACK = 1;

    // Connection statuses.
    private static final int DISCONNECTING = 0;
    private static final int DISCONNECTED = 1;
    private static final int SDP = 2;
    private static final int CONNECTING = 3;
    private static final int CONNECTED = 4;

    // MapServiceHandler message types.
    private static final int MSG_MAS_SDP = 1;
    private static final int MSG_MAS_SDP_DONE = 2;
    private static final int MSG_MAS_CONNECT_DONE = 3;
    private static final int MSG_ENABLE_NOTIFICATIONS = 4;
    private static final int MSG_SET_PATH = 5;
    private static final int MSG_PUSH_MESSAGE = 6;
    private static final int MSG_GET_MESSAGE = 7;
    private static final int MSG_GET_MESSAGES_LISTING = 8;

    // SMS is supported via GSM or CDMA is marked by Bit 1 or Bit 2 of the supported features.
    private static final int SMS_SUPPORT = 6; // (0110)
    // Folder names.
    private static final String FOLDER_TELECOM = "telecom";
    private static final String FOLDER_MSG = "msg";
    private static final String FOLDER_OUTBOX = "outbox";
    private static final String FOLDER_INBOX = "inbox";
    // By default we will be in the ROOT folder.
    private String mFolder = "";

    // Handler to run all service methods in. We don't execute the methods in a separate
    // thread since the BluetoothMasClient already has its thread to execute long running calls
    // We still use Handler for synchronization and atomicity of incoming/outgoing operations.
    private MapServiceHandler mHandler = new MapServiceHandler(this);

    private ServiceBinder mBinder;

    private int mMapConnectionStatus = DISCONNECTED;

    // Bluetooth MAP related variables.
    private BluetoothDevice mDevice;
    private BluetoothMasClient mClient;
    private SdpMasRecord mMasInstance;
    private IBluetoothMapServiceCallbacks mCallbacks;
    private Object mCallbacksLock = new Object();
    private boolean mEnableNotifications = false;

    // Listen to SDP broadcasts.
    private final BroadcastReceiver mBtReceiver = new BroadcastReceiver() {
        @Override
        public void onReceive(Context context, Intent intent) {
            if (DBG) {
                Log.d(TAG, "Received broadcast intent " + intent);
            }

            if (BluetoothDevice.ACTION_SDP_RECORD.equals(intent.getAction())) {
                // Check if we have a valid SDP record.
                SdpMasRecord masRecord =
                    intent.getParcelableExtra(BluetoothDevice.EXTRA_SDP_RECORD);
                int status = intent.getIntExtra(BluetoothDevice.EXTRA_SDP_SEARCH_STATUS, -1);
                if (masRecord == null) {
                    Log.w(TAG, "SDP search ended with no MAS record. Status: " + status);
                    disconnectInternal(false);
                    return;
                }
                synchronized (BluetoothMapService.this) {
                    // Since the discovery for MAS record is successful, connect to device.
                    mHandler.obtainMessage(MSG_MAS_SDP_DONE, masRecord).sendToTarget();
                }
            }
        }
    };

    private static class MapServiceHandler extends Handler {
        private WeakReference<BluetoothMapService> mBluetoothMapService;

        public MapServiceHandler(BluetoothMapService service) {
            mBluetoothMapService = new WeakReference<BluetoothMapService>(service);
        }

        @Override
        public void handleMessage(Message msg) {
            BluetoothMapService service = mBluetoothMapService.get();
            // If the service has been GCed but we have a dangling static class left, just ignore
            // the request.
            if (service == null) {
                return;
            }

            if (DBG) {
                Log.d(TAG, "Handling " + msg);
            }

            // If this is not a connect() request, then any other message in disconnected state
            // should be ignored.
            int connStatus = service.getConnectionStatus();
            if (connStatus == DISCONNECTED || connStatus == DISCONNECTING) {
                Log.d(TAG,
                    "Ignoring msg: " + msg + " because service not connected: " + connStatus);
                return;
            }

            switch (msg.what) {
                case MSG_MAS_SDP:
                    // First step to connection is to figure out the right channel to connect to.
                    BluetoothDevice device = (BluetoothDevice) msg.obj;
                    boolean ret = device.sdpSearch(BluetoothUuid.MAS);
                    if (!ret) {
                        Log.e(TAG, "SDP failed initiation.");
                        service.disconnectInternal(true);
                    }
                    break;

                case MSG_MAS_SDP_DONE:
                    // Check if we have the SMS capability for the MAS record reported.
                    SdpMasRecord sdpRecord = (SdpMasRecord) msg.obj;
                    if (DBG) {
                        Log.d(TAG, "SDP record: " + sdpRecord);
                    }

                    if ((sdpRecord.getSupportedMessageTypes() & SMS_SUPPORT) != 0) {
                        service.connectToSdpRecord(sdpRecord);
                    }
                    break;

                case MSG_MAS_CONNECT_DONE:
                    // We have connected successfully, now change into the appropriate directory.
                    this.obtainMessage(MSG_SET_PATH).sendToTarget();
                    break;

                case MSG_ENABLE_NOTIFICATIONS:
                    boolean status = (boolean) msg.obj;
                    service.enableNotifications(status);
                    break;

                case MSG_SET_PATH:
                    // This case is ONLY used to transition into telecom/msg folder.
                    String currFolder = service.getFolder();
                    if (DBG) {
                        Log.d(TAG, "Current folder: " + currFolder);
                    }

                    if (currFolder.equals("")) {
                        service.setPathDown(FOLDER_TELECOM);
                    } else if (currFolder.endsWith(FOLDER_TELECOM)) {
                        service.setPathDown(FOLDER_MSG);
                    } else if (currFolder.endsWith(FOLDER_MSG)) {
                        service.connectionSuccessful();
                    } else {
                        Log.e(TAG, "Should not be here. " + currFolder);
                    }
                    break;

                case MSG_PUSH_MESSAGE:
                    service.pushMessage((BluetoothMapMessage) msg.obj);
                    break;

                case MSG_GET_MESSAGE:
                    service.getMessage((String) msg.obj);
                    break;

                case MSG_GET_MESSAGES_LISTING:
                    service.getMessagesListing((String) msg.obj, msg.arg1, msg.arg2);
                    break;

                default:
                    Log.e(TAG, "Invalid message in MapServiceHandler.handleMessage() " + msg.what);
                    break;
            }
        }
    }

    // Handle the callbacks from the BluetoothMasClient (see mClient).
    private static class BluetoothMapEventHandler extends Handler {
        private WeakReference<BluetoothMapService> mBluetoothMapService;

        public BluetoothMapEventHandler(BluetoothMapService service) {
            mBluetoothMapService = new WeakReference<BluetoothMapService>(service);
        }

        @Override
        public void handleMessage(Message msg) {
            BluetoothMapService service = mBluetoothMapService.get();
            // If the service has been GCed but we have a dangling static class left, just ignore
            // the request.
            if (service == null) {
                return;
            }

            if (DBG) {
                Log.d(TAG, "Received message from MAP client: " + msg);
            }
            switch (msg.what) {
                case BluetoothMasClient.EVENT_CONNECT:
                    if (DBG) {
                        Log.d(TAG, "Connected via OBEX with status " + msg.arg1);
                    }

                    if (msg.arg1 == BluetoothMasClient.STATUS_FAILED) {
                        Log.d(TAG, "Remote device disconnected.");
                        service.disconnectInternal(true);
                        return;
                    } else {
                        service.onConnectToSdpDone();
                    }
                    break;

                case BluetoothMasClient.EVENT_SET_NOTIFICATION_REGISTRATION:
                    if (DBG) {
                        Log.d(TAG, "Set notifications: " + msg.obj);
                    }
                    service.onEnableNotifications();
                    break;

                case BluetoothMasClient.EVENT_SET_PATH:
                    if (DBG) {
                        Log.d(TAG, "Set path: " + msg.obj);
                    }
                    service.onSetPath((String) msg.obj);
                    break;

                case BluetoothMasClient.EVENT_PUSH_MESSAGE:
                    if (DBG) {
                        Log.d(TAG, "Push message: " + msg.obj);
                    }
                    service.onPushMessage((String) msg.obj);
                    break;

                case BluetoothMasClient.EVENT_EVENT_REPORT:
                    if (DBG) {
                        Log.d(TAG, "Event report: " + msg.obj);
                    }
                    service.onEventReport(
                        (android.bluetooth.client.map.BluetoothMapEventReport) msg.obj);
                    break;

                case BluetoothMasClient.EVENT_GET_MESSAGE:
                    if (DBG) {
                        Log.d(TAG, "New message: " + msg.obj);
                    }
                    service.onGetMessage((BluetoothMapBmessage) msg.obj);
                    break;

                case BluetoothMasClient.EVENT_GET_MESSAGES_LISTING:
                    if (DBG) {
                        Log.d(TAG, "Messages Listing: " + msg.obj);
                    }
                    service.onGetMessagesListing(
                        (ArrayList<android.bluetooth.client.map.BluetoothMapMessage>) msg.obj);
                    break;

                default:
                    Log.w(TAG, "Cannot handle map client event of type: " + msg.what);
            }
        }
    }

    // Interface which defines the capabilities of this service.
    private static class ServiceBinder extends IBluetoothMapService.Stub {
        WeakReference<BluetoothMapService> mBluetoothMapService;
        ServiceBinder(BluetoothMapService service) {
            mBluetoothMapService = new WeakReference<BluetoothMapService>(service);
        }

        @Override
        public boolean connect(
            IBluetoothMapServiceCallbacks callbacks,
            BluetoothDevice device) {
            if (callbacks == null || device == null) {
                throw new IllegalArgumentException("Callback or device cannot be null.");
            }

            BluetoothMapService service = mBluetoothMapService.get();
            if (service != null) {
                return service.connectInternal(callbacks, device);
            } else {
                return false;
            }
        }

        @Override
        public void disconnect(IBluetoothMapServiceCallbacks callback) {
            BluetoothMapService service = mBluetoothMapService.get();
            if (service == null) return;

            if (callback == null) {
                throw new IllegalArgumentException("Callback cannot be null.");
            }

            IBluetoothMapServiceCallbacks callbackRef = service.mCallbacks;
            if (service.mCallbacks.asBinder() != callback.asBinder()) {
                Log.e(TAG, "Original: " + service.mCallbacks.asBinder() +
                    " Given: " + callback.asBinder());
                throw new IllegalStateException(BluetoothMapManager.CALLBACK_MISMATCH);
            }

            service.disconnectInternal(false);
            return;
        }

        @Override
        public boolean enableNotifications(IBluetoothMapServiceCallbacks callback, boolean enable) {
            BluetoothMapService service = mBluetoothMapService.get();
            if (service == null) return false;

            if (callback == null) {
                throw new IllegalArgumentException("Callback cannot be null.");
            }

            synchronized (service) {
                if (service.mMapConnectionStatus != CONNECTED) {
                    if (DBG) {
                        Log.d(TAG, "enableRegistration: Not connected.");
                    }
                    return false;
                } else if (service.mCallbacks.asBinder() != callback.asBinder()) {
                    throw new IllegalStateException(BluetoothMapManager.CALLBACK_MISMATCH);
                }
                service.mHandler.obtainMessage(MSG_ENABLE_NOTIFICATIONS, enable).sendToTarget();
            }
            return true;
        }

        @Override
        public boolean pushMessage(IBluetoothMapServiceCallbacks callback,
            BluetoothMapMessage message) {
            BluetoothMapService service = mBluetoothMapService.get();
            if (service == null) return false;

            if (DBG) {
                Log.d(TAG, "pushMessage called.");
            }
            if (callback == null || message == null) {
                throw new IllegalArgumentException("Callback or message cannot be null.");
            }

            synchronized (service) {
                if (service.mMapConnectionStatus != CONNECTED) {
                    return false;
                } else if (service.mCallbacks.asBinder() != callback.asBinder()) {
                    throw new IllegalStateException(BluetoothMapManager.CALLBACK_MISMATCH);
                }
                service.mHandler.obtainMessage(MSG_PUSH_MESSAGE, message).sendToTarget();
            }
            return true;
        }

        @Override
        public boolean getMessage(IBluetoothMapServiceCallbacks callback, String handle) {
            BluetoothMapService service = mBluetoothMapService.get();
            if (service == null) return false;

            if (DBG) {
                Log.d(TAG, "getMessge called.");
            }
            if (callback == null) {
              throw new IllegalArgumentException("Callback cannot be null.");
            }

            synchronized (service) {
                if (service.mMapConnectionStatus != CONNECTED) {
                    return false;
                } else if (service.mCallbacks.asBinder() != callback.asBinder()) {
                    throw new IllegalStateException(BluetoothMapManager.CALLBACK_MISMATCH);
                }
                service.mHandler.obtainMessage(MSG_GET_MESSAGE, handle).sendToTarget();
            }
            return true;
        }

        @Override
        public boolean getMessagesListing(
            IBluetoothMapServiceCallbacks callback, String folder, int count, int offset) {
            BluetoothMapService service = mBluetoothMapService.get();
            if (service == null) return false;

            if (DBG) {
                Log.d(TAG, "getMessgesListing called.");
            }
            if (callback == null) {
                throw new IllegalArgumentException("Callback cannot be null.");
            }

            if (count < 0) {
                throw new IllegalArgumentException("Count cannot be < 0: " + count);
            }

            if (offset < 0) {
                throw new IllegalArgumentException("Offset cannot be < 0: " + offset);
            }

            synchronized (service) {
                if (service.mMapConnectionStatus != CONNECTED) {
                    return false;
                } else if (service.mCallbacks.asBinder() != callback.asBinder()) {
                    throw new IllegalStateException(BluetoothMapManager.CALLBACK_MISMATCH);
                }
                service.mHandler.obtainMessage(
                    MSG_GET_MESSAGES_LISTING, count, offset, folder).sendToTarget();
            }
            return true;
        }
    }

    // Death recipient to tell if the binder connection is gone.
    private final class BinderDeath implements IBinder.DeathRecipient {
        @Override
        public void binderDied() {
            if (DBG) {
                Log.d(TAG, "Binder died, disconnecting ...");
            }
            disconnectInternal(false);
        }
    }

    @Override
    public void onCreate() {
        super.onCreate();

        // Initialize binder interface.
        mBinder = new ServiceBinder(this);

        IntentFilter filter = new IntentFilter();
        filter.addAction(BluetoothDevice.ACTION_SDP_RECORD);
        registerReceiver(mBtReceiver, filter);
    }

    @Override
    public void onDestroy() {
        if (DBG) {
            Log.d(TAG, "Unregistering receiver and shutting down the service.");
        }
        disconnectInternal(false);
        unregisterReceiver(mBtReceiver);
    }

    @Override
    public IBinder onBind(Intent intent) {
        return mBinder;
    }

    private int getConnectionStatus() {
        return mMapConnectionStatus;
    }

    private synchronized void setConnectionStatus(int status) {
        mMapConnectionStatus = status;
    }

    private synchronized void connectToSdpRecord(SdpMasRecord sdpRecord) {
        if (mMapConnectionStatus != SDP) return;
        mMapConnectionStatus = CONNECTING;
        mClient =
            new BluetoothMasClient(
                mDevice, sdpRecord, new BluetoothMapEventHandler(this));
        mClient.connect();
    }

    private synchronized void onConnectToSdpDone() {
        mHandler.obtainMessage(MSG_MAS_CONNECT_DONE).sendToTarget();
    }

    private synchronized void enableNotifications(boolean status) {
        if (mMapConnectionStatus != CONNECTED) return;
        mClient.setNotificationRegistration(status);
        mEnableNotifications = status;
    }

    private boolean getNotificationStatus() {
        return mEnableNotifications;
    }

    private void setNotificationStatus(boolean status) {
        mEnableNotifications = status;
    }

    private synchronized void setPathDown(String path) {
        if (mMapConnectionStatus != CONNECTING) return;
        mClient.setFolderDown(path);
    }

    private synchronized void onEnableNotifications() {
        if (mMapConnectionStatus != CONNECTED) return;
        try {
            mCallbacks.onEnableNotifications();
        } catch (RemoteException ex) {
            disconnectInternalNoLock(false);
        }
    }

    private synchronized void onPushMessage(String handle) {
        if (mMapConnectionStatus != CONNECTED) return;
        try {
            mCallbacks.onPushMessage(handle);
        } catch (RemoteException ex) {
            disconnectInternalNoLock(false);
        }
    }

    private synchronized void onEventReport(
        android.bluetooth.client.map.BluetoothMapEventReport eventReport) {
        // Convert the Event Report format from the one spcified by BluetoothMasClient to the one
        // consumable by the BluetoothMapManager.
        BluetoothMapEventReport eventReportCallback = new BluetoothMapEventReport();
        switch (eventReport.getType()) {
            case NEW_MESSAGE:
                eventReportCallback.setType(BluetoothMapEventReport.TYPE_NEW_MESSAGE);
                eventReportCallback.setHandle(eventReport.getHandle());
                eventReportCallback.setFolder(eventReport.getFolder());
                break;

            default:
                Log.e(TAG, "onEventReport cannot understand the report: " + eventReport);
                return;
        }

        if (mMapConnectionStatus != CONNECTED) {
            Log.e(TAG, "onEventReport(): Returning early because not connected: " +
                mMapConnectionStatus);
        }

        try {
            mCallbacks.onEvent(eventReportCallback);
        } catch (RemoteException ex) {
            disconnectInternalNoLock(false);
        }
    }

    private synchronized void onGetMessage(BluetoothMapBmessage msg) {
        // Msg encoding.
        Log.d(TAG, "Msg encoding: " + msg.getEncoding());

        BluetoothMapMessage retMsg = new BluetoothMapMessage();

        // Source of message.
        switch (msg.getType()) {
            case SMS_GSM:
                retMsg.setType(BluetoothMapMessage.TYPE_SMS_GSM);
                break;
            case SMS_CDMA:
                retMsg.setType(BluetoothMapMessage.TYPE_SMS_CDMA);
                break;
            default:
                retMsg.setType(BluetoothMapMessage.TYPE_UNKNOWN);
                Log.w(TAG, "Unknown/Unsupported MAP message type: " + msg.getType());
        }

        // Status of message.
        switch (msg.getStatus()) {
          case READ:
              retMsg.setStatus(BluetoothMapMessage.STATUS_READ);
              break;
          case UNREAD:
              retMsg.setStatus(BluetoothMapMessage.STATUS_UNREAD);
              break;
          default:
              retMsg.setStatus(BluetoothMapMessage.STATUS_UNKNOWN);
        }

        // Folder in which it is stored on remote device.
        retMsg.setFolder(msg.getFolder());

        // Set the sender. Since we are receiving the message we don't need to set the recipient
        // here. We assume the first number is the primary sender.
        boolean sendRetMsg = true;
        VCardEntry origin = msg.getOriginator();
        if (origin == null) {
            Log.e(TAG, "No originator found. " + msg);
            // Return a null object so that the Manager can notify the client of the failure of the
            // get message call.
            try {
                mCallbacks.onGetMessage(null);
            } catch (RemoteException ex) {
                disconnectInternalNoLock(false);
            }
            sendRetMsg = false;
        }

        if (origin.getPhoneList() != null &&
            origin.getPhoneList().size() > 0 &&
            origin.getPhoneList().get(0) != null &&
            origin.getPhoneList().get(0).getNumber() != null) {
            retMsg.setSender(origin.getPhoneList().get(0).getNumber());
        } else {
            sendRetMsg = false;
        }

        // Set the message.
        retMsg.setMessage(msg.getBodyContent());

        if (mMapConnectionStatus != CONNECTED) return;
        try {
            if (!sendRetMsg) {
                Log.e(TAG, "Parsing BluetoothMapBmessage failed." + msg);
                mCallbacks.onGetMessage(null);
            } else {
                mCallbacks.onGetMessage(retMsg);
            }
        } catch (RemoteException ex) {
            disconnectInternalNoLock(false);
        }
    }

    private synchronized void onGetMessagesListing(
        ArrayList<android.bluetooth.client.map.BluetoothMapMessage> msgsListing) {
        List<BluetoothMapMessagesListing> retMsgsListing =
            new ArrayList<BluetoothMapMessagesListing>();

        for (android.bluetooth.client.map.BluetoothMapMessage msg : msgsListing) {
            BluetoothMapMessagesListing listing = new BluetoothMapMessagesListing();

            // Transform the various fields to target object.
            listing.setHandle(msg.getHandle());
            listing.setSubject(msg.getSubject());
            listing.setDate(msg.getDateTime());
            listing.setSender(msg.getSenderName());

            // TODO: Fill in the rest of the fields.

            retMsgsListing.add(listing);
        }

        if (mMapConnectionStatus != CONNECTED) return;
        try {
            mCallbacks.onGetMessagesListing(retMsgsListing);
        } catch (RemoteException ex) {
            disconnectInternalNoLock(false);
        }
    }

    private synchronized void onSetPath(String path) {
        if (path.endsWith(FOLDER_TELECOM)) {
            mFolder = FOLDER_TELECOM;
        } else if (path.endsWith(FOLDER_MSG)) {
            mFolder = FOLDER_MSG;
        } else {
            throw new IllegalStateException(TAG + " incorrect change folder: " + path);
        }
        mHandler.obtainMessage(MSG_SET_PATH).sendToTarget();
    }

    private synchronized void pushMessage(BluetoothMapMessage msg) {
        BluetoothMapBmessage bmsg = new BluetoothMapBmessage();
        // Set type and status.
        bmsg.setType(BluetoothMapBmessage.Type.SMS_GSM);
        bmsg.setStatus(BluetoothMapBmessage.Status.READ);

        // Who to send the message to.
        VCardEntry dest_entry = new VCardEntry();
        VCardProperty dest_entry_phone = new VCardProperty();
        dest_entry_phone.setName(VCardConstants.PROPERTY_TEL);
        dest_entry_phone.addValues(msg.getRecipient());
        Log.d(TAG, "Recipient: " + msg.getRecipient());
        dest_entry.addProperty(dest_entry_phone);
        bmsg.addRecipient(dest_entry);

        // Message of the body.
        bmsg.setBodyContent(msg.getMessage());

        boolean status = mClient.pushMessage(FOLDER_OUTBOX, bmsg, null);
        if (status == false) {
            try {
                mCallbacks.onPushMessage(null);
            } catch (RemoteException ex) {
                disconnectInternalNoLock(false);
            }
        }
    }

    private synchronized void getMessage(String handle) {
        // Added charset to make it compile.
        boolean status = mClient.getMessage(handle, false  /* attachments */);
        if (status == false) {
            try {
                mCallbacks.onGetMessage(null);
            } catch (RemoteException ex) {
                disconnectInternalNoLock(false);
            }
        }
    }

    private synchronized void getMessagesListing(String folder, int count, int offset) {
        boolean status = mClient.getMessagesListing(
            folder,
            0  /* all parameters */,
            null  /* no filter */,
            (byte) 0  /* subject length */,
            count,
            offset);
        if (status == false) {
            try {
                mCallbacks.onGetMessagesListing(null);
            } catch (RemoteException ex) {
                disconnectInternalNoLock(false);
            }
        }
    }

    private synchronized boolean connectInternal(IBluetoothMapServiceCallbacks callbacks,
        BluetoothDevice device) {
        if (mMapConnectionStatus != DISCONNECTED) {
            Log.d(TAG, "Service not in disconnected state. " + mMapConnectionStatus);
            return false;
        }
        // Change the connection status here so that subsequent connect() calls would return
        // false in the previous IF statement.
        mMapConnectionStatus = SDP;

        // Make sure we know about any deaths.
        try {
            callbacks.asBinder().linkToDeath(new BinderDeath(), 0);
        } catch (RemoteException ex) {
            Log.e(TAG, "", ex);
            return false;
        }

        // In order to connect to device we need to do the following:
        // a) Do a service discovery to check for available MAS instances, connect to one with
        // SMS availability.
        // b) On ACTION_SEARCH_INTENT use the record from (a) to connect to device.
        // c) On callback from (b) do a registernotifications.
        // d) On callback from (c) change directory into telecom/msg.
        mDevice = device;
        mCallbacks = callbacks;
        mHandler.obtainMessage(MSG_MAS_SDP, device).sendToTarget();
        return true;
    }

    // Removes the connection to remote device and remotes the binder connection to client holding
    // the manager.
    // The disconnect call first sets the status as DISCONNECTED so that no further callbacks are
    // sent to manager. Then it removes all messages from the handler queue. This ensures that
    // previous (non-inflight) handler messages are discarded.
    // NOTE: The function should only be called from within the Handler so that its call are
    // synchornized. Calling from outside the Handler could lead to race conditions w.r.t to
    // connection status.
    private synchronized void disconnectInternal(boolean failCallback) {
        disconnectInternalNoLock(failCallback);
    }
    private void disconnectInternalNoLock(boolean failCallback) {
        mMapConnectionStatus = DISCONNECTED;
        clearCommandQueue();
        if (mCallbacks != null && failCallback) {
            try {
                mCallbacks.onConnectFailed();
            } catch (RemoteException ex) {
                Log.e(TAG, "", ex);
            }
        }

        if (mClient != null) {
          mClient.disconnect();
        }

        mClient = null;
        mDevice = null;
        mCallbacks = null;
        mEnableNotifications = false;
    }

    // Clears all messages from mHandler queue.
    void clearCommandQueue() {
        // Enumerate all the message types and remove them from the message queue.
        mHandler.removeMessages(MSG_MAS_SDP);
        mHandler.removeMessages(MSG_MAS_SDP_DONE);
        mHandler.removeMessages(MSG_MAS_CONNECT_DONE);
        mHandler.removeMessages(MSG_ENABLE_NOTIFICATIONS);
        mHandler.removeMessages(MSG_SET_PATH);
        mHandler.removeMessages(MSG_PUSH_MESSAGE);
        mHandler.removeMessages(MSG_GET_MESSAGE);
        mHandler.removeMessages(MSG_GET_MESSAGES_LISTING);
    }

    private IBluetoothMapServiceCallbacks getCallbacks() {
        return mCallbacks;
    }

    private String getFolder() {
        return mFolder;
    }

    private Handler getHandler() {
        return mHandler;
    }

    private synchronized void connectionSuccessful() {
        if (mMapConnectionStatus != CONNECTING) return;
        mMapConnectionStatus = CONNECTED;
        try {
            mCallbacks.onConnect();
        } catch (RemoteException ex) {
            Log.e(TAG, "Binder exception. " + ex);
            disconnectInternalNoLock(false);
        }
    }
}
