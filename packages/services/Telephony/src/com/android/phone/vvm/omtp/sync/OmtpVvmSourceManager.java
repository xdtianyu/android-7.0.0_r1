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
 * limitations under the License
 */
package com.android.phone.vvm.omtp.sync;

import android.content.Context;
import android.provider.VoicemailContract;
import android.telecom.PhoneAccountHandle;
import android.telephony.PhoneStateListener;
import android.telephony.SubscriptionManager;
import android.telephony.TelephonyManager;

import com.android.internal.telephony.Phone;
import com.android.phone.PhoneUtils;
import com.android.phone.vvm.omtp.VvmPhoneStateListener;

import java.util.Collections;
import java.util.Map;
import java.util.Set;
import java.util.concurrent.ConcurrentHashMap;

/**
 * A singleton class designed to remember the active OMTP visual voicemail sources. Because a
 * voicemail source is tied 1:1 to a phone account, the phone account handle is used as the key
 * for each voicemail source and the associated data.
 */
public class OmtpVvmSourceManager {
    public static final String TAG = "OmtpVvmSourceManager";

    private static OmtpVvmSourceManager sInstance = new OmtpVvmSourceManager();

    private Context mContext;
    private SubscriptionManager mSubscriptionManager;
    private TelephonyManager mTelephonyManager;
    // Each phone account is associated with a phone state listener for updates to whether the
    // device is able to sync.
    private Set<PhoneAccountHandle> mActiveVvmSources;
    private Map<PhoneAccountHandle, PhoneStateListener> mPhoneStateListenerMap;

    /**
     * Private constructor. Instance should only be acquired through getInstance().
     */
    private OmtpVvmSourceManager() {}

    public static OmtpVvmSourceManager getInstance(Context context) {
        sInstance.setup(context);
        return sInstance;
    }

    /**
     * Set the context and system services so they do not need to be retrieved every time.
     * @param context The context to get the subscription and telephony manager for.
     */
    private void setup(Context context) {
        if (mContext == null) {
            mContext = context;
            mSubscriptionManager = SubscriptionManager.from(context);
            mTelephonyManager = (TelephonyManager)
                    mContext.getSystemService(Context.TELEPHONY_SERVICE);
            mActiveVvmSources = Collections.newSetFromMap(
                    new ConcurrentHashMap<PhoneAccountHandle, Boolean>(8, 0.9f, 1));
            mPhoneStateListenerMap =
                    new ConcurrentHashMap<PhoneAccountHandle, PhoneStateListener>(8, 0.9f, 1);
        }
    }

    public void addSource(PhoneAccountHandle phoneAccount) {
        mActiveVvmSources.add(phoneAccount);
    }

    /**
     * When a voicemail source is removed, we don't always know which one was removed. Check the
     * list of registered phone accounts against the active subscriptions list and remove the
     * inactive sources.
     */
    public void removeInactiveSources() {
        for (PhoneAccountHandle phoneAccount : mActiveVvmSources) {
            if (!PhoneUtils.isPhoneAccountActive(mSubscriptionManager, phoneAccount)) {
                removeSource(phoneAccount);
            }
        }

        // Remove any orphaned phone state listeners as well.
        for (PhoneAccountHandle phoneAccount : mPhoneStateListenerMap.keySet()) {
            if (!PhoneUtils.isPhoneAccountActive(mSubscriptionManager, phoneAccount)) {
                removePhoneStateListener(phoneAccount);
            }
        }
    }

    public void removeSource(Phone phone) {
        removeSource(PhoneUtils.makePstnPhoneAccountHandle(phone));
    }

    public void removeSource(PhoneAccountHandle phoneAccount) {
        VoicemailContract.Status.setStatus(mContext, phoneAccount,
                VoicemailContract.Status.CONFIGURATION_STATE_NOT_CONFIGURED,
                VoicemailContract.Status.DATA_CHANNEL_STATE_NO_CONNECTION,
                VoicemailContract.Status.NOTIFICATION_CHANNEL_STATE_NO_CONNECTION);
        removePhoneStateListener(phoneAccount);
        mActiveVvmSources.remove(phoneAccount);
        OmtpVvmSyncService.cancelAllRetries(mContext, phoneAccount);
    }

    public void addPhoneStateListener(Phone phone) {
        addPhoneStateListener(PhoneUtils.makePstnPhoneAccountHandle(phone));
    }

    public void addPhoneStateListener(PhoneAccountHandle phoneAccount) {
        if (!mPhoneStateListenerMap.containsKey(phoneAccount)) {
            VvmPhoneStateListener phoneStateListener = new VvmPhoneStateListener(mContext,
                    PhoneUtils.makePstnPhoneAccountHandle(phoneAccount.getId()));
            mPhoneStateListenerMap.put(phoneAccount, phoneStateListener);
            mTelephonyManager.listen(phoneStateListener, PhoneStateListener.LISTEN_SERVICE_STATE);
        }
    }

    public void removePhoneStateListener(PhoneAccountHandle phoneAccount) {
        PhoneStateListener phoneStateListener =
                mPhoneStateListenerMap.remove(phoneAccount);
        mTelephonyManager.listen(phoneStateListener, 0);
    }

    public Set<PhoneAccountHandle> getOmtpVvmSources() {
        return mActiveVvmSources;
    }

    /**
     * Check if a certain account is registered.
     *
     * @param phoneAccount The account to look for.
     * @return {@code true} if the account is in the list of registered OMTP voicemail sources.
     * {@code false} otherwise.
     */
    public boolean isVvmSourceRegistered(PhoneAccountHandle phoneAccount) {
        if (phoneAccount == null) {
            return false;
        }

        return mActiveVvmSources.contains(phoneAccount);
    }
}