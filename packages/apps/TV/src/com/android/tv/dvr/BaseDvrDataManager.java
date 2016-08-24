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

import android.annotation.TargetApi;
import android.content.Context;
import android.os.Build;
import android.support.annotation.MainThread;
import android.util.ArraySet;
import android.util.Log;

import com.android.tv.common.SoftPreconditions;
import com.android.tv.common.feature.CommonFeatures;
import com.android.tv.common.recording.RecordedProgram;
import com.android.tv.util.Clock;

import java.util.ArrayList;
import java.util.List;
import java.util.Set;

/**
 * Base implementation of @{link DataManagerInternal}.
 */
@MainThread
@TargetApi(Build.VERSION_CODES.N)
public abstract class BaseDvrDataManager implements WritableDvrDataManager {
    private final static String TAG = "BaseDvrDataManager";
    private final static boolean DEBUG = false;
    protected final Clock mClock;

    private final Set<ScheduledRecordingListener> mScheduledRecordingListeners = new ArraySet<>();
    private final Set<RecordedProgramListener> mRecordedProgramListeners = new ArraySet<>();

    BaseDvrDataManager(Context context, Clock clock) {
        SoftPreconditions.checkFeatureEnabled(context, CommonFeatures.DVR, TAG);
        mClock = clock;
    }

    @Override
    public final void addScheduledRecordingListener(ScheduledRecordingListener listener) {
        mScheduledRecordingListeners.add(listener);
    }

    @Override
    public final void removeScheduledRecordingListener(ScheduledRecordingListener listener) {
        mScheduledRecordingListeners.remove(listener);
    }

    @Override
    public final void addRecordedProgramListener(RecordedProgramListener listener) {
        mRecordedProgramListeners.add(listener);
    }

    @Override
    public final void removeRecordedProgramListener(RecordedProgramListener listener) {
        mRecordedProgramListeners.remove(listener);
    }

    /**
     * Calls {@link RecordedProgramListener#onRecordedProgramAdded(RecordedProgram)}
     * for each listener.
     */
    protected final void notifyRecordedProgramAdded(RecordedProgram recordedProgram) {
        for (RecordedProgramListener l : mRecordedProgramListeners) {
            if (DEBUG) Log.d(TAG, "notify " + l + "added " + recordedProgram);
            l.onRecordedProgramAdded(recordedProgram);
        }
    }

    /**
     * Calls {@link RecordedProgramListener#onRecordedProgramChanged(RecordedProgram)}
     * for each listener.
     */
    protected final void notifyRecordedProgramChanged(RecordedProgram recordedProgram) {
        for (RecordedProgramListener l : mRecordedProgramListeners) {
            if (DEBUG) Log.d(TAG, "notify " + l + "changed " + recordedProgram);
            l.onRecordedProgramChanged(recordedProgram);
        }
    }

    /**
     * Calls {@link RecordedProgramListener#onRecordedProgramRemoved(RecordedProgram)}
     * for each  listener.
     */
    protected final void notifyRecordedProgramRemoved(RecordedProgram recordedProgram) {
        for (RecordedProgramListener l : mRecordedProgramListeners) {
            if (DEBUG) Log.d(TAG, "notify " + l + "removed " + recordedProgram);
            l.onRecordedProgramRemoved(recordedProgram);
        }
    }

    /**
     * Calls {@link ScheduledRecordingListener#onScheduledRecordingAdded(ScheduledRecording)}
     * for each listener.
     */
    protected final void notifyScheduledRecordingAdded(ScheduledRecording scheduledRecording) {
        for (ScheduledRecordingListener l : mScheduledRecordingListeners) {
            if (DEBUG) Log.d(TAG, "notify " + l + "added  " + scheduledRecording);
            l.onScheduledRecordingAdded(scheduledRecording);
        }
    }

    /**
     * Calls {@link ScheduledRecordingListener#onScheduledRecordingRemoved(ScheduledRecording)}
     * for each listener.
     */
    protected final void notifyScheduledRecordingRemoved(ScheduledRecording scheduledRecording) {
        for (ScheduledRecordingListener l : mScheduledRecordingListeners) {
            if (DEBUG) {
                Log.d(TAG, "notify " + l + "removed " + scheduledRecording);
            }
            l.onScheduledRecordingRemoved(scheduledRecording);
        }
    }

    /**
     * Calls
     * {@link ScheduledRecordingListener#onScheduledRecordingStatusChanged(ScheduledRecording)}
     * for each listener.
     */
    protected final void notifyScheduledRecordingStatusChanged(
            ScheduledRecording scheduledRecording) {
        for (ScheduledRecordingListener l : mScheduledRecordingListeners) {
            if (DEBUG) Log.d(TAG, "notify " + l + "changed " + scheduledRecording);
            l.onScheduledRecordingStatusChanged(scheduledRecording);
        }
    }

    /**
     * Returns a new list with only {@link ScheduledRecording} with a {@link
     * ScheduledRecording#getEndTimeMs() endTime} after now.
     */
    private List<ScheduledRecording> filterEndTimeIsPast(List<ScheduledRecording> originals) {
        List<ScheduledRecording> results = new ArrayList<>(originals.size());
        for (ScheduledRecording r : originals) {
            if (r.getEndTimeMs() > mClock.currentTimeMillis()) {
                results.add(r);
            }
        }
        return results;
    }

    @Override
    public List<ScheduledRecording> getStartedRecordings() {
        return filterEndTimeIsPast(
                getRecordingsWithState(ScheduledRecording.STATE_RECORDING_IN_PROGRESS));
    }

    @Override
    public List<ScheduledRecording> getNonStartedScheduledRecordings() {
        return filterEndTimeIsPast(
                getRecordingsWithState(ScheduledRecording.STATE_RECORDING_NOT_STARTED));
    }

    protected abstract List<ScheduledRecording> getRecordingsWithState(int state);
}
