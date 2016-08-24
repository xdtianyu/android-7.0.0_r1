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
 * limitations under the License
 */

package com.android.tv.dvr;

import android.support.annotation.MainThread;
import android.support.annotation.Nullable;
import android.util.Range;

import com.android.tv.common.recording.RecordedProgram;

import java.util.List;

/**
 * Read only data manager.
 */
@MainThread
public interface DvrDataManager {
    long NEXT_START_TIME_NOT_FOUND = -1;

    boolean isInitialized();

    /**
     * Returns past recordings.
     */
    List<RecordedProgram> getRecordedPrograms();

    /**
     * Returns all {@link ScheduledRecording} regardless of state.
     */
    List<ScheduledRecording> getAllScheduledRecordings();

    /**
     * Returns started recordings that expired.
     */
    List<ScheduledRecording> getStartedRecordings();

    /**
     * Returns scheduled but not started recordings that have not expired.
     */
    List<ScheduledRecording> getNonStartedScheduledRecordings();

    /**
     * Returns season recordings.
     */
    List<SeasonRecording> getSeasonRecordings();

    /**
     * Returns the next start time after {@code time} or {@link #NEXT_START_TIME_NOT_FOUND}
     * if none is found.
     *
     * @param time time milliseconds
     */
    long getNextScheduledStartTimeAfter(long time);

    /**
     * Returns a list of all Recordings with a overlap with the given time period inclusive.
     *
     * <p> A recording overlaps with a period when
     * {@code recording.getStartTime() <= period.getUpper() &&
     * recording.getEndTime() >= period.getLower()}.
     *
     * @param period a time period in milliseconds.
     */
    List<ScheduledRecording> getRecordingsThatOverlapWith(Range<Long> period);

    /**
     * Add a {@link ScheduledRecordingListener}.
     */
    void addScheduledRecordingListener(ScheduledRecordingListener scheduledRecordingListener);

    /**
     * Remove a {@link ScheduledRecordingListener}.
     */
    void removeScheduledRecordingListener(ScheduledRecordingListener scheduledRecordingListener);

    /**
     * Add a {@link RecordedProgramListener}.
     */
    void addRecordedProgramListener(RecordedProgramListener listener);

    /**
     * Remove a {@link RecordedProgramListener}.
     */
    void removeRecordedProgramListener(RecordedProgramListener listener);

    /**
     * Returns the scheduled recording program with the given recordingId or null if is not found.
     */
    @Nullable
    ScheduledRecording getScheduledRecording(long recordingId);


    /**
     * Returns the scheduled recording program with the given programId or null if is not found.
     */
    @Nullable
    ScheduledRecording getScheduledRecordingForProgramId(long programId);

    /**
     * Returns the recorded program with the given recordingId or null if is not found.
     */
    @Nullable
    RecordedProgram getRecordedProgram(long recordingId);

    interface ScheduledRecordingListener {
        void onScheduledRecordingAdded(ScheduledRecording scheduledRecording);

        void onScheduledRecordingRemoved(ScheduledRecording scheduledRecording);

        void onScheduledRecordingStatusChanged(ScheduledRecording scheduledRecording);
    }

    interface RecordedProgramListener {
        void onRecordedProgramAdded(RecordedProgram recordedProgram);

        void onRecordedProgramChanged(RecordedProgram recordedProgram);

        void onRecordedProgramRemoved(RecordedProgram recordedProgram);
    }
}
