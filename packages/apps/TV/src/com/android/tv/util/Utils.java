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

import android.annotation.SuppressLint;
import android.content.ComponentName;
import android.content.ContentResolver;
import android.content.ContentValues;
import android.content.Context;
import android.content.Intent;
import android.content.pm.PackageManager;
import android.content.res.ColorStateList;
import android.content.res.Configuration;
import android.content.res.Resources;
import android.content.res.Resources.Theme;
import android.database.Cursor;
import android.media.tv.TvContract;
import android.media.tv.TvContract.Channels;
import android.media.tv.TvInputInfo;
import android.media.tv.TvTrackInfo;
import android.net.Uri;
import android.os.Build;
import android.preference.PreferenceManager;
import android.support.annotation.Nullable;
import android.support.annotation.VisibleForTesting;
import android.support.annotation.WorkerThread;
import android.text.TextUtils;
import android.text.format.DateUtils;
import android.util.Log;
import android.view.View;
import android.widget.Toast;

import com.android.tv.R;
import com.android.tv.TvApplication;
import com.android.tv.data.Channel;
import com.android.tv.data.Program;
import com.android.tv.data.StreamInfo;

import java.text.SimpleDateFormat;
import java.util.ArrayList;
import java.util.Calendar;
import java.util.Collection;
import java.util.Date;
import java.util.HashSet;
import java.util.List;
import java.util.Locale;
import java.util.Set;
import java.util.TimeZone;
import java.util.concurrent.TimeUnit;

/**
 * A class that includes convenience methods for accessing TvProvider database.
 */
public class Utils {
    private static final String TAG = "Utils";
    private static final boolean DEBUG = false;

    private static final SimpleDateFormat ISO_8601 = new SimpleDateFormat("yyyy-MM-dd'T'HH:mm:ssZ");

    public static final String EXTRA_KEY_KEYCODE = "keycode";
    public static final String EXTRA_KEY_ACTION = "action";
    public static final String EXTRA_ACTION_SHOW_TV_INPUT ="show_tv_input";
    public static final String EXTRA_KEY_FROM_LAUNCHER = "from_launcher";
    public static final String EXTRA_KEY_RECORDING_URI = "recording_uri";

    // Query parameter in the intent of starting MainActivity.
    public static final String PARAM_SOURCE = "source";

    private static final String PATH_CHANNEL = "channel";
    private static final String PATH_PROGRAM = "program";

    private static final String PREF_KEY_LAST_WATCHED_CHANNEL_ID = "last_watched_channel_id";
    private static final String PREF_KEY_LAST_WATCHED_CHANNEL_ID_FOR_INPUT =
            "last_watched_channel_id_for_input_";
    private static final String PREF_KEY_LAST_WATCHED_CHANNEL_URI = "last_watched_channel_uri";

    private static final int VIDEO_SD_WIDTH = 704;
    private static final int VIDEO_SD_HEIGHT = 480;
    private static final int VIDEO_HD_WIDTH = 1280;
    private static final int VIDEO_HD_HEIGHT = 720;
    private static final int VIDEO_FULL_HD_WIDTH = 1920;
    private static final int VIDEO_FULL_HD_HEIGHT = 1080;
    private static final int VIDEO_ULTRA_HD_WIDTH = 2048;
    private static final int VIDEO_ULTRA_HD_HEIGHT = 1536;

    private static final int AUDIO_CHANNEL_NONE = 0;
    private static final int AUDIO_CHANNEL_MONO = 1;
    private static final int AUDIO_CHANNEL_STEREO = 2;
    private static final int AUDIO_CHANNEL_SURROUND_6 = 6;
    private static final int AUDIO_CHANNEL_SURROUND_8 = 8;

    private enum AspectRatio {
        ASPECT_RATIO_4_3(4, 3),
        ASPECT_RATIO_16_9(16, 9),
        ASPECT_RATIO_21_9(21, 9);

        final int width;
        final int height;

        AspectRatio(int width, int height) {
            this.width = width;
            this.height = height;
        }

        @Override
        @SuppressLint("DefaultLocale")
        public String toString() {
            return String.format("%d:%d", width, height);
        }
    }

    private Utils() {
    }

    public static String buildSelectionForIds(String idName, List<Long> ids) {
        StringBuilder sb = new StringBuilder();
        sb.append(idName).append(" in (")
                .append(ids.get(0));
        for (int i = 1; i < ids.size(); ++i) {
            sb.append(",").append(ids.get(i));
        }
        sb.append(")");
        return sb.toString();
    }

    @WorkerThread
    public static String getInputIdForChannel(Context context, long channelId) {
        if (channelId == Channel.INVALID_ID) {
            return null;
        }
        Uri channelUri = TvContract.buildChannelUri(channelId);
        String[] projection = {TvContract.Channels.COLUMN_INPUT_ID};
        try (Cursor cursor = context.getContentResolver()
                .query(channelUri, projection, null, null, null)) {
            if (cursor != null && cursor.moveToNext()) {
                return Utils.intern(cursor.getString(0));
            }
        }
        return null;
    }

    public static void setLastWatchedChannel(Context context, Channel channel) {
        if (channel == null) {
            Log.e(TAG, "setLastWatchedChannel: channel cannot be null");
            return;
        }
        PreferenceManager.getDefaultSharedPreferences(context).edit()
                .putString(PREF_KEY_LAST_WATCHED_CHANNEL_URI, channel.getUri().toString()).apply();
        if (!channel.isPassthrough()) {
            long channelId = channel.getId();
            if (channel.getId() < 0) {
                throw new IllegalArgumentException("channelId should be equal to or larger than 0");
            }
            PreferenceManager.getDefaultSharedPreferences(context).edit()
                    .putLong(PREF_KEY_LAST_WATCHED_CHANNEL_ID, channelId).apply();
            PreferenceManager.getDefaultSharedPreferences(context).edit()
                    .putLong(PREF_KEY_LAST_WATCHED_CHANNEL_ID_FOR_INPUT + channel.getInputId(),
                            channelId).apply();
        }
    }

    public static long getLastWatchedChannelId(Context context) {
        return PreferenceManager.getDefaultSharedPreferences(context)
                .getLong(PREF_KEY_LAST_WATCHED_CHANNEL_ID, Channel.INVALID_ID);
    }

    public static long getLastWatchedChannelIdForInput(Context context, String inputId) {
        return PreferenceManager.getDefaultSharedPreferences(context)
                .getLong(PREF_KEY_LAST_WATCHED_CHANNEL_ID_FOR_INPUT + inputId, Channel.INVALID_ID);
    }

    public static String getLastWatchedChannelUri(Context context) {
        return PreferenceManager.getDefaultSharedPreferences(context)
                .getString(PREF_KEY_LAST_WATCHED_CHANNEL_URI, null);
    }

    /**
     * Returns {@code true}, if {@code uri} specifies an input, which is usually generated
     * from {@link TvContract#buildChannelsUriForInput}.
     */
    public static boolean isChannelUriForInput(Uri uri) {
        return isTvUri(uri) && PATH_CHANNEL.equals(uri.getPathSegments().get(0))
                && !TextUtils.isEmpty(uri.getQueryParameter("input"));
    }

    /**
     * Returns {@code true}, if {@code uri} is a channel URI for a specific channel. It is copied
     * from the hidden method TvContract.isChannelUri.
     */
    public static boolean isChannelUriForOneChannel(Uri uri) {
        return isChannelUriForTunerInput(uri) || TvContract.isChannelUriForPassthroughInput(uri);
    }

    /**
     * Returns {@code true}, if {@code uri} is a channel URI for a tuner input. It is copied from
     * the hidden method TvContract.isChannelUriForTunerInput.
     */
    public static boolean isChannelUriForTunerInput(Uri uri) {
        return isTvUri(uri) && isTwoSegmentUriStartingWith(uri, PATH_CHANNEL);
    }

    private static boolean isTvUri(Uri uri) {
        return uri != null && ContentResolver.SCHEME_CONTENT.equals(uri.getScheme())
                && TvContract.AUTHORITY.equals(uri.getAuthority());
    }

    private static boolean isTwoSegmentUriStartingWith(Uri uri, String pathSegment) {
        List<String> pathSegments = uri.getPathSegments();
        return pathSegments.size() == 2 && pathSegment.equals(pathSegments.get(0));
    }

    /**
     * Returns {@code true}, if {@code uri} is a programs URI.
     */
    public static boolean isProgramsUri(Uri uri) {
        return isTvUri(uri) && PATH_PROGRAM.equals(uri.getPathSegments().get(0));
    }

    /**
     * Gets the info of the program on particular time.
     */
    @WorkerThread
    public static Program getProgramAt(Context context, long channelId, long timeMs) {
        if (channelId == Channel.INVALID_ID) {
            Log.e(TAG, "getCurrentProgramAt - channelId is invalid");
            return null;
        }
        if (context.getMainLooper().getThread().equals(Thread.currentThread())) {
            String message = "getCurrentProgramAt called on main thread";
            if (DEBUG) {
                // Generating a stack trace can be expensive, only do it in debug mode.
                Log.w(TAG, message, new IllegalStateException(message));
            } else {
                Log.w(TAG, message);
            }
        }
        Uri uri = TvContract.buildProgramsUriForChannel(TvContract.buildChannelUri(channelId),
                timeMs, timeMs);
        try (Cursor cursor = context.getContentResolver().query(uri, Program.PROJECTION,
                null, null, null)) {
            if (cursor != null && cursor.moveToNext()) {
                return Program.fromCursor(cursor);
            }
        }
        return null;
    }

    /**
     * Gets the info of the current program.
     */
    @WorkerThread
    public static Program getCurrentProgram(Context context, long channelId) {
        return getProgramAt(context, channelId, System.currentTimeMillis());
    }

    /**
     * Returns duration string according to the date & time format.
     * If {@code startUtcMillis} and {@code endUtcMills} are equal,
     * formatted time will be returned instead.
     *
     * @param startUtcMillis start of duration in millis. Should be less than {code endUtcMillis}.
     * @param endUtcMillis end of duration in millis. Should be larger than {@code startUtcMillis}.
     * @param useShortFormat {@code true} if abbreviation is needed to save space.
     *                       In that case, date will be omitted if duration starts from today
     *                       and is less than a day. If it's necessary,
     *                       {@link DateUtils#FORMAT_NUMERIC_DATE} is used otherwise.
     */
    public static String getDurationString(
            Context context, long startUtcMillis, long endUtcMillis, boolean useShortFormat) {
        return getDurationString(context, System.currentTimeMillis(), startUtcMillis, endUtcMillis,
                useShortFormat, 0);
    }

    @VisibleForTesting
    static String getDurationString(Context context, long baseMillis,
            long startUtcMillis, long endUtcMillis, boolean useShortFormat, int flag) {
        flag |= DateUtils.FORMAT_ABBREV_MONTH | DateUtils.FORMAT_SHOW_TIME
                | ((useShortFormat) ? DateUtils.FORMAT_NUMERIC_DATE : 0);
        if (!isInGivenDay(baseMillis, startUtcMillis)) {
            flag |= DateUtils.FORMAT_SHOW_DATE;
        }
        if (startUtcMillis != endUtcMillis && useShortFormat) {
            // Do special handling for 12:00 AM when checking if it's in the given day.
            // If it's start, it's considered as beginning of the day. (e.g. 12:00 AM - 12:30 AM)
            // If it's end, it's considered as end of the day (e.g. 11:00 PM - 12:00 AM)
            if (!isInGivenDay(startUtcMillis, endUtcMillis - 1)
                    && endUtcMillis - startUtcMillis < TimeUnit.HOURS.toMillis(11)) {
                // Do not show date for short format.
                // Extracting a day is needed because {@link DateUtils@formatDateRange}
                // adds date if the duration covers multiple days.
                return DateUtils.formatDateRange(context,
                        startUtcMillis, endUtcMillis - TimeUnit.DAYS.toMillis(1), flag);
            }
        }
        return DateUtils.formatDateRange(context, startUtcMillis, endUtcMillis, flag);
    }

    @VisibleForTesting
    public static boolean isInGivenDay(long dayToMatchInMillis, long subjectTimeInMillis) {
        final long DAY_IN_MS = TimeUnit.DAYS.toMillis(1);
        TimeZone timeZone = Calendar.getInstance().getTimeZone();
        long offset = timeZone.getRawOffset();
        if (timeZone.inDaylightTime(new Date(dayToMatchInMillis))) {
            offset += timeZone.getDSTSavings();
        }
        return Utils.floorTime(dayToMatchInMillis + offset, DAY_IN_MS)
                == Utils.floorTime(subjectTimeInMillis + offset, DAY_IN_MS);
    }

    public static String getAspectRatioString(int width, int height) {
        if (width == 0 || height == 0) {
            return "";
        }

        for (AspectRatio ratio: AspectRatio.values()) {
            if (Math.abs((float) ratio.height / ratio.width - (float) height / width) < 0.05f) {
                return ratio.toString();
            }
        }
        return "";
    }

    public static String getAspectRatioString(float videoDisplayAspectRatio) {
        if (videoDisplayAspectRatio <= 0) {
            return "";
        }

        for (AspectRatio ratio : AspectRatio.values()) {
            if (Math.abs((float) ratio.width / ratio.height - videoDisplayAspectRatio) < 0.05f) {
                return ratio.toString();
            }
        }
        return "";
    }

    public static int getVideoDefinitionLevelFromSize(int width, int height) {
        if (width >= VIDEO_ULTRA_HD_WIDTH && height >= VIDEO_ULTRA_HD_HEIGHT) {
            return StreamInfo.VIDEO_DEFINITION_LEVEL_ULTRA_HD;
        } else if (width >= VIDEO_FULL_HD_WIDTH && height >= VIDEO_FULL_HD_HEIGHT) {
            return StreamInfo.VIDEO_DEFINITION_LEVEL_FULL_HD;
        } else if (width >= VIDEO_HD_WIDTH && height >= VIDEO_HD_HEIGHT) {
            return StreamInfo.VIDEO_DEFINITION_LEVEL_HD;
        } else if (width >= VIDEO_SD_WIDTH && height >= VIDEO_SD_HEIGHT) {
            return StreamInfo.VIDEO_DEFINITION_LEVEL_SD;
        }
        return StreamInfo.VIDEO_DEFINITION_LEVEL_UNKNOWN;
    }

    public static String getVideoDefinitionLevelString(Context context, int videoFormat) {
        switch (videoFormat) {
            case StreamInfo.VIDEO_DEFINITION_LEVEL_ULTRA_HD:
                return context.getResources().getString(
                        R.string.video_definition_level_ultra_hd);
            case StreamInfo.VIDEO_DEFINITION_LEVEL_FULL_HD:
                return context.getResources().getString(
                        R.string.video_definition_level_full_hd);
            case StreamInfo.VIDEO_DEFINITION_LEVEL_HD:
                return context.getResources().getString(R.string.video_definition_level_hd);
            case StreamInfo.VIDEO_DEFINITION_LEVEL_SD:
                return context.getResources().getString(R.string.video_definition_level_sd);
        }
        return "";
    }

    public static String getAudioChannelString(Context context, int channelCount) {
        switch (channelCount) {
            case 1:
                return context.getResources().getString(R.string.audio_channel_mono);
            case 2:
                return context.getResources().getString(R.string.audio_channel_stereo);
            case 6:
                return context.getResources().getString(R.string.audio_channel_5_1);
            case 8:
                return context.getResources().getString(R.string.audio_channel_7_1);
        }
        return "";
    }

    public static boolean needToShowSampleRate(Context context, List<TvTrackInfo> tracks) {
        Set<String> multiAudioStrings = new HashSet<>();
        for (TvTrackInfo track : tracks) {
            String multiAudioString = getMultiAudioString(context, track, false);
            if (multiAudioStrings.contains(multiAudioString)) {
                return true;
            }
            multiAudioStrings.add(multiAudioString);
        }
        return false;
    }

    public static String getMultiAudioString(Context context, TvTrackInfo track,
            boolean showSampleRate) {
        if (track.getType() != TvTrackInfo.TYPE_AUDIO) {
            throw new IllegalArgumentException("Not an audio track: " + track);
        }
        String language = context.getString(R.string.default_language);
        if (!TextUtils.isEmpty(track.getLanguage())) {
            language = new Locale(track.getLanguage()).getDisplayName();
        } else {
            Log.d(TAG, "No language information found for the audio track: " + track);
        }

        StringBuilder metadata = new StringBuilder();
        switch (track.getAudioChannelCount()) {
            case AUDIO_CHANNEL_NONE:
                break;
            case AUDIO_CHANNEL_MONO:
                metadata.append(context.getString(R.string.multi_audio_channel_mono));
                break;
            case AUDIO_CHANNEL_STEREO:
                metadata.append(context.getString(R.string.multi_audio_channel_stereo));
                break;
            case AUDIO_CHANNEL_SURROUND_6:
                metadata.append(context.getString(R.string.multi_audio_channel_surround_6));
                break;
            case AUDIO_CHANNEL_SURROUND_8:
                metadata.append(context.getString(R.string.multi_audio_channel_surround_8));
                break;
            default:
                if (track.getAudioChannelCount() > 0) {
                    metadata.append(context.getString(R.string.multi_audio_channel_suffix,
                            track.getAudioChannelCount()));
                } else {
                    Log.d(TAG, "Invalid audio channel count (" + track.getAudioChannelCount()
                            + ") found for the audio track: " + track);
                }
                break;
        }
        if (showSampleRate) {
            int sampleRate = track.getAudioSampleRate();
            if (sampleRate > 0) {
                if (metadata.length() > 0) {
                    metadata.append(", ");
                }
                int integerPart = sampleRate / 1000;
                int tenths = (sampleRate % 1000) / 100;
                metadata.append(integerPart);
                if (tenths != 0) {
                    metadata.append(".");
                    metadata.append(tenths);
                }
                metadata.append("kHz");
            }
        }

        if (metadata.length() == 0) {
            return language;
        }
        return context.getString(R.string.multi_audio_display_string_with_channel, language,
                metadata.toString());
    }

    public static boolean isEqualLanguage(String lang1, String lang2) {
        if (lang1 == null) {
            return lang2 == null;
        } else if (lang2 == null) {
            return false;
        }
        try {
            return TextUtils.equals(
                    new Locale(lang1).getISO3Language(), new Locale(lang2).getISO3Language());
        } catch (Exception ignored) {
        }
        return false;
    }

    public static boolean isIntentAvailable(Context context, Intent intent) {
       return context.getPackageManager().queryIntentActivities(
               intent, PackageManager.MATCH_DEFAULT_ONLY).size() > 0;
    }

    /**
     * Returns the label for a given input. Returns the custom label, if any.
     */
    public static String loadLabel(Context context, TvInputInfo input) {
        if (input == null) {
            return null;
        }
        CharSequence customLabel = input.loadCustomLabel(context);
        String label = (customLabel == null) ? null : customLabel.toString();
        if (TextUtils.isEmpty(label)) {
            label = input.loadLabel(context).toString();
        }
        return label;
    }

    /**
     * Enable all channels synchronously.
     */
    @WorkerThread
    public static void enableAllChannels(Context context) {
        ContentValues values = new ContentValues();
        values.put(Channels.COLUMN_BROWSABLE, 1);
        context.getContentResolver().update(Channels.CONTENT_URI, values, null, null);
    }

    /**
     * Converts time in milliseconds to a String.
     */
    public static String toTimeString(long timeMillis) {
        return new Date(timeMillis).toString();
    }

    /**
     * Converts time in milliseconds to a ISO 8061 string.
     */
    public static String toIsoDateTimeString(long timeMillis) {
        return ISO_8601.format(new Date(timeMillis));
    }

    /**
     * Returns a {@link String} object which contains the layout information of the {@code view}.
     */
    public static String toRectString(View view) {
        return "{"
                + "l=" + view.getLeft()
                + ",r=" + view.getRight()
                + ",t=" + view.getTop()
                + ",b=" + view.getBottom()
                + ",w=" + view.getWidth()
                + ",h=" + view.getHeight() + "}";
    }

    /**
     * Floors time to the given {@code timeUnit}. For example, if time is 5:32:11 and timeUnit is
     * one hour (60 * 60 * 1000), then the output will be 5:00:00.
     */
    public static long floorTime(long timeMs, long timeUnit) {
        return timeMs - (timeMs % timeUnit);
    }

    /**
     * Ceils time to the given {@code timeUnit}. For example, if time is 5:32:11 and timeUnit is
     * one hour (60 * 60 * 1000), then the output will be 6:00:00.
     */
    public static long ceilTime(long timeMs, long timeUnit) {
        return timeMs + timeUnit - (timeMs % timeUnit);
    }

    /**
     * Returns an {@link String#intern() interned} string or null if the input is null.
     */
    @Nullable
    public static String intern(@Nullable String string) {
        return string == null ? null : string.intern();
    }

    /**
     * Check if the index is valid for the collection,
     * @param collection the collection
     * @param index the index position to test
     * @return index >= 0 && index < collection.size().
     */
    public static boolean isIndexValid(@Nullable Collection<?> collection, int index) {
        return collection == null ? false : index >= 0 && index < collection.size();
    }

    /**
     * Returns a color integer associated with a particular resource ID.
     *
     * @see #getColor(android.content.res.Resources,int,Theme)
     */
    public static int getColor(Resources res, int id) {
        return getColor(res, id, null);
    }

    /**
     * Returns a color integer associated with a particular resource ID.
     *
     * <p>In M version, {@link android.content.res.Resources#getColor(int)} was deprecated and
     * {@link android.content.res.Resources#getColor(int,Theme)} was newly added.
     *
     * @see android.content.res.Resources#getColor(int)
     */
    public static int getColor(Resources res, int id, @Nullable Theme theme) {
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.M) {
            return res.getColor(id, theme);
        } else {
            return res.getColor(id);
        }
    }

    /**
     * Returns a color state list associated with a particular resource ID.
     *
     * @see #getColorStateList(android.content.res.Resources,int,Theme)
     */
    public static ColorStateList getColorStateList(Resources res, int id) {
        return getColorStateList(res, id, null);
    }

    /**
     * Returns a color state list associated with a particular resource ID.
     *
     * <p>In M version, {@link android.content.res.Resources#getColorStateList(int)} was deprecated
     * and {@link android.content.res.Resources#getColorStateList(int,Theme)} was newly added.
     *
     * @see android.content.res.Resources#getColorStateList(int)
     */
    public static ColorStateList getColorStateList(Resources res, int id, @Nullable Theme theme) {
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.M) {
            return res.getColorStateList(id, theme);
        } else {
            return res.getColorStateList(id);
        }
    }

    /**
     * Returns a localized version of the text resource specified by resourceId.
     */
    public static CharSequence getTextForLocale(Context context, Locale locale, int resourceId) {
        if (locale.equals(context.getResources().getConfiguration().locale)) {
            return context.getText(resourceId);
        }
        Configuration config = new Configuration(context.getResources().getConfiguration());
        config.setLocale(locale);
        return context.createConfigurationContext(config).getText(resourceId);
    }

    /**
     * Returns the internal TV inputs.
     */
    public static List<TvInputInfo> getInternalTvInputs(Context context, boolean tunerInputOnly) {
        List<TvInputInfo> inputs = new ArrayList<>();
        String contextPackageName = context.getPackageName();
        for (TvInputInfo input : TvApplication.getSingletons(context).getTvInputManagerHelper()
                .getTvInputInfos(true, tunerInputOnly)) {
            if (contextPackageName.equals(ComponentName.unflattenFromString(input.getId())
                    .getPackageName())) {
                inputs.add(input);
            }
        }
        return inputs;
    }

    /**
     * Checks whether the input is internal or not.
     */
    public static boolean isInternalTvInput(Context context, String inputId) {
        return context.getPackageName().equals(ComponentName.unflattenFromString(inputId)
                .getPackageName());
    }

    /**
     * Shows a toast message to notice that the current feature is a developer feature.
     */
    public static void showToastMessageForDeveloperFeature(Context context) {
        Toast.makeText(context, "This feature is for developer preview.", Toast.LENGTH_SHORT)
                .show();
    }
}
