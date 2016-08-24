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

package com.android.tv.data;

import android.content.Context;
import android.database.Cursor;
import android.graphics.Bitmap.CompressFormat;
import android.media.tv.TvContract;
import android.media.tv.TvContract.Channels;
import android.net.Uri;
import android.os.AsyncTask;
import android.support.annotation.WorkerThread;
import android.text.TextUtils;
import android.util.Log;

import com.android.tv.util.AsyncDbTask;
import com.android.tv.util.BitmapUtils;
import com.android.tv.util.BitmapUtils.ScaledBitmapInfo;
import com.android.tv.util.PermissionUtils;

import java.io.BufferedReader;
import java.io.IOException;
import java.io.InputStreamReader;
import java.io.OutputStream;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.HashSet;
import java.util.List;
import java.util.Locale;
import java.util.Map;
import java.util.Set;

/**
 * Utility class for TMS data.
 * This class is thread safe.
 */
public class ChannelLogoFetcher {
    private static final String TAG = "ChannelLogoFetcher";
    private static final boolean DEBUG = false;

    /**
     * The name of the file which contains the TMS data.
     * The file has multiple records and each of them is a string separated by '|' like
     * STATION_NAME|SHORT_NAME|CALL_SIGN|LOGO_URI.
     */
    private static final String TMS_US_TABLE_FILE = "tms_us.table";
    private static final String TMS_KR_TABLE_FILE = "tms_kr.table";
    private static final String FIELD_SEPARATOR = "\\|";
    private static final String NAME_SEPARATOR_FOR_TMS = "\\(|\\)|\\{|\\}|\\[|\\]";
    private static final String NAME_SEPARATOR_FOR_DB = "\\W";
    private static final int INDEX_NAME = 0;
    private static final int INDEX_SHORT_NAME = 1;
    private static final int INDEX_CALL_SIGN = 2;
    private static final int INDEX_LOGO_URI = 3;

    private static final String COLUMN_CHANNEL_LOGO = "logo";

    private static final Object sLock = new Object();
    private static final Set<Long> sChannelIdBlackListSet = new HashSet<>();
    private static LoadChannelTask sQueryTask;
    private static FetchLogoTask sFetchTask;

    /**
     * Fetch the channel logos from TMS data and insert them into TvProvider.
     * The previous task is canceled and a new task starts.
     */
    public static void startFetchingChannelLogos(Context context) {
        if (!PermissionUtils.hasAccessAllEpg(context)) {
            // TODO: support this feature for non-system LC app. b/23939816
            return;
        }
        synchronized (sLock) {
            stopFetchingChannelLogos();
            if (DEBUG) Log.d(TAG, "Request to start fetching logos.");
            sQueryTask = new LoadChannelTask(context);
            sQueryTask.executeOnDbThread();
        }
    }

    /**
     * Stops the current fetching tasks. This can be called when the Activity pauses.
     */
    public static void stopFetchingChannelLogos() {
        synchronized (sLock) {
            if (DEBUG) Log.d(TAG, "Request to stop fetching logos.");
            if (sQueryTask != null) {
                sQueryTask.cancel(true);
                sQueryTask = null;
            }
            if (sFetchTask != null) {
                sFetchTask.cancel(true);
                sFetchTask = null;
            }
        }
    }

    private ChannelLogoFetcher() {
    }

    private static final class LoadChannelTask extends AsyncDbTask<Void, Void, List<Channel>> {
        private final Context mContext;

        public LoadChannelTask(Context context) {
            mContext = context;
        }

        @Override
        protected List<Channel> doInBackground(Void... arg) {
            // Load channels which doesn't have channel logos.
            if (DEBUG) Log.d(TAG, "Starts loading the channels from DB");
            String[] projection =
                    new String[] { Channels._ID, Channels.COLUMN_DISPLAY_NAME };
            String selection = COLUMN_CHANNEL_LOGO + " IS NULL AND "
                    + Channels.COLUMN_PACKAGE_NAME + "=?";
            String[] selectionArgs = new String[] { mContext.getPackageName() };
            try (Cursor c = mContext.getContentResolver().query(Channels.CONTENT_URI,
                    projection, selection, selectionArgs, null)) {
                if (c == null) {
                    Log.e(TAG, "Query returns null cursor", new RuntimeException());
                    return null;
                }
                List<Channel> channels = new ArrayList<>();
                while (!isCancelled() && c.moveToNext()) {
                    long channelId = c.getLong(0);
                    if (sChannelIdBlackListSet.contains(channelId)) {
                        continue;
                    }
                    channels.add(new Channel.Builder().setId(c.getLong(0))
                            .setDisplayName(c.getString(1).toUpperCase(Locale.getDefault()))
                            .build());
                }
                return channels;
            }
        }

        @Override
        protected void onPostExecute(List<Channel> channels) {
            synchronized (sLock) {
                if (DEBUG) {
                    int count = channels == null ? 0 : channels.size();
                    Log.d(TAG, count + " channels are loaded");
                }
                if (sQueryTask == this) {
                    sQueryTask = null;
                    if (channels != null && !channels.isEmpty()) {
                        sFetchTask = new FetchLogoTask(mContext, channels);
                        sFetchTask.execute();
                    }
                }
            }
        }
    }

    private static final class FetchLogoTask extends AsyncTask<Void, Void, Void> {
        private final Context mContext;
        private final List<Channel> mChannels;

        public FetchLogoTask(Context context, List<Channel> channels) {
            mContext = context;
            mChannels = channels;
        }

        @Override
        protected Void doInBackground(Void... arg) {
            if (isCancelled()) {
                if (DEBUG) Log.d(TAG, "Fetching the channel logos has been canceled");
                return null;
            }
            // Load the TMS table data.
            if (DEBUG) Log.d(TAG, "Loads TMS data");
            Map<String, String> channelNameLogoUriMap = new HashMap<>();
            try {
                channelNameLogoUriMap.putAll(readTmsFile(mContext, TMS_US_TABLE_FILE));
                if (isCancelled()) {
                    if (DEBUG) Log.d(TAG, "Fetching the channel logos has been canceled");
                    return null;
                }
                channelNameLogoUriMap.putAll(readTmsFile(mContext, TMS_KR_TABLE_FILE));
            } catch (IOException e) {
                Log.e(TAG, "Loading TMS data failed.", e);
                return null;
            }
            if (isCancelled()) {
                if (DEBUG) Log.d(TAG, "Fetching the channel logos has been canceled");
                return null;
            }

            // Iterating channels.
            for (Channel channel : mChannels) {
                if (isCancelled()) {
                    if (DEBUG) Log.d(TAG, "Fetching the channel logos has been canceled");
                    return null;
                }
                // Download the channel logo.
                if (TextUtils.isEmpty(channel.getDisplayName())) {
                    if (DEBUG) {
                        Log.d(TAG, "The channel with ID (" + channel.getId()
                                + ") doesn't have the display name.");
                    }
                    sChannelIdBlackListSet.add(channel.getId());
                    continue;
                }
                String channelName = channel.getDisplayName().trim();
                String logoUri = channelNameLogoUriMap.get(channelName);
                if (TextUtils.isEmpty(logoUri)) {
                    if (DEBUG) {
                        Log.d(TAG, "Can't find a logo URI for channel '" + channelName + "'");
                    }
                    // Find the candidate names. If the channel name is CNN-HD, then find CNNHD
                    // and CNN. Or if the channel name is KQED+, then find KQED.
                    String[] splitNames = channelName.split(NAME_SEPARATOR_FOR_DB);
                    if (splitNames.length > 1) {
                        StringBuilder sb = new StringBuilder();
                        for (String splitName : splitNames) {
                            sb.append(splitName);
                        }
                        logoUri = channelNameLogoUriMap.get(sb.toString());
                        if (DEBUG) {
                            if (TextUtils.isEmpty(logoUri)) {
                                Log.d(TAG, "Can't find a logo URI for channel '" + sb.toString()
                                        + "'");
                            }
                        }
                    }
                    if (TextUtils.isEmpty(logoUri)
                            && splitNames[0].length() != channelName.length()) {
                        logoUri = channelNameLogoUriMap.get(splitNames[0]);
                        if (DEBUG) {
                            if (TextUtils.isEmpty(logoUri)) {
                                Log.d(TAG, "Can't find a logo URI for channel '" + splitNames[0]
                                        + "'");
                            }
                        }
                    }
                }
                if (TextUtils.isEmpty(logoUri)) {
                    sChannelIdBlackListSet.add(channel.getId());
                    continue;
                }
                ScaledBitmapInfo bitmapInfo = BitmapUtils.decodeSampledBitmapFromUriString(
                        mContext, logoUri, Integer.MAX_VALUE, Integer.MAX_VALUE);
                if (bitmapInfo == null) {
                    Log.e(TAG, "Failed to load bitmap. {channelName=" + channel.getDisplayName()
                            + ", " + "logoUri=" + logoUri + "}");
                    sChannelIdBlackListSet.add(channel.getId());
                    continue;
                }
                if (isCancelled()) {
                    if (DEBUG) Log.d(TAG, "Fetching the channel logos has been canceled");
                    return null;
                }

                // Insert the logo to DB.
                Uri dstLogoUri = TvContract.buildChannelLogoUri(channel.getId());
                try (OutputStream os = mContext.getContentResolver().openOutputStream(dstLogoUri)) {
                    bitmapInfo.bitmap.compress(CompressFormat.PNG, 100, os);
                } catch (IOException e) {
                    Log.e(TAG, "Failed to write " + logoUri + "  to " + dstLogoUri, e);
                    continue;
                }
                if (DEBUG) {
                    Log.d(TAG, "Inserting logo file to DB succeeded. {from=" + logoUri + ", to="
                            + dstLogoUri + "}");
                }
            }
            if (DEBUG) Log.d(TAG, "Fetching logos has been finished successfully.");
            return null;
        }

        @WorkerThread
        private Map<String, String> readTmsFile(Context context, String fileName)
                throws IOException {
            try (BufferedReader reader = new BufferedReader(new InputStreamReader(
                    context.getAssets().open(fileName)))) {
                Map<String, String> channelNameLogoUriMap = new HashMap<>();
                String line;
                while ((line = reader.readLine()) != null && !isCancelled()) {
                    String[] data = line.split(FIELD_SEPARATOR);
                    if (data.length != INDEX_LOGO_URI + 1) {
                        if (DEBUG) Log.d(TAG, "Invalid or comment row: " + line);
                        continue;
                    }
                    addChannelNames(channelNameLogoUriMap,
                            data[INDEX_NAME].toUpperCase(Locale.getDefault()),
                            data[INDEX_LOGO_URI]);
                    addChannelNames(channelNameLogoUriMap,
                            data[INDEX_SHORT_NAME].toUpperCase(Locale.getDefault()),
                            data[INDEX_LOGO_URI]);
                    addChannelNames(channelNameLogoUriMap,
                            data[INDEX_CALL_SIGN].toUpperCase(Locale.getDefault()),
                            data[INDEX_LOGO_URI]);
                }
                return channelNameLogoUriMap;
            }
        }

        private void addChannelNames(Map<String, String> channelNameLogoUriMap, String channelName,
                String logoUri) {
            if (!TextUtils.isEmpty(channelName)) {
                channelNameLogoUriMap.put(channelName, logoUri);
                // Find the candidate names.
                // If the name is like "W05AAD (W05AA-D)", then split the names into "W05AAD" and
                // "W05AA-D"
                String[] splitNames = channelName.split(NAME_SEPARATOR_FOR_TMS);
                if (splitNames.length > 1) {
                    for (String name : splitNames) {
                        name = name.trim();
                        if (channelNameLogoUriMap.get(name) == null) {
                            channelNameLogoUriMap.put(name, logoUri);
                        }
                    }
                }
            }
        }

        @Override
        protected void onPostExecute(Void result) {
            synchronized (sLock) {
                if (sFetchTask == this) {
                    sFetchTask = null;
                }
            }
        }
    }
}
