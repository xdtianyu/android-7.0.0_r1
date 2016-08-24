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

package com.android.tv.dvr;

import android.support.annotation.MainThread;
import android.support.annotation.VisibleForTesting;

import com.android.tv.util.Clock;

import java.util.List;
import java.util.concurrent.TimeUnit;

/**
 * Deletes {@link ScheduledRecording} older than {@value @DAYS} days.
 */
class ScheduledProgramReaper implements Runnable {

    @VisibleForTesting
    static final  int DAYS = 2;
    private final WritableDvrDataManager mDvrDataManager;
    private final Clock mClock;

    ScheduledProgramReaper(WritableDvrDataManager dvrDataManager, Clock clock) {
        mDvrDataManager = dvrDataManager;
        mClock = clock;
    }

    @Override
    @MainThread
    public void run() {
        List<ScheduledRecording> recordings = mDvrDataManager.getAllScheduledRecordings();
        long cutoff = mClock.currentTimeMillis() - TimeUnit.DAYS.toMillis(DAYS);
        for (ScheduledRecording r : recordings) {
            if (r.getEndTimeMs() < cutoff) {
                mDvrDataManager.removeScheduledRecording(r);
            }
        }
    }
}
