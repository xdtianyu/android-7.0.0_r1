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

import com.google.android.auto.mapservice.BluetoothMapEventReport;
import com.google.android.auto.mapservice.BluetoothMapMessage;
import com.google.android.auto.mapservice.BluetoothMapMessagesListing;

/**
 * Any binder to IBluetoothMapService will have to provide this class as the callbacks so that
 * it can listen to various events that happen in the inbox of the phone such as new messages or
 * message delivery reports (see BluetoothMapEventReport above). Additionally it also informs the
 * binder about the state of service connection (see onConnect and onDisconnect functions below).
 *
 * NOTE: Callbacks which are initiated by client i.e. callbacks in response to any of the API
 * functions defined in IBluetoothMapService will be executed in order of functions called. i.e.
 * if getMessage(handle1) is called followed by getMessage(handle2) then the reply of handle1 will
 * be received before handle2. Hence there's synchronization in order of callbacks. Although there
 * is no order for onEvent calls because they are not initiated by client in
 * IBluetoothMapService.
 * @hide
 */
oneway interface IBluetoothMapServiceCallbacks {
    // Notifies the client binding to the service if the callback has been registered successfully.
    void onConnect();

    // Notifies the client binding to the service if the callback is registered but not connected to
    // external Map profile via any device. The client should wait until it receives onConnect()
    // again before using the API.
    void onConnectFailed();

    // Callback for notification registration.
    // @status - The final status of the notifications.
    void onEnableNotifications();

    // Returns the string handle for a pushMessage request.
    void onPushMessage(String handle);

    // Callback for getMessage().
    // @message - The message.
    void onGetMessage(in BluetoothMapMessage message);

    // Callback for getMessagesListing().
    void onGetMessagesListing(in List<BluetoothMapMessagesListing> msgsListing);

    // Whenever there is an event (see list of events on top of BluetoothMapEventReport) this
    // callback will be executed with the event report. Client can take subsequent action on each
    // such report.
    //
    // Eg. If the report is NEW_MESSAGE then the client should grab the handle from the event report
    // and send a getMessage request to fetch the message.
    // @eventReport - The event report which contains details of how to proceed with notification.
    void onEvent(in BluetoothMapEventReport eventReport);
}
