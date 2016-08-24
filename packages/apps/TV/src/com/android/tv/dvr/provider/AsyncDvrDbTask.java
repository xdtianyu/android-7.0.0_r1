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

package com.android.tv.dvr.provider;

import android.content.Context;
import android.database.Cursor;
import android.os.AsyncTask;
import android.support.annotation.Nullable;

import com.android.tv.dvr.ScheduledRecording;
import com.android.tv.dvr.provider.DvrContract.Recordings;
import com.android.tv.util.NamedThreadFactory;

import java.util.ArrayList;
import java.util.List;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;

/**
 * {@link AsyncTask} that defaults to executing on its own single threaded Executor Service.
 */
public abstract class AsyncDvrDbTask<Params, Progress, Result>
        extends AsyncTask<Params, Progress, Result> {
    private static final NamedThreadFactory THREAD_FACTORY = new NamedThreadFactory(
            AsyncDvrDbTask.class.getSimpleName());
    private static final ExecutorService DB_EXECUTOR = Executors
            .newSingleThreadExecutor(THREAD_FACTORY);

    private static DvrDatabaseHelper sDbHelper;

    private static synchronized DvrDatabaseHelper initializeDbHelper(Context context) {
        if (sDbHelper == null) {
            sDbHelper = new DvrDatabaseHelper(context.getApplicationContext());
        }
        return sDbHelper;
    }

    final Context mContext;

    private AsyncDvrDbTask(Context context) {
        mContext = context;
    }

    /**
     * Execute the task on the {@link #DB_EXECUTOR} thread.
     */
    @SafeVarargs
    public final void executeOnDbThread(Params... params) {
        executeOnExecutor(DB_EXECUTOR, params);
    }

    @Override
    protected final Result doInBackground(Params... params) {
        initializeDbHelper(mContext);
        return doInDvrBackground(params);
    }

    /**
     * Executes in the background after {@link #initializeDbHelper(Context)}
     */
    @Nullable
    protected abstract Result doInDvrBackground(Params... params);

     /**
     * Inserts recordings returning the list of recordings with id set.
     * The id will be -1 if there was an error.
     */
    public abstract static class AsyncAddRecordingTask
            extends AsyncDvrDbTask<ScheduledRecording, Void, List<ScheduledRecording>> {

        public AsyncAddRecordingTask(Context context) {
            super(context);
        }

        @Override
        protected final List<ScheduledRecording> doInDvrBackground(ScheduledRecording... params) {
            return sDbHelper.insertRecordings(params);
        }
    }

    /**
     * Update recordings.
     *
     * @return list of row update counts.  The count will be -1 if there was an error or 0
     * if no match was found. The count is expected to be exactly 1 for each recording.
     */
    public abstract static class AsyncUpdateRecordingTask
            extends AsyncDvrDbTask<ScheduledRecording, Void, List<Integer>> {
        public AsyncUpdateRecordingTask(Context context) {
            super(context);
        }

        @Override
        protected final List<Integer> doInDvrBackground(ScheduledRecording... params) {
            return sDbHelper.updateRecordings(params);
        }
    }

    /**
     * Delete recordings.
     *
     * @return list of row delete counts.  The count will be -1 if there was an error or 0
     * if no match was found. The count is expected to be exactly 1 for each recording.
     */
    public abstract static class AsyncDeleteRecordingTask
            extends AsyncDvrDbTask<ScheduledRecording, Void, List<Integer>> {
        public AsyncDeleteRecordingTask(Context context) {
            super(context);
        }

        @Override
        protected final List<Integer> doInDvrBackground(ScheduledRecording... params) {
            return sDbHelper.deleteRecordings(params);
        }
    }

    public abstract static class AsyncDvrQueryTask
            extends AsyncDvrDbTask<Void, Void, List<ScheduledRecording>> {
        public AsyncDvrQueryTask(Context context) {
            super(context);
        }

        @Override
        @Nullable
        protected final List<ScheduledRecording> doInDvrBackground(Void... params) {
            if (isCancelled()) {
                return null;
            }

            if (isCancelled()) {
                return null;
            }
            if (isCancelled()) {
                return null;
            }
            List<ScheduledRecording> scheduledRecordings = new ArrayList<>();
            try (Cursor c = sDbHelper.query(Recordings.TABLE_NAME, ScheduledRecording.PROJECTION)) {
                while (c.moveToNext() && !isCancelled()) {
                    scheduledRecordings.add(ScheduledRecording.fromCursor(c));
                }
            }
            return scheduledRecordings;
        }
    }
}
