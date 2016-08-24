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

import android.car.cluster.demorenderer.CallStateMonitor.PhoneStateListener;
import android.content.Context;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.telephony.TelephonyManager;
import android.util.Log;

/**
 * Demo for rendering phone status in instrument cluster.
 */
public class DemoPhoneRenderer implements PhoneStateListener {
    private static final String TAG = DemoPhoneRenderer.class.getSimpleName();

    private final DemoInstrumentClusterView mView;
    private final Context mContext;

    private static Bitmap sDefaultAvatar;

    private int mCurrentState;
    private String mCurrentNumber;

    DemoPhoneRenderer(DemoInstrumentClusterView view) {
        mView = view;
        mContext = view.getContext();
    }

    @Override
    public void onCallStateChanged(int state, String number) {
        Log.d(TAG, "onCallStateChanged, state: " + state + ", number: " + number);
        mCurrentState = state;
        mCurrentNumber = PhoneBook.getFormattedNumber(number);

        if (TelephonyManager.CALL_STATE_IDLE == state) {
            mView.hidePhone();
        } else {
            mView.showPhone();
            setPhoneTitleWithState(mCurrentNumber);
            mView.setPhoneImage(getDefaultAvatar());
        }
    }

    @Override
    public void onContactDetailsUpdated(CharSequence name, CharSequence typeLabel,
            boolean isVoiceMail) {
        Log.d(TAG, "onContactDetailsUpdated, name: " + name + ", typeLabel: " + typeLabel
                + ", isVoicemail: " + isVoiceMail);
        setPhoneTitleWithState(name.toString());
        mView.setPhoneSubtitle(mCurrentNumber);
    }

    private void setPhoneTitleWithState(String text) {
        mView.setPhoneTitle(getCallStateToDisplay(mCurrentState) + " · " + text);
    }

    @Override
    public void onContactPhotoUpdated(Bitmap picture) {
        Log.d(TAG, "onContactPhotoUpdated, picture: " + picture);
        if (picture != null) {
            mView.setPhoneImage(picture);
        }
    }


    private Bitmap getDefaultAvatar() {
        if (sDefaultAvatar == null) {
            sDefaultAvatar = BitmapFactory.decodeResource(mContext.getResources(),
                    R.drawable.ic_contactavatar_large_light);
        }
        return sDefaultAvatar;
    }

    private String getCallStateToDisplay(int state) {
        switch (state) {
            case TelephonyManager.CALL_STATE_OFFHOOK:
                return mContext.getString(R.string.call_state_active);
            case TelephonyManager.CALL_STATE_RINGING:
                return mContext.getString(R.string.call_state_ringing);
            default:
                Log.w(TAG, "Unexpected call state: " + state);
                return "";
        }
    }
}
