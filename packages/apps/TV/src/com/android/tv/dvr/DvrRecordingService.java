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

import android.app.AlarmManager;
import android.app.Service;
import android.content.Context;
import android.content.Intent;
import android.os.Binder;
import android.os.HandlerThread;
import android.os.IBinder;
import android.support.annotation.Nullable;
import android.support.annotation.VisibleForTesting;
import android.util.Log;

import com.android.tv.ApplicationSingletons;
import com.android.tv.TvApplication;
import com.android.tv.common.feature.CommonFeatures;
import com.android.tv.util.Clock;
import com.android.tv.util.RecurringRunner;
import com.android.tv.common.SoftPreconditions;

/**
 * DVR Scheduler service.
 *
 * <p> This service is responsible for:
 * <ul>
 *     <li>Send record commands to TV inputs</li>
 *     <li>Wake up at proper timing for recording</li>
 *     <li>Deconflict schedule, handling overlapping times etc.</li>
 *     <li>
 *
 * </ul>
 *
 * <p>The service does not stop it self.
 */
public class DvrRecordingService extends Service {
    private static final String TAG = "DvrRecordingService";
    private static final boolean DEBUG = false;
    public static final String HANDLER_THREAD_NAME = "DvrRecordingService-handler";

    public static void startService(Context context) {
        Intent dvrSchedulerIntent = new Intent(context, DvrRecordingService.class);
        context.startService(dvrSchedulerIntent);
    }

    private final Clock mClock = Clock.SYSTEM;
    private RecurringRunner mReaperRunner;
    private WritableDvrDataManager mDataManager;

    /**
     * Class for clients to access. Because we know this service always
     * runs in the same process as its clients, we don't need to deal with
     * IPC.
     */
    public class SchedulerBinder extends Binder {
        Scheduler getScheduler() {
            return mScheduler;
        }
    }

    private final IBinder mBinder = new SchedulerBinder();

    private Scheduler mScheduler;
    private HandlerThread mHandlerThread;

    @Override
    public void onCreate() {
        if (DEBUG) Log.d(TAG, "onCreate");
        super.onCreate();
        SoftPreconditions.checkFeatureEnabled(this, CommonFeatures.DVR, TAG);
        ApplicationSingletons singletons = TvApplication.getSingletons(this);
        mDataManager = (WritableDvrDataManager) singletons.getDvrDataManager();

        AlarmManager alarmManager = (AlarmManager) getSystemService(Context.ALARM_SERVICE);
        // mScheduler may have been set for testing.
        if (mScheduler == null) {
            mHandlerThread = new HandlerThread(HANDLER_THREAD_NAME);
            mHandlerThread.start();
            mScheduler = new Scheduler(mHandlerThread.getLooper(), singletons.getDvrManager(),
                    singletons.getDvrSessionManger(), mDataManager,
                    singletons.getChannelDataManager(), this, mClock, alarmManager);
        }
        mDataManager.addScheduledRecordingListener(mScheduler);
        mReaperRunner = new RecurringRunner(this, java.util.concurrent.TimeUnit.DAYS.toMillis(1),
                new ScheduledProgramReaper(mDataManager, mClock), null);
        mReaperRunner.start();
    }

    @Override
    public int onStartCommand(Intent intent, int flags, int startId) {
        if (DEBUG) Log.d(TAG, "onStartCommand (" + intent + "," + flags + "," + startId + ")");
        mScheduler.update();
        return START_STICKY;
    }

    @Override
    public void onDestroy() {
        if (DEBUG) Log.d(TAG, "onDestroy");
        mReaperRunner.stop();
        mDataManager.removeScheduledRecordingListener(mScheduler);
        mScheduler = null;
        if (mHandlerThread != null) {
            mHandlerThread.quit();
            mHandlerThread = null;
        }
        super.onDestroy();
    }

    @Nullable
    @Override
    public IBinder onBind(Intent intent) {
        return mBinder;
    }

    @VisibleForTesting
    void setScheduler(Scheduler scheduler) {
        Log.i(TAG, "Setting scheduler for tests to " + scheduler);
        mScheduler = scheduler;
    }
}
