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

import android.annotation.TargetApi;
import android.content.ContentResolver;
import android.content.ContentUris;
import android.content.Context;
import android.database.ContentObserver;
import android.database.Cursor;
import android.media.tv.TvContract;
import android.net.Uri;
import android.os.AsyncTask;
import android.os.Build;
import android.os.Handler;
import android.os.Looper;
import android.support.annotation.MainThread;
import android.support.annotation.Nullable;
import android.support.annotation.VisibleForTesting;
import android.util.ArraySet;
import android.util.Log;
import android.util.Range;

import com.android.tv.common.SoftPreconditions;
import com.android.tv.common.recording.RecordedProgram;
import com.android.tv.dvr.ScheduledRecording.RecordingState;
import com.android.tv.dvr.provider.AsyncDvrDbTask;
import com.android.tv.dvr.provider.AsyncDvrDbTask.AsyncDvrQueryTask;
import com.android.tv.util.AsyncDbTask;
import com.android.tv.util.Clock;

import java.util.ArrayList;
import java.util.Collections;
import java.util.HashMap;
import java.util.Iterator;
import java.util.List;
import java.util.Set;

/**
 * DVR Data manager to handle recordings and schedules.
 */
@MainThread
@TargetApi(Build.VERSION_CODES.N)
public class DvrDataManagerImpl extends BaseDvrDataManager {
    private static final String TAG = "DvrDataManagerImpl";
    private static final boolean DEBUG = false;

    private final HashMap<Long, ScheduledRecording> mScheduledRecordings = new HashMap<>();
    private final HashMap<Long, ScheduledRecording> mProgramId2ScheduledRecordings =
            new HashMap<>();
    private final HashMap<Long, RecordedProgram> mRecordedPrograms = new HashMap<>();

    private final Context mContext;
    private final Handler mMainThreadHandler = new Handler(Looper.getMainLooper());
    private final ContentObserver mContentObserver = new ContentObserver(mMainThreadHandler) {

        @Override
        public void onChange(boolean selfChange) {
            onChange(selfChange, null);
        }

        @Override
        public void onChange(boolean selfChange, @Nullable final Uri uri) {
            if (uri == null) {
                // TODO reload everything.
            }
            AsyncRecordedProgramQueryTask task = new AsyncRecordedProgramQueryTask(
                    mContext.getContentResolver(), uri);
            task.executeOnDbThread();
            mPendingTasks.add(task);
        }
    };

    private void onObservedChange(Uri uri, RecordedProgram recordedProgram) {
        long id = ContentUris.parseId(uri);
        if (DEBUG) {
            Log.d(TAG, "changed recorded program #" + id + " to " + recordedProgram);
        }
        if (recordedProgram == null) {
            RecordedProgram old = mRecordedPrograms.remove(id);
            if (old != null) {
                notifyRecordedProgramRemoved(old);
            } else {
                Log.w(TAG, "Could not find old version of deleted program #" + id);
            }
        } else {
            RecordedProgram old = mRecordedPrograms.put(id, recordedProgram);
            if (old == null) {
                notifyRecordedProgramAdded(recordedProgram);
            } else {
                notifyRecordedProgramChanged(recordedProgram);
            }
        }
    }

    private boolean mDvrLoadFinished;
    private boolean mRecordedProgramLoadFinished;
    private final Set<AsyncTask> mPendingTasks = new ArraySet<>();

    public DvrDataManagerImpl(Context context, Clock clock) {
        super(context, clock);
        mContext = context;
    }

    public void start() {
        AsyncDvrQueryTask mDvrQueryTask = new AsyncDvrQueryTask(mContext) {

            @Override
            protected void onCancelled(List<ScheduledRecording> scheduledRecordings) {
                mPendingTasks.remove(this);
            }

            @Override
            protected void onPostExecute(List<ScheduledRecording> result) {
                mPendingTasks.remove(this);
                mDvrLoadFinished = true;
                for (ScheduledRecording r : result) {
                    mScheduledRecordings.put(r.getId(), r);
                }
            }
        };
        mDvrQueryTask.executeOnDbThread();
        mPendingTasks.add(mDvrQueryTask);
        AsyncRecordedProgramsQueryTask mRecordedProgramQueryTask =
                new AsyncRecordedProgramsQueryTask(mContext.getContentResolver());
        mRecordedProgramQueryTask.executeOnDbThread();
        ContentResolver cr = mContext.getContentResolver();
        cr.registerContentObserver(TvContract.RecordedPrograms.CONTENT_URI, true, mContentObserver);
    }

    public void stop() {
        ContentResolver cr = mContext.getContentResolver();
        cr.unregisterContentObserver(mContentObserver);
        Iterator<AsyncTask> i = mPendingTasks.iterator();
        while (i.hasNext()) {
            AsyncTask task = i.next();
            i.remove();
            task.cancel(true);
        }
    }

    @Override
    public boolean isInitialized() {
        return mDvrLoadFinished && mRecordedProgramLoadFinished;
    }

    private List<ScheduledRecording> getScheduledRecordingsPrograms() {
        if (!mDvrLoadFinished) {
            return Collections.emptyList();
        }
        ArrayList<ScheduledRecording> list = new ArrayList<>(mScheduledRecordings.size());
        list.addAll(mScheduledRecordings.values());
        Collections.sort(list, ScheduledRecording.START_TIME_COMPARATOR);
        return list;
    }

    @Override
    public List<RecordedProgram> getRecordedPrograms() {
        if (!mRecordedProgramLoadFinished) {
            return Collections.emptyList();
        }
        return new ArrayList<>(mRecordedPrograms.values());
    }

    @Override
    public List<ScheduledRecording> getAllScheduledRecordings() {
        return new ArrayList<>(mScheduledRecordings.values());
    }

    protected List<ScheduledRecording> getRecordingsWithState(@RecordingState int state) {
        List<ScheduledRecording> result = new ArrayList<>();
        for (ScheduledRecording r : mScheduledRecordings.values()) {
            if (r.getState() == state) {
                result.add(r);
            }
        }
        return result;
    }

    @Override
    public List<SeasonRecording> getSeasonRecordings() {
        // If we return dummy data here, we can implement UI part independently.
        return Collections.emptyList();
    }

    @Override
    public long getNextScheduledStartTimeAfter(long startTime) {
        return getNextStartTimeAfter(getScheduledRecordingsPrograms(), startTime);
    }

    @VisibleForTesting
    static long getNextStartTimeAfter(List<ScheduledRecording> scheduledRecordings, long startTime) {
        int start = 0;
        int end = scheduledRecordings.size() - 1;
        while (start <= end) {
            int mid = (start + end) / 2;
            if (scheduledRecordings.get(mid).getStartTimeMs() <= startTime) {
                start = mid + 1;
            } else {
                end = mid - 1;
            }
        }
        return start < scheduledRecordings.size() ? scheduledRecordings.get(start).getStartTimeMs()
                : NEXT_START_TIME_NOT_FOUND;
    }

    @Override
    public List<ScheduledRecording> getRecordingsThatOverlapWith(Range<Long> period) {
        List<ScheduledRecording> result = new ArrayList<>();
        for (ScheduledRecording r : mScheduledRecordings.values()) {
            if (r.isOverLapping(period)) {
                result.add(r);
            }
        }
        return result;
    }

    @Nullable
    @Override
    public ScheduledRecording getScheduledRecording(long recordingId) {
        if (mDvrLoadFinished) {
            return mScheduledRecordings.get(recordingId);
        }
        return null;
    }

    @Nullable
    @Override
    public ScheduledRecording getScheduledRecordingForProgramId(long programId) {
        if (mDvrLoadFinished) {
            return mProgramId2ScheduledRecordings.get(programId);
        }
        return null;
    }

    @Nullable
    @Override
    public RecordedProgram getRecordedProgram(long recordingId) {
        return mRecordedPrograms.get(recordingId);
    }

    @Override
    public void addScheduledRecording(final ScheduledRecording scheduledRecording) {
        new AsyncDvrDbTask.AsyncAddRecordingTask(mContext) {
            @Override
            protected void onPostExecute(List<ScheduledRecording> scheduledRecordings) {
                super.onPostExecute(scheduledRecordings);
                SoftPreconditions.checkArgument(scheduledRecordings.size() == 1);
                for (ScheduledRecording r : scheduledRecordings) {
                    if (r.getId() != -1) {
                        mScheduledRecordings.put(r.getId(), r);
                        if (r.getProgramId() != ScheduledRecording.ID_NOT_SET) {
                            mProgramId2ScheduledRecordings.put(r.getProgramId(), r);
                        }
                        notifyScheduledRecordingAdded(r);
                    } else {
                        Log.w(TAG, "Error adding " + r);
                    }
                }

            }
        }.executeOnDbThread(scheduledRecording);
    }

    @Override
    public void addSeasonRecording(SeasonRecording seasonRecording) { }

    @Override
    public void removeScheduledRecording(final ScheduledRecording scheduledRecording) {
        new AsyncDvrDbTask.AsyncDeleteRecordingTask(mContext) {
            @Override
            protected void onPostExecute(List<Integer> counts) {
                super.onPostExecute(counts);
                SoftPreconditions.checkArgument(counts.size() == 1);
                for (Integer c : counts) {
                    if (c == 1) {
                        mScheduledRecordings.remove(scheduledRecording.getId());
                        if (scheduledRecording.getProgramId() != ScheduledRecording.ID_NOT_SET) {
                            mProgramId2ScheduledRecordings
                                    .remove(scheduledRecording.getProgramId());
                        }
                        //TODO change to notifyRecordingUpdated
                        notifyScheduledRecordingRemoved(scheduledRecording);
                    } else {
                        Log.w(TAG, "Error removing " + scheduledRecording);
                    }
                }

            }
        }.executeOnDbThread(scheduledRecording);
    }

    @Override
    public void removeSeasonSchedule(SeasonRecording seasonSchedule) { }

    @Override
    public void updateScheduledRecording(final ScheduledRecording scheduledRecording) {
        new AsyncDvrDbTask.AsyncUpdateRecordingTask(mContext) {
            @Override
            protected void onPostExecute(List<Integer> counts) {
                super.onPostExecute(counts);
                SoftPreconditions.checkArgument(counts.size() == 1);
                for (Integer c : counts) {
                    if (c == 1) {
                        ScheduledRecording oldScheduledRecording = mScheduledRecordings
                                .put(scheduledRecording.getId(), scheduledRecording);
                        long programId = scheduledRecording.getProgramId();
                        if (oldScheduledRecording != null
                                && oldScheduledRecording.getProgramId() != programId
                                && oldScheduledRecording.getProgramId()
                                != ScheduledRecording.ID_NOT_SET) {
                            ScheduledRecording oldValueForProgramId = mProgramId2ScheduledRecordings
                                    .get(oldScheduledRecording.getProgramId());
                            if (oldValueForProgramId.getId() == scheduledRecording.getId()) {
                                //Only remove the old ScheduledRecording if it has the same ID as
                                // the new one.
                                mProgramId2ScheduledRecordings
                                        .remove(oldScheduledRecording.getProgramId());
                            }
                        }
                        if (programId != ScheduledRecording.ID_NOT_SET) {
                            mProgramId2ScheduledRecordings.put(programId, scheduledRecording);
                        }
                        //TODO change to notifyRecordingUpdated
                        notifyScheduledRecordingStatusChanged(scheduledRecording);
                    } else {
                        Log.w(TAG, "Error updating " + scheduledRecording);
                    }
                }
            }
        }.executeOnDbThread(scheduledRecording);
    }

    private final class AsyncRecordedProgramsQueryTask
            extends AsyncDbTask.AsyncQueryListTask<RecordedProgram> {
        public AsyncRecordedProgramsQueryTask(ContentResolver contentResolver) {
            super(contentResolver, TvContract.RecordedPrograms.CONTENT_URI,
                    RecordedProgram.PROJECTION, null, null, null);
        }

        @Override
        protected RecordedProgram fromCursor(Cursor c) {
            return RecordedProgram.fromCursor(c);
        }

        @Override
        protected void onCancelled(List<RecordedProgram> scheduledRecordings) {
            mPendingTasks.remove(this);
        }

        @Override
        protected void onPostExecute(List<RecordedProgram> result) {
            mPendingTasks.remove(this);
            mRecordedProgramLoadFinished = true;
            if (result != null) {
                for (RecordedProgram r : result) {
                    mRecordedPrograms.put(r.getId(), r);
                }
            }
        }
    }

    private final class AsyncRecordedProgramQueryTask
            extends AsyncDbTask.AsyncQueryItemTask<RecordedProgram> {

        private final Uri mUri;

        public AsyncRecordedProgramQueryTask(ContentResolver contentResolver, Uri uri) {
            super(contentResolver, uri, RecordedProgram.PROJECTION, null, null, null);
            mUri = uri;
        }

        @Override
        protected RecordedProgram fromCursor(Cursor c) {
            return RecordedProgram.fromCursor(c);
        }

        @Override
        protected void onCancelled(RecordedProgram recordedProgram) {
            mPendingTasks.remove(this);
        }

        @Override
        protected void onPostExecute(RecordedProgram recordedProgram) {
            mPendingTasks.remove(this);
            onObservedChange(mUri, recordedProgram);
        }
    }
}
