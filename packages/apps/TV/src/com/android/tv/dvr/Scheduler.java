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

import android.app.AlarmManager;
import android.app.PendingIntent;
import android.content.Context;
import android.content.Intent;
import android.os.Handler;
import android.os.Looper;
import android.os.Message;
import android.support.annotation.VisibleForTesting;
import android.util.Log;
import android.util.LongSparseArray;
import android.util.Range;

import com.android.tv.data.Channel;
import com.android.tv.data.ChannelDataManager;
import com.android.tv.util.Clock;

import java.util.List;
import java.util.concurrent.TimeUnit;

/**
 * The core class to manage schedule and run actual recording.
 */
@VisibleForTesting
public class Scheduler implements DvrDataManager.ScheduledRecordingListener {
    private static final String TAG = "Scheduler";
    private static final boolean DEBUG = false;

    private final static long SOON_DURATION_IN_MS = TimeUnit.MINUTES.toMillis(5);
    @VisibleForTesting final static long MS_TO_WAKE_BEFORE_START = TimeUnit.MINUTES.toMillis(1);

    /**
     * Wraps a {@link RecordingTask} removing it from {@link #mPendingRecordings} when it is done.
     */
    public final class HandlerWrapper extends Handler {
        public static final int MESSAGE_REMOVE = 999;
        private final long mId;

        HandlerWrapper(Looper looper, ScheduledRecording scheduledRecording, RecordingTask recordingTask) {
            super(looper, recordingTask);
            mId = scheduledRecording.getId();
        }

        @Override
        public void handleMessage(Message msg) {
            // The RecordingTask gets a chance first.
            // It must return false to pass this message to here.
            if (msg.what == MESSAGE_REMOVE) {
                if (DEBUG)  Log.d(TAG, "done " + mId);
                mPendingRecordings.remove(mId);
            }
            removeCallbacksAndMessages(null);
            super.handleMessage(msg);
        }
    }

    private final LongSparseArray<HandlerWrapper> mPendingRecordings = new LongSparseArray<>();
    private final Looper mLooper;
    private final DvrSessionManager mSessionManager;
    private final WritableDvrDataManager mDataManager;
    private final DvrManager mDvrManager;
    private final ChannelDataManager mChannelDataManager;
    private final Context mContext;
    private final Clock mClock;
    private final AlarmManager mAlarmManager;

    public Scheduler(Looper looper, DvrManager dvrManager, DvrSessionManager sessionManager,
            WritableDvrDataManager dataManager, ChannelDataManager channelDataManager,
            Context context, Clock clock,
            AlarmManager alarmManager) {
        mLooper = looper;
        mDvrManager = dvrManager;
        mSessionManager = sessionManager;
        mDataManager = dataManager;
        mChannelDataManager = channelDataManager;
        mContext = context;
        mClock = clock;
        mAlarmManager = alarmManager;
    }

    private void updatePendingRecordings() {
        List<ScheduledRecording> scheduledRecordings = mDataManager.getRecordingsThatOverlapWith(
                new Range(mClock.currentTimeMillis(),
                        mClock.currentTimeMillis() + SOON_DURATION_IN_MS));
        // TODO(DVR): handle removing and updating exiting recordings.
        for (ScheduledRecording r : scheduledRecordings) {
            scheduleRecordingSoon(r);
        }
    }

    /**
     * Start recording that will happen soon, and set the next alarm time.
     */
    public void update() {
        if (DEBUG) Log.d(TAG, "update");
        updatePendingRecordings();
        updateNextAlarm();
    }

    @Override
    public void onScheduledRecordingAdded(ScheduledRecording scheduledRecording) {
        if (DEBUG) Log.d(TAG, "added " + scheduledRecording);
        if (startsWithin(scheduledRecording, SOON_DURATION_IN_MS)) {
            scheduleRecordingSoon(scheduledRecording);
        } else {
            updateNextAlarm();
        }
    }

    @Override
    public void onScheduledRecordingRemoved(ScheduledRecording scheduledRecording) {
        long id = scheduledRecording.getId();
        HandlerWrapper wrapper = mPendingRecordings.get(id);
        if (wrapper != null) {
            wrapper.removeCallbacksAndMessages(null);
            mPendingRecordings.remove(id);
        } else {
            updateNextAlarm();
        }
    }

    @Override
    public void onScheduledRecordingStatusChanged(ScheduledRecording scheduledRecording) {
        //TODO(DVR): implement
    }

    private void scheduleRecordingSoon(ScheduledRecording scheduledRecording) {
        Channel channel = mChannelDataManager.getChannel(scheduledRecording.getChannelId());
        RecordingTask recordingTask = new RecordingTask(scheduledRecording, channel, mDvrManager,
                mSessionManager, mDataManager, mClock);
        HandlerWrapper handlerWrapper = new HandlerWrapper(mLooper, scheduledRecording,
                recordingTask);
        recordingTask.setHandler(handlerWrapper);
        mPendingRecordings.put(scheduledRecording.getId(), handlerWrapper);
        handlerWrapper.sendEmptyMessage(RecordingTask.MESSAGE_INIT);
    }

    private void updateNextAlarm() {
        long lastStartTimePending = getLastStartTimePending();
        long nextStartTime = mDataManager.getNextScheduledStartTimeAfter(lastStartTimePending);
        if (nextStartTime != DvrDataManager.NEXT_START_TIME_NOT_FOUND) {
            long wakeAt = nextStartTime - MS_TO_WAKE_BEFORE_START;
            if (DEBUG) Log.d(TAG, "Set alarm to record at " + wakeAt);
            Intent intent = new Intent(mContext, DvrStartRecordingReceiver.class);
            PendingIntent alarmIntent = PendingIntent.getBroadcast(mContext, 0, intent, 0);
            //This will cancel the previous alarm.
            mAlarmManager.set(AlarmManager.RTC_WAKEUP, wakeAt, alarmIntent);
        } else {
            if (DEBUG) Log.d(TAG, "No future recording, alarm not set");
        }
    }

    private long getLastStartTimePending() {
        // TODO(DVR): implement
        return mClock.currentTimeMillis();
    }

    @VisibleForTesting
    boolean startsWithin(ScheduledRecording scheduledRecording, long durationInMs) {
        return mClock.currentTimeMillis() >= scheduledRecording.getStartTimeMs() - durationInMs;
    }
}
