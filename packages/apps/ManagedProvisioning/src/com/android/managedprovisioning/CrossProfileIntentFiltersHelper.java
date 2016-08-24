/*
 * Copyright 2014, The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
package com.android.managedprovisioning;

import android.content.Intent;
import android.content.IntentFilter;
import android.content.pm.PackageManager;
import android.provider.AlarmClock;
import android.provider.MediaStore;

import static android.speech.RecognizerIntent.ACTION_RECOGNIZE_SPEECH;
/**
 * Class to set CrossProfileIntentFilters during managed profile creation, and reset them after an
 * ota.
 */
public class CrossProfileIntentFiltersHelper {

    public static void setFilters(PackageManager pm, int parentUserId, int managedProfileUserId) {
        ProvisionLogger.logd("Setting cross-profile intent filters");

        // All Emergency/privileged calls are sent directly to the parent user.
        IntentFilter mimeTypeCallEmergency = new IntentFilter();
        mimeTypeCallEmergency.addAction(Intent.ACTION_CALL_EMERGENCY);
        mimeTypeCallEmergency.addAction(Intent.ACTION_CALL_PRIVILEGED);
        mimeTypeCallEmergency.addCategory(Intent.CATEGORY_DEFAULT);
        mimeTypeCallEmergency.addCategory(Intent.CATEGORY_BROWSABLE);
        try {
            mimeTypeCallEmergency.addDataType("vnd.android.cursor.item/phone");
            mimeTypeCallEmergency.addDataType("vnd.android.cursor.item/phone_v2");
            mimeTypeCallEmergency.addDataType("vnd.android.cursor.item/person");
            mimeTypeCallEmergency.addDataType("vnd.android.cursor.dir/calls");
            mimeTypeCallEmergency.addDataType("vnd.android.cursor.item/calls");
        } catch (IntentFilter.MalformedMimeTypeException e) {
            //will not happen
        }
        pm.addCrossProfileIntentFilter(mimeTypeCallEmergency, managedProfileUserId, parentUserId,
                PackageManager.SKIP_CURRENT_PROFILE);

        IntentFilter callWithDataEmergency = new IntentFilter();
        callWithDataEmergency.addAction(Intent.ACTION_CALL_EMERGENCY);
        callWithDataEmergency.addAction(Intent.ACTION_CALL_PRIVILEGED);
        callWithDataEmergency.addCategory(Intent.CATEGORY_DEFAULT);
        callWithDataEmergency.addCategory(Intent.CATEGORY_BROWSABLE);
        callWithDataEmergency.addDataScheme("tel");
        callWithDataEmergency.addDataScheme("sip");
        callWithDataEmergency.addDataScheme("voicemail");
        pm.addCrossProfileIntentFilter(callWithDataEmergency, managedProfileUserId, parentUserId,
                PackageManager.SKIP_CURRENT_PROFILE);

        // Dial intent with mime type can be handled by either managed profile or its parent user.
        IntentFilter mimeTypeDial = new IntentFilter();
        mimeTypeDial.addAction(Intent.ACTION_DIAL);
        mimeTypeDial.addAction(Intent.ACTION_VIEW);
        mimeTypeDial.addCategory(Intent.CATEGORY_DEFAULT);
        mimeTypeDial.addCategory(Intent.CATEGORY_BROWSABLE);
        try {
            mimeTypeDial.addDataType("vnd.android.cursor.item/phone");
            mimeTypeDial.addDataType("vnd.android.cursor.item/phone_v2");
            mimeTypeDial.addDataType("vnd.android.cursor.item/person");
            mimeTypeDial.addDataType("vnd.android.cursor.dir/calls");
            mimeTypeDial.addDataType("vnd.android.cursor.item/calls");
        } catch (IntentFilter.MalformedMimeTypeException e) {
            //will not happen
            ProvisionLogger.loge(e);
        }
        pm.addCrossProfileIntentFilter(mimeTypeDial, managedProfileUserId, parentUserId,
                PackageManager.ONLY_IF_NO_MATCH_FOUND);

        // Dial intent with tel, sip and voicemail scheme can be handled by either managed profile
        // or its parent user.
        IntentFilter dialWithData = new IntentFilter();
        dialWithData.addAction(Intent.ACTION_DIAL);
        dialWithData.addAction(Intent.ACTION_VIEW);
        dialWithData.addCategory(Intent.CATEGORY_DEFAULT);
        dialWithData.addCategory(Intent.CATEGORY_BROWSABLE);
        dialWithData.addDataScheme("tel");
        dialWithData.addDataScheme("sip");
        dialWithData.addDataScheme("voicemail");
        pm.addCrossProfileIntentFilter(dialWithData, managedProfileUserId, parentUserId,
                PackageManager.ONLY_IF_NO_MATCH_FOUND);

        // Dial intent with no data can be handled by either managed profile or its parent user.
        IntentFilter dialNoData = new IntentFilter();
        dialNoData.addAction(Intent.ACTION_DIAL);
        dialNoData.addCategory(Intent.CATEGORY_DEFAULT);
        dialNoData.addCategory(Intent.CATEGORY_BROWSABLE);
        pm.addCrossProfileIntentFilter(dialNoData, managedProfileUserId, parentUserId,
                PackageManager.ONLY_IF_NO_MATCH_FOUND);

        // Call button intent can be handled by either managed profile or its parent user.
        IntentFilter callButton = new IntentFilter();
        callButton.addAction(Intent.ACTION_CALL_BUTTON);
        callButton.addCategory(Intent.CATEGORY_DEFAULT);
        pm.addCrossProfileIntentFilter(callButton, managedProfileUserId, parentUserId,
                PackageManager.ONLY_IF_NO_MATCH_FOUND);

        IntentFilter smsMms = new IntentFilter();
        smsMms.addAction(Intent.ACTION_VIEW);
        smsMms.addAction(Intent.ACTION_SENDTO);
        smsMms.addCategory(Intent.CATEGORY_DEFAULT);
        smsMms.addCategory(Intent.CATEGORY_BROWSABLE);
        smsMms.addDataScheme("sms");
        smsMms.addDataScheme("smsto");
        smsMms.addDataScheme("mms");
        smsMms.addDataScheme("mmsto");
        pm.addCrossProfileIntentFilter(smsMms, managedProfileUserId, parentUserId,
                PackageManager.SKIP_CURRENT_PROFILE);

        IntentFilter mobileNetworkSettings = new IntentFilter();
        mobileNetworkSettings.addAction(android.provider.Settings.ACTION_DATA_ROAMING_SETTINGS);
        mobileNetworkSettings.addAction(android.provider.Settings.ACTION_NETWORK_OPERATOR_SETTINGS);
        mobileNetworkSettings.addCategory(Intent.CATEGORY_DEFAULT);
        pm.addCrossProfileIntentFilter(mobileNetworkSettings, managedProfileUserId,
                parentUserId, PackageManager.SKIP_CURRENT_PROFILE);

        IntentFilter home = new IntentFilter();
        home.addAction(Intent.ACTION_MAIN);
        home.addCategory(Intent.CATEGORY_DEFAULT);
        home.addCategory(Intent.CATEGORY_HOME);
        pm.addCrossProfileIntentFilter(home, managedProfileUserId, parentUserId,
                PackageManager.SKIP_CURRENT_PROFILE);

        IntentFilter send = new IntentFilter();
        send.addAction(Intent.ACTION_SEND);
        send.addAction(Intent.ACTION_SEND_MULTIPLE);
        send.addCategory(Intent.CATEGORY_DEFAULT);
        try {
            send.addDataType("*/*");
        } catch (IntentFilter.MalformedMimeTypeException e) {
            //will not happen
            ProvisionLogger.loge(e);
        }
        // This is the only filter set on the opposite direction (from parent to managed profile).
        pm.addCrossProfileIntentFilter(send, parentUserId, managedProfileUserId, 0);

        IntentFilter getContent = new IntentFilter();
        getContent.addAction(Intent.ACTION_GET_CONTENT);
        getContent.addCategory(Intent.CATEGORY_DEFAULT);
        getContent.addCategory(Intent.CATEGORY_OPENABLE);
        try {
            getContent.addDataType("*/*");
        } catch (IntentFilter.MalformedMimeTypeException e) {
            //will not happen
            ProvisionLogger.loge(e);
        }
        pm.addCrossProfileIntentFilter(getContent, managedProfileUserId, parentUserId, 0);

        IntentFilter openDocument = new IntentFilter();
        openDocument.addAction(Intent.ACTION_OPEN_DOCUMENT);
        openDocument.addCategory(Intent.CATEGORY_DEFAULT);
        openDocument.addCategory(Intent.CATEGORY_OPENABLE);
        try {
            openDocument.addDataType("*/*");
        } catch (IntentFilter.MalformedMimeTypeException e) {
            //will not happen
            ProvisionLogger.loge(e);
        }
        pm.addCrossProfileIntentFilter(openDocument, managedProfileUserId, parentUserId, 0);

        IntentFilter pick = new IntentFilter();
        pick.addAction(Intent.ACTION_PICK);
        pick.addCategory(Intent.CATEGORY_DEFAULT);
        try {
            pick.addDataType("*/*");
        } catch (IntentFilter.MalformedMimeTypeException e) {
            //will not happen
            ProvisionLogger.loge(e);
        }
        pm.addCrossProfileIntentFilter(pick, managedProfileUserId, parentUserId, 0);

        IntentFilter pickNoData = new IntentFilter();
        pickNoData.addAction(Intent.ACTION_PICK);
        pickNoData.addCategory(Intent.CATEGORY_DEFAULT);
        pm.addCrossProfileIntentFilter(pickNoData, managedProfileUserId,
                parentUserId, 0);

        IntentFilter recognizeSpeech = new IntentFilter();
        recognizeSpeech.addAction(ACTION_RECOGNIZE_SPEECH);
        recognizeSpeech.addCategory(Intent.CATEGORY_DEFAULT);
        pm.addCrossProfileIntentFilter(recognizeSpeech, managedProfileUserId, parentUserId, 0);

        IntentFilter capture = new IntentFilter();
        capture.addAction(MediaStore.ACTION_IMAGE_CAPTURE);
        capture.addAction(MediaStore.ACTION_IMAGE_CAPTURE_SECURE);
        capture.addAction(MediaStore.ACTION_VIDEO_CAPTURE);
        capture.addAction(MediaStore.Audio.Media.RECORD_SOUND_ACTION);
        capture.addAction(MediaStore.INTENT_ACTION_STILL_IMAGE_CAMERA);
        capture.addAction(MediaStore.INTENT_ACTION_STILL_IMAGE_CAMERA_SECURE);
        capture.addAction(MediaStore.INTENT_ACTION_VIDEO_CAMERA);
        capture.addCategory(Intent.CATEGORY_DEFAULT);
        pm.addCrossProfileIntentFilter(capture, managedProfileUserId, parentUserId, 0);

        IntentFilter setClock = new IntentFilter();
        setClock.addAction(AlarmClock.ACTION_SET_ALARM);
        setClock.addAction(AlarmClock.ACTION_SHOW_ALARMS);
        setClock.addAction(AlarmClock.ACTION_SET_TIMER);
        setClock.addCategory(Intent.CATEGORY_DEFAULT);
        pm.addCrossProfileIntentFilter(setClock, managedProfileUserId, parentUserId, 0);
    }
}
