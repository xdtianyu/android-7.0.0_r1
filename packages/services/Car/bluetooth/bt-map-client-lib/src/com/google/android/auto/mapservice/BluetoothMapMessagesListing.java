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

import java.util.Calendar;
import java.util.Date;
import java.util.List;
import java.util.Locale;
import java.util.TimeZone;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

import org.json.JSONException;
import org.json.JSONObject;

public class BluetoothMapMessagesListing implements Parcelable {

    // Fields for Messages Listing object as defined in MAPv12 Spec.
    private String mHandle;
    private String mSubject;
    private Date mDateTime;
    private String mSender;
    private String mSenderAddress;
    private String mReplyAddress;
    private String mRecipient;
    private String mRecipientAddress;
    private int mType;
    private int mSize;
    private boolean mText;
    private int mReceptionStatus;
    private int mAttachmentSize;
    private boolean mPriority;
    private boolean mRead;
    private boolean mSent;
    private boolean mProtected;

    public BluetoothMapMessagesListing() { }

    public static final Parcelable.Creator<BluetoothMapMessagesListing> CREATOR =
        new Parcelable.Creator<BluetoothMapMessagesListing>() {
        public BluetoothMapMessagesListing createFromParcel(Parcel in) {
            return new BluetoothMapMessagesListing(in);
        }

        public BluetoothMapMessagesListing[] newArray(int size) {
            return new BluetoothMapMessagesListing[size];
        }
    };

    private BluetoothMapMessagesListing(Parcel in) {
        readFromParcel(in);
    }

    public void writeToParcel(Parcel out, int flags) {
        out.writeString(mHandle);
        out.writeString(mSubject);
        out.writeString(dateToString(mDateTime));
        out.writeString(mSender);
        out.writeString(mSenderAddress);
        out.writeString(mReplyAddress);
        out.writeString(mRecipient);
        out.writeString(mRecipientAddress);
        out.writeInt(mType);
        out.writeInt(mSize);
        out.writeByte((byte) (mText ? 1 : 0));
        out.writeInt(mReceptionStatus);
        out.writeInt(mAttachmentSize);
        out.writeByte((byte) (mPriority ? 1 : 0));
        out.writeByte((byte) (mRead ? 1 : 0));
        out.writeByte((byte) (mSent ? 1 : 0));
        out.writeByte((byte) (mProtected ? 1 : 0));
    }

    public int describeContents() {
      return 0;
    }

    public void readFromParcel(Parcel in) {
        mHandle = in.readString();
        mSubject = in.readString();
        mDateTime = stringToDate(in.readString());
        mSender = in.readString();
        mSenderAddress = in.readString();
        mReplyAddress = in.readString();
        mRecipient = in.readString();
        mRecipientAddress = in.readString();
        mType = in.readInt();
        mSize = in.readInt();
        mText = in.readByte() != 0;
        mReceptionStatus = in.readInt();
        mAttachmentSize = in.readInt();
        mPriority = in.readByte() != 0;
        mRead = in.readByte() != 0;
        mSent = in.readByte() != 0;
        mProtected = in.readByte() != 0;
    }

    public String getHandle() {
        return mHandle;
    }

    public void setHandle(String handle) {
        mHandle = handle;
    }

    public String getSubject() {
        return mSubject;
    }

    public void setSubject(String subject) {
        mSubject = subject;
    }

    public Date getDate() {
        return mDateTime;
    }

    public void setDate(Date date) {
        mDateTime = date;
    }

    public String getSender() {
        return mSender;
    }

    public void setSender(String sender) {
        mSender = sender;
    }

    @Override
    public String toString() {
        JSONObject json = new JSONObject();
        try {
            json.put("handle", mHandle);
            json.put("subject", mSubject);
            json.put("datetime", mDateTime);
            json.put("sender_name", mSender);
            json.put("sender_addressing", mSenderAddress);
            json.put("replyto_addressing", mReplyAddress);
            json.put("recipient_name", mRecipient);
            json.put("recipient_addressing", mRecipientAddress);
            json.put("type", mType);
            json.put("size", mSize);
            json.put("text", mText);
            json.put("reception_status", mReceptionStatus);
            json.put("attachment_size", mAttachmentSize);
            json.put("priority", mPriority);
            json.put("read", mRead);
            json.put("sent", mSent);
            json.put("protected", mProtected);
        } catch (JSONException ex) {
            // do nothing.
        }

        return json.toString();
    }

    private String dateToString(Date date) {
        // Convert to MAPv12 acceptable format.
        Calendar cal = Calendar.getInstance();
        cal.setTime(date);
        return String.format(Locale.US, "%04d%02d%02dT%02d%02d%02d",
            cal.get(Calendar.YEAR), cal.get(Calendar.MONTH) + 1,
            cal.get(Calendar.DATE), cal.get(Calendar.HOUR_OF_DAY),
            cal.get(Calendar.MINUTE), cal.get(Calendar.SECOND));
    }

    private Date stringToDate(String time) {
        // match OBEX time string: YYYYMMDDTHHMMSS with optional UTF offset
        // +/-hhmm.
        Pattern p = Pattern.compile(
            "(\\d{4})(\\d{2})(\\d{2})T(\\d{2})(\\d{2})(\\d{2})(([+-])(\\d{2})(\\d{2}))?");
        Matcher m = p.matcher(time);

        if (m.matches()) {
            /*
             * matched groups are numberes as follows: YYYY MM DD T HH MM SS +
             * hh mm ^^^^ ^^ ^^ ^^ ^^ ^^ ^ ^^ ^^ 1 2 3 4 5 6 8 9 10 all groups
             * are guaranteed to be numeric so conversion will always succeed
             * (except group 8 which is either + or -)
             */

            Calendar cal = Calendar.getInstance();
            cal.set(Integer.parseInt(m.group(1)), Integer.parseInt(m.group(2)) - 1,
                    Integer.parseInt(m.group(3)), Integer.parseInt(m.group(4)),
                    Integer.parseInt(m.group(5)), Integer.parseInt(m.group(6)));

            /*
             * if 7th group is matched then we have UTC offset information
             * included
             */
            if (m.group(7) != null) {
                int ohh = Integer.parseInt(m.group(9));
                int omm = Integer.parseInt(m.group(10));

                /* time zone offset is specified in miliseconds */
                int offset = (ohh * 60 + omm) * 60 * 1000;

                if (m.group(8).equals("-")) {
                    offset = -offset;
                }

                TimeZone tz = TimeZone.getTimeZone("UTC");
                tz.setRawOffset(offset);

                cal.setTimeZone(tz);
            }
            return cal.getTime();
        } else {
            throw new IllegalStateException("Incorrect datetime format: " + mDateTime);
        }
    }

    // TODO: Finish the getters and setters.
}
