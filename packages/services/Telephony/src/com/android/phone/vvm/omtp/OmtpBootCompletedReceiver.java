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
 * limitations under the License
 */

package com.android.phone.vvm.omtp;

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.SharedPreferences;
import android.preference.PreferenceManager;
import android.util.ArraySet;
import android.util.Log;

import com.android.internal.annotations.VisibleForTesting;

import java.util.Set;

/**
 * Stores subscription ID of SIMs while the device is locked to process them after the device is
 * unlocked. This class is only intended to be used within {@link SimChangeReceiver}. subId is used
 * for Visual voicemail activation/deactivation, which need to be done when the device is unlocked.
 * But the enumeration of subIds happen on boot, when the device could be locked. This class is used
 * to defer all activation/deactivation until the device is unlocked.
 *
 * The subIds are stored in device encrypted {@link SharedPreferences} (readable/writable even
 * locked). after the device is unlocked the list is read and deleted.
 */
public class OmtpBootCompletedReceiver extends BroadcastReceiver {

    private static final String TAG = "OmtpBootCompletedRcvr";

    private static final String DEFERRED_SUBID_LIST_KEY = "deferred_sub_id_key";

    @VisibleForTesting
    interface SubIdProcessor{
        void process(Context context,int subId);
    }

    private SubIdProcessor mSubIdProcessor = new SubIdProcessor() {
        @Override
        public void process(Context context, int subId) {
            SimChangeReceiver.processSubId(context,subId);
        }
    };

    /**
     * Write the subId to the the list.
     */
    public static void addDeferredSubId(Context context, int subId) {
        SharedPreferences sharedPreferences = getSubIdSharedPreference(context);
        Set<String> subIds =
                new ArraySet<>(sharedPreferences.getStringSet(DEFERRED_SUBID_LIST_KEY, null));
        subIds.add(String.valueOf(subId));
        sharedPreferences.edit().putStringSet(DEFERRED_SUBID_LIST_KEY, subIds).apply();
    }

    @Override
    public void onReceive(Context context, Intent intent) {
        // Listens to android.intent.action.BOOT_COMPLETED
        if(!intent.getAction().equals(Intent.ACTION_BOOT_COMPLETED)) {
            return;
        }

        Log.v(TAG, "processing deferred subId list");
        Set<Integer> subIds = readAndDeleteSubIds(context);
        for (Integer subId : subIds) {
            Log.v(TAG, "processing subId " + subId);
            mSubIdProcessor.process(context, subId);
        }
    }

    /**
     * Read all subId from the list to a unique integer set, and delete the preference.
     */
    private static Set<Integer> readAndDeleteSubIds(Context context) {
        SharedPreferences sharedPreferences = getSubIdSharedPreference(context);
        Set<String> subIdStrings = sharedPreferences.getStringSet(DEFERRED_SUBID_LIST_KEY, null);
        Set<Integer> subIds = new ArraySet<>();
        if(subIdStrings == null) {
            return subIds;
        }
        for(String string : subIdStrings){
            subIds.add(Integer.valueOf(string));
        }
        getSubIdSharedPreference(context).edit().remove(DEFERRED_SUBID_LIST_KEY).apply();
        return subIds;
    }

    @VisibleForTesting
    void setSubIdProcessorForTest(SubIdProcessor processor){
        mSubIdProcessor = processor;
    }

    private static SharedPreferences getSubIdSharedPreference(Context context) {
        return PreferenceManager
                .getDefaultSharedPreferences(context.createDeviceProtectedStorageContext());
    }
}
