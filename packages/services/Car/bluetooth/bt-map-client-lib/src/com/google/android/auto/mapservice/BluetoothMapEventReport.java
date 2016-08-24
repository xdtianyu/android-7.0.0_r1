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

/** Class representing various events that can be triggered for SMS/MMS on the phone. */
public final class BluetoothMapEventReport implements Parcelable {
    // List of all codes. To ensure there is no name space collision they are all listed here but
    // segmented by their types.

    /** Status that command executed successfully. */
    public static final int STATUS_OK = 0;
    /** Status that command executed but failed. */
    public static final int STATUS_FAILED = 1;

    /** Type of events. To avoid name space clash they are listed together */

    /** A new message has been received. */
    public static final int TYPE_NEW_MESSAGE = 1;

    /**
     * A message send goes through (not all compulsary) following phases:
     * a) The message is first pushed to remote device.
     * b) The message is then sent out by the remote device to SMSC server.
     * c) Depending on the configuration between remote device and SMSC
     * server there can be a delivery notification.
     */
    public static final int TYPE_DELIVERY_SUCCESS = 1;
    public static final int TYPE_SENDING_SUCCESS = 2;
    public static final int TYPE_DELIVERY_FAILURE = 3;
    public static final int TYPE_SENDING_FAILURE = 4;

    /** Message has been deleted */
    public static final int TYPE_MESSAGE_DELETED = 5;
    /** Message shifted from one folder to another */
    public static final int TYPE_MESSAGE_SHIFT = 6;

    // Contains the handle of message in context.
    private String mHandle;

    // Folder in which the message is currently stored.
    private String mFolder;

    // Folder in which the message was stored previously.
    private String mOldFolder;

    // Return code for the event.
    private int mReturnCode;

    // Type of message.
    private int mType;

    public static final Parcelable.Creator<BluetoothMapEventReport> CREATOR =
        new Parcelable.Creator<BluetoothMapEventReport>() {

        public BluetoothMapEventReport createFromParcel(Parcel in) {
            return new BluetoothMapEventReport(in);
        }

        public BluetoothMapEventReport[] newArray(int size) {
            return new BluetoothMapEventReport[size];
        }
    };

    public BluetoothMapEventReport() {
    }

    private BluetoothMapEventReport(Parcel in) {
        readFromParcel(in);
    }

    public int describeContents() {
      return 0;
    }

    public void writeToParcel(Parcel out, int flags) {
        out.writeString(mHandle);
        out.writeString(mFolder);
        out.writeString(mOldFolder);
        out.writeInt(mType);
    }

    public void readFromParcel(Parcel in) {
        mHandle = in.readString();
        mFolder = in.readString();
        mOldFolder = in.readString();
        mType = in.readInt();
    }

    public void setType(int type) {
        mType = type;
    }

    public int getType() {
        return mType;
    }

    public void setReturnCode(int retCode) {
        mReturnCode = retCode;
    }

    public int getReturnCode() {
        return mReturnCode;
    }

    public void setHandle(String handle) {
        mHandle = handle;
    }

    public String getHandle() {
        return mHandle;
    }

    public String getFolder() {
        return mFolder;
    }

    public void setFolder(String folder) {
        mFolder = folder;
    }

    public String getOldFolder() {
        return mOldFolder;
    }

    public void setOldFolder(String oldFolder) {
        mOldFolder = oldFolder;
    }

    public String toString() {
        return "Type: " + getType() + " return code: " + getReturnCode() + "Handle: " + mHandle;
    }
}
