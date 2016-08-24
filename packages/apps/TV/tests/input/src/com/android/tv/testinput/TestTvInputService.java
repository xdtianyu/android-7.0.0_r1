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

package com.android.tv.testinput;

import android.content.ComponentName;
import android.content.Context;
import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.Paint;
import android.media.PlaybackParams;
import android.media.tv.TvContract;
import android.media.tv.TvInputManager;
import android.media.tv.TvInputService;
import android.media.tv.TvTrackInfo;
import android.net.Uri;
import android.os.Build;
import android.os.Handler;
import android.os.Looper;
import android.os.Message;
import android.util.Log;
import android.view.KeyEvent;
import android.view.Surface;

import com.android.tv.testing.ChannelInfo;
import com.android.tv.testing.testinput.ChannelState;

import java.util.Date;

/**
 * Simple TV input service which provides test channels.
 */
public class TestTvInputService extends TvInputService {
    private static final String TAG = "TestTvInputServices";
    private static final int REFRESH_DELAY_MS = 1000 / 5;
    private static final boolean DEBUG = false;
    private static final boolean HAS_TIME_SHIFT_API = Build.VERSION.SDK_INT
            >= Build.VERSION_CODES.M;
    private final TestInputControl mBackend = TestInputControl.getInstance();

    public static String buildInputId(Context context) {
        return TvContract.buildInputId(new ComponentName(context, TestTvInputService.class));
    }

    @Override
    public void onCreate() {
        super.onCreate();
        mBackend.init(this, buildInputId(this));
    }

    @Override
    public Session onCreateSession(String inputId) {
        Log.v(TAG, "Creating session for " + inputId);
        return new SimpleSessionImpl(this);
    }

    /**
     * Simple session implementation that just display some text.
     */
    private class SimpleSessionImpl extends Session {
        private static final int MSG_SEEK = 1000;
        private static final int SEEK_DELAY_MS = 300;

        private final Paint mTextPaint = new Paint();
        private final DrawRunnable mDrawRunnable = new DrawRunnable();
        private Surface mSurface = null;
        private ChannelInfo mChannel = null;
        private ChannelState mCurrentState = null;
        private String mCurrentVideoTrackId = null;
        private String mCurrentAudioTrackId = null;

        private long mRecordStartTimeMs;
        private long mPausedTimeMs;
        // The time in milliseconds when the current position is lastly updated.
        private long mLastCurrentPositionUpdateTimeMs;
        // The current playback position.
        private long mCurrentPositionMs;
        // The current playback speed rate.
        private float mSpeed;

        private final Handler mHandler = new Handler(Looper.myLooper()) {
            @Override
            public void handleMessage(Message msg) {
                if (msg.what == MSG_SEEK) {
                    // Actually, this input doesn't play any videos, it just shows the image.
                    // So we should simulate the playback here by changing the current playback
                    // position periodically in order to test the time shift.
                    // If the playback is paused, the current playback position doesn't need to be
                    // changed.
                    if (mPausedTimeMs == 0) {
                        long currentTimeMs = System.currentTimeMillis();
                        mCurrentPositionMs += (long) ((currentTimeMs
                                - mLastCurrentPositionUpdateTimeMs) * mSpeed);
                        mCurrentPositionMs = Math.max(mRecordStartTimeMs,
                                Math.min(mCurrentPositionMs, currentTimeMs));
                        mLastCurrentPositionUpdateTimeMs = currentTimeMs;
                    }
                    sendEmptyMessageDelayed(MSG_SEEK, SEEK_DELAY_MS);
                }
                super.handleMessage(msg);
            }
        };

        SimpleSessionImpl(Context context) {
            super(context);
            mTextPaint.setColor(Color.BLACK);
            mTextPaint.setTextSize(150);
            mHandler.post(mDrawRunnable);
            if (DEBUG) {
                Log.v(TAG, "Created session " + this);
            }
        }

        private void setAudioTrack(String selectedAudioTrackId) {
            Log.i(TAG, "Set audio track to " + selectedAudioTrackId);
            mCurrentAudioTrackId = selectedAudioTrackId;
            notifyTrackSelected(TvTrackInfo.TYPE_AUDIO, mCurrentAudioTrackId);
        }

        private void setVideoTrack(String selectedVideoTrackId) {
            Log.i(TAG, "Set video track to " + selectedVideoTrackId);
            mCurrentVideoTrackId = selectedVideoTrackId;
            notifyTrackSelected(TvTrackInfo.TYPE_VIDEO, mCurrentVideoTrackId);
        }

        @Override
        public void onRelease() {
            if (DEBUG) {
                Log.v(TAG, "Releasing session " + this);
            }
            mDrawRunnable.cancel();
            mHandler.removeCallbacks(mDrawRunnable);
            mSurface = null;
            mChannel = null;
            mCurrentState = null;
        }

        @Override
        public boolean onSetSurface(Surface surface) {
            synchronized (mDrawRunnable) {
                mSurface = surface;
            }
            if (surface != null) {
                if (DEBUG) {
                    Log.v(TAG, "Surface set");
                }
            } else {
                if (DEBUG) {
                    Log.v(TAG, "Surface unset");
                }
            }

            return true;
        }

        @Override
        public void onSurfaceChanged(int format, int width, int height) {
            super.onSurfaceChanged(format, width, height);
            Log.d(TAG, "format=" + format + " width=" + width + " height=" + height);
        }

        @Override
        public void onSetStreamVolume(float volume) {
            // No-op
        }

        @Override
        public boolean onTune(Uri channelUri) {
            Log.i(TAG, "Tune to " + channelUri);
            ChannelInfo info = mBackend.getChannelInfo(channelUri);
            synchronized (mDrawRunnable) {
                if (info == null || mChannel == null
                        || mChannel.originalNetworkId != info.originalNetworkId) {
                    mCurrentState = null;
                }
                mChannel = info;
                mCurrentVideoTrackId = null;
                mCurrentAudioTrackId = null;
            }
            if (mChannel == null) {
                Log.i(TAG, "Channel not found for " + channelUri);
                notifyVideoUnavailable(TvInputManager.VIDEO_UNAVAILABLE_REASON_UNKNOWN);
            } else {
                Log.i(TAG, "Tuning to " + mChannel);
            }
            if (HAS_TIME_SHIFT_API) {
                notifyTimeShiftStatusChanged(TvInputManager.TIME_SHIFT_STATUS_AVAILABLE);
                mRecordStartTimeMs = mCurrentPositionMs = mLastCurrentPositionUpdateTimeMs
                        = System.currentTimeMillis();
                mPausedTimeMs = 0;
                mHandler.sendEmptyMessageDelayed(MSG_SEEK, SEEK_DELAY_MS);
                mSpeed = 1;
            }
            return true;
        }

        @Override
        public void onSetCaptionEnabled(boolean enabled) {
            // No-op
        }

        @Override
        public boolean onKeyDown(int keyCode, KeyEvent event) {
            Log.d(TAG, "onKeyDown (keyCode=" + keyCode + ", event=" + event + ")");
            return true;
        }

        @Override
        public boolean onKeyUp(int keyCode, KeyEvent event) {
            Log.d(TAG, "onKeyUp (keyCode=" + keyCode + ", event=" + event + ")");
            return true;
        }

        @Override
        public long onTimeShiftGetCurrentPosition() {
            Log.d(TAG, "currentPositionMs=" + mCurrentPositionMs);
            return mCurrentPositionMs;
        }

        @Override
        public long onTimeShiftGetStartPosition() {
            return mRecordStartTimeMs;
        }

        @Override
        public void onTimeShiftPause() {
            mCurrentPositionMs = mPausedTimeMs = mLastCurrentPositionUpdateTimeMs
                    = System.currentTimeMillis();
        }

        @Override
        public void onTimeShiftResume() {
            mSpeed = 1;
            mPausedTimeMs = 0;
            mLastCurrentPositionUpdateTimeMs = System.currentTimeMillis();
        }

        @Override
        public void onTimeShiftSeekTo(long timeMs) {
            mLastCurrentPositionUpdateTimeMs = System.currentTimeMillis();
            mCurrentPositionMs = Math.max(mRecordStartTimeMs,
                    Math.min(timeMs, mLastCurrentPositionUpdateTimeMs));
        }

        @Override
        public void onTimeShiftSetPlaybackParams(PlaybackParams params) {
            mSpeed = params.getSpeed();
        }

        private final class DrawRunnable implements Runnable {
            private volatile boolean mIsCanceled = false;

            @Override
            public void run() {
                if (mIsCanceled) {
                    return;
                }
                if (DEBUG) {
                    Log.v(TAG, "Draw task running");
                }
                boolean updatedState = false;
                ChannelState oldState;
                ChannelState newState = null;
                Surface currentSurface;
                ChannelInfo currentChannel;

                synchronized (this) {
                    oldState = mCurrentState;
                    currentSurface = mSurface;
                    currentChannel = mChannel;
                    if (currentChannel != null) {
                        newState = mBackend.getChannelState(currentChannel.originalNetworkId);
                        if (oldState == null || newState.getVersion() > oldState.getVersion()) {
                            mCurrentState = newState;
                            updatedState = true;
                        }
                    } else {
                        mCurrentState = null;
                    }
                }

                draw(currentSurface, currentChannel);
                if (updatedState) {
                    update(oldState, newState, currentChannel);
                }

                if (!mIsCanceled) {
                    mHandler.postDelayed(this, REFRESH_DELAY_MS);
                }
            }

            private void update(ChannelState oldState, ChannelState newState,
                    ChannelInfo currentChannel) {
                Log.i(TAG, "Updating channel " + currentChannel.number + " state to " + newState);
                notifyTracksChanged(newState.getTrackInfoList());
                if (oldState == null || oldState.getTuneStatus() != newState.getTuneStatus()) {
                    if (newState.getTuneStatus() == ChannelState.TUNE_STATUS_VIDEO_AVAILABLE) {
                        notifyVideoAvailable();
                        //TODO handle parental controls.
                        notifyContentAllowed();
                        setAudioTrack(newState.getSelectedAudioTrackId());
                        setVideoTrack(newState.getSelectedVideoTrackId());
                    } else {
                        notifyVideoUnavailable(newState.getTuneStatus());
                    }
                }
            }

            private void draw(Surface surface, ChannelInfo currentChannel) {
                if (surface != null) {
                    String now = HAS_TIME_SHIFT_API
                            ? new Date(mCurrentPositionMs).toString() : new Date().toString();
                    String name = currentChannel == null ? "Null" : currentChannel.name;
                    Canvas c = surface.lockCanvas(null);
                    c.drawColor(0xFF888888);
                    c.drawText(name, 100f, 200f, mTextPaint);
                    c.drawText(now, 100f, 400f, mTextPaint);
                    surface.unlockCanvasAndPost(c);
                    if (DEBUG) {
                        Log.v(TAG, "Post to canvas");
                    }
                } else {
                    if (DEBUG) {
                        Log.v(TAG, "No surface");
                    }
                }
            }

            public void cancel() {
                mIsCanceled = true;
            }
        }
    }
}