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

import android.content.Context;
import android.preference.PreferenceManager;
import android.support.annotation.IntDef;

import java.lang.annotation.Retention;
import java.lang.annotation.RetentionPolicy;
import java.util.Collections;
import java.util.HashSet;
import java.util.Set;


/**
 * A class about the constants for TV settings.
 * Objects that are returned from the various {@code get} methods must be treated as immutable.
 */
public final class TvSettings {
    private TvSettings() {}

    public static final String PREFS_FILE = "settings";
    public static final String PREF_TV_WATCH_LOGGING_ENABLED = "tv_watch_logging_enabled";
    public static final String PREF_CLOSED_CAPTION_ENABLED = "is_cc_enabled";  // boolean value
    public static final String PREF_DISPLAY_MODE = "display_mode";  // int value
    public static final String PREF_PIP_LAYOUT = "pip_layout"; // int value
    public static final String PREF_PIP_SIZE = "pip_size";  // int value
    public static final String PREF_PIN = "pin"; // 4-digit string value. Otherwise, it's not set.

    // PIP sounds
    @Retention(RetentionPolicy.SOURCE)
    @IntDef({
            PIP_SOUND_MAIN, PIP_SOUND_PIP_WINDOW })
    public @interface PipSound {}
    public static final int PIP_SOUND_MAIN = 0;
    public static final int PIP_SOUND_PIP_WINDOW = PIP_SOUND_MAIN + 1;
    public static final int PIP_SOUND_LAST = PIP_SOUND_PIP_WINDOW;

    // PIP layouts
    @Retention(RetentionPolicy.SOURCE)
    @IntDef({
            PIP_LAYOUT_BOTTOM_RIGHT, PIP_LAYOUT_TOP_RIGHT, PIP_LAYOUT_TOP_LEFT,
            PIP_LAYOUT_BOTTOM_LEFT, PIP_LAYOUT_SIDE_BY_SIDE })
    public @interface PipLayout {}
    public static final int PIP_LAYOUT_BOTTOM_RIGHT = 0;
    public static final int PIP_LAYOUT_TOP_RIGHT = PIP_LAYOUT_BOTTOM_RIGHT + 1;
    public static final int PIP_LAYOUT_TOP_LEFT = PIP_LAYOUT_TOP_RIGHT + 1;
    public static final int PIP_LAYOUT_BOTTOM_LEFT = PIP_LAYOUT_TOP_LEFT + 1;
    public static final int PIP_LAYOUT_SIDE_BY_SIDE = PIP_LAYOUT_BOTTOM_LEFT + 1;
    public static final int PIP_LAYOUT_LAST = PIP_LAYOUT_SIDE_BY_SIDE;

    // PIP sizes
    @Retention(RetentionPolicy.SOURCE)
    @IntDef({ PIP_SIZE_SMALL, PIP_SIZE_BIG })
    public @interface PipSize {}
    public static final int PIP_SIZE_SMALL = 0;
    public static final int PIP_SIZE_BIG = PIP_SIZE_SMALL + 1;
    public static final int PIP_SIZE_LAST = PIP_SIZE_BIG;

    // Multi-track audio settings
    private static final String PREF_MULTI_AUDIO_ID = "pref.multi_audio_id";
    private static final String PREF_MULTI_AUDIO_LANGUAGE = "pref.multi_audio_language";
    private static final String PREF_MULTI_AUDIO_CHANNEL_COUNT = "pref.multi_audio_channel_count";

    // Parental Control settings
    private static final String PREF_CONTENT_RATING_SYSTEMS = "pref.content_rating_systems";
    private static final String PREF_CONTENT_RATING_LEVEL = "pref.content_rating_level";
    private static final String PREF_DISABLE_PIN_UNTIL = "pref.disable_pin_until";

    @Retention(RetentionPolicy.SOURCE)
    @IntDef({
            CONTENT_RATING_LEVEL_NONE, CONTENT_RATING_LEVEL_HIGH, CONTENT_RATING_LEVEL_MEDIUM,
            CONTENT_RATING_LEVEL_LOW, CONTENT_RATING_LEVEL_CUSTOM })
    public @interface ContentRatingLevel {}
    public static final int CONTENT_RATING_LEVEL_NONE = 0;
    public static final int CONTENT_RATING_LEVEL_HIGH = 1;
    public static final int CONTENT_RATING_LEVEL_MEDIUM = 2;
    public static final int CONTENT_RATING_LEVEL_LOW = 3;
    public static final int CONTENT_RATING_LEVEL_CUSTOM = 4;

    // PIP settings
    /**
     * Returns the layout of the PIP window stored in the shared preferences.
     *
     * @return the saved layout of the PIP window. This value is one of
     *         {@link #PIP_LAYOUT_TOP_LEFT}, {@link #PIP_LAYOUT_TOP_RIGHT},
     *         {@link #PIP_LAYOUT_BOTTOM_LEFT}, {@link #PIP_LAYOUT_BOTTOM_RIGHT} and
     *         {@link #PIP_LAYOUT_SIDE_BY_SIDE}. If the preference value does not exist,
     *         {@link #PIP_LAYOUT_BOTTOM_RIGHT} is returned.
     */
    @SuppressWarnings("ResourceType")
    @PipLayout
    public static int getPipLayout(Context context) {
        return PreferenceManager.getDefaultSharedPreferences(context).getInt(
                PREF_PIP_LAYOUT, PIP_LAYOUT_BOTTOM_RIGHT);
    }

    /**
     * Stores the layout of PIP window to the shared preferences.
     *
     * @param pipLayout This value should be one of {@link #PIP_LAYOUT_TOP_LEFT},
     *            {@link #PIP_LAYOUT_TOP_RIGHT}, {@link #PIP_LAYOUT_BOTTOM_LEFT},
     *            {@link #PIP_LAYOUT_BOTTOM_RIGHT} and {@link #PIP_LAYOUT_SIDE_BY_SIDE}.
     */
    public static void setPipLayout(Context context, @PipLayout int pipLayout) {
        PreferenceManager.getDefaultSharedPreferences(context).edit().putInt(
                PREF_PIP_LAYOUT, pipLayout).apply();
    }

    /**
     * Returns the size of the PIP view stored in the shared preferences.
     *
     * @return the saved size of the PIP view. This value is one of
     *         {@link #PIP_SIZE_SMALL} and {@link #PIP_SIZE_BIG}. If the preference value does not
     *         exist, {@link #PIP_SIZE_SMALL} is returned.
     */
    @SuppressWarnings("ResourceType")
    @PipSize
    public static int getPipSize(Context context) {
        return PreferenceManager.getDefaultSharedPreferences(context).getInt(
                PREF_PIP_SIZE, PIP_SIZE_SMALL);
    }

    /**
     * Stores the size of PIP view to the shared preferences.
     *
     * @param pipSize This value should be one of {@link #PIP_SIZE_SMALL} and {@link #PIP_SIZE_BIG}.
     */
    public static void setPipSize(Context context, @PipSize int pipSize) {
        PreferenceManager.getDefaultSharedPreferences(context).edit().putInt(
                PREF_PIP_SIZE, pipSize).apply();
    }

    // Multi-track audio settings
    public static String getMultiAudioId(Context context) {
        return PreferenceManager.getDefaultSharedPreferences(context).getString(
                PREF_MULTI_AUDIO_ID, null);
    }

    public static void setMultiAudioId(Context context, String language) {
        PreferenceManager.getDefaultSharedPreferences(context).edit().putString(
                PREF_MULTI_AUDIO_ID, language).apply();
    }

    public static String getMultiAudioLanguage(Context context) {
        return PreferenceManager.getDefaultSharedPreferences(context).getString(
                PREF_MULTI_AUDIO_LANGUAGE, null);
    }

    public static void setMultiAudioLanguage(Context context, String language) {
        PreferenceManager.getDefaultSharedPreferences(context).edit().putString(
                PREF_MULTI_AUDIO_LANGUAGE, language).apply();
    }

    public static int getMultiAudioChannelCount(Context context) {
        return PreferenceManager.getDefaultSharedPreferences(context).getInt(
                PREF_MULTI_AUDIO_CHANNEL_COUNT, 0);
    }

    public static void setMultiAudioChannelCount(Context context, int channelCount) {
        PreferenceManager.getDefaultSharedPreferences(context).edit().putInt(
                PREF_MULTI_AUDIO_CHANNEL_COUNT, channelCount).apply();
    }

    // Parental Control settings
    public static void addContentRatingSystems(Context context, Set<String> ids) {
        Set<String> contentRatingSystemSet = getContentRatingSystemSet(context);
        if (contentRatingSystemSet.addAll(ids)) {
            PreferenceManager.getDefaultSharedPreferences(context).edit()
                    .putStringSet(PREF_CONTENT_RATING_SYSTEMS, contentRatingSystemSet).apply();
        }
    }

    public static void addContentRatingSystem(Context context, String id) {
        Set<String> contentRatingSystemSet = getContentRatingSystemSet(context);
        if (contentRatingSystemSet.add(id)) {
            PreferenceManager.getDefaultSharedPreferences(context).edit()
                    .putStringSet(PREF_CONTENT_RATING_SYSTEMS, contentRatingSystemSet).apply();
        }
    }

    public static void removeContentRatingSystems(Context context, Set<String> ids) {
        Set<String> contentRatingSystemSet = getContentRatingSystemSet(context);
        if (contentRatingSystemSet.removeAll(ids)) {
            PreferenceManager.getDefaultSharedPreferences(context).edit()
                    .putStringSet(PREF_CONTENT_RATING_SYSTEMS, contentRatingSystemSet).apply();
        }
    }

    public static void removeContentRatingSystem(Context context, String id) {
        Set<String> contentRatingSystemSet = getContentRatingSystemSet(context);
        if (contentRatingSystemSet.remove(id)) {
            PreferenceManager.getDefaultSharedPreferences(context).edit()
                    .putStringSet(PREF_CONTENT_RATING_SYSTEMS, contentRatingSystemSet).apply();
        }
    }

    public static boolean hasContentRatingSystem(Context context, String id) {
        return getContentRatingSystemSet(context).contains(id);
    }

    /**
     * Returns whether the content rating system is ever set. Returns {@code false} only when the
     * user changes parental control settings for the first time.
     */
    public static boolean isContentRatingSystemSet(Context context) {
        return PreferenceManager.getDefaultSharedPreferences(context)
                .getStringSet(PREF_CONTENT_RATING_SYSTEMS, null) != null;
    }

    private static Set<String> getContentRatingSystemSet(Context context) {
        return new HashSet<>(PreferenceManager.getDefaultSharedPreferences(context)
                .getStringSet(PREF_CONTENT_RATING_SYSTEMS, Collections.<String>emptySet()));
    }

    @ContentRatingLevel
    @SuppressWarnings("ResourceType")
    public static int getContentRatingLevel(Context context) {
        return PreferenceManager.getDefaultSharedPreferences(context).getInt(
                PREF_CONTENT_RATING_LEVEL, CONTENT_RATING_LEVEL_NONE);
    }

    public static void setContentRatingLevel(Context context,
            @ContentRatingLevel int level) {
        PreferenceManager.getDefaultSharedPreferences(context).edit().putInt(
                PREF_CONTENT_RATING_LEVEL, level).apply();
    }

    /**
     * Returns the time until we should disable the PIN dialog (because the user input wrong PINs
     * repeatedly).
     */
    public static long getDisablePinUntil(Context context) {
        return PreferenceManager.getDefaultSharedPreferences(context).getLong(
                PREF_DISABLE_PIN_UNTIL, 0);
    }

    /**
     * Saves the time until we should disable the PIN dialog (because the user input wrong PINs
     * repeatedly).
     */
    public static void setDisablePinUntil(Context context, long timeMillis) {
        PreferenceManager.getDefaultSharedPreferences(context).edit().putLong(
                PREF_DISABLE_PIN_UNTIL, timeMillis).apply();
    }
}
