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
import android.content.Context;
import android.database.Cursor;
import android.media.MediaDataSource;
import android.media.MediaFormat;
import android.media.PlaybackParams;
import android.media.tv.TvContentRating;
import android.media.tv.TvContract;
import android.media.tv.TvInputManager;
import android.media.tv.TvTrackInfo;
import android.net.Uri;
import android.os.Handler;
import android.os.HandlerThread;
import android.os.Message;
import android.os.SystemClock;
import android.text.Html;
import android.util.Log;
import android.util.Pair;
import android.util.Size;
import android.util.SparseArray;
import android.view.Surface;
import android.view.accessibility.CaptioningManager;

import com.google.android.exoplayer.audio.AudioCapabilities;
import com.android.tv.common.TvContentRatingCache;
import com.android.usbtuner.FileDataSource;
import com.android.usbtuner.InputStreamSource;
import com.android.usbtuner.TunerHal;
import com.android.usbtuner.UsbTunerDataSource;
import com.android.usbtuner.data.Cea708Data;
import com.android.usbtuner.data.Channel;
import com.android.usbtuner.data.PsipData.EitItem;
import com.android.usbtuner.data.PsipData.TvTracksInterface;
import com.android.usbtuner.data.Track.AtscAudioTrack;
import com.android.usbtuner.data.Track.AtscCaptionTrack;
import com.android.usbtuner.data.TunerChannel;
import com.android.usbtuner.exoplayer.MpegTsPassthroughAc3RendererBuilder;
import com.android.usbtuner.exoplayer.MpegTsPlayer;
import com.android.usbtuner.exoplayer.cache.CacheManager;
import com.android.usbtuner.exoplayer.cache.DvrStorageManager;
import com.android.usbtuner.util.IsoUtils;
import com.android.usbtuner.util.StatusTextUtils;

import junit.framework.Assert;

import java.io.File;
import java.io.FileNotFoundException;
import java.io.IOException;
import java.util.ArrayList;
import java.util.Iterator;
import java.util.List;
import java.util.Objects;
import java.util.concurrent.CountDownLatch;

/**
 * {@link TunerSessionWorker} implements a handler thread which processes TV input jobs
 * such as handling {@link ExoPlayer}, managing a tuner device, trickplay, and so on.
 */
public class TunerSessionWorker implements PlaybackCacheListener,
        MpegTsPlayer.VideoEventListener, MpegTsPlayer.Listener, EventDetector.EventListener,
        ChannelDataManager.ProgramInfoListener, Handler.Callback {
    private static final String TAG = "TunerSessionWorker";
    private static final boolean DEBUG = false;
    private static final boolean ENABLE_PROFILER = true;
    private static final String PLAY_FROM_CHANNEL = "channel";

    // Public messages
    public static final int MSG_SELECT_TRACK = 1;
    public static final int MSG_SET_CAPTION_ENABLED = 2;
    public static final int MSG_SET_SURFACE = 3;
    public static final int MSG_SET_STREAM_VOLUME = 4;
    public static final int MSG_TIMESHIFT_PAUSE = 5;
    public static final int MSG_TIMESHIFT_RESUME = 6;
    public static final int MSG_TIMESHIFT_SEEK_TO = 7;
    public static final int MSG_TIMESHIFT_SET_PLAYBACKPARAMS = 8;
    public static final int MSG_AUDIO_CAPABILITIES_CHANGED = 9;
    public static final int MSG_UNBLOCKED_RATING = 10;

    // Private messages
    private static final int MSG_TUNE = 1000;
    private static final int MSG_RELEASE = 1001;
    private static final int MSG_RETRY_PLAYBACK = 1002;
    private static final int MSG_START_PLAYBACK = 1003;
    private static final int MSG_PLAYBACK_STATE_CHANGED = 1004;
    private static final int MSG_PLAYBACK_ERROR = 1005;
    private static final int MSG_PLAYBACK_VIDEO_SIZE_CHANGED = 1006;
    private static final int MSG_AUDIO_UNPLAYABLE = 1007;
    private static final int MSG_UPDATE_PROGRAM = 1008;
    private static final int MSG_SCHEDULE_OF_PROGRAMS = 1009;
    private static final int MSG_UPDATE_CHANNEL_INFO = 1010;
    private static final int MSG_TRICKPLAY = 1011;
    private static final int MSG_DRAWN_TO_SURFACE = 1012;
    private static final int MSG_PARENTAL_CONTROLS = 1013;
    private static final int MSG_RESCHEDULE_PROGRAMS = 1014;
    private static final int MSG_CACHE_START_TIME_CHANGED = 1015;
    private static final int MSG_CHECK_SIGNAL = 1016;
    private static final int MSG_DISCOVER_CAPTION_SERVICE_NUMBER = 1017;
    private static final int MSG_RECOVER_STOPPED_PLAYBACK = 1018;
    private static final int MSG_CACHE_STATE_CHANGED = 1019;
    private static final int MSG_PROGRAM_DATA_RESULT = 1020;
    private static final int MSG_STOP_TUNE = 1021;

    private static final int TS_PACKET_SIZE = 188;
    private static final int CHECK_NO_SIGNAL_INITIAL_DELAY_MS = 4000;
    private static final int CHECK_NO_SIGNAL_PERIOD_MS = 500;
    private static final int RECOVER_STOPPED_PLAYBACK_PERIOD_MS = 2500;
    private static final int PARENTAL_CONTROLS_INTERVAL_MS = 5000;
    private static final int RESCHEDULE_PROGRAMS_INITIAL_DELAY_MS = 4000;
    private static final int RESCHEDULE_PROGRAMS_INTERVAL_MS = 10000;
    private static final int RESCHEDULE_PROGRAMS_TOLERANCE_MS = 2000;
    private static final int MAX_RETRY_COUNT = 2;

    // Some examples of the track ids of the audio tracks, "a0", "a1", "a2".
    // The number after prefix is being used for indicating a index of the given audio track.
    private static final String AUDIO_TRACK_PREFIX = "a";

    // Some examples of the tracks id of the caption tracks, "s1", "s2", "s3".
    // The number after prefix is being used for indicating a index of a caption service number
    // of the given caption track.
    private static final String SUBTITLE_TRACK_PREFIX = "s";
    private static final int TRACK_PREFIX_SIZE = 1;
    private static final String VIDEO_TRACK_ID = "v";
    private static final long CACHE_UNDERFLOW_BUFFER_MS = 5000;

    // Actual interval would be divided by the speed.
    private static final int TRICKPLAY_SEEK_INTERVAL_MS = 2000;
    private static final int MIN_TRICKPLAY_SEEK_INTERVAL_MS = 500;

    private final Context mContext;
    private final ChannelDataManager mChannelDataManager;
    private final TunerHal mTunerHal;
    private UsbTunerDataSource mTunerSource;
    private FileDataSource mFileSource;
    private InputStreamSource mSource;
    private Surface mSurface;
    private int mPlayerGeneration;
    private int mPreparingGeneration;
    private int mEndedGeneration;
    private volatile MpegTsPlayer mPlayer;
    private volatile TunerChannel mChannel;
    private String mRecordingId;
    private volatile Long mRecordingDuration;
    private final Handler mHandler;
    private int mRetryCount;
    private float mVolume;
    private final ArrayList<TvTrackInfo> mTvTracks;
    private SparseArray<AtscAudioTrack> mAudioTrackMap;
    private SparseArray<AtscCaptionTrack> mCaptionTrackMap;
    private AtscCaptionTrack mCaptionTrack;
    private boolean mCaptionEnabled;
    private volatile long mRecordStartTimeMs;
    private volatile long mCacheStartTimeMs;
    private PlaybackParams mPlaybackParams = new PlaybackParams();
    private boolean mPlayerStarted = false;
    private boolean mReportedDrawnToSurface = false;
    private boolean mReportedSignalAvailable = false;
    private EitItem mProgram;
    private List<EitItem> mPrograms;
    private TvInputManager mTvInputManager;
    private boolean mChannelBlocked;
    private TvContentRating mUnblockedContentRating;
    private long mLastPositionMs;
    private AudioCapabilities mAudioCapabilities;
    private final CountDownLatch mReleaseLatch = new CountDownLatch(1);
    private long mLastLimitInBytes = 0L;
    private long mLastPositionInBytes = 0L;
    private final CacheManager mCacheManager;
    private final TvContentRatingCache mTvContentRatingCache = TvContentRatingCache.getInstance();
    private final TunerSession mSession;

    public TunerSessionWorker(Context context, ChannelDataManager channelDataManager,
                CacheManager cacheManager, TunerSession tunerSession) {
        mContext = context;
        mTunerHal = TunerHal.createInstance(context);
        if (mTunerHal == null) {
            throw new RuntimeException("Failed to open a DVB device");
        }

        // HandlerThread should be set up before it is registered as a listener in the all other
        // components.
        HandlerThread handlerThread = new HandlerThread(TAG);
        handlerThread.start();
        mHandler = new Handler(handlerThread.getLooper(), this);
        mSession = tunerSession;
        mChannelDataManager = channelDataManager;
        // TODO: need to refactor it for multi-tuner support.
        mChannelDataManager.setListener(this);
        mChannelDataManager.checkDataVersion(mContext);
        mTvInputManager = (TvInputManager) context.getSystemService(Context.TV_INPUT_SERVICE);
        mTunerSource = new UsbTunerDataSource(mTunerHal, this);
        mFileSource = new FileDataSource(this);
        mVolume = 1.0f;
        mTvTracks = new ArrayList<>();
        mAudioTrackMap = new SparseArray<>();
        mCaptionTrackMap = new SparseArray<>();
        CaptioningManager captioningManager =
                (CaptioningManager) context.getSystemService(Context.CAPTIONING_SERVICE);
        mCaptionEnabled = captioningManager.isEnabled();
        mPlaybackParams.setSpeed(1.0f);
        mCacheManager = cacheManager;
    }

    // Public methods
    public void tune(Uri channelUri) {
        if (mSurface != null) {  // To avoid removing MSG_SET_SURFACE
            mHandler.removeCallbacksAndMessages(null);
        }
        sendMessage(MSG_TUNE, channelUri);
    }

    public void stopTune() {
        mHandler.removeCallbacksAndMessages(null);
        sendMessage(MSG_STOP_TUNE);
    }

    public TunerChannel getCurrentChannel() {
        return mChannel;
    }

    public long getStartPosition() {
        return mCacheStartTimeMs;
    }


    private String getRecordingPath() {
        return Uri.parse(mRecordingId).getPath();
    }

    public Long getDurationForRecording() {
        return mRecordingDuration;
    }

    private Long getDurationForRecording(String recordingId) {
        try {
            DvrStorageManager storageManager =
                    new DvrStorageManager(new File(getRecordingPath()), false);
            Pair<String, MediaFormat> trackInfo = null;
            try {
                trackInfo = storageManager.readTrackInfoFile(false);
            } catch (FileNotFoundException e) {
            }
            if (trackInfo == null) {
                trackInfo = storageManager.readTrackInfoFile(true);
            }
            Long durationUs = trackInfo.second.getLong(MediaFormat.KEY_DURATION);
            // we need duration by milli for trickplay notification.
            return durationUs != null ? durationUs / 1000 : null;
        } catch (IOException e) {
            Log.e(TAG, "meta file for recording was not found: " + recordingId);
            return null;
        }
    }

    public long getCurrentPosition() {
        // TODO: More precise time may be necessary.
        MpegTsPlayer mpegTsPlayer = mPlayer;
        long currentTime = mpegTsPlayer != null
                ? mRecordStartTimeMs + mpegTsPlayer.getCurrentPosition() : mRecordStartTimeMs;
        if (DEBUG) {
            long systemCurrentTime = System.currentTimeMillis();
            Log.d(TAG, "currentTime = " + currentTime
                    + " ; System.currentTimeMillis() = " + systemCurrentTime
                    + " ; diff = " + (currentTime - systemCurrentTime));
        }
        return currentTime;
    }

    public void sendMessage(int messageType) {
        mHandler.sendEmptyMessage(messageType);
    }

    public void sendMessage(int messageType, Object object) {
        mHandler.obtainMessage(messageType, object).sendToTarget();
    }

    public void sendMessage(int messageType, int arg1, int arg2, Object object) {
        mHandler.obtainMessage(messageType, arg1, arg2, object).sendToTarget();
    }

    public void release() {
        mHandler.removeCallbacksAndMessages(null);
        mHandler.sendEmptyMessage(MSG_RELEASE);
        try {
            mReleaseLatch.await();
        } catch (InterruptedException e) {
            Log.e(TAG, "Couldn't wait for finish of MSG_RELEASE", e);
        } finally {
            mHandler.getLooper().quitSafely();
        }
    }

    // MpegTsPlayer.Listener
    @Override
    public void onStateChanged(int generation, boolean playWhenReady, int playbackState) {
        sendMessage(MSG_PLAYBACK_STATE_CHANGED, generation, playbackState, playWhenReady);
    }

    @Override
    public void onError(int generation, Exception e) {
        sendMessage(MSG_PLAYBACK_ERROR, generation, 0, e);
    }

    @Override
    public void onVideoSizeChanged(int generation, int width, int height, float pixelWidthHeight) {
        sendMessage(MSG_PLAYBACK_VIDEO_SIZE_CHANGED, generation, 0, new Size(width, height));
    }

    @Override
    public void onDrawnToSurface(MpegTsPlayer player, Surface surface) {
        sendMessage(MSG_DRAWN_TO_SURFACE, player);
    }

    @Override
    public void onAudioUnplayable(int generation) {
        sendMessage(MSG_AUDIO_UNPLAYABLE, generation);
    }

    // MpegTsPlayer.VideoEventListener
    @Override
    public void onEmitCaptionEvent(Cea708Data.CaptionEvent event) {
        mSession.sendUiMessage(TunerSession.MSG_UI_PROCESS_CAPTION_TRACK, event);
    }

    @Override
    public void onDiscoverCaptionServiceNumber(int serviceNumber) {
        sendMessage(MSG_DISCOVER_CAPTION_SERVICE_NUMBER, serviceNumber);
    }

    // ChannelDataManager.ProgramInfoListener
    @Override
    public void onProgramsArrived(TunerChannel channel, List<EitItem> programs) {
        sendMessage(MSG_SCHEDULE_OF_PROGRAMS, new Pair<>(channel, programs));
    }

    @Override
    public void onChannelArrived(TunerChannel channel) {
        sendMessage(MSG_UPDATE_CHANNEL_INFO, channel);
    }

    @Override
    public void onRescanNeeded() {
        mSession.sendUiMessage(TunerSession.MSG_UI_TOAST_RESCAN_NEEDED);
    }

    @Override
    public void onRequestProgramsResponse(TunerChannel channel, List<EitItem> programs) {
        sendMessage(MSG_PROGRAM_DATA_RESULT, new Pair<>(channel, programs));
    }

    // PlaybackCacheListener
    @Override
    public void onCacheStartTimeChanged(long startTimeMs) {
        sendMessage(MSG_CACHE_START_TIME_CHANGED, startTimeMs);
    }

    @Override
    public void onCacheStateChanged(boolean available) {
        sendMessage(MSG_CACHE_STATE_CHANGED, available);
    }

    @Override
    public void onDiskTooSlow() {
        sendMessage(MSG_RETRY_PLAYBACK, mPlayer);
    }

    // EventDetector.EventListener
    @Override
    public void onChannelDetected(TunerChannel channel, boolean channelArrivedAtFirstTime) {
        mChannelDataManager.notifyChannelDetected(channel, channelArrivedAtFirstTime);
    }

    @Override
    public void onEventDetected(TunerChannel channel, List<EitItem> items) {
        mChannelDataManager.notifyEventDetected(channel, items);
    }

    private long parseChannel(Uri uri) {
        try {
            List<String> paths = uri.getPathSegments();
            if (paths.size() > 1 && paths.get(0).equals(PLAY_FROM_CHANNEL)) {
                return ContentUris.parseId(uri);
            }
        } catch (UnsupportedOperationException | NumberFormatException e) {
        }
        return -1;
    }

    private static class RecordedProgram {
        private long mChannelId;
        private String mDataUri;

        private static final String[] PROJECTION = {
            TvContract.Programs.COLUMN_CHANNEL_ID,
            TvContract.RecordedPrograms.COLUMN_RECORDING_DATA_URI,
        };

        public RecordedProgram(Cursor cursor) {
            int index = 0;
            mChannelId = cursor.getLong(index++);
            mDataUri = cursor.getString(index++);
        }

        public RecordedProgram(long channelId, String dataUri) {
            mChannelId = channelId;
            mDataUri = dataUri;
        }

        public static RecordedProgram onQuery(Cursor c) {
            RecordedProgram recording = null;
            if (c != null && c.moveToNext()) {
                recording = new RecordedProgram(c);
            }
            return recording;
        }

        public String getDataUri() {
            return mDataUri;
        }
    }

    private RecordedProgram getRecordedProgram(Uri recordedUri) {
        ContentResolver resolver = mContext.getContentResolver();
        try(Cursor c = resolver.query(recordedUri, RecordedProgram.PROJECTION, null, null, null)) {
            if (c != null) {
                 RecordedProgram result = RecordedProgram.onQuery(c);
                if (DEBUG) {
                    Log.d(TAG, "Finished query for " + this);
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

    private String parseRecording(Uri uri) {
        RecordedProgram recording = getRecordedProgram(uri);
        if (recording != null) {
            return recording.getDataUri();
        }
        return null;
    }

    @Override
    public boolean handleMessage(Message msg) {
        switch (msg.what) {
            case MSG_TUNE: {
                if (DEBUG) Log.d(TAG, "MSG_TUNE");

                // When sequential tuning messages arrived, it skips middle tuning messages in order
                // to change to the last requested channel quickly.
                if (mHandler.hasMessages(MSG_TUNE)) {
                    return true;
                }
                Uri channelUri = (Uri) msg.obj;
                String recording = null;
                long channelId = parseChannel(channelUri);
                TunerChannel channel = (channelId == -1) ? null
                        : mChannelDataManager.getChannel(channelId);
                if (channelId == -1) {
                    recording = parseRecording(channelUri);
                }
                if (channel == null && recording == null) {
                    Log.w(TAG, "onTune() is failed. Can't find channel for " + channelUri);
                    stopTune();
                    mSession.notifyVideoUnavailable(
                            TvInputManager.VIDEO_UNAVAILABLE_REASON_UNKNOWN);
                    return true;
                }
                mHandler.removeCallbacksAndMessages(null);
                if (channel != null) {
                    mChannelDataManager.requestProgramsData(channel);
                }
                prepareTune(channel, recording);
                // TODO: Need to refactor. notifyContentAllowed() should not be called if parental
                // control is turned on.
                mSession.notifyContentAllowed();
                resetPlayback();
                resetTvTracks();
                mHandler.sendEmptyMessageDelayed(MSG_RESCHEDULE_PROGRAMS,
                        RESCHEDULE_PROGRAMS_INITIAL_DELAY_MS);
                mHandler.sendEmptyMessageDelayed(MSG_CHECK_SIGNAL,
                        CHECK_NO_SIGNAL_INITIAL_DELAY_MS);
                return true;
            }
            case MSG_STOP_TUNE: {
                if (DEBUG) {
                    Log.d(TAG, "MSG_STOP_TUNE");
                }
                mChannel = null;
                stopPlayback();
                stopCaptionTrack();
                resetTvTracks();
                mTunerHal.stopTune();
                mSource = null;
                mSession.notifyVideoUnavailable(
                        TvInputManager.VIDEO_UNAVAILABLE_REASON_UNKNOWN);
                return true;
            }
            case MSG_RELEASE: {
                if (DEBUG) {
                    Log.d(TAG, "MSG_RELEASE");
                }
                mHandler.removeCallbacksAndMessages(null);
                stopPlayback();
                stopCaptionTrack();
                try {
                    mTunerHal.close();
                } catch (Exception ex) {
                    Log.e(TAG, "Error on closing tuner HAL.", ex);
                }
                mSource = null;
                mReleaseLatch.countDown();
                return true;
            }
            case MSG_RETRY_PLAYBACK: {
                if (mPlayer == msg.obj) {
                    mHandler.removeMessages(MSG_RETRY_PLAYBACK);
                    mRetryCount++;
                    if (DEBUG) {
                        Log.d(TAG, "MSG_RETRY_PLAYBACK " + mRetryCount);
                    }
                    if (mRetryCount <= MAX_RETRY_COUNT) {
                        resetPlayback();
                    } else {
                        // When it reaches this point, it may be due to an error that occurred in
                        // the tuner device. Calling stopPlayback() and TunerHal.stopTune()
                        // resets the tuner device to recover from the error.
                        stopPlayback();
                        stopCaptionTrack();
                        mTunerHal.stopTune();

                        mSession.notifyVideoUnavailable(
                                TvInputManager.VIDEO_UNAVAILABLE_REASON_UNKNOWN);

                        // After MAX_RETRY_COUNT, give some delay of an empirically chosen value
                        // before recovering the playback.
                        mHandler.sendEmptyMessageDelayed(MSG_RECOVER_STOPPED_PLAYBACK,
                                RECOVER_STOPPED_PLAYBACK_PERIOD_MS);
                    }
                }
                return true;
            }
            case MSG_RECOVER_STOPPED_PLAYBACK: {
                if (DEBUG) {
                    Log.d(TAG, "MSG_RECOVER_STOPPED_PLAYBACK");
                }
                resetPlayback();
                return true;
            }
            case MSG_START_PLAYBACK: {
                if (DEBUG) {
                    Log.d(TAG, "MSG_START_PLAYBACK");
                }
                if (mChannel != null || mRecordingId != null) {
                    startPlayback(msg.obj);
                }
                return true;
            }
            case MSG_PLAYBACK_STATE_CHANGED: {
                int generation = msg.arg1;
                int playbackState = msg.arg2;
                boolean playWhenReady = (boolean) msg.obj;
                if (DEBUG) {
                    Log.d(TAG, "ExoPlayer state change: " + generation + " "
                            + playbackState + " " + playWhenReady);
                }

                // Generation starts from 1 not 0.
                if (playbackState == MpegTsPlayer.STATE_READY
                        && mPreparingGeneration == mPlayerGeneration) {
                    if (DEBUG) {
                        Log.d(TAG, "ExoPlayer ready: " + mPlayerGeneration);
                    }

                    // mPreparingGeneration was set to mPlayerGeneration in order to indicate that
                    // ExoPlayer is in its preparing status when MpegTsPlayer::prepare() was called.
                    // Now MpegTsPlayer::prepare() is finished. Clear preparing state in order to
                    // ensure another DO_START_PLAYBACK will not be sent for same generation.
                    mPreparingGeneration = 0;
                    sendMessage(MSG_START_PLAYBACK, mPlayer);
                } else if (playbackState == MpegTsPlayer.STATE_ENDED
                        && mEndedGeneration != generation) {
                    // Final status
                    // notification of STATE_ENDED from MpegTsPlayer will be ignored afterwards.
                    mEndedGeneration = generation;
                    Log.i(TAG, "Player ended: end of stream " + generation);
                    sendMessage(MSG_RETRY_PLAYBACK, mPlayer);
                }
                return true;
            }
            case MSG_PLAYBACK_ERROR: {
                int generation = msg.arg1;
                Exception exception = (Exception) msg.obj;
                Log.i(TAG, "ExoPlayer Error: " + generation + " " + mPlayerGeneration);
                if (generation != mPlayerGeneration) {
                    return true;
                }
                mHandler.obtainMessage(MSG_RETRY_PLAYBACK, mPlayer).sendToTarget();
                return true;
            }
            case MSG_PLAYBACK_VIDEO_SIZE_CHANGED: {
                int generation = msg.arg1;
                Size size = (Size) msg.obj;
                if (generation != mPlayerGeneration) {
                    return true;
                }
                if (mChannel != null && mChannel.hasVideo()) {
                    updateVideoTrack(size.getWidth(), size.getHeight());
                }
                if (mRecordingId != null) {
                    updateVideoTrack(size.getWidth(), size.getHeight());
                }
                return true;
            }
            case MSG_AUDIO_UNPLAYABLE: {
                int generation = (int) msg.obj;
                if (mPlayer == null || generation != mPlayerGeneration) {
                    return true;
                }
                Log.i(TAG, "AC3 audio cannot be played due to device limitation");
                mSession.sendUiMessage(
                        TunerSession.MSG_UI_SHOW_AUDIO_UNPLAYABLE);
                return true;
            }
            case MSG_UPDATE_PROGRAM: {
                if (mChannel != null) {
                    EitItem program = (EitItem) msg.obj;
                    updateTvTracks(program);
                    mHandler.sendEmptyMessage(MSG_PARENTAL_CONTROLS);
                }
                return true;
            }
            case MSG_SCHEDULE_OF_PROGRAMS: {
                mHandler.removeMessages(MSG_UPDATE_PROGRAM);
                Pair<TunerChannel, List<EitItem>> pair =
                        (Pair<TunerChannel, List<EitItem>>) msg.obj;
                TunerChannel channel = pair.first;
                if (mChannel == null) {
                    return true;
                }
                if (mChannel != null && mChannel.compareTo(channel) != 0) {
                    return true;
                }
                mPrograms = pair.second;
                EitItem currentProgram = getCurrentProgram();
                if (currentProgram == null) {
                    mProgram = null;
                }
                long currentTimeMs = getCurrentPosition();
                if (mPrograms != null) {
                    for (EitItem item : mPrograms) {
                        if (currentProgram != null && currentProgram.compareTo(item) == 0) {
                            if (DEBUG) {
                                Log.d(TAG, "Update current TvTracks " + item);
                            }
                            if (mProgram != null && mProgram.compareTo(item) == 0) {
                                continue;
                            }
                            mProgram = item;
                            updateTvTracks(item);
                        } else if (item.getStartTimeUtcMillis() > currentTimeMs) {
                            if (DEBUG) {
                                Log.d(TAG, "Update next TvTracks " + item + " "
                                        + (item.getStartTimeUtcMillis() - currentTimeMs));
                            }
                            mHandler.sendMessageDelayed(
                                    mHandler.obtainMessage(MSG_UPDATE_PROGRAM, item),
                                    item.getStartTimeUtcMillis() - currentTimeMs);
                        }
                    }
                }
                mHandler.sendEmptyMessage(MSG_PARENTAL_CONTROLS);
                return true;
            }
            case MSG_UPDATE_CHANNEL_INFO: {
                TunerChannel channel = (TunerChannel) msg.obj;
                if (mChannel != null && mChannel.compareTo(channel) == 0) {
                    updateChannelInfo(channel);
                }
                return true;
            }
            case MSG_PROGRAM_DATA_RESULT: {
                TunerChannel channel = (TunerChannel) ((Pair) msg.obj).first;

                // If there already exists, skip it since real-time data is a top priority,
                if (mChannel != null && mChannel.compareTo(channel) == 0
                        && mPrograms == null && mProgram == null) {
                    sendMessage(MSG_SCHEDULE_OF_PROGRAMS, msg.obj);
                }
                return true;
            }
            case MSG_DRAWN_TO_SURFACE: {
                if (mPlayer == msg.obj && mSurface != null && mPlayerStarted) {
                    if (DEBUG) {
                        Log.d(TAG, "MSG_DRAWN_TO_SURFACE");
                    }
                    mCacheStartTimeMs = mRecordStartTimeMs =
                            (mRecordingId != null) ? 0 : System.currentTimeMillis();
                    mSession.notifyVideoAvailable();
                    mReportedDrawnToSurface = true;

                    // If surface is drawn successfully, it means that the playback was brought back
                    // to normal and therefore, the playback recovery status will be reset through
                    // setting a zero value to the retry count.
                    // TODO: Consider audio only channels for detecting playback status changes to
                    //       be normal.
                    mRetryCount = 0;
                    if (mCaptionEnabled && mCaptionTrack != null) {
                        startCaptionTrack();
                    } else {
                        stopCaptionTrack();
                    }
                }
                return true;
            }
            case MSG_TRICKPLAY: {
                doTrickplay(msg.arg1);
                return true;
            }
            case MSG_RESCHEDULE_PROGRAMS: {
                doReschedulePrograms();
                return true;
            }
            case MSG_PARENTAL_CONTROLS: {
                doParentalControls();
                mHandler.removeMessages(MSG_PARENTAL_CONTROLS);
                mHandler.sendEmptyMessageDelayed(MSG_PARENTAL_CONTROLS,
                        PARENTAL_CONTROLS_INTERVAL_MS);
                return true;
            }
            case MSG_UNBLOCKED_RATING: {
                mUnblockedContentRating = (TvContentRating) msg.obj;
                doParentalControls();
                mHandler.removeMessages(MSG_PARENTAL_CONTROLS);
                mHandler.sendEmptyMessageDelayed(MSG_PARENTAL_CONTROLS,
                        PARENTAL_CONTROLS_INTERVAL_MS);
                return true;
            }
            case MSG_DISCOVER_CAPTION_SERVICE_NUMBER: {
                int serviceNumber = (int) msg.obj;
                doDiscoverCaptionServiceNumber(serviceNumber);
                return true;
            }
            case MSG_SELECT_TRACK: {
                if (mChannel != null) {
                    doSelectTrack(msg.arg1, (String) msg.obj);
                } else if (mRecordingId != null) {
                    // TODO : mChannel == null && mRecordingId != null
                    Log.d(TAG, "track selected for recording");
                }
                return true;
            }
            case MSG_SET_CAPTION_ENABLED: {
                mCaptionEnabled = (boolean) msg.obj;
                if (mCaptionEnabled) {
                    startCaptionTrack();
                } else {
                    stopCaptionTrack();
                }
                return true;
            }
            case MSG_TIMESHIFT_PAUSE: {
                doTimeShiftPause();
                return true;
            }
            case MSG_TIMESHIFT_RESUME: {
                doTimeShiftResume();
                return true;
            }
            case MSG_TIMESHIFT_SEEK_TO: {
                doTimeShiftSeekTo((long) msg.obj);
                return true;
            }
            case MSG_TIMESHIFT_SET_PLAYBACKPARAMS: {
                doTimeShiftSetPlaybackParams((PlaybackParams) msg.obj);
                return true;
            }
            case MSG_AUDIO_CAPABILITIES_CHANGED: {
                AudioCapabilities capabilities = (AudioCapabilities) msg.obj;
                if (DEBUG) {
                    Log.d(TAG, "MSG_AUDIO_CAPABILITIES_CHANGED " + capabilities);
                }
                if (capabilities == null) {
                    return true;
                }
                if (!capabilities.equals(mAudioCapabilities)) {
                    // HDMI supported encodings are changed. restart player.
                    mAudioCapabilities = capabilities;
                    resetPlayback();
                }
                return true;
            }
            case MSG_SET_SURFACE: {
                Surface surface = (Surface) msg.obj;
                if (DEBUG) {
                    Log.d(TAG, "MSG_SET_SURFACE " + surface);
                }
                if (surface != null && !surface.isValid()) {
                    Log.w(TAG, "Ignoring invalid surface.");
                    return true;
                }
                mSurface = surface;
                resetPlayback();
                return true;
            }
            case MSG_SET_STREAM_VOLUME: {
                mVolume = (float) msg.obj;
                if (mPlayer != null && mPlayer.isPlaying()) {
                    mPlayer.setVolume(mVolume);
                }
                return true;
            }
            case MSG_CACHE_START_TIME_CHANGED: {
                if (mPlayer == null) {
                    return true;
                }
                mCacheStartTimeMs = (long) msg.obj;
                if (!hasEnoughBackwardCache()
                        && (!mPlayer.isPlaying() || mPlaybackParams.getSpeed() < 1.0f)) {
                    mPlayer.setPlayWhenReady(true);
                    mPlayer.setAudioTrack(true);
                    mPlaybackParams.setSpeed(1.0f);
                }
                return true;
            }
            case MSG_CACHE_STATE_CHANGED: {
                boolean available = (boolean) msg.obj;
                mSession.notifyTimeShiftStatusChanged(available
                        ? TvInputManager.TIME_SHIFT_STATUS_AVAILABLE
                        : TvInputManager.TIME_SHIFT_STATUS_UNAVAILABLE);
                return true;
            }
            case MSG_CHECK_SIGNAL: {
                if (mChannel == null) {
                    return true;
                }
                long limitInBytes = mSource != null ? mSource.getLimit() : 0L;
                long positionInBytes = mSource != null ? mSource.getPosition() : 0L;
                if (UsbTunerDebug.ENABLED) {
                    UsbTunerDebug.calculateDiff();
                    mSession.sendUiMessage(TunerSession.MSG_UI_SET_STATUS_TEXT,
                            Html.fromHtml(
                                    StatusTextUtils.getStatusWarningInHTML(
                                            (limitInBytes - mLastLimitInBytes)
                                                    / TS_PACKET_SIZE,
                                            UsbTunerDebug.getVideoFrameDrop(),
                                            UsbTunerDebug.getBytesInQueue(),
                                            UsbTunerDebug.getAudioPositionUs(),
                                            UsbTunerDebug.getAudioPositionUsRate(),
                                            UsbTunerDebug.getAudioPtsUs(),
                                            UsbTunerDebug.getAudioPtsUsRate(),
                                            UsbTunerDebug.getVideoPtsUs(),
                                            UsbTunerDebug.getVideoPtsUsRate()
                                    )));
                }
                if (DEBUG) {
                    Log.d(TAG, String.format("MSG_CHECK_SIGNAL position: %d, limit: %d",
                            positionInBytes, limitInBytes));
                }
                mSession.sendUiMessage(TunerSession.MSG_UI_HIDE_MESSAGE);
                if (mSource != null && mChannel.getType() == Channel.TYPE_TUNER
                        && positionInBytes == mLastPositionInBytes
                        && limitInBytes == mLastLimitInBytes) {
                    mSession.notifyVideoUnavailable(
                            TvInputManager.VIDEO_UNAVAILABLE_REASON_WEAK_SIGNAL);

                    mReportedSignalAvailable = false;
                } else {
                    if (mReportedDrawnToSurface && !mReportedSignalAvailable) {
                        mSession.notifyVideoAvailable();
                        mReportedSignalAvailable = true;
                    }
                }
                mLastLimitInBytes = limitInBytes;
                mLastPositionInBytes = positionInBytes;
                mHandler.sendEmptyMessageDelayed(MSG_CHECK_SIGNAL,
                        CHECK_NO_SIGNAL_PERIOD_MS);
                return true;
            }
            default: {
                Log.w(TAG, "Unhandled message code: " + msg.what);
                return false;
            }
        }
    }

    // Private methods
    private void doSelectTrack(int type, String trackId) {
        int numTrackId = trackId != null
                ? Integer.parseInt(trackId.substring(TRACK_PREFIX_SIZE)) : -1;
        if (type == TvTrackInfo.TYPE_AUDIO) {
            if (trackId == null) {
                return;
            }
            AtscAudioTrack audioTrack = mAudioTrackMap.get(numTrackId);
            if (audioTrack == null) {
                return;
            }
            int oldAudioPid = mChannel.getAudioPid();
            mChannel.selectAudioTrack(audioTrack.index);
            int newAudioPid = mChannel.getAudioPid();
            if (oldAudioPid != newAudioPid) {
                // TODO: Implement a switching between tracks more smoothly.
                resetPlayback();
            }
            mSession.notifyTrackSelected(type, trackId);
        } else if (type == TvTrackInfo.TYPE_SUBTITLE) {
            if (trackId == null) {
                mSession.notifyTrackSelected(type, null);
                mCaptionTrack = null;
                stopCaptionTrack();
                return;
            }
            for (TvTrackInfo track : mTvTracks) {
                if (track.getId().equals(trackId)) {
                    // The service number of the caption service is used for track id of a
                    // subtitle track. Passes the following track id on to TsParser.
                    mSession.notifyTrackSelected(type, trackId);
                    mCaptionTrack = mCaptionTrackMap.get(numTrackId);
                    startCaptionTrack();
                    return;
                }
            }
        }
    }

    private MpegTsPlayer createPlayer(AudioCapabilities capabilities, CacheManager cacheManager) {
        if (capabilities == null) {
            Log.w(TAG, "No Audio Capabilities");
        }
        ++mPlayerGeneration;

        MpegTsPlayer player = new MpegTsPlayer(mPlayerGeneration,
                new MpegTsPassthroughAc3RendererBuilder(mContext, cacheManager, this),
                mHandler, capabilities, this);
        Log.i(TAG, "Passthrough AC3 renderer");
        if (DEBUG) Log.d(TAG, "ExoPlayer created: " + mPlayerGeneration);
        return player;
    }

    private void startCaptionTrack() {
        if (mCaptionEnabled && mCaptionTrack != null) {
            mSession.sendUiMessage(
                    TunerSession.MSG_UI_START_CAPTION_TRACK, mCaptionTrack);
            if (mPlayer != null) {
                mPlayer.setCaptionServiceNumber(mCaptionTrack.serviceNumber);
            }
        }
    }

    private void stopCaptionTrack() {
        if (mPlayer != null) {
            mPlayer.setCaptionServiceNumber(Cea708Data.EMPTY_SERVICE_NUMBER);
        }
        mSession.sendUiMessage(TunerSession.MSG_UI_STOP_CAPTION_TRACK);
    }

    private void resetTvTracks() {
        mTvTracks.clear();
        mAudioTrackMap.clear();
        mCaptionTrackMap.clear();
        mSession.sendUiMessage(TunerSession.MSG_UI_RESET_CAPTION_TRACK);
        mSession.notifyTracksChanged(mTvTracks);
    }

    private void updateTvTracks(TvTracksInterface tvTracksInterface) {
        if (DEBUG) {
            Log.d(TAG, "UpdateTvTracks " + tvTracksInterface);
        }
        List<AtscAudioTrack> audioTracks = tvTracksInterface.getAudioTracks();
        List<AtscCaptionTrack> captionTracks = tvTracksInterface.getCaptionTracks();
        if (audioTracks != null && !audioTracks.isEmpty()) {
            updateAudioTracks(audioTracks);
        }
        if (captionTracks == null || captionTracks.isEmpty()) {
            if (tvTracksInterface.hasCaptionTrack()) {
                updateCaptionTracks(captionTracks);
            }
        } else {
            updateCaptionTracks(captionTracks);
        }
    }

    private void removeTvTracks(int trackType) {
        Iterator<TvTrackInfo> iterator = mTvTracks.iterator();
        while (iterator.hasNext()) {
            TvTrackInfo tvTrackInfo = iterator.next();
            if (tvTrackInfo.getType() == trackType) {
                iterator.remove();
            }
        }
    }

    private void updateVideoTrack(int width, int height) {
        removeTvTracks(TvTrackInfo.TYPE_VIDEO);
        mTvTracks.add(new TvTrackInfo.Builder(TvTrackInfo.TYPE_VIDEO, VIDEO_TRACK_ID)
                .setVideoWidth(width).setVideoHeight(height).build());
        mSession.notifyTracksChanged(mTvTracks);
        mSession.notifyTrackSelected(TvTrackInfo.TYPE_VIDEO, VIDEO_TRACK_ID);
    }

    private void updateAudioTracks(List<AtscAudioTrack> audioTracks) {
        if (DEBUG) {
            Log.d(TAG, "Update AudioTracks " + audioTracks);
        }
        removeTvTracks(TvTrackInfo.TYPE_AUDIO);
        mAudioTrackMap.clear();
        if (audioTracks != null) {
            int index = 0;
            for (AtscAudioTrack audioTrack : audioTracks) {
                String language = audioTrack.language;
                if (language == null && mChannel.getAudioTracks() != null
                        && mChannel.getAudioTracks().size() == audioTracks.size()) {
                    // If a language is not present, use a language field in PMT section parsed.
                    language = mChannel.getAudioTracks().get(index).language;
                }

                // Save the index to the audio track.
                // Later, when a audio track is selected, Both an audio pid and its audio stream
                // type reside in the selected index position of the tuner channel's audio data.
                audioTrack.index = index;
                TvTrackInfo.Builder builder = new TvTrackInfo.Builder(
                                TvTrackInfo.TYPE_AUDIO, AUDIO_TRACK_PREFIX + index);
                if (IsoUtils.isValidIso3Language(language)) {
                    builder.setLanguage(language);
                }
                if (audioTrack.channelCount != 0) {
                    builder.setAudioChannelCount(audioTrack.channelCount);
                }
                if (audioTrack.sampleRate != 0) {
                    builder.setAudioSampleRate(audioTrack.sampleRate);
                }
                TvTrackInfo track = builder.build();
                mTvTracks.add(track);
                mAudioTrackMap.put(index, audioTrack);
                ++index;
            }
        }
        mSession.notifyTracksChanged(mTvTracks);
    }

    private void updateCaptionTracks(List<AtscCaptionTrack> captionTracks) {
        if (DEBUG) {
            Log.d(TAG, "Update CaptionTrack " + captionTracks);
        }
        removeTvTracks(TvTrackInfo.TYPE_SUBTITLE);
        mCaptionTrackMap.clear();
        if (captionTracks != null) {
            for (AtscCaptionTrack captionTrack : captionTracks) {
                if (mCaptionTrackMap.indexOfKey(captionTrack.serviceNumber) >= 0) {
                    continue;
                }
                String language = captionTrack.language;

                // The service number of the caption service is used for track id of a subtitle.
                // Later, when a subtitle is chosen, track id will be passed on to TsParser.
                TvTrackInfo.Builder builder =
                        new TvTrackInfo.Builder(TvTrackInfo.TYPE_SUBTITLE,
                                SUBTITLE_TRACK_PREFIX + captionTrack.serviceNumber);
                if (IsoUtils.isValidIso3Language(language)) {
                    builder.setLanguage(language);
                }
                mTvTracks.add(builder.build());
                mCaptionTrackMap.put(captionTrack.serviceNumber, captionTrack);
            }
        }
        mSession.notifyTracksChanged(mTvTracks);
    }

    private void updateChannelInfo(TunerChannel channel) {
        if (DEBUG) {
            Log.d(TAG, String.format("Channel Info (old) videoPid: %d audioPid: %d " +
                    "audioSize: %d", mChannel.getVideoPid(), mChannel.getAudioPid(),
                    mChannel.getAudioPids().size()));
        }

        // The list of the audio tracks resided in a channel is often changed depending on a
        // program being on the air. So, we should update the streaming PIDs and types of the
        // tuned channel according to the newly received channel data.
        int oldVideoPid = mChannel.getVideoPid();
        int oldAudioPid = mChannel.getAudioPid();
        List<Integer> audioPids = channel.getAudioPids();
        List<Integer> audioStreamTypes = channel.getAudioStreamTypes();
        int size = audioPids.size();
        mChannel.setVideoPid(channel.getVideoPid());
        mChannel.setAudioPids(audioPids);
        mChannel.setAudioStreamTypes(audioStreamTypes);
        updateTvTracks(mChannel);
        int index = audioPids.isEmpty() ? -1 : 0;
        for (int i = 0; i < size; ++i) {
            if (audioPids.get(i) == oldAudioPid) {
                index = i;
                break;
            }
        }
        mChannel.selectAudioTrack(index);
        mSession.notifyTrackSelected(TvTrackInfo.TYPE_AUDIO,
                index == -1 ? null : AUDIO_TRACK_PREFIX + index);

        // Reset playback if there is a change in the listening streaming PIDs.
        if (oldVideoPid != mChannel.getVideoPid()
                || oldAudioPid != mChannel.getAudioPid()) {
            // TODO: Implement a switching between tracks more smoothly.
            resetPlayback();
        }
        if (DEBUG) {
            Log.d(TAG, String.format("Channel Info (new) videoPid: %d audioPid: %d " +
                    " audioSize: %d", mChannel.getVideoPid(), mChannel.getAudioPid(),
                    mChannel.getAudioPids().size()));
        }
    }

    private void stopPlayback() {
        if (mPlayer != null) {
            if (mSource != null) {
                mSource.stopStream();
            }
            mPlayer.setPlayWhenReady(false);
            mPlayer.release();
            mPlayer = null;
            mPlaybackParams.setSpeed(1.0f);
            mPlayerStarted = false;
            mReportedDrawnToSurface = false;
            mReportedSignalAvailable = false;
            mSession.sendUiMessage(TunerSession.MSG_UI_HIDE_AUDIO_UNPLAYABLE);
        }
    }

    private void startPlayback(Object playerObj) {
        // TODO: provide hasAudio()/hasVideo() for play recordings.
        if (mPlayer == null || mPlayer != playerObj) {
            return;
        }
        if (mChannel != null && !mChannel.hasAudio()) {
            // A channel needs to have a audio stream at least to play in exoPlayer.
            mSession.notifyVideoUnavailable(
                    TvInputManager.VIDEO_UNAVAILABLE_REASON_UNKNOWN);
            return;
        }
        if (mSurface != null && !mPlayerStarted) {
            mPlayer.setSurface(mSurface);
            mPlayer.setPlayWhenReady(true);
            mPlayer.setVolume(mVolume);
            if (mChannel != null && !mChannel.hasVideo() && mChannel.hasAudio()) {
                mSession.notifyVideoUnavailable(
                        TvInputManager.VIDEO_UNAVAILABLE_REASON_AUDIO_ONLY);
            } else {
                mSession.notifyVideoUnavailable(
                        TvInputManager.VIDEO_UNAVAILABLE_REASON_BUFFERING);
            }
            mSession.sendUiMessage(TunerSession.MSG_UI_HIDE_MESSAGE);
            mPlayerStarted = true;
        }
    }

    private void playFromChannel(long timestamp) {
        long oldTimestamp;
        mSource = null;
        if (mChannel.getType() == Channel.TYPE_TUNER) {
            mSource = mTunerSource;
        } else if (mChannel.getType() == Channel.TYPE_FILE) {
            mSource = mFileSource;
        }
        Assert.assertNotNull(mSource);
        if (mSource.tuneToChannel(mChannel)) {
            if (ENABLE_PROFILER) {
                oldTimestamp = timestamp;
                timestamp = SystemClock.elapsedRealtime();
                Log.i(TAG, "[Profiler] tuneToChannel() takes " + (timestamp - oldTimestamp)
                        + " ms");
            }
            mSource.startStream();
            mPlayer = createPlayer(mAudioCapabilities, mCacheManager);
            mPlayer.setCaptionServiceNumber(Cea708Data.EMPTY_SERVICE_NUMBER);
            mPlayer.setVideoEventListener(this);
            mPlayer.setCaptionServiceNumber(mCaptionTrack != null ?
                    mCaptionTrack.serviceNumber : Cea708Data.EMPTY_SERVICE_NUMBER);
            mPreparingGeneration = mPlayerGeneration;
            mPlayer.prepare((MediaDataSource) mSource);
            mPlayerStarted = false;
        } else {
            // Close TunerHal when tune fails.
            mTunerHal.stopTune();
            mSession.notifyVideoUnavailable(
                    TvInputManager.VIDEO_UNAVAILABLE_REASON_UNKNOWN);
        }
    }

    private void playFromRecording() {
        // TODO: Handle errors.
        CacheManager cacheManager =
                new CacheManager(new DvrStorageManager(new File(getRecordingPath()), false));
        mSource = null;
        mPlayer = createPlayer(mAudioCapabilities, cacheManager);
        mPlayer.setCaptionServiceNumber(Cea708Data.EMPTY_SERVICE_NUMBER);
        mPlayer.setVideoEventListener(this);
        mPlayer.setCaptionServiceNumber(mCaptionTrack != null ?
                mCaptionTrack.serviceNumber : Cea708Data.EMPTY_SERVICE_NUMBER);
        mPreparingGeneration = mPlayerGeneration;
        mPlayer.prepare(null);
        mPlayerStarted = false;
    }

    private void resetPlayback() {
        long timestamp, oldTimestamp;
        timestamp = SystemClock.elapsedRealtime();
        stopPlayback();
        stopCaptionTrack();
        if (ENABLE_PROFILER) {
            oldTimestamp = timestamp;
            timestamp = SystemClock.elapsedRealtime();
            Log.i(TAG, "[Profiler] stopPlayback() takes " + (timestamp - oldTimestamp) + " ms");
        }
        if (!mChannelBlocked && mSurface != null) {
            mSession.notifyVideoUnavailable(
                    TvInputManager.VIDEO_UNAVAILABLE_REASON_TUNING);
            if (mChannel != null) {
                playFromChannel(timestamp);
            } else if (mRecordingId != null){
                playFromRecording();
            }
        }
    }

    private void prepareTune(TunerChannel channel, String recording) {
        mChannelBlocked = false;
        mUnblockedContentRating = null;
        mRetryCount = 0;
        mChannel = channel;
        mRecordingId = recording;
        mRecordingDuration = recording != null ? getDurationForRecording(recording) : null;
        mProgram = null;
        mPrograms = null;
        mCacheStartTimeMs = mRecordStartTimeMs =
                (mRecordingId != null) ? 0 : System.currentTimeMillis();
        mLastPositionMs = 0;
        mCaptionTrack = null;
        mHandler.sendEmptyMessage(MSG_PARENTAL_CONTROLS);
    }

    private void doReschedulePrograms() {
        long currentPositionMs = getCurrentPosition();
        long forwardDifference = Math.abs(currentPositionMs - mLastPositionMs
                - RESCHEDULE_PROGRAMS_INTERVAL_MS);
        mLastPositionMs = currentPositionMs;

        // A gap is measured as the time difference between previous and next current position
        // periodically. If the gap has a significant difference with an interval of a period,
        // this means that there is a change of playback status and the programs of the current
        // channel should be rescheduled to new playback timeline.
        if (forwardDifference > RESCHEDULE_PROGRAMS_TOLERANCE_MS) {
            if (DEBUG) {
                Log.d(TAG, "reschedule programs size:"
                        + (mPrograms != null ? mPrograms.size() : 0) + " current program: "
                        + getCurrentProgram());
            }
            mHandler.obtainMessage(MSG_SCHEDULE_OF_PROGRAMS, new Pair<>(mChannel, mPrograms))
                    .sendToTarget();
        }
        mHandler.removeMessages(MSG_RESCHEDULE_PROGRAMS);
        mHandler.sendEmptyMessageDelayed(MSG_RESCHEDULE_PROGRAMS,
                RESCHEDULE_PROGRAMS_INTERVAL_MS);
    }

    private int getTrickPlaySeekIntervalMs() {
        return Math.max(MIN_TRICKPLAY_SEEK_INTERVAL_MS,
                (int) Math.abs(TRICKPLAY_SEEK_INTERVAL_MS / mPlaybackParams.getSpeed()));
    }

    private void doTrickplay(int seekPositionMs) {
        mHandler.removeMessages(MSG_TRICKPLAY);
        if (mPlaybackParams.getSpeed() == 1.0f || !mPlayer.isPlaying()) {
            return;
        }
        if (seekPositionMs < mCacheStartTimeMs - mRecordStartTimeMs) {
            mPlayer.seekTo(mCacheStartTimeMs - mRecordStartTimeMs);
            mPlaybackParams.setSpeed(1.0f);
            mPlayer.setAudioTrack(true);
            return;
        } else if (seekPositionMs > System.currentTimeMillis() - mRecordStartTimeMs) {
            mPlayer.seekTo(System.currentTimeMillis() - mRecordStartTimeMs);
            mPlaybackParams.setSpeed(1.0f);
            mPlayer.setAudioTrack(true);
            return;
        }

        if (!mPlayer.isBuffering()) {
            mPlayer.seekTo(seekPositionMs);
        }
        seekPositionMs += mPlaybackParams.getSpeed() * getTrickPlaySeekIntervalMs();
        mHandler.sendMessageDelayed(mHandler.obtainMessage(MSG_TRICKPLAY, seekPositionMs, 0),
                getTrickPlaySeekIntervalMs());
    }

    private void doTimeShiftPause() {
        if (!hasEnoughBackwardCache()) {
            return;
        }
        mPlaybackParams.setSpeed(1.0f);
        mPlayer.setPlayWhenReady(false);
        mPlayer.setAudioTrack(true);
    }

    private void doTimeShiftResume() {
        mPlaybackParams.setSpeed(1.0f);
        mPlayer.setPlayWhenReady(true);
        mPlayer.setAudioTrack(true);
    }

    private void doTimeShiftSeekTo(long timeMs) {
        mPlayer.seekTo((int) (timeMs - mRecordStartTimeMs));
    }

    private void doTimeShiftSetPlaybackParams(PlaybackParams params) {
        if (!hasEnoughBackwardCache() && params.getSpeed() < 1.0f) {
            return;
        }
        mPlaybackParams = params;
        if (!mHandler.hasMessages(MSG_TRICKPLAY)) {
            // Initiate trickplay
            float rate = mPlaybackParams.getSpeed();
            if (rate != 1.0f) {
                mPlayer.setAudioTrack(false);
                mPlayer.setPlayWhenReady(true);
            }
            mHandler.sendMessage(mHandler.obtainMessage(MSG_TRICKPLAY,
                    (int) (mPlayer.getCurrentPosition() + rate * getTrickPlaySeekIntervalMs()), 0));
        }
    }

    private EitItem getCurrentProgram() {
        if (mPrograms == null) {
            return null;
        }
        long currentTimeMs = getCurrentPosition();
        for (EitItem item : mPrograms) {
            if (item.getStartTimeUtcMillis() <= currentTimeMs
                    && item.getEndTimeUtcMillis() >= currentTimeMs) {
                return item;
            }
        }
        return null;
    }

    private void doParentalControls() {
        boolean isParentalControlsEnabled = mTvInputManager.isParentalControlsEnabled();
        if (isParentalControlsEnabled) {
            TvContentRating blockContentRating = getContentRatingOfCurrentProgramBlocked();
            if (DEBUG) {
                if (blockContentRating != null) {
                    Log.d(TAG, "Check parental controls: blocked by content rating - "
                            + blockContentRating);
                } else {
                    Log.d(TAG, "Check parental controls: available");
                }
            }
            updateChannelBlockStatus(blockContentRating != null, blockContentRating);
        } else {
            if (DEBUG) {
                Log.d(TAG, "Check parental controls: available");
            }
            updateChannelBlockStatus(false, null);
        }
    }

    private void doDiscoverCaptionServiceNumber(int serviceNumber) {
        int index = mCaptionTrackMap.indexOfKey(serviceNumber);
        if (index < 0) {
            AtscCaptionTrack captionTrack = new AtscCaptionTrack();
            captionTrack.serviceNumber = serviceNumber;
            captionTrack.wideAspectRatio = false;
            captionTrack.easyReader = false;
            mCaptionTrackMap.put(serviceNumber, captionTrack);
            mTvTracks.add(new TvTrackInfo.Builder(TvTrackInfo.TYPE_SUBTITLE,
                    SUBTITLE_TRACK_PREFIX + serviceNumber).build());
            mSession.notifyTracksChanged(mTvTracks);
        }
    }

    private TvContentRating getContentRatingOfCurrentProgramBlocked() {
        EitItem currentProgram = getCurrentProgram();
        if (currentProgram == null) {
            return null;
        }
        TvContentRating[] ratings = mTvContentRatingCache
                .getRatings(currentProgram.getContentRating());
        if (ratings == null) {
            return null;
        }
        for (TvContentRating rating : ratings) {
            if (!Objects.equals(mUnblockedContentRating, rating) && mTvInputManager
                    .isRatingBlocked(rating)) {
                return rating;
            }
        }
        return null;
    }

    private void updateChannelBlockStatus(boolean channelBlocked,
            TvContentRating contentRating) {
        if (mChannelBlocked == channelBlocked) {
            return;
        }
        mChannelBlocked = channelBlocked;
        if (mChannelBlocked) {
            mHandler.removeCallbacksAndMessages(null);
            mTunerHal.stopTune();
            stopPlayback();
            resetTvTracks();
            if (contentRating != null) {
                mSession.notifyContentBlocked(contentRating);
            }
            mHandler.sendEmptyMessageDelayed(MSG_PARENTAL_CONTROLS, PARENTAL_CONTROLS_INTERVAL_MS);
        } else {
            mHandler.removeCallbacksAndMessages(null);
            resetPlayback();
            mSession.notifyContentAllowed();
            mHandler.sendEmptyMessageDelayed(MSG_RESCHEDULE_PROGRAMS,
                    RESCHEDULE_PROGRAMS_INITIAL_DELAY_MS);
            mHandler.sendEmptyMessageDelayed(MSG_CHECK_SIGNAL, CHECK_NO_SIGNAL_INITIAL_DELAY_MS);
        }
    }

    private boolean hasEnoughBackwardCache() {
        return mPlayer.getCurrentPosition() + CACHE_UNDERFLOW_BUFFER_MS
                >= mCacheStartTimeMs - mRecordStartTimeMs;
    }
}
