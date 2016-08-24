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

import android.content.Context;
import android.support.annotation.MainThread;
import android.support.annotation.NonNull;
import android.support.annotation.Nullable;
import android.support.annotation.VisibleForTesting;
import android.util.Range;

import com.android.tv.common.SoftPreconditions;
import com.android.tv.common.recording.RecordedProgram;
import com.android.tv.util.Clock;

import java.util.ArrayList;
import java.util.Collections;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.concurrent.atomic.AtomicLong;

/**
 * A DVR Data manager that stores values in memory suitable for testing.
 */
@VisibleForTesting // TODO(DVR): move to testing dir.
@MainThread
public final class DvrDataManagerInMemoryImpl extends BaseDvrDataManager {
    private final static String TAG = "DvrDataManagerInMemory";
    private final AtomicLong mNextId = new AtomicLong(1);
    private final Map<Long, ScheduledRecording> mScheduledRecordings = new HashMap<>();
    private final Map<Long, RecordedProgram> mRecordedPrograms = new HashMap<>();
    private final List<SeasonRecording> mSeasonSchedule = new ArrayList<>();

    public DvrDataManagerInMemoryImpl(Context context, Clock clock) {
        super(context, clock);
    }

    @Override
    public boolean isInitialized() {
        return true;
    }

    private List<ScheduledRecording> getScheduledRecordingsPrograms() {
        return new ArrayList(mScheduledRecordings.values());
    }

    @Override
    public List<RecordedProgram> getRecordedPrograms() {
        return new ArrayList<>(mRecordedPrograms.values());
    }

    @Override
    public List<ScheduledRecording> getAllScheduledRecordings() {
        return new ArrayList<>(mScheduledRecordings.values());
    }

    public List<SeasonRecording> getSeasonRecordings() {
        return mSeasonSchedule;
    }

    @Override
    public long getNextScheduledStartTimeAfter(long startTime) {

        List<ScheduledRecording> temp =  getNonStartedScheduledRecordings();
        Collections.sort(temp, ScheduledRecording.START_TIME_COMPARATOR);
        for (ScheduledRecording r : temp) {
            if (r.getStartTimeMs() > startTime) {
                return r.getStartTimeMs();
            }
        }
        return DvrDataManager.NEXT_START_TIME_NOT_FOUND;
    }

    @Override
    public List<ScheduledRecording> getRecordingsThatOverlapWith(Range<Long> period) {
        List<ScheduledRecording> temp = getScheduledRecordingsPrograms();
        List<ScheduledRecording> result = new ArrayList<>();
        for (ScheduledRecording r : temp) {
            if (r.isOverLapping(period)) {
                result.add(r);
            }
        }
        return result;
    }

    /**
     * Add a new scheduled recording.
     */
    @Override
    public void addScheduledRecording(ScheduledRecording scheduledRecording) {
        addScheduledRecordingInternal(scheduledRecording);
    }


    public void addRecordedProgram(RecordedProgram recordedProgram) {
        addRecordedProgramInternal(recordedProgram);
    }

    public void updateRecordedProgram(RecordedProgram r) {
        long id = r.getId();
        if (mRecordedPrograms.containsKey(id)) {
            mRecordedPrograms.put(id, r);
            notifyRecordedProgramChanged(r);
        } else {
            throw new IllegalArgumentException("Recording not found:" + r);
        }
    }

    public void removeRecordedProgram(RecordedProgram scheduledRecording) {
        mRecordedPrograms.remove(scheduledRecording.getId());
        notifyRecordedProgramRemoved(scheduledRecording);
    }


    public ScheduledRecording addScheduledRecordingInternal(ScheduledRecording scheduledRecording) {
        SoftPreconditions
                .checkState(scheduledRecording.getId() == ScheduledRecording.ID_NOT_SET, TAG,
                        "expected id of " + ScheduledRecording.ID_NOT_SET + " but was "
                                + scheduledRecording);
        scheduledRecording = ScheduledRecording.buildFrom(scheduledRecording)
                .setId(mNextId.incrementAndGet())
                .build();
        mScheduledRecordings.put(scheduledRecording.getId(), scheduledRecording);
        notifyScheduledRecordingAdded(scheduledRecording);
        return scheduledRecording;
    }

    public RecordedProgram addRecordedProgramInternal(RecordedProgram recordedProgram) {
        SoftPreconditions.checkState(recordedProgram.getId() == RecordedProgram.ID_NOT_SET, TAG,
                "expected id of " + RecordedProgram.ID_NOT_SET + " but was " + recordedProgram);
        recordedProgram = RecordedProgram.buildFrom(recordedProgram)
                .setId(mNextId.incrementAndGet())
                .build();
        mRecordedPrograms.put(recordedProgram.getId(), recordedProgram);
        notifyRecordedProgramAdded(recordedProgram);
        return recordedProgram;
    }

    @Override
    public void addSeasonRecording(SeasonRecording seasonRecording) {
        mSeasonSchedule.add(seasonRecording);
    }

    @Override
    public void removeScheduledRecording(ScheduledRecording scheduledRecording) {
        mScheduledRecordings.remove(scheduledRecording.getId());
        notifyScheduledRecordingRemoved(scheduledRecording);
    }

    @Override
    public void removeSeasonSchedule(SeasonRecording seasonSchedule) {
        mSeasonSchedule.remove(seasonSchedule);
    }

    @Override
    public void updateScheduledRecording(ScheduledRecording r) {
        long id = r.getId();
        if (mScheduledRecordings.containsKey(id)) {
            mScheduledRecordings.put(id, r);
            notifyScheduledRecordingStatusChanged(r);
        } else {
            throw new IllegalArgumentException("Recording not found:" + r);
        }
    }

    @Nullable
    @Override
    public ScheduledRecording getScheduledRecording(long id) {
        return mScheduledRecordings.get(id);
    }

    @Nullable
    @Override
    public ScheduledRecording getScheduledRecordingForProgramId(long programId) {
        for (ScheduledRecording r : mScheduledRecordings.values()) {
            if (r.getProgramId() == programId) {
                    return r;
            }
        }
        return null;
    }

    @Nullable
    @Override
    public RecordedProgram getRecordedProgram(long recordingId) {
        return mRecordedPrograms.get(recordingId);
    }

    @Override
    @NonNull
    protected List<ScheduledRecording> getRecordingsWithState(int state) {
        ArrayList<ScheduledRecording> result = new ArrayList<>();
        for (ScheduledRecording r : mScheduledRecordings.values()) {
            if(r.getState() == state){
                result.add(r);
            }
        }
        return result;
    }
}
