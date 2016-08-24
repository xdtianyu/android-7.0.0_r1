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

public final class BluetoothMapMessage implements Parcelable {
    /** Types of Messages. */
    public static final int TYPE_SMS_GSM = 1;
    public static final int TYPE_SMS_CDMA = 2;
    public static final int TYPE_MMS = 3;
    public static final int TYPE_UNKNOWN = 99;

    /** Status of the message. */
    public static final int STATUS_READ = 1;
    public static final int STATUS_UNREAD = 2;
    public static final int STATUS_UNKNOWN = 99;

    // Defines the various metadata field for the message.

    // If defined, represents a decimal separated number representation.
    private int mStatus;
    private int mType;
    private String mFolder;
    private int mBodyLength;
    // When sending a message mRecipient must be populated, and while receiving a message mSender
    // must be populated.
    private String mSender;
    private String mRecipient;

    // Not used currently.
    private int mLanguage;
    private String mVersion;

    // Actual message content.
    private String mMessage;

    public static final Parcelable.Creator<BluetoothMapMessage> CREATOR =
        new Parcelable.Creator<BluetoothMapMessage>() {
        public BluetoothMapMessage createFromParcel(Parcel in) {
            return new BluetoothMapMessage(in);
        }

        public BluetoothMapMessage[] newArray(int size) {
            return new BluetoothMapMessage[size];
        }
    };

    public BluetoothMapMessage() { }

    public void setRecipient(String recipient) {
        mRecipient = recipient;
    }

    public String getRecipient() {
        return mRecipient;
    }

    public void setSender(String sender) {
        mSender = sender;
    }

    public String getSender() {
        return mSender;
    }

    public void setType(int type) {
        mType = type;
    }

    public int getType() {
        return mType;
    }

    public void setStatus(int status) {
        mStatus = status;
    }

    public  int getStatus() {
        return mStatus;
    }

    public void setFolder(String folder) {
        mFolder = folder;
    }

    public String getFolder() {
        return mFolder;
    }

    public void setMessage(String message) {
        mMessage = message;
    }

    public String getMessage() {
        return mMessage;
    }

    private BluetoothMapMessage(Parcel in) {
        readFromParcel(in);
    }

    public void writeToParcel(Parcel out, int flags) {
        // Write metadata.
        out.writeInt(mStatus);
        out.writeInt(mType);
        out.writeString(mFolder);
        out.writeInt(mBodyLength);
        // Currently not in use.
        out.writeInt(mLanguage);
        out.writeString(mVersion);

        // Write sender/receiver info.
        out.writeString(mSender);
        out.writeString(mRecipient);

        // Actual message.
        out.writeString(mMessage);
    }

    public int describeContents() {
      return 0;
    }

    public void readFromParcel(Parcel in) {
        // Read metadata.
        mStatus = in.readInt();
        mType = in.readInt();
        mFolder = in.readString();
        mBodyLength = in.readInt();
        // Currently not used metadata.
        mLanguage = in.readInt();
        mVersion = in.readString();

        // Read sender/receiver info.
        mSender = in.readString();
        mRecipient = in.readString();

        // Read actual content.
        mMessage = in.readString();
    }

    public String toString() {
        return "Message:\nSender: " + mSender+ "\nMessage: " + mMessage;
    }
}
