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

package com.android.tv.dvr.ui;

import android.support.v17.leanback.widget.PresenterSelector;

import com.android.tv.dvr.DvrDataManager;
import com.android.tv.dvr.ScheduledRecording;

/**
 * Adapter for {@link ScheduledRecording} filtered by
 * {@link com.android.tv.dvr.ScheduledRecording.RecordingState}.
 */
final class ScheduledRecordingsAdapter extends SortedArrayAdapter<ScheduledRecording>
        implements DvrDataManager.ScheduledRecordingListener {
    private final int mState;
    private final DvrDataManager mDataManager;

    ScheduledRecordingsAdapter(DvrDataManager dataManager, int state,
            PresenterSelector presenterSelector) {
        super(presenterSelector, ScheduledRecording.START_TIME_THEN_PRIORITY_COMPARATOR);
        mDataManager = dataManager;
        mState = state;
    }

    public void start() {
        clear();
        switch (mState) {
            case ScheduledRecording.STATE_RECORDING_NOT_STARTED:
                addAll(mDataManager.getNonStartedScheduledRecordings());
                break;
            case ScheduledRecording.STATE_RECORDING_IN_PROGRESS:
                addAll(mDataManager.getStartedRecordings());
                break;
            default:
                throw new IllegalStateException("Unknown recording state " + mState);

        }
        mDataManager.addScheduledRecordingListener(this);
    }

    public void stop() {
        mDataManager.removeScheduledRecordingListener(this);
    }

    @Override
    long getId(ScheduledRecording item) {
        return item.getId();
    }

    @Override  //DvrDataManager.ScheduledRecordingListener
    public void onScheduledRecordingAdded(ScheduledRecording scheduledRecording) {
        if (scheduledRecording.getState() == mState) {
            add(scheduledRecording);
        }
    }

    @Override  //DvrDataManager.ScheduledRecordingListener
    public void onScheduledRecordingRemoved(ScheduledRecording scheduledRecording) {
        remove(scheduledRecording);
    }

    @Override  //DvrDataManager.ScheduledRecordingListener
    public void onScheduledRecordingStatusChanged(ScheduledRecording scheduledRecording) {
        if (scheduledRecording.getState() == mState) {
            change(scheduledRecording);
        } else {
            remove(scheduledRecording);
        }
    }
}
