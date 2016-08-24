/*
 * Copyright (C) 2014 The Android Open Source Project
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

package com.android.phone.settings;

import android.content.Context;
import android.content.SharedPreferences;
import android.net.Uri;
import android.preference.PreferenceManager;
import android.provider.Settings;
import android.telephony.TelephonyManager;
import android.text.TextUtils;

import com.android.internal.telephony.Phone;
import com.android.phone.R;

public class VoicemailNotificationSettingsUtil {
    private static final String VOICEMAIL_NOTIFICATION_RINGTONE_SHARED_PREFS_KEY_PREFIX =
            "voicemail_notification_ringtone_";
    private static final String VOICEMAIL_NOTIFICATION_VIBRATION_SHARED_PREFS_KEY_PREFIX =
            "voicemail_notification_vibrate_";

    // Old voicemail notification vibration string constants used for migration.
    private static final String OLD_VOICEMAIL_NOTIFICATION_RINGTONE_SHARED_PREFS_KEY =
            "button_voicemail_notification_ringtone_key";
    private static final String OLD_VOICEMAIL_NOTIFICATION_VIBRATION_SHARED_PREFS_KEY =
            "button_voicemail_notification_vibrate_key";
    private static final String OLD_VOICEMAIL_VIBRATE_WHEN_SHARED_PREFS_KEY =
            "button_voicemail_notification_vibrate_when_key";
    private static final String OLD_VOICEMAIL_RINGTONE_SHARED_PREFS_KEY =
            "button_voicemail_notification_ringtone_key";
    private static final String OLD_VOICEMAIL_VIBRATION_ALWAYS = "always";
    private static final String OLD_VOICEMAIL_VIBRATION_NEVER = "never";

    public static void setVibrationEnabled(Phone phone, boolean isEnabled) {
        SharedPreferences prefs = PreferenceManager.getDefaultSharedPreferences(phone.getContext());
        SharedPreferences.Editor editor = prefs.edit();
        editor.putBoolean(getVoicemailVibrationSharedPrefsKey(phone), isEnabled);
        editor.commit();
    }

    public static boolean isVibrationEnabled(Phone phone) {
        SharedPreferences prefs = PreferenceManager.getDefaultSharedPreferences(phone.getContext());
        migrateVoicemailVibrationSettingsIfNeeded(phone, prefs);
        return prefs.getBoolean(getVoicemailVibrationSharedPrefsKey(phone), false /* defValue */);
    }

   public static void setRingtoneUri(Phone phone, Uri ringtoneUri) {
        SharedPreferences prefs = PreferenceManager.getDefaultSharedPreferences(phone.getContext());
        String ringtoneUriStr = ringtoneUri != null ? ringtoneUri.toString() : "";

        SharedPreferences.Editor editor = prefs.edit();
        editor.putString(getVoicemailRingtoneSharedPrefsKey(phone), ringtoneUriStr);
        editor.commit();
    }

    public static Uri getRingtoneUri(Phone phone) {
        SharedPreferences prefs = PreferenceManager.getDefaultSharedPreferences(phone.getContext());
        migrateVoicemailRingtoneSettingsIfNeeded(phone, prefs);
        String uriString = prefs.getString(
                getVoicemailRingtoneSharedPrefsKey(phone),
                Settings.System.DEFAULT_NOTIFICATION_URI.toString());
        return !TextUtils.isEmpty(uriString) ? Uri.parse(uriString) : null;
    }

    /**
     * Migrate voicemail settings from {@link #OLD_VIBRATE_WHEN_KEY} or
     * {@link #OLD_VOICEMAIL_NOTIFICATION_VIBRATE_KEY}.
     *
     * TODO: Add helper which migrates settings from old version to new version.
     */
    private static void migrateVoicemailVibrationSettingsIfNeeded(
            Phone phone, SharedPreferences prefs) {
        String key = getVoicemailVibrationSharedPrefsKey(phone);
        TelephonyManager telephonyManager = TelephonyManager.from(phone.getContext());

        // Skip if a preference exists, or if phone is MSIM.
        if (prefs.contains(key) || telephonyManager.getPhoneCount() != 1) {
            return;
        }

        if (prefs.contains(OLD_VOICEMAIL_NOTIFICATION_VIBRATION_SHARED_PREFS_KEY)) {
            boolean voicemailVibrate = prefs.getBoolean(
                    OLD_VOICEMAIL_NOTIFICATION_VIBRATION_SHARED_PREFS_KEY, false /* defValue */);

            SharedPreferences.Editor editor = prefs.edit();
            editor.putBoolean(key, voicemailVibrate)
                    .remove(OLD_VOICEMAIL_VIBRATE_WHEN_SHARED_PREFS_KEY)
                    .commit();
        }

        if (prefs.contains(OLD_VOICEMAIL_VIBRATE_WHEN_SHARED_PREFS_KEY)) {
            // If vibrateWhen is always, then voicemailVibrate should be true.
            // If it is "only in silent mode", or "never", then voicemailVibrate should be false.
            String vibrateWhen = prefs.getString(
                    OLD_VOICEMAIL_VIBRATE_WHEN_SHARED_PREFS_KEY, OLD_VOICEMAIL_VIBRATION_NEVER);
            boolean voicemailVibrate = vibrateWhen.equals(OLD_VOICEMAIL_VIBRATION_ALWAYS);

            SharedPreferences.Editor editor = prefs.edit();
            editor.putBoolean(key, voicemailVibrate)
                    .remove(OLD_VOICEMAIL_NOTIFICATION_VIBRATION_SHARED_PREFS_KEY)
                    .commit();
        }
    }

    /**
     * Migrate voicemail settings from OLD_VOICEMAIL_NOTIFICATION_RINGTONE_SHARED_PREFS_KEY.
     *
     * TODO: Add helper which migrates settings from old version to new version.
     */
    private static void migrateVoicemailRingtoneSettingsIfNeeded(
            Phone phone, SharedPreferences prefs) {
        String key = getVoicemailRingtoneSharedPrefsKey(phone);
        TelephonyManager telephonyManager = TelephonyManager.from(phone.getContext());

        // Skip if a preference exists, or if phone is MSIM.
        if (prefs.contains(key) || telephonyManager.getPhoneCount() != 1) {
            return;
        }

        if (prefs.contains(OLD_VOICEMAIL_NOTIFICATION_RINGTONE_SHARED_PREFS_KEY)) {
            String uriString = prefs.getString(
                    OLD_VOICEMAIL_NOTIFICATION_RINGTONE_SHARED_PREFS_KEY, null /* defValue */);

            SharedPreferences.Editor editor = prefs.edit();
            editor.putString(key, uriString)
                    .remove(OLD_VOICEMAIL_NOTIFICATION_RINGTONE_SHARED_PREFS_KEY)
                    .commit();
        }
    }

    private static String getVoicemailVibrationSharedPrefsKey(Phone phone) {
        return VOICEMAIL_NOTIFICATION_VIBRATION_SHARED_PREFS_KEY_PREFIX + phone.getSubId();
    }

    public static String getVoicemailRingtoneSharedPrefsKey(Phone phone) {
        return VOICEMAIL_NOTIFICATION_RINGTONE_SHARED_PREFS_KEY_PREFIX + phone.getSubId();
    }
}
