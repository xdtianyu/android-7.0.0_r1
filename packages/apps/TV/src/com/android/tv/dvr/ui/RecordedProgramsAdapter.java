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

import com.android.tv.common.recording.RecordedProgram;
import com.android.tv.dvr.DvrDataManager;

/**
 * Adapter for {@link RecordedProgram}.
 */
final class RecordedProgramsAdapter extends SortedArrayAdapter<RecordedProgram>
        implements DvrDataManager.RecordedProgramListener {
    private final DvrDataManager mDataManager;

    RecordedProgramsAdapter(DvrDataManager dataManager, PresenterSelector presenterSelector) {
        super(presenterSelector, RecordedProgram.START_TIME_THEN_ID_COMPARATOR);
        mDataManager = dataManager;
    }

    public void start() {
        clear();
        addAll(mDataManager.getRecordedPrograms());
        mDataManager.addRecordedProgramListener(this);
    }

    public void stop() {
        mDataManager.removeRecordedProgramListener(this);
    }

    @Override
    long getId(RecordedProgram item) {
        return item.getId();
    }

    @Override  // DvrDataManager.RecordedProgramListener
    public void onRecordedProgramAdded(RecordedProgram recordedProgram) {
        add(recordedProgram);
    }

    @Override  // DvrDataManager.RecordedProgramListener
    public void onRecordedProgramChanged(RecordedProgram recordedProgram) {
        change(recordedProgram);
    }

    @Override  // DvrDataManager.RecordedProgramListener
    public void onRecordedProgramRemoved(RecordedProgram recordedProgram) {
        remove(recordedProgram);
    }
}
