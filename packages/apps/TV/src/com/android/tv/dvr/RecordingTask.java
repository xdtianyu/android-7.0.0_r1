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

import android.media.tv.TvContract;
import android.media.tv.TvRecordingClient;
import android.net.Uri;
import android.os.Handler;
import android.os.Looper;
import android.os.Message;
import android.support.annotation.VisibleForTesting;
import android.support.annotation.WorkerThread;
import android.util.Log;

import com.android.tv.common.SoftPreconditions;
import com.android.tv.data.Channel;
import com.android.tv.util.Clock;
import com.android.tv.util.Utils;

import java.util.concurrent.TimeUnit;

/**
 * A Handler that actually starts and stop a recording at the right time.
 *
 * <p>This is run on the looper of thread named {@value DvrRecordingService#HANDLER_THREAD_NAME}.
 * There is only one looper so messages must be handled quickly or start a separate thread.
 */
@WorkerThread
class RecordingTask extends TvRecordingClient.RecordingCallback
        implements Handler.Callback, DvrManager.Listener {
    private static final String TAG = "RecordingTask";
    private static final boolean DEBUG = false;

    @VisibleForTesting
    static final int MESSAGE_INIT = 1;
    @VisibleForTesting
    static final int MESSAGE_START_RECORDING = 2;
    @VisibleForTesting
    static final int MESSAGE_STOP_RECORDING = 3;

    @VisibleForTesting
    static final long MS_BEFORE_START = TimeUnit.SECONDS.toMillis(5);
    @VisibleForTesting
    static final long MS_AFTER_END = TimeUnit.SECONDS.toMillis(5);

    @VisibleForTesting
    enum State {
        NOT_STARTED,
        SESSION_ACQUIRED,
        CONNECTION_PENDING,
        CONNECTED,
        RECORDING_START_REQUESTED,
        RECORDING_STARTED,
        RECORDING_STOP_REQUESTED,
        ERROR,
        RELEASED,
    }
    private final DvrSessionManager mSessionManager;
    private final DvrManager mDvrManager;

    private final WritableDvrDataManager mDataManager;
    private final Handler mMainThreadHandler = new Handler(Looper.getMainLooper());
    private TvRecordingClient mTvRecordingClient;
    private Handler mHandler;
    private ScheduledRecording mScheduledRecording;
    private final Channel mChannel;
    private State mState = State.NOT_STARTED;
    private final Clock mClock;

    RecordingTask(ScheduledRecording scheduledRecording, Channel channel,
            DvrManager dvrManager, DvrSessionManager sessionManager,
            WritableDvrDataManager dataManager, Clock clock) {
        mScheduledRecording = scheduledRecording;
        mChannel = channel;
        mSessionManager = sessionManager;
        mDataManager = dataManager;
        mClock = clock;
        mDvrManager = dvrManager;

        if (DEBUG) Log.d(TAG, "created recording task " + mScheduledRecording);
    }

    public void setHandler(Handler handler) {
        mHandler = handler;
    }

    @Override
    public boolean handleMessage(Message msg) {
        if (DEBUG) Log.d(TAG, "handleMessage " + msg);
        SoftPreconditions
                .checkState(msg.what == Scheduler.HandlerWrapper.MESSAGE_REMOVE || mHandler != null,
                        TAG, "Null handler trying to handle " + msg);
        try {
            switch (msg.what) {
                case MESSAGE_INIT:
                    handleInit();
                    break;
                case MESSAGE_START_RECORDING:
                    handleStartRecording();
                    break;
                case MESSAGE_STOP_RECORDING:
                    handleStopRecording();
                    break;
                case Scheduler.HandlerWrapper.MESSAGE_REMOVE:
                    // Clear the handler
                    mHandler = null;
                    release();
                    return false;
                default:
                    SoftPreconditions.checkArgument(false, TAG, "unexpected message type " + msg);
            }
            return true;
        } catch (Exception e) {
            Log.w(TAG, "Error processing message " + msg + "  for " + mScheduledRecording, e);
            failAndQuit();
        }
        return false;
    }

    @Override
    public void onTuned(Uri channelUri) {
        if (DEBUG) {
            Log.d(TAG, "onTuned");
        }
        super.onTuned(channelUri);
        mState = State.CONNECTED;
        if (mHandler == null || !sendEmptyMessageAtAbsoluteTime(MESSAGE_START_RECORDING,
                mScheduledRecording.getStartTimeMs() - MS_BEFORE_START)) {
            mState = State.ERROR;
            return;
        }
    }


    @Override
    public void onRecordingStopped(Uri recordedProgramUri) {
        super.onRecordingStopped(recordedProgramUri);
        mState = State.CONNECTED;
        updateRecording(ScheduledRecording.buildFrom(mScheduledRecording)
                .setState(ScheduledRecording.STATE_RECORDING_FINISHED).build());
        sendRemove();
    }

    @Override
    public void onError(int reason) {
        if (DEBUG) Log.d(TAG, "onError reason " + reason);
        super.onError(reason);
        // TODO(dvr) handle success
        switch (reason) {
            default:
                updateRecording(ScheduledRecording.buildFrom(mScheduledRecording)
                        .setState(ScheduledRecording.STATE_RECORDING_FAILED)
                        .build());
        }
        release();
        sendRemove();
    }

    private void handleInit() {
        if (DEBUG) Log.d(TAG, "handleInit " + mScheduledRecording);
        //TODO check recording preconditions

        if (mScheduledRecording.getEndTimeMs() < mClock.currentTimeMillis()) {
            Log.w(TAG, "End time already past, not recording " + mScheduledRecording);
            failAndQuit();
            return;
        }

        if (mChannel == null) {
            Log.w(TAG, "Null channel for " + mScheduledRecording);
            failAndQuit();
            return;
        }
        if (mChannel.getId() != mScheduledRecording.getChannelId()) {
            Log.w(TAG, "Channel" + mChannel + " does not match scheduled recording "
                    + mScheduledRecording);
            failAndQuit();
            return;
        }

        String inputId = mChannel.getInputId();
        if (mSessionManager.canAcquireDvrSession(inputId, mChannel)) {
            mTvRecordingClient = mSessionManager
                    .createTvRecordingClient("recordingTask-" + mScheduledRecording.getId(), this,
                            mHandler);
            mState = State.SESSION_ACQUIRED;
        } else {
            Log.w(TAG, "Unable to acquire a session for " + mScheduledRecording);
            failAndQuit();
            return;
        }
        mDvrManager.addListener(this, mHandler);
        mTvRecordingClient.tune(inputId, mChannel.getUri());
        mState = State.CONNECTION_PENDING;
    }

    private void failAndQuit() {
        if (DEBUG) Log.d(TAG, "failAndQuit");
        updateRecordingState(ScheduledRecording.STATE_RECORDING_FAILED);
        mState = State.ERROR;
        sendRemove();
    }

    private void sendRemove() {
        if (DEBUG) Log.d(TAG, "sendRemove");
        if (mHandler != null) {
            mHandler.sendEmptyMessage(Scheduler.HandlerWrapper.MESSAGE_REMOVE);
        }
    }

    private void handleStartRecording() {
        if (DEBUG) Log.d(TAG, "handleStartRecording " + mScheduledRecording);
        // TODO(DVR) handle errors
        long programId = mScheduledRecording.getProgramId();
        mTvRecordingClient.startRecording(programId == ScheduledRecording.ID_NOT_SET ? null
                : TvContract.buildProgramUri(programId));
        updateRecording(ScheduledRecording.buildFrom(mScheduledRecording)
                .setState(ScheduledRecording.STATE_RECORDING_IN_PROGRESS).build());
        mState = State.RECORDING_STARTED;

        if (mHandler == null || !sendEmptyMessageAtAbsoluteTime(MESSAGE_STOP_RECORDING,
                mScheduledRecording.getEndTimeMs() + MS_AFTER_END)) {
            mState = State.ERROR;
            return;
        }
    }

    private void handleStopRecording() {
        if (DEBUG) Log.d(TAG, "handleStopRecording " + mScheduledRecording);
        mTvRecordingClient.stopRecording();
        mState = State.RECORDING_STOP_REQUESTED;
    }

    @VisibleForTesting
    State getState() {
        return mState;
    }

    private void release() {
        if (mTvRecordingClient != null) {
           mSessionManager.releaseTvRecordingClient(mTvRecordingClient);
        }
        mDvrManager.removeListener(this);
    }

    private boolean sendEmptyMessageAtAbsoluteTime(int what, long when) {
        long now = mClock.currentTimeMillis();
        long delay = Math.max(0L, when - now);
        if (DEBUG) {
            Log.d(TAG, "Sending message " + what + " with a delay of " + delay / 1000
                    + " seconds to arrive at " + Utils.toIsoDateTimeString(when));
        }
        return mHandler.sendEmptyMessageDelayed(what, delay);
    }

    private void updateRecordingState(@ScheduledRecording.RecordingState int state) {
        updateRecording(ScheduledRecording.buildFrom(mScheduledRecording).setState(state).build());
    }

    @VisibleForTesting
    static Uri getIdAsMediaUri(ScheduledRecording scheduledRecording) {
            // TODO define the URI format
        return new Uri.Builder().appendPath(String.valueOf(scheduledRecording.getId())).build();
    }

    private void updateRecording(ScheduledRecording updatedScheduledRecording) {
        if (DEBUG) Log.d(TAG, "updateScheduledRecording " + updatedScheduledRecording);
        mScheduledRecording = updatedScheduledRecording;
        mMainThreadHandler.post(new Runnable() {
            @Override
            public void run() {
                mDataManager.updateScheduledRecording(mScheduledRecording);
            }
        });
    }

    @Override
    public void onStopRecordingRequested(ScheduledRecording recording) {
        if (recording.getId() != mScheduledRecording.getId()) {
            return;
        }
        switch (mState) {
            case RECORDING_STARTED:
                mHandler.removeMessages(MESSAGE_STOP_RECORDING);
                handleStopRecording();
                break;
            case RECORDING_STOP_REQUESTED:
                // Do nothing
                break;
            case NOT_STARTED:
            case SESSION_ACQUIRED:
            case CONNECTION_PENDING:
            case CONNECTED:
            case RECORDING_START_REQUESTED:
            case ERROR:
            case RELEASED:
            default:
                sendRemove();
                break;
        }
    }

    @Override
    public String toString() {
        return getClass().getName() + "(" + mScheduledRecording + ")";
    }
}
