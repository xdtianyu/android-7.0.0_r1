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

package com.android.tv.util;

import android.content.ContentResolver;
import android.content.Context;
import android.content.Intent;
import android.database.Cursor;
import android.media.tv.TvContract.Channels;
import android.net.Uri;
import android.preference.PreferenceManager;
import android.support.annotation.UiThread;

import com.android.tv.TvApplication;
import com.android.tv.data.ChannelDataManager;

/**
 * A utility class related to onboarding experience.
 */
public final class OnboardingUtils {
    private static final String PREF_KEY_IS_FIRST_BOOT = "pref_onbaording_is_first_boot";
    private static final String PREF_KEY_ONBOARDING_VERSION_CODE = "pref_onbaording_versionCode";
    private static final int ONBOARDING_VERSION = 1;

    private static final String MERCHANT_COLLECTION_URL_STRING =
            "TODO: put a market link to show TV input apps";
    /**
     * Intent to show merchant collection in play store.
     */
    public static final Intent PLAY_STORE_INTENT = new Intent(Intent.ACTION_VIEW,
            Uri.parse(MERCHANT_COLLECTION_URL_STRING));

    /**
     * Checks if this is the first boot after the onboarding experience has been applied.
     */
    public static boolean isFirstBoot(Context context) {
        return PreferenceManager.getDefaultSharedPreferences(context)
                .getBoolean(PREF_KEY_IS_FIRST_BOOT, true);
    }

    /**
     * Marks that the first boot has been completed.
     */
    public static void setFirstBootCompleted(Context context) {
        PreferenceManager.getDefaultSharedPreferences(context)
                .edit()
                .putBoolean(PREF_KEY_IS_FIRST_BOOT, false)
                .apply();
    }

    /**
     * Checks if this is the first run of {@link com.android.tv.MainActivity} with the
     * current onboarding version.
     */
    public static boolean isFirstRunWithCurrentVersion(Context context) {
        int versionCode = PreferenceManager.getDefaultSharedPreferences(context)
                .getInt(PREF_KEY_ONBOARDING_VERSION_CODE, 0);
        return versionCode != ONBOARDING_VERSION;
    }

    /**
     * Marks that the first run of {@link com.android.tv.MainActivity} with the current
     * onboarding version has been completed.
     */
    public static void setFirstRunWithCurrentVersionCompleted(Context context) {
        PreferenceManager.getDefaultSharedPreferences(context).edit()
                .putInt(PREF_KEY_ONBOARDING_VERSION_CODE, ONBOARDING_VERSION).apply();
    }

    /**
     * Checks whether the onboarding screen should be shown or not.
     */
    public static boolean needToShowOnboarding(Context context) {
        return isFirstRunWithCurrentVersion(context) || !areChannelsAvailable(context);
    }

    /**
     * Checks if there are any available tuner channels.
     */
    @UiThread
    public static boolean areChannelsAvailable(Context context) {
        ChannelDataManager manager = TvApplication.getSingletons(context).getChannelDataManager();
        if (manager.isDbLoadFinished()) {
            return manager.getChannelCount() != 0;
        }
        // This method should block the UI thread.
        ContentResolver resolver = context.getContentResolver();
        try (Cursor c = resolver.query(Channels.CONTENT_URI, new String[] {Channels._ID}, null,
                null, null)) {
            return c != null && c.getCount() != 0;
        }
    }

    /**
     * Checks if there are any available TV inputs.
     */
    public static boolean areInputsAvailable(Context context) {
        return TvApplication.getSingletons(context).getTvInputManagerHelper()
                .getTvInputInfos(true, false).size() > 0;
    }
}
