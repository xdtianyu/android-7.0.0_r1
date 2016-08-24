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

import android.os.Parcel;
import android.os.Parcelable;
import android.bluetooth.BluetoothDevice;

import com.google.android.auto.mapservice.IBluetoothMapServiceCallbacks;
import com.google.android.auto.mapservice.BluetoothMapMessage;


/**
 * Bluetooth Message service provides the API for Car to send and receive messages by using the MAP
 * protocol over Bluetooth. The current set of functions that may be achieved are:
 * A) Send SMS/MMS over bluetooth.
 * B) Receive notifications for all SMS/MMS events on the phone (see Callback AIDL).
 * @hide
 */
interface IBluetoothMapService {
    // NOTE: Function calls that return a boolean status indicate the success or failure of the call
    // itself. For example a false return status could mean that the service is not connected and
    // that the client should not expect a callback (if one was going to be sent back) OR the
    // successful execution of the request (for instance delete message).

    // Connect a manager with a callback.
    // After connect is called any events that happen on remote
    // end of MAP protocol (Phone side) will be reported to the client
    // (see IBluetoothMapCallbacks interface). Also, ONLY after callback is called should it make
    // sense to use the other functions in the protocol here.
    // @callback - Callback to receive events from Bluetooth MAP protocol.
    boolean connect(in IBluetoothMapServiceCallbacks callback, in BluetoothDevice device);

    // Disconnect the client from the service.
    void disconnect(in IBluetoothMapServiceCallbacks callback);

    // Enable/Disable notifications.
    // @status: Whether to enable or disable the notifications.
    boolean enableNotifications(in IBluetoothMapServiceCallbacks callbacks, boolean status);

    // Pushes the message to the outbox of remote device.
    // Sending message is only supported for text based messages.
    // @message - Message to be sent.
    boolean pushMessage(in IBluetoothMapServiceCallbacks callback, in BluetoothMapMessage message);

    // Fetch a message with a given handle.
    // @handle - Handle of the message most likely obtained from a BluetoothMapEventReport object.
    boolean getMessage(in IBluetoothMapServiceCallbacks callback, String handle);

    // Get a listing of message handles.
    // @folder - Folder to fetch the messages from.
    // @count - Number of messages to fetch, if 0 is provided then no message listings are fetched.
    // @offset - When paging (to be done by the client), offset can be provided to fetch further
    // messages. Client should be careful of new messages/deletions/message moves which could
    // invalidate the paging and it has to be done over.
    boolean getMessagesListing(in IBluetoothMapServiceCallbacks callback,
        String folder, int count, int offset);
}
