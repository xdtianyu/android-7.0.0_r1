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

package com.android.usbtuner.tvinput;

import android.content.ContentResolver;
import android.content.ContentUris;
import android.content.ContentValues;
import android.content.Context;
import android.database.Cursor;
import android.media.MediaDataSource;
import android.media.tv.TvContract;
import android.media.tv.TvInputManager;
import android.net.Uri;
import android.os.Handler;
import android.os.HandlerThread;
import android.os.Looper;
import android.os.Message;
import android.support.annotation.IntDef;
import android.support.annotation.Nullable;
import android.util.Log;
import android.widget.Toast;

import com.google.android.exoplayer.util.Assertions;
import com.android.tv.common.recording.RecordedProgram;
import com.android.tv.common.recording.RecordingCapability;
import com.android.usbtuner.DvbDeviceAccessor;
import com.android.usbtuner.TunerHal;
import com.android.usbtuner.UsbTunerDataSource;
import com.android.usbtuner.data.PsipData;
import com.android.usbtuner.data.TunerChannel;
import com.android.usbtuner.exoplayer.Recorder;
import com.android.usbtuner.exoplayer.cache.CacheManager;
import com.android.usbtuner.exoplayer.cache.DvrStorageManager;

import java.io.File;
import java.io.IOException;
import java.lang.annotation.Retention;
import java.lang.annotation.RetentionPolicy;
import java.util.List;
import java.util.Locale;
import java.util.Random;

/**
 * Implements a DVR feature.
 */
public class TunerRecordingSessionWorker implements PlaybackCacheListener,
        EventDetector.EventListener, Recorder.RecordListener,
        Handler.Callback {
    private static String TAG = "TunerRecordingSessionWorker";
    private static final boolean DEBUG = false;

    private static final String SORT_BY_TIME = TvContract.Programs.COLUMN_START_TIME_UTC_MILLIS
            + ", " + TvContract.Programs.COLUMN_CHANNEL_ID + ", "
            + TvContract.Programs.COLUMN_END_TIME_UTC_MILLIS;
    private static final int MSG_CONNECT = 1;
    private static final int MSG_DISCONNECT = 2;
    private static final int MSG_START_RECORDING = 3;
    private static final int MSG_STOP_RECORDING = 4;
    private static final int MSG_RECORDING_RESULT = 5;
    private static final int MSG_DELETE_RECORDING = 6;
    private static final int MSG_RELEASE = 7;
    private RecordingCapability mCapabilities;

    public RecordingCapability getCapabilities() {
        return mCapabilities;
    }

    @IntDef({STATE_IDLE, STATE_CONNECTED, STATE_RECORDING})
    @Retention(RetentionPolicy.SOURCE)
    public @interface DvrSessionState {}
    private static final int STATE_IDLE = 1;
    private static final int STATE_CONNECTED = 2;
    private static final int STATE_RECORDING = 3;

    private static final long CHANNEL_ID_NONE = -1;

    private final Context mContext;
    private final ChannelDataManager mChannelDataManager;
    private final Handler mHandler;
    private final Random mRandom = new Random();

    private TunerHal mTunerHal;
    private UsbTunerDataSource mTunerSource;
    private TunerChannel mChannel;
    private File mStorageDir;
    private long mRecordStartTime;
    private long mRecordEndTime;
    private CacheManager mCacheManager;
    private Recorder mRecorder;
    private final TunerRecordingSession mSession;
    @DvrSessionState private int mSessionState = STATE_IDLE;
    private final String mInputId;

    public TunerRecordingSessionWorker(Context context, String inputId,
            ChannelDataManager dataManager, TunerRecordingSession session) {
        mRandom.setSeed(System.nanoTime());
        mContext = context;
        HandlerThread handlerThread = new HandlerThread(TAG);
        handlerThread.start();
        mHandler = new Handler(handlerThread.getLooper(), this);
        mChannelDataManager = dataManager;
        mChannelDataManager.checkDataVersion(context);
        mCapabilities = new DvbDeviceAccessor(context).getRecordingCapability(inputId);
        mInputId = inputId;
        if (DEBUG) Log.d(TAG, mCapabilities.toString());
        mSession = session;
    }

    // PlaybackCacheListener
    @Override
    public void onCacheStartTimeChanged(long startTimeMs) {
    }

    @Override
    public void onCacheStateChanged(boolean available) {
    }

    @Override
    public void onDiskTooSlow() {
    }

    // EventDetector.EventListener
    @Override
    public void onChannelDetected(TunerChannel channel, boolean channelArrivedAtFirstTime) {
        if (mChannel == null || mChannel.compareTo(channel) != 0) {
            return;
        }
        mChannelDataManager.notifyChannelDetected(channel, channelArrivedAtFirstTime);
    }

    @Override
    public void onEventDetected(TunerChannel channel, List<PsipData.EitItem> items) {
        if (mChannel == null || mChannel.compareTo(channel) != 0) {
            return;
        }
        mChannelDataManager.notifyEventDetected(channel, items);
    }

    public void connect(Uri channelUri) {
        mHandler.removeCallbacksAndMessages(null);
        mHandler.obtainMessage(MSG_CONNECT, channelUri).sendToTarget();
    }

    public void disconnect() {
        mHandler.sendEmptyMessage(MSG_DISCONNECT);
    }

    public void startRecording() {
        mHandler.sendEmptyMessage(MSG_START_RECORDING);
    }

    public void stopRecording() {
        mHandler.sendEmptyMessage(MSG_STOP_RECORDING);
    }

    public void notifyRecordingFinished(boolean success) {
        mHandler.obtainMessage(MSG_RECORDING_RESULT, success).sendToTarget();
    }

    public void deleteRecording(Uri mediaUri) {
        mHandler.obtainMessage(MSG_DELETE_RECORDING, mediaUri).sendToTarget();
    }

    public void release() {
        mHandler.removeCallbacksAndMessages(null);
        mHandler.sendEmptyMessage(MSG_RELEASE);
    }

    @Override
    public boolean handleMessage(Message msg) {
        // TODO: Add RecordStopped status
        switch (msg.what) {
            case MSG_CONNECT: {
                Uri channelUri = (Uri) msg.obj;
                if (onConnect(channelUri)) {
                    mSession.onTuned(channelUri);
                } else {
                    Log.w(TAG, "Recording session connect failed");
                    mSession.onConnectFailed();
                }
                return true;
            }
            case MSG_START_RECORDING: {
                if(onStartRecording()) {
                    Toast.makeText(mContext, "USB TV tuner: Recording started",
                            Toast.LENGTH_SHORT).show();
                }
                else {
                    mSession.onRecordUnexpectedlyStopped(TvInputManager.RECORDING_ERROR_UNKNOWN);
                }
                return true;
            }
            case MSG_DISCONNECT: {
                return true;
            }
            case MSG_STOP_RECORDING: {
                onStopRecording();
                new Handler(Looper.getMainLooper()).post(new Runnable() {
                    @Override
                    public void run() {
                        Toast.makeText(mContext, "USB TV tuner: Recording stopped",
                                Toast.LENGTH_SHORT).show();
                    }
                });
                return true;
            }
            case MSG_RECORDING_RESULT: {
                onRecordingResult((Boolean) msg.obj);
                return true;
            }
            case MSG_DELETE_RECORDING: {
                Uri toDelete = (Uri) msg.obj;
                onDeleteRecording(toDelete);
                return true;
            }
            case MSG_RELEASE: {
                onRelease();
                return true;
            }
        }
        return false;
    }

    @Nullable
    private TunerChannel getChannel(Uri channelUri) {
        if (channelUri == null) {
            return null;
        }
        long channelId;
        try {
            channelId = ContentUris.parseId(channelUri);
        } catch (UnsupportedOperationException | NumberFormatException e) {
            channelId = CHANNEL_ID_NONE;
        }
        return (channelId == CHANNEL_ID_NONE) ? null : mChannelDataManager.getChannel(channelId);
    }

    private String getStorageKey() {
        long prefix = System.currentTimeMillis();
        int suffix = mRandom.nextInt();
        return String.format(Locale.ENGLISH, "%016x_%016x", prefix, suffix);
    }

    private File getMediaDir(String storageKey) {
        return new File(mContext.getCacheDir().getAbsolutePath() + "/recording/" + storageKey);
    }

    private File getMediaDir(Uri mediaUri) {
        String mediaPath = mediaUri.getPath();
        if (mediaPath == null || mediaPath.length() == 0) {
            return null;
        }
        return new File(mContext.getCacheDir().getAbsolutePath() + "/recording" +
                mediaUri.getPath());
    }

    private void reset() {
        if (mRecorder != null) {
            mRecorder.release();
            mRecorder = null;
        }
        if (mCacheManager != null) {
            mCacheManager.close();
            mCacheManager = null;
        }
        if (mTunerSource != null) {
            mTunerSource.stopStream();
            mTunerSource = null;
        }
        if (mTunerHal != null) {
            try {
                mTunerHal.close();
            } catch (Exception ex) {
                Log.e(TAG, "Error on closing tuner HAL.", ex);
            }
            mTunerHal = null;
        }
        mSessionState = STATE_IDLE;
    }

    private void resetRecorder() {
        Assertions.checkArgument(mSessionState != STATE_IDLE);
        if (mRecorder != null) {
            mRecorder.release();
            mRecorder = null;
        }
        if (mCacheManager != null) {
            mCacheManager.close();
            mCacheManager = null;
        }
        if (mTunerSource != null) {
            mTunerSource.stopStream();
            mTunerSource = null;
        }
        mSessionState = STATE_CONNECTED;
    }

    private boolean onConnect(Uri channelUri) {
        if (mSessionState == STATE_RECORDING) {
            return false;
        }
        mChannel = getChannel(channelUri);
        if (mChannel == null) {
            Log.w(TAG, "Failed to start recording. Couldn't find the channel for " + mChannel);
            return false;
        }
        if (mSessionState == STATE_CONNECTED) {
            return true;
        }
        mTunerHal = TunerHal.createInstance(mContext);
        if (mTunerHal == null) {
            Log.w(TAG, "Failed to start recording. Couldn't open a DVB device");
            reset();
            return false;
        }
        mSessionState = STATE_CONNECTED;
        return true;
    }

    private boolean onStartRecording() {
        if (mSessionState != STATE_CONNECTED) {
            return false;
        }
        mStorageDir = getMediaDir(getStorageKey());
        mTunerSource = new UsbTunerDataSource(mTunerHal, this);
        if (!mTunerSource.tuneToChannel(mChannel)) {
            Log.w(TAG, "Failed to start recording. Couldn't tune to the channel for " +
                    mChannel.toString());
            resetRecorder();
            return false;
        }
        mCacheManager = new CacheManager(new DvrStorageManager(mStorageDir, true));
        mTunerSource.startStream();
        mRecordStartTime = System.currentTimeMillis();
        mRecorder = new Recorder((MediaDataSource) mTunerSource,
                mCacheManager, this, this);
        try {
            mRecorder.prepare();
        } catch (IOException e) {
            Log.w(TAG, "Failed to start recording. Couldn't prepare a extractor");
            resetRecorder();
            return false;
        }
        mSessionState = STATE_RECORDING;
        return true;
    }

    private void onStopRecording() {
        if (mSessionState != STATE_RECORDING) {
            return;
        }
        // Do not change session status.
        if (mRecorder != null) {
            mRecorder.release();
            mRecordEndTime = System.currentTimeMillis();
            mRecorder = null;
        }
    }

    private static class Program {
        private long mChannelId;
        private String mTitle;
        private String mEpisodeTitle;
        private int mSeasonNumber;
        private int mEpisodeNumber;
        private String mDescription;
        private String mPosterArtUri;
        private String mThumbnailUri;
        private String mCanonicalGenres;
        private String mContentRatings;
        private long mStartTimeUtcMillis;
        private long mEndTimeUtcMillis;
        private long mVideoWidth;
        private long mVideoHeight;

        private static final String[] PROJECTION = {
                TvContract.Programs.COLUMN_CHANNEL_ID,
                TvContract.Programs.COLUMN_TITLE,
                TvContract.Programs.COLUMN_EPISODE_TITLE,
                TvContract.Programs.COLUMN_SEASON_NUMBER,
                TvContract.Programs.COLUMN_EPISODE_NUMBER,
                TvContract.Programs.COLUMN_SHORT_DESCRIPTION,
                TvContract.Programs.COLUMN_POSTER_ART_URI,
                TvContract.Programs.COLUMN_THUMBNAIL_URI,
                TvContract.Programs.COLUMN_CANONICAL_GENRE,
                TvContract.Programs.COLUMN_CONTENT_RATING,
                TvContract.Programs.COLUMN_START_TIME_UTC_MILLIS,
                TvContract.Programs.COLUMN_END_TIME_UTC_MILLIS,
                TvContract.Programs.COLUMN_VIDEO_WIDTH,
                TvContract.Programs.COLUMN_VIDEO_HEIGHT
        };

        public Program(Cursor cursor) {
            int index = 0;
            mChannelId = cursor.getLong(index++);
            mTitle = cursor.getString(index++);
            mEpisodeTitle = cursor.getString(index++);
            mSeasonNumber = cursor.getInt(index++);
            mEpisodeNumber = cursor.getInt(index++);
            mDescription = cursor.getString(index++);
            mPosterArtUri = cursor.getString(index++);
            mThumbnailUri = cursor.getString(index++);
            mCanonicalGenres = cursor.getString(index++);
            mContentRatings = cursor.getString(index++);
            mStartTimeUtcMillis = cursor.getLong(index++);
            mEndTimeUtcMillis = cursor.getLong(index++);
            mVideoWidth = cursor.getLong(index++);
            mVideoHeight = cursor.getLong(index++);
        }

        public Program(long channelId) {
            mChannelId = channelId;
            mTitle = "Unknown";
            mEpisodeTitle = "";
            mSeasonNumber = 0;
            mEpisodeNumber = 0;
            mDescription = "Unknown";
            mPosterArtUri = null;
            mThumbnailUri = null;
            mCanonicalGenres = null;
            mContentRatings = null;
            mStartTimeUtcMillis = 0;
            mEndTimeUtcMillis = 0;
            mVideoWidth = 0;
            mVideoHeight = 0;
        }

        public static Program onQuery(Cursor c) {
            Program program = null;
            if (c != null && c.moveToNext()) {
                program = new Program(c);
            }
            return program;
        }

        public ContentValues buildValues() {
            ContentValues values = new ContentValues();
            values.put(PROJECTION[0], mChannelId);
            values.put(PROJECTION[1], mTitle);
            values.put(PROJECTION[2], mEpisodeTitle);
            values.put(PROJECTION[3], mSeasonNumber);
            values.put(PROJECTION[4], mEpisodeNumber);
            values.put(PROJECTION[5], mDescription);
            values.put(PROJECTION[6], mPosterArtUri);
            values.put(PROJECTION[7], mThumbnailUri);
            values.put(PROJECTION[8], mCanonicalGenres);
            values.put(PROJECTION[9], mContentRatings);
            values.put(PROJECTION[10], mStartTimeUtcMillis);
            values.put(PROJECTION[11], mEndTimeUtcMillis);
            values.put(PROJECTION[12], mVideoWidth);
            values.put(PROJECTION[13], mVideoHeight);
            return values;
        }
    }

    private Program getRecordedProgram() {
        ContentResolver resolver = mContext.getContentResolver();
        long avg = mRecordStartTime / 2 + mRecordEndTime / 2;
        Uri programUri = TvContract.buildProgramsUriForChannel(mChannel.getChannelId(), avg, avg);
        try (Cursor c = resolver.query(programUri, Program.PROJECTION, null, null, SORT_BY_TIME)) {
            if (c != null) {
                Program result = Program.onQuery(c);
                if (DEBUG) {
                    Log.v(TAG, "Finished query for " + this);
                }
                return result;
            } else {
                if (c == null) {
                    Log.e(TAG, "Unknown query error for " + this);
                } else {
                    if (DEBUG) {
                        Log.d(TAG, "Canceled query for " + this);
                    }
                }
                return null;
            }
        }
    }

    private Uri insertRecordedProgram(Program program, long channelId, String storageUri,
            long totalBytes, long startTime, long endTime) {
        RecordedProgram recordedProgram = RecordedProgram.builder()
                .setInputId(mInputId)
                .setChannelId(channelId)
                .setDataUri(storageUri)
                .setDurationMillis(endTime - startTime)
                .setDataBytes(totalBytes)
                .build();
        Uri uri = mContext.getContentResolver().insert(TvContract.RecordedPrograms.CONTENT_URI,
                RecordedProgram.toValues(recordedProgram));
        return uri;
    }

    private boolean onRecordingResult(boolean success) {
        if (mSessionState == STATE_RECORDING && success) {
            Uri uri = insertRecordedProgram(getRecordedProgram(), mChannel.getChannelId(),
                    mStorageDir.toURI().toString(), 1024 * 1024,
                    mRecordStartTime, mRecordEndTime);
            if (uri != null) {
                mSession.onRecordFinished(uri);
            }
            resetRecorder();
            return true;
        }

        if (mSessionState == STATE_RECORDING) {
            mSession.onRecordUnexpectedlyStopped(TvInputManager.RECORDING_ERROR_UNKNOWN);
            Log.w(TAG, "Recording failed: " + mChannel == null ? "" : mChannel.toString());
            resetRecorder();
        } else {
            Log.e(TAG, "Recording session status abnormal");
            reset();
        }
        return false;
    }

    private void onDeleteRecording(Uri mediaUri) {
        // TODO: notify the deletion result to LiveChannels
        File mediaDir = getMediaDir(mediaUri);
        if (mediaDir == null) {
            return;
        }
        for(File file: mediaDir.listFiles()) {
            file.delete();
        }
        mediaDir.delete();
    }

    private void onRelease() {
        // Current recording will be canceled.
        reset();
        mHandler.getLooper().quitSafely();
        // TODO: Remove failed recording files.
    }
}
