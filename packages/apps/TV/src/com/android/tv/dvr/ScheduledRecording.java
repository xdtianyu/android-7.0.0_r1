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

import android.content.ContentValues;
import android.database.Cursor;
import android.support.annotation.IntDef;
import android.support.annotation.VisibleForTesting;
import android.util.Range;

import com.android.tv.common.SoftPreconditions;
import com.android.tv.data.Channel;
import com.android.tv.data.Program;
import com.android.tv.dvr.provider.DvrContract;
import com.android.tv.util.Utils;

import java.lang.annotation.Retention;
import java.lang.annotation.RetentionPolicy;
import java.util.Comparator;

/**
 * A data class for one recording contents.
 */
@VisibleForTesting
public final class ScheduledRecording {
    private static final String TAG = "Recording";

    public static final String RECORDING_ID_EXTRA = "extra.dvr.recording.id";  //TODO(DVR) move
    public static final String PARAM_INPUT_ID = "input_id";

    public static final long ID_NOT_SET = -1;

    public static final Comparator<ScheduledRecording> START_TIME_COMPARATOR = new Comparator<ScheduledRecording>() {
        @Override
        public int compare(ScheduledRecording lhs, ScheduledRecording rhs) {
            return Long.compare(lhs.mStartTimeMs, rhs.mStartTimeMs);
        }
    };

    public static final Comparator<ScheduledRecording> PRIORITY_COMPARATOR = new Comparator<ScheduledRecording>() {
        @Override
        public int compare(ScheduledRecording lhs, ScheduledRecording rhs) {
            int value = Long.compare(lhs.mPriority, rhs.mPriority);
            if (value == 0) {
                value = Long.compare(lhs.mId, rhs.mId);
            }
            return value;
        }
    };

    public static final Comparator<ScheduledRecording> START_TIME_THEN_PRIORITY_COMPARATOR
            = new Comparator<ScheduledRecording>() {
        @Override
        public int compare(ScheduledRecording lhs, ScheduledRecording rhs) {
            int value = START_TIME_COMPARATOR.compare(lhs, rhs);
            if (value == 0) {
                value = PRIORITY_COMPARATOR.compare(lhs, rhs);
            }
            return value;
        }
    };

    public static Builder builder(Program p) {
        return new Builder()
                .setStartTime(p.getStartTimeUtcMillis()).setEndTime(p.getEndTimeUtcMillis())
                .setProgramId(p.getId())
                .setType(TYPE_PROGRAM);
    }

    public static Builder builder(long startTime, long endTime) {
        return new Builder()
                .setStartTime(startTime)
                .setEndTime(endTime)
                .setType(TYPE_TIMED);
    }

    public static final class Builder {
        private long mId = ID_NOT_SET;
        private long mPriority = Long.MAX_VALUE;
        private long mChannelId;
        private long mProgramId = ID_NOT_SET;
        private @RecordingType int mType;
        private long mStartTime;
        private long mEndTime;
        private @RecordingState int mState;
        private SeasonRecording mParentSeasonRecording;

        private Builder() { }

        public Builder setId(long id) {
            mId = id;
            return this;
        }

        public Builder setPriority(long priority) {
            mPriority = priority;
            return this;
        }

        public Builder setChannelId(long channelId) {
            mChannelId = channelId;
            return this;
        }

        public Builder setProgramId(long programId) {
            mProgramId = programId;
            return this;
        }

        private Builder setType(@RecordingType int type) {
            mType = type;
            return this;
        }

        public Builder setStartTime(long startTime) {
            mStartTime = startTime;
            return this;
        }

        public Builder setEndTime(long endTime) {
            mEndTime = endTime;
            return this;
        }

        public Builder setState(@RecordingState int state) {
            mState = state;
            return this;
        }

        public Builder setParentSeasonRecording(SeasonRecording parentSeasonRecording) {
            mParentSeasonRecording = parentSeasonRecording;
            return this;
        }

        public ScheduledRecording build() {
            return new ScheduledRecording(mId, mPriority, mChannelId, mProgramId, mType, mStartTime,
                    mEndTime, mState, mParentSeasonRecording);
        }
    }

    /**
     * Creates {@link Builder} object from the given original {@code Recording}.
     */
    public static Builder buildFrom(ScheduledRecording orig) {
        return new Builder()
                .setId(orig.mId).setChannelId(orig.mChannelId)
                .setEndTime(orig.mEndTimeMs).setParentSeasonRecording(orig.mParentSeasonRecording)
                .setProgramId(orig.mProgramId)
                .setStartTime(orig.mStartTimeMs).setState(orig.mState).setType(orig.mType);
    }

    @Retention(RetentionPolicy.SOURCE)
    @IntDef({STATE_RECORDING_NOT_STARTED, STATE_RECORDING_IN_PROGRESS,
        STATE_RECORDING_UNEXPECTEDLY_STOPPED, STATE_RECORDING_FINISHED, STATE_RECORDING_FAILED})
    public @interface RecordingState {}
    public static final int STATE_RECORDING_NOT_STARTED = 0;
    public static final int STATE_RECORDING_IN_PROGRESS = 1;
    @Deprecated // It is not used.
    public static final int STATE_RECORDING_UNEXPECTEDLY_STOPPED = 2;
    public static final int STATE_RECORDING_FINISHED = 3;
    public static final int STATE_RECORDING_FAILED = 4;

    @Retention(RetentionPolicy.SOURCE)
    @IntDef({TYPE_TIMED, TYPE_PROGRAM})
    public @interface RecordingType {}
    /**
     * Record with given time range.
     */
    static final int TYPE_TIMED = 1;
    /**
     * Record with a given program.
     */
    static final int TYPE_PROGRAM = 2;

    @RecordingType private final int mType;

    /**
     * Use this projection if you want to create {@link ScheduledRecording} object using {@link #fromCursor}.
     */
    public static final String[] PROJECTION = {
            // Columns must match what is read in Recording.fromCursor()
            DvrContract.Recordings._ID,
            DvrContract.Recordings.COLUMN_PRIORITY,
            DvrContract.Recordings.COLUMN_TYPE,
            DvrContract.Recordings.COLUMN_CHANNEL_ID,
            DvrContract.Recordings.COLUMN_PROGRAM_ID,
            DvrContract.Recordings.COLUMN_START_TIME_UTC_MILLIS,
            DvrContract.Recordings.COLUMN_END_TIME_UTC_MILLIS,
            DvrContract.Recordings.COLUMN_STATE};
    /**
     * Creates {@link ScheduledRecording} object from the given {@link Cursor}.
     */
    public static ScheduledRecording fromCursor(Cursor c) {
        int index = -1;
        return new Builder()
                .setId(c.getLong(++index))
                .setPriority(c.getLong(++index))
                .setType(recordingType(c.getString(++index)))
                .setChannelId(c.getLong(++index))
                .setProgramId(c.getLong(++index))
                .setStartTime(c.getLong(++index))
                .setEndTime(c.getLong(++index))
                .setState(recordingState(c.getString(++index)))
                .build();
    }

    public static ContentValues toContentValues(ScheduledRecording r) {
        ContentValues values = new ContentValues();
        values.put(DvrContract.Recordings.COLUMN_CHANNEL_ID, r.getChannelId());
        values.put(DvrContract.Recordings.COLUMN_PROGRAM_ID, r.getProgramId());
        values.put(DvrContract.Recordings.COLUMN_PRIORITY, r.getPriority());
        values.put(DvrContract.Recordings.COLUMN_START_TIME_UTC_MILLIS, r.getStartTimeMs());
        values.put(DvrContract.Recordings.COLUMN_END_TIME_UTC_MILLIS, r.getEndTimeMs());
        values.put(DvrContract.Recordings.COLUMN_STATE, r.getState());
        values.put(DvrContract.Recordings.COLUMN_TYPE, r.getType());
        return values;
    }

    /**
     * The ID internal to Live TV
     */
    private final long mId;

    /**
     * The priority of this recording.
     *
     * <p> The lowest number is recorded first. If there is a tie in priority then the lower id
     * wins.
     */
    private final long mPriority;


    private final long mChannelId;
    /**
     * Optional id of the associated program.
     *
     */
    private final long mProgramId;

    private final long mStartTimeMs;
    private final long mEndTimeMs;
    @RecordingState private final int mState;

    private final SeasonRecording mParentSeasonRecording;

    private ScheduledRecording(long id, long priority, long channelId, long programId,
            @RecordingType int type, long startTime, long endTime,
            @RecordingState int state, SeasonRecording parentSeasonRecording) {
        mId = id;
        mPriority = priority;
        mChannelId = channelId;
        mProgramId = programId;
        mType = type;
        mStartTimeMs = startTime;
        mEndTimeMs = endTime;
        mState = state;
        mParentSeasonRecording = parentSeasonRecording;
    }

    /**
     * Returns recording schedule type. The possible types are {@link #TYPE_PROGRAM} and
     * {@link #TYPE_TIMED}.
     */
    @RecordingType
    public int getType() {
        return mType;
    }

    /**
     * Returns recorded {@link Channel}.
     */
    public long getChannelId() {
        return mChannelId;
    }

    /**
     * Return the optional program id
     */
    public long getProgramId() {
        return mProgramId;
    }

    /**
     * Returns started time.
     */
    public long getStartTimeMs() {
        return mStartTimeMs;
    }

    /**
     * Returns ended time.
     */
    public long getEndTimeMs() {
        return mEndTimeMs;
    }

    /**
     * Returns duration.
     */
    public long getDuration() {
        return mEndTimeMs - mStartTimeMs;
    }

    /**
     * Returns the state. The possible states are {@link #STATE_RECORDING_FINISHED},
     * {@link #STATE_RECORDING_IN_PROGRESS} and {@link #STATE_RECORDING_UNEXPECTEDLY_STOPPED}.
     */
    @RecordingState public int getState() {
        return mState;
    }

    /**
     * Returns {@link SeasonRecording} including this schedule.
     */
    public SeasonRecording getParentSeasonRecording() {
        return mParentSeasonRecording;
    }

    public long getId() {
        return mId;
    }

    public long getPriority() {
        return mPriority;
    }

    /**
     * Converts a string to a @RecordingType int, defaulting to {@link #TYPE_TIMED}.
     */
    private static @RecordingType int recordingType(String type) {
        int t;
        try {
            t = Integer.valueOf(type);
        } catch (NullPointerException | NumberFormatException e) {
            SoftPreconditions.checkArgument(false, TAG, "Unknown recording type " + type);
            return TYPE_TIMED;
        }
        switch (t) {
            case TYPE_TIMED:
                return TYPE_TIMED;
            case TYPE_PROGRAM:
                return TYPE_PROGRAM;
            default:
                SoftPreconditions.checkArgument(false, TAG, "Unknown recording type " + type);
                return TYPE_TIMED;
        }
    }

    /**
     * Converts a string to a @RecordingState int, defaulting to
     * {@link #STATE_RECORDING_NOT_STARTED}.
     */
    private static @RecordingState int recordingState(String state) {
        int s;
        try {
            s = Integer.valueOf(state);
        } catch (NullPointerException | NumberFormatException e) {
            SoftPreconditions.checkArgument(false, TAG, "Unknown recording state" + state);
            return STATE_RECORDING_NOT_STARTED;
        }
        switch (s) {
            case STATE_RECORDING_NOT_STARTED:
                return STATE_RECORDING_NOT_STARTED;
            case STATE_RECORDING_IN_PROGRESS:
                return STATE_RECORDING_IN_PROGRESS;
            case STATE_RECORDING_FINISHED:
                return STATE_RECORDING_FINISHED;
            case STATE_RECORDING_UNEXPECTEDLY_STOPPED:
                return STATE_RECORDING_UNEXPECTEDLY_STOPPED;
            case STATE_RECORDING_FAILED:
                return STATE_RECORDING_FAILED;
            default:
                SoftPreconditions.checkArgument(false, TAG, "Unknown recording state" + state);
                return STATE_RECORDING_NOT_STARTED;
        }
    }

    /**
     * Checks if the {@code period} overlaps with the recording time.
     */
    public boolean isOverLapping(Range<Long> period) {
        return mStartTimeMs <= period.getUpper() && mEndTimeMs >= period.getLower();
    }

    @Override
    public String toString() {
        return "ScheduledRecording[" + mId
                + "]"
                + "(startTime=" + Utils.toIsoDateTimeString(mStartTimeMs)
                + ",endTime=" + Utils.toIsoDateTimeString(mEndTimeMs)
                + ",state=" + mState
                + ",priority=" + mPriority
                + ")";
    }
}
