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

package com.android.tv.data.epg;

import android.content.ContentProviderOperation;
import android.content.ContentResolver;
import android.content.ContentValues;
import android.content.Context;
import android.content.OperationApplicationException;
import android.database.Cursor;
import android.media.tv.TvContract;
import android.media.tv.TvContract.Programs;
import android.media.tv.TvInputInfo;
import android.media.tv.TvInputManager.TvInputCallback;
import android.os.HandlerThread;
import android.os.Looper;
import android.os.Message;
import android.os.RemoteException;
import android.preference.PreferenceManager;
import android.support.annotation.NonNull;
import android.text.TextUtils;
import android.util.Log;

import com.android.tv.Features;
import com.android.tv.TvApplication;
import com.android.tv.common.WeakHandler;
import com.android.tv.data.Channel;
import com.android.tv.data.Program;
import com.android.tv.util.RecurringRunner;
import com.android.tv.util.TvInputManagerHelper;
import com.android.tv.util.Utils;

import java.util.ArrayList;
import java.util.Collections;
import java.util.List;
import java.util.Objects;
import java.util.concurrent.TimeUnit;

/**
 * An utility class to fetch the EPG. This class isn't thread-safe.
 */
public class EpgFetcher {
    private static final String TAG = "EpgFetcher";
    private static final boolean DEBUG = false;

    private static final int MSG_FETCH_EPG = 1;

    private static final long EPG_PREFETCH_RECURRING_PERIOD_MS = TimeUnit.HOURS.toMillis(4);
    private static final long EPG_READER_INIT_WAIT_MS = TimeUnit.MINUTES.toMillis(1);
    private static final long PROGRAM_QUERY_DURATION = TimeUnit.DAYS.toMillis(30);

    private static final int BATCH_OPERATION_COUNT = 100;

    // Value: Long
    private static final String KEY_LAST_UPDATED_EPG_TIMESTAMP =
            "com.android.tv.data.epg.EpgFetcher.LastUpdatedEpgTimestamp";

    private final Context mContext;
    private final TvInputManagerHelper mInputHelper;
    private final TvInputCallback mInputCallback;
    private HandlerThread mHandlerThread;
    private EpgFetcherHandler mHandler;
    private RecurringRunner mRecurringRunner;

    private long mLastEpgTimestamp = -1;

    public EpgFetcher(Context context) {
        mContext = context;
        mInputHelper = TvApplication.getSingletons(mContext).getTvInputManagerHelper();
        mInputCallback = new TvInputCallback() {
            @Override
            public void onInputAdded(String inputId) {
                if (Utils.isInternalTvInput(mContext, inputId)) {
                    mHandler.removeMessages(MSG_FETCH_EPG);
                    mHandler.sendEmptyMessage(MSG_FETCH_EPG);
                }
            }
        };
    }

    /**
     * Starts fetching EPG.
     */
    public void start() {
        if (DEBUG) Log.d(TAG, "Request to start fetching EPG.");
        if (!Features.FETCH_EPG.isEnabled(mContext)) {
            return;
        }
        if (mHandlerThread == null) {
            mHandlerThread = new HandlerThread("EpgFetcher");
            mHandlerThread.start();
            mHandler = new EpgFetcherHandler(mHandlerThread.getLooper(), this);
            mInputHelper.addCallback(mInputCallback);
            mRecurringRunner = new RecurringRunner(mContext, EPG_PREFETCH_RECURRING_PERIOD_MS,
                    new Runnable() {
                        @Override
                        public void run() {
                            mHandler.removeMessages(MSG_FETCH_EPG);
                            mHandler.sendEmptyMessage(MSG_FETCH_EPG);
                        }
                    }, null);
            mRecurringRunner.start();
        }
    }

    /**
     * Stops fetching EPG.
     */
    public void stop() {
        if (mHandlerThread == null) {
            return;
        }
        mRecurringRunner.stop();
        mHandler.removeCallbacksAndMessages(null);
        mHandler = null;
        mHandlerThread.quit();
        mHandlerThread = null;
    }

    private void onFetchEpg() {
        if (DEBUG) Log.d(TAG, "Start fetching EPG.");
        // Check for the internal inputs.
        boolean hasInternalInput = false;
        for (TvInputInfo input : mInputHelper.getTvInputInfos(true, true)) {
            if (Utils.isInternalTvInput(mContext, input.getId())) {
                hasInternalInput = true;
                break;
            }
        }
        if (!hasInternalInput) {
            if (DEBUG) Log.d(TAG, "No internal input found.");
            return;
        }
        // Check if EPG reader is available.
        EpgReader epgReader = new StubEpgReader(mContext);
        if (!epgReader.isAvailable()) {
            if (DEBUG) Log.d(TAG, "EPG reader is not temporarily available.");
            mHandler.removeMessages(MSG_FETCH_EPG);
            mHandler.sendEmptyMessageDelayed(MSG_FETCH_EPG, EPG_READER_INIT_WAIT_MS);
            return;
        }
        // Check the EPG Timestamp.
        long epgTimestamp = epgReader.getEpgTimestamp();
        if (epgTimestamp <= getLastUpdatedEpgTimestamp()) {
            if (DEBUG) Log.d(TAG, "No new EPG.");
            return;
        }

        List<Channel> channels = epgReader.getChannels();
        for (Channel channel : channels) {
            List<Program> programs = new ArrayList<>(epgReader.getPrograms(channel.getId()));
            Collections.sort(programs);
            if (DEBUG) {
                Log.d(TAG, "Fetching " + programs.size() + " programs for channel " + channel);
            }
            updateEpg(channel.getId(), programs);
        }

        setLastUpdatedEpgTimestamp(epgTimestamp);
    }

    private long getLastUpdatedEpgTimestamp() {
        if (mLastEpgTimestamp < 0) {
            mLastEpgTimestamp = PreferenceManager.getDefaultSharedPreferences(mContext).getLong(
                    KEY_LAST_UPDATED_EPG_TIMESTAMP, 0);
        }
        return mLastEpgTimestamp;
    }

    private void setLastUpdatedEpgTimestamp(long timestamp) {
        mLastEpgTimestamp = timestamp;
        PreferenceManager.getDefaultSharedPreferences(mContext).edit().putLong(
                KEY_LAST_UPDATED_EPG_TIMESTAMP, timestamp);
    }

    private void updateEpg(long channelId, List<Program> newPrograms) {
        final int fetchedProgramsCount = newPrograms.size();
        if (fetchedProgramsCount == 0) {
            return;
        }
        long startTimeMs = System.currentTimeMillis();
        long endTimeMs = startTimeMs + PROGRAM_QUERY_DURATION;
        List<Program> oldPrograms = queryPrograms(mContext.getContentResolver(), channelId,
                startTimeMs, endTimeMs);
        Program currentOldProgram = oldPrograms.size() > 0 ? oldPrograms.get(0) : null;
        int oldProgramsIndex = 0;
        int newProgramsIndex = 0;
        // Skip the past programs. They will be automatically removed by the system.
        if (currentOldProgram != null) {
            long oldStartTimeUtcMillis = currentOldProgram.getStartTimeUtcMillis();
            for (Program program : newPrograms) {
                if (program.getEndTimeUtcMillis() > oldStartTimeUtcMillis) {
                    break;
                }
                newProgramsIndex++;
            }
        }
        // Compare the new programs with old programs one by one and update/delete the old one
        // or insert new program if there is no matching program in the database.
        ArrayList<ContentProviderOperation> ops = new ArrayList<>();
        while (newProgramsIndex < fetchedProgramsCount) {
            // TODO: Extract to method and make test.
            Program oldProgram = oldProgramsIndex < oldPrograms.size()
                    ? oldPrograms.get(oldProgramsIndex) : null;
            Program newProgram = newPrograms.get(newProgramsIndex);
            boolean addNewProgram = false;
            if (oldProgram != null) {
                if (oldProgram.equals(newProgram)) {
                    // Exact match. No need to update. Move on to the next programs.
                    oldProgramsIndex++;
                    newProgramsIndex++;
                } else if (isSameTitleAndOverlap(oldProgram, newProgram)) {
                    if (!oldProgram.equals(oldProgram)) {
                        // Partial match. Update the old program with the new one.
                        // NOTE: Use 'update' in this case instead of 'insert' and 'delete'. There
                        // could be application specific settings which belong to the old program.
                        ops.add(ContentProviderOperation.newUpdate(
                                TvContract.buildProgramUri(oldProgram.getId()))
                                .withValues(toContentValues(newProgram))
                                .build());
                    }
                    oldProgramsIndex++;
                    newProgramsIndex++;
                } else if (oldProgram.getEndTimeUtcMillis()
                        < newProgram.getEndTimeUtcMillis()) {
                    // No match. Remove the old program first to see if the next program in
                    // {@code oldPrograms} partially matches the new program.
                    ops.add(ContentProviderOperation.newDelete(
                            TvContract.buildProgramUri(oldProgram.getId()))
                            .build());
                    oldProgramsIndex++;
                } else {
                    // No match. The new program does not match any of the old programs. Insert
                    // it as a new program.
                    addNewProgram = true;
                    newProgramsIndex++;
                }
            } else {
                // No old programs. Just insert new programs.
                addNewProgram = true;
                newProgramsIndex++;
            }
            if (addNewProgram) {
                ops.add(ContentProviderOperation
                        .newInsert(TvContract.Programs.CONTENT_URI)
                        .withValues(toContentValues(newProgram))
                        .build());
            }
            // Throttle the batch operation not to cause TransactionTooLargeException.
            if (ops.size() > BATCH_OPERATION_COUNT || newProgramsIndex >= fetchedProgramsCount) {
                try {
                    if (DEBUG) {
                        int size = ops.size();
                        Log.d(TAG, "Running " + size + " operations for channel " + channelId);
                        for (int i = 0; i < size; ++i) {
                            Log.d(TAG, "Operation(" + i + "): " + ops.get(i));
                        }
                    }
                    mContext.getContentResolver().applyBatch(TvContract.AUTHORITY, ops);
                } catch (RemoteException | OperationApplicationException e) {
                    Log.e(TAG, "Failed to insert programs.", e);
                    return;
                }
                ops.clear();
            }
        }
        if (DEBUG) {
            Log.d(TAG, "Fetched " + fetchedProgramsCount + " programs for channel " + channelId);
        }
    }

    private List<Program> queryPrograms(ContentResolver contentResolver, long channelId,
            long startTimeMs, long endTimeMs) {
        try (Cursor c = mContext.getContentResolver().query(
                TvContract.buildProgramsUriForChannel(channelId, startTimeMs, endTimeMs),
                Program.PROJECTION, null, null, Programs.COLUMN_START_TIME_UTC_MILLIS)) {
            if (c == null) {
                return Collections.EMPTY_LIST;
            }
            ArrayList<Program> programs = new ArrayList<>();
            while (c.moveToNext()) {
                programs.add(Program.fromCursor(c));
            }
            return programs;
        }
    }

    /**
     * Returns {@code true} if the {@code oldProgram} program needs to be updated with the
     * {@code newProgram} program.
     */
    private boolean isSameTitleAndOverlap(Program oldProgram, Program newProgram) {
        // NOTE: Here, we update the old program if it has the same title and overlaps with the
        // new program. The test logic is just an example and you can modify this. E.g. check
        // whether the both programs have the same program ID if your EPG supports any ID for
        // the programs.
        return Objects.equals(oldProgram.getTitle(), newProgram.getTitle())
                && oldProgram.getStartTimeUtcMillis() <= newProgram.getEndTimeUtcMillis()
                && newProgram.getStartTimeUtcMillis() <= oldProgram.getEndTimeUtcMillis();
    }

    private static ContentValues toContentValues(Program program) {
        ContentValues values = new ContentValues();
        values.put(TvContract.Programs.COLUMN_CHANNEL_ID, program.getChannelId());
        putValue(values, TvContract.Programs.COLUMN_TITLE, program.getTitle());
        putValue(values, TvContract.Programs.COLUMN_EPISODE_TITLE, program.getEpisodeTitle());
        putValue(values, TvContract.Programs.COLUMN_SEASON_NUMBER, program.getSeasonNumber());
        putValue(values, TvContract.Programs.COLUMN_EPISODE_NUMBER, program.getEpisodeNumber());
        putValue(values, TvContract.Programs.COLUMN_SHORT_DESCRIPTION, program.getDescription());
        putValue(values, TvContract.Programs.COLUMN_POSTER_ART_URI, program.getPosterArtUri());
        values.put(TvContract.Programs.COLUMN_START_TIME_UTC_MILLIS,
                program.getStartTimeUtcMillis());
        values.put(TvContract.Programs.COLUMN_END_TIME_UTC_MILLIS, program.getEndTimeUtcMillis());
        return values;
    }

    private static void putValue(ContentValues contentValues, String key, String value) {
        if (TextUtils.isEmpty(value)) {
            contentValues.putNull(key);
        } else {
            contentValues.put(key, value);
        }
    }

    private static class EpgFetcherHandler extends WeakHandler<EpgFetcher> {
        public EpgFetcherHandler (@NonNull Looper looper, EpgFetcher ref) {
            super(looper, ref);
        }

        @Override
        public void handleMessage(Message msg, @NonNull EpgFetcher epgFetcher) {
            switch (msg.what) {
                case MSG_FETCH_EPG:
                    epgFetcher.onFetchEpg();
                    break;
                default:
                    super.handleMessage(msg);
                    break;
            }
        }
    }
}
