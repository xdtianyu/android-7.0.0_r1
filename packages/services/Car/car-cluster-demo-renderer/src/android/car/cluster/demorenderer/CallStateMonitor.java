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

package android.car.cluster.demorenderer;

import android.annotation.Nullable;
import android.car.cluster.demorenderer.PhoneBook.Contact;
import android.car.cluster.demorenderer.PhoneBook.ContactLoadedListener;
import android.car.cluster.demorenderer.PhoneBook.ContactPhotoLoadedListener;
import android.content.Context;
import android.graphics.Bitmap;
import android.telephony.TelephonyManager;
import android.util.Log;

import java.lang.ref.WeakReference;

/**
 * Monitors call state.
 */
public class CallStateMonitor implements ContactLoadedListener, ContactPhotoLoadedListener {
    private final static String TAG = CallStateMonitor.class.getSimpleName();

    private final PhoneBook mPhoneBook;
    private final TelephonyManager mTelephonyManager;
    private final PhoneStateListener mListener;
    private final CallStateListener mCallStateListener;

    CallStateMonitor(Context context, PhoneStateListener listener) {
        Log.d(TAG, "ctor, context: " + context + ", phoneRenderer: " + listener +
                ", contentResolver: " + context.getContentResolver() +
                ", applicationContext: " + context.getApplicationContext());

        mListener = listener;
        mTelephonyManager = context.getSystemService(TelephonyManager.class);
        mPhoneBook = new PhoneBook(context, mTelephonyManager);
        mCallStateListener = new CallStateListener(this);
        mTelephonyManager.listen(mCallStateListener,
                android.telephony.PhoneStateListener.LISTEN_CALL_STATE);

        updateRendererPhoneStatusIfAvailable();
    }

    public void release() {
        mTelephonyManager.listen(mCallStateListener,
                android.telephony.PhoneStateListener.LISTEN_NONE);
    }

    private void updateRendererPhoneStatusIfAvailable() {
        onCallStateChanged(mTelephonyManager.getCallState(), null);
    }

    private void onCallStateChanged(int state, final String number) {
        Log.d(TAG, "onCallStateChanged, state:" + state + ", phoneNumber: " + number);

        // Update call state immediately on instrument cluster.
        mListener.onCallStateChanged(state, PhoneBook.getFormattedNumber(number));

        // Now fetching details asynchronously.
        mPhoneBook.getContactDetailsAsync(number, this);
    }

    @Override
    public void onContactLoaded(String number, @Nullable Contact contact) {
        if (contact != null) {
            mListener.onContactDetailsUpdated(contact.getName(), contact.getType(),
                    mPhoneBook.isVoicemail(number));

            mPhoneBook.getContactPictureAsync(contact.getId(), this);
        }
    }

    @Override
    public void onPhotoLoaded(int contactId, @Nullable Bitmap photo) {
        mListener.onContactPhotoUpdated(photo);
    }

    public interface PhoneStateListener {
        void onCallStateChanged(int state, @Nullable String number);
        void onContactDetailsUpdated(
                @Nullable  CharSequence name,
                @Nullable CharSequence typeLabel,
                boolean isVoiceMail);
        void onContactPhotoUpdated(Bitmap picture);
    }

    private static class CallStateListener extends android.telephony.PhoneStateListener {
        private final WeakReference<CallStateMonitor> mServiceRef;

        CallStateListener(CallStateMonitor service) {
            mServiceRef = new WeakReference<>(service);
        }

        @Override
        public void onCallStateChanged(int state, String incomingNumber) {
            CallStateMonitor service = mServiceRef.get();
            if (service != null) {
                service.onCallStateChanged(state, incomingNumber);
            }
        }
    }
}
