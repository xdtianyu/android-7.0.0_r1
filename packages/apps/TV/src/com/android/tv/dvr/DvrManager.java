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

package com.android.tv.dvr;

import android.content.ContentResolver;
import android.content.Context;
import android.media.tv.TvInputInfo;
import android.os.Handler;
import android.support.annotation.MainThread;
import android.support.annotation.NonNull;
import android.support.annotation.WorkerThread;
import android.util.Log;
import android.util.Range;
import android.widget.Toast;

import com.android.tv.ApplicationSingletons;
import com.android.tv.TvApplication;
import com.android.tv.common.SoftPreconditions;
import com.android.tv.common.feature.CommonFeatures;
import com.android.tv.common.recording.RecordedProgram;
import com.android.tv.data.Channel;
import com.android.tv.data.ChannelDataManager;
import com.android.tv.data.Program;
import com.android.tv.util.AsyncDbTask;
import com.android.tv.util.Utils;

import java.util.Collections;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.Map.Entry;

/**
 * DVR manager class to add and remove recordings. UI can modify recording list through this class,
 * instead of modifying them directly through {@link DvrDataManager}.
 */
@MainThread
public class DvrManager {
    private final static String TAG = "DvrManager";
    private final WritableDvrDataManager mDataManager;
    private final ChannelDataManager mChannelDataManager;
    private final DvrSessionManager mDvrSessionManager;
    // @GuardedBy("mListener")
    private final Map<Listener, Handler> mListener = new HashMap<>();
    private final Context mAppContext;

    public DvrManager(Context context) {
        SoftPreconditions.checkFeatureEnabled(context, CommonFeatures.DVR, TAG);
        ApplicationSingletons appSingletons = TvApplication.getSingletons(context);
        mDataManager = (WritableDvrDataManager) appSingletons.getDvrDataManager();
        mAppContext = context.getApplicationContext();
        mChannelDataManager = appSingletons.getChannelDataManager();
        mDvrSessionManager = appSingletons.getDvrSessionManger();
    }

    /**
     * Schedules a recording for {@code program} instead of the list of recording that conflict.
     * @param program the program to record
     * @param recordingsToOverride the possible empty list of recordings that will not be recorded
     */
    public void addSchedule(Program program, List<ScheduledRecording> recordingsToOverride) {
        Log.i(TAG,
                "Adding scheduled recording of " + program + " instead of " + recordingsToOverride);
        Collections.sort(recordingsToOverride, ScheduledRecording.PRIORITY_COMPARATOR);
        Channel c = mChannelDataManager.getChannel(program.getChannelId());
        long priority = recordingsToOverride.isEmpty() ? Long.MAX_VALUE
                : recordingsToOverride.get(0).getPriority() - 1;
        ScheduledRecording r = ScheduledRecording.builder(program)
                .setPriority(priority)
                .setChannelId(c.getId())
                .build();
        mDataManager.addScheduledRecording(r);
    }

    /**
     * Adds a recording schedule with a time range.
     */
    public void addSchedule(Channel channel, long startTime, long endTime) {
        Log.i(TAG, "Adding scheduled recording of channel" + channel + " starting at " +
                Utils.toTimeString(startTime) + " and ending at " + Utils.toTimeString(endTime));
        //TODO: handle error cases
        ScheduledRecording r = ScheduledRecording.builder(startTime, endTime)
                .setChannelId(channel.getId())
                .build();
        mDataManager.addScheduledRecording(r);
    }

    /**
     * Adds a season recording schedule based on {@code program}.
     */
    public void addSeasonSchedule(Program program) {
        Log.i(TAG, "Adding season recording of " + program);
        // TODO: implement
    }

    /**
     * Stops the currently recorded program
     */
    public void stopRecording(final ScheduledRecording recording) {
        synchronized (mListener) {
            for (final Entry<Listener, Handler> entry : mListener.entrySet()) {
                entry.getValue().post(new Runnable() {
                    @Override
                    public void run() {
                        entry.getKey().onStopRecordingRequested(recording);
                    }
                });
            }
        }
    }

    /**
     * Removes a scheduled recording or an existing recording.
     */
    public void removeScheduledRecording(ScheduledRecording scheduledRecording) {
        Log.i(TAG, "Removing " + scheduledRecording);
        mDataManager.removeScheduledRecording(scheduledRecording);
    }

    public void removeRecordedProgram(final RecordedProgram recordedProgram) {
        // TODO(dvr): implement
        Log.i(TAG, "To delete " + recordedProgram
                + "\nyou should manually delete video data at"
                + "\nadb shell rm -rf " + recordedProgram.getDataUri()
        );
        Toast.makeText(mAppContext, "Deleting recorded programs is not fully implemented yet",
                Toast.LENGTH_SHORT).show();
        new AsyncDbTask<Void, Void, Void>() {
            @Override
            protected Void doInBackground(Void... params) {
                ContentResolver resolver = mAppContext.getContentResolver();
                resolver.delete(recordedProgram.getUri(), null, null);
                return null;
            }
        }.execute();
    }

    /**
     * Returns priority ordered list of all scheduled recording that will not be recorded if
     * this program is.
     *
     * <p>Any empty list means there is no conflicts.  If there is conflict the program must be
     * scheduled to record with a Priority lower than the first Recording in the list returned.
     */
    public List<ScheduledRecording> getScheduledRecordingsThatConflict(Program program) {
        //TODO(DVR): move to scheduler.
        //TODO(DVR): deal with more than one DvrInputService
        List<ScheduledRecording> overLap = mDataManager.getRecordingsThatOverlapWith(getPeriod(program));
        if (!overLap.isEmpty()) {
            // TODO(DVR): ignore shows that already won't record.
            Channel channel = mChannelDataManager.getChannel(program.getChannelId());
            if (channel != null) {
                TvInputInfo info = mDvrSessionManager.getTvInputInfo(channel.getInputId());
                if (info == null) {
                    Log.w(TAG,
                            "Could not find a recording TvInputInfo for " + channel.getInputId());
                    return overLap;
                }
                int remove = Math.max(0, info.getTunerCount() - 1);
                if (remove >= overLap.size()) {
                    return Collections.EMPTY_LIST;
                }
                overLap = overLap.subList(remove, overLap.size() - 1);
            }
        }
        return overLap;
    }

    @NonNull
    private static Range getPeriod(Program program) {
        return new Range(program.getStartTimeUtcMillis(), program.getEndTimeUtcMillis());
    }

    /**
     * Checks whether {@code channel} can be tuned without any conflict with existing recordings
     * in progress. If there is any conflict, {@code outConflictRecordings} will be filled.
     */
    public boolean canTuneTo(Channel channel, List<ScheduledRecording> outConflictScheduledRecordings) {
        // TODO: implement
        return true;
    }

    /**
     * Returns true is the inputId supports recording.
     */
    public boolean canRecord(String inputId) {
        TvInputInfo info = mDvrSessionManager.getTvInputInfo(inputId);
        return info != null && info.getTunerCount() > 0;
    }

    @WorkerThread
    void addListener(Listener listener, @NonNull Handler handler) {
        SoftPreconditions.checkNotNull(handler);
        synchronized (mListener) {
            mListener.put(listener, handler);
        }
    }

    @WorkerThread
    void removeListener(Listener listener) {
        synchronized (mListener) {
            mListener.remove(listener);
        }
    }

    /**
     * Listener internally used inside dvr package.
     */
    interface Listener {
        void onStopRecordingRequested(ScheduledRecording scheduledRecording);
    }
}
