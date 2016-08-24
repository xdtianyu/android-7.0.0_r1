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

package com.android.tv.dvr.ui;

import android.os.Bundle;
import android.support.annotation.IntDef;
import android.support.v17.leanback.app.BrowseFragment;
import android.support.v17.leanback.widget.ArrayObjectAdapter;
import android.support.v17.leanback.widget.ClassPresenterSelector;
import android.support.v17.leanback.widget.HeaderItem;
import android.support.v17.leanback.widget.ListRow;
import android.support.v17.leanback.widget.ListRowPresenter;
import android.support.v17.leanback.widget.ObjectAdapter;
import android.util.Log;

import com.android.tv.R;
import com.android.tv.TvApplication;
import com.android.tv.common.recording.RecordedProgram;
import com.android.tv.dvr.DvrDataManager;
import com.android.tv.dvr.ScheduledRecording;

import java.lang.annotation.Retention;
import java.lang.annotation.RetentionPolicy;
import java.util.LinkedHashMap;

/**
 * {@link BrowseFragment} for DVR functions.
 */
public class DvrBrowseFragment extends BrowseFragment {
    private static final String TAG = "DvrBrowseFragment";
    private static final boolean DEBUG = false;

    private ScheduledRecordingsAdapter mRecordingsInProgressAdapter;
    private ScheduledRecordingsAdapter mRecordingsNotStatedAdapter;
    private RecordedProgramsAdapter mRecordedProgramsAdapter;

    @IntDef({DVR_CURRENT_RECORDINGS, DVR_SCHEDULED_RECORDINGS, DVR_RECORDED_PROGRAMS, DVR_SETTINGS})
    @Retention(RetentionPolicy.SOURCE)
    public @interface DVR_HEADERS_MODE {}
    public static final int DVR_CURRENT_RECORDINGS = 0;
    public static final int DVR_SCHEDULED_RECORDINGS = 1;
    public static final int DVR_RECORDED_PROGRAMS = 2;
    public static final int DVR_SETTINGS = 3;

    private static final LinkedHashMap<Integer, Integer> sHeaders =
            new LinkedHashMap<Integer, Integer>() {{
        put(DVR_CURRENT_RECORDINGS, R.string.dvr_main_current_recordings);
        put(DVR_SCHEDULED_RECORDINGS, R.string.dvr_main_scheduled_recordings);
        put(DVR_RECORDED_PROGRAMS, R.string.dvr_main_recorded_programs);
        /* put(DVR_SETTINGS, R.string.dvr_main_settings); */ // TODO: Temporarily remove it for DP.
    }};

    private DvrDataManager mDvrDataManager;
    private ArrayObjectAdapter mRowsAdapter;

    @Override
    public void onCreate(Bundle savedInstanceState) {
        if (DEBUG) Log.d(TAG, "onCreate");
        super.onCreate(savedInstanceState);
        mDvrDataManager = TvApplication.getSingletons(getContext()).getDvrDataManager();
        setupUiElements();
        setupAdapters();
        mRecordingsInProgressAdapter.start();
        mRecordingsNotStatedAdapter.start();
        mRecordedProgramsAdapter.start();
        initRows();
        prepareEntranceTransition();
        startEntranceTransition();
    }

    @Override
    public void onStart() {
        if (DEBUG) Log.d(TAG, "onStart");
        super.onStart();
        // TODO: It's a workaround for a bug that a progress bar isn't hidden.
        // We need to remove it later.
        getProgressBarManager().disableProgressBar();
    }

    @Override
    public void onDestroy() {
        if (DEBUG) Log.d(TAG, "onDestroy");
        mRecordingsInProgressAdapter.stop();
        mRecordingsNotStatedAdapter.stop();
        mRecordedProgramsAdapter.stop();
        super.onDestroy();
    }

    private void setupUiElements() {
        setHeadersState(HEADERS_ENABLED);
        setHeadersTransitionOnBackEnabled(false);
    }

    private void setupAdapters() {
        mRowsAdapter = new ArrayObjectAdapter(new ListRowPresenter());
        setAdapter(mRowsAdapter);
        ClassPresenterSelector presenterSelector = new ClassPresenterSelector();
        EmptyItemPresenter emptyItemPresenter = new EmptyItemPresenter(this);
        ScheduledRecordingPresenter scheduledRecordingPresenter = new ScheduledRecordingPresenter(
                getContext());
        RecordedProgramPresenter recordedProgramPresenter = new RecordedProgramPresenter(
                getContext());
        presenterSelector.addClassPresenter(ScheduledRecording.class, scheduledRecordingPresenter);
        presenterSelector.addClassPresenter(RecordedProgram.class, recordedProgramPresenter);
        presenterSelector.addClassPresenter(EmptyHolder.class, emptyItemPresenter);
        mRecordingsInProgressAdapter = new ScheduledRecordingsAdapter(mDvrDataManager,
                ScheduledRecording.STATE_RECORDING_IN_PROGRESS, presenterSelector);
        mRecordingsNotStatedAdapter = new ScheduledRecordingsAdapter(mDvrDataManager,
                ScheduledRecording.STATE_RECORDING_NOT_STARTED, presenterSelector);
        mRecordedProgramsAdapter = new RecordedProgramsAdapter(mDvrDataManager, presenterSelector);
    }

    private void initRows() {
        mRowsAdapter.clear();
        for (@DVR_HEADERS_MODE int i : sHeaders.keySet()) {
            HeaderItem gridHeader = new HeaderItem(i, getContext().getString(sHeaders.get(i)));
            ObjectAdapter gridRowAdapter = null;
            switch (i) {
                case DVR_CURRENT_RECORDINGS: {
                    gridRowAdapter = mRecordingsInProgressAdapter;
                    break;
                }
                case DVR_SCHEDULED_RECORDINGS: {
                    gridRowAdapter = mRecordingsNotStatedAdapter;
                }
                    break;
                case DVR_RECORDED_PROGRAMS: {
                    gridRowAdapter = mRecordedProgramsAdapter;
                }
                    break;
                case DVR_SETTINGS:
                    gridRowAdapter = new ArrayObjectAdapter(new EmptyItemPresenter(this));
                    // TODO: provide setup rows.
                    break;
            }
            if (gridRowAdapter != null) {
                mRowsAdapter.add(new ListRow(gridHeader, gridRowAdapter));
            }
        }
    }
}
