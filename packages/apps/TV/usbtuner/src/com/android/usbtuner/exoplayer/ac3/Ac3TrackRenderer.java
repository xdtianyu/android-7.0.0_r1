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

package com.android.usbtuner.exoplayer.ac3;

import android.os.Handler;
import android.os.SystemClock;
import android.util.Log;

import com.google.android.exoplayer.CodecCounters;
import com.google.android.exoplayer.ExoPlaybackException;
import com.google.android.exoplayer.MediaClock;
import com.google.android.exoplayer.MediaCodecAudioTrackRenderer;
import com.google.android.exoplayer.MediaFormat;
import com.google.android.exoplayer.MediaFormatHolder;
import com.google.android.exoplayer.MediaFormatUtil;
import com.google.android.exoplayer.SampleHolder;
import com.google.android.exoplayer.SampleSource;
import com.google.android.exoplayer.TrackRenderer;
import com.google.android.exoplayer.audio.AudioTrack;
import com.google.android.exoplayer.util.Assertions;
import com.google.android.exoplayer.util.MimeTypes;
import com.android.usbtuner.tvinput.UsbTunerDebug;

import java.io.IOException;
import java.nio.ByteBuffer;

/**
 * Decodes and renders AC3 audio.
 */
public class Ac3TrackRenderer extends TrackRenderer implements Ac3Decoder.DecodeListener,
        MediaClock {
    public static final int MSG_SET_VOLUME = MediaCodecAudioTrackRenderer.MSG_SET_VOLUME;
    public static final int MSG_SET_AUDIO_TRACK = MSG_SET_VOLUME + 1;

    // ATSC/53 allows sample rate to be only 48Khz.
    // One AC3 sample has 1536 frames, and its duration is 32ms.
    public static final long AC3_SAMPLE_DURATION_US = 32000;

    private static final String TAG = "Ac3TrackRenderer";
    private static final boolean DEBUG = false;

    /**
     * Interface definition for a callback to be notified of
     * {@link com.google.android.exoplayer.audio.AudioTrack} error.
     */
    public interface EventListener {
        void onAudioTrackInitializationError(AudioTrack.InitializationException e);
        void onAudioTrackWriteError(AudioTrack.WriteException e);
    }

    private static final int DEFAULT_INPUT_BUFFER_SIZE = 16384 * 2;
    private static final int DEFAULT_OUTPUT_BUFFER_SIZE = 1024*1024;
    private static final int MONITOR_DURATION_MS = 1000;
    private static final int AC3_HEADER_BITRATE_OFFSET = 4;

    // Keep this as static in order to prevent new framework AudioTrack creation
    // while old AudioTrack is being released.
    private static final AudioTrackWrapper AUDIO_TRACK = new AudioTrackWrapper();
    private static final long KEEP_ALIVE_AFTER_EOS_DURATION_MS = 3000;

    // Ignore AudioTrack backward movement if duration of movement is below the threshold.
    private static final long BACKWARD_AUDIO_TRACK_MOVE_THRESHOLD_US = 3000;

    // AudioTrack position cannot go ahead beyond this limit.
    private static final long CURRENT_POSITION_FROM_PTS_LIMIT_US = 1000000;

    // Since MediaCodec processing and AudioTrack playing add delay,
    // PTS interpolated time should be delayed reasonably when AudioTrack is not used.
    private static final long ESTIMATED_TRACK_RENDERING_DELAY_US = 500000;

    private final CodecCounters mCodecCounters;
    private final SampleSource.SampleSourceReader mSource;
    private final SampleHolder mSampleHolder;
    private final MediaFormatHolder mFormatHolder;
    private final EventListener mEventListener;
    private final Handler mEventHandler;
    private final boolean mIsSoftware;
    private final AudioTrackMonitor mMonitor;
    private final AudioClock mAudioClock;

    private MediaFormat mFormat;
    private Ac3Decoder mDecoder;
    private ByteBuffer mOutputBuffer;
    private boolean mOutputReady;
    private int mTrackIndex;
    private boolean mSourceStateReady;
    private boolean mInputStreamEnded;
    private boolean mOutputStreamEnded;
    private long mEndOfStreamMs;
    private long mCurrentPositionUs;
    private int mPresentationCount;
    private long mPresentationTimeUs;
    private long mInterpolatedTimeUs;
    private long mPreviousPositionUs;

    public Ac3TrackRenderer(SampleSource source, Handler eventHandler,
            EventListener listener, boolean isSoftware) {
        mSource = source.register();
        mEventHandler = eventHandler;
        mEventListener = listener;
        mDecoder = Ac3Decoder.createAc3Decoder(isSoftware);
        mIsSoftware = isSoftware;
        mTrackIndex = -1;
        mSampleHolder = new SampleHolder(SampleHolder.BUFFER_REPLACEMENT_MODE_DIRECT);
        mSampleHolder.ensureSpaceForWrite(DEFAULT_INPUT_BUFFER_SIZE);
        mOutputBuffer = ByteBuffer.allocate(DEFAULT_OUTPUT_BUFFER_SIZE);
        mFormatHolder = new MediaFormatHolder();
        AUDIO_TRACK.restart();
        mCodecCounters = new CodecCounters();
        mMonitor = new AudioTrackMonitor();
        mAudioClock = new AudioClock();
    }

    @Override
    protected MediaClock getMediaClock() {
        return this;
    }

    private static boolean handlesMimeType(String mimeType) {
        return mimeType.equals(MimeTypes.AUDIO_AC3) || mimeType.equals(MimeTypes.AUDIO_E_AC3);
    }

    @Override
    protected boolean doPrepare(long positionUs) throws ExoPlaybackException {
        boolean sourcePrepared = mSource.prepare(positionUs);
        if (!sourcePrepared) {
            return false;
        }
        for (int i = 0; i < mSource.getTrackCount(); i++) {
            if (handlesMimeType(mSource.getFormat(i).mimeType)) {
                mTrackIndex = i;
                return true;
            }
        }

        // TODO: Check this case. Source does not have the proper mime type.
        return true;
    }

    @Override
    protected int getTrackCount() {
        return mTrackIndex < 0 ? 0 : 1;
    }

    @Override
    protected MediaFormat getFormat(int track) {
        Assertions.checkArgument(mTrackIndex != -1 && track == 0);
        return mSource.getFormat(mTrackIndex);
    }

    @Override
    protected void onEnabled(int track, long positionUs, boolean joining) {
        Assertions.checkArgument(mTrackIndex != -1 && track == 0);
        mSource.enable(mTrackIndex, positionUs);
        mDecoder.startDecoder(this);
        seekToInternal(positionUs);
    }

    @Override
    protected void onDisabled() {
        AUDIO_TRACK.resetSessionId();
        clearDecodeState();
        mFormat = null;
        mSource.disable(mTrackIndex);
    }

    @Override
    protected void onReleased() {
        AUDIO_TRACK.release();
        mSource.release();
    }

    @Override
    protected boolean isEnded() {
        return mOutputStreamEnded && AUDIO_TRACK.isEnded();
    }

    @Override
    protected boolean isReady() {
        return AUDIO_TRACK.isReady() || (mFormat != null && (mSourceStateReady || mOutputReady));
    }

    private void seekToInternal(long positionUs) {
        mMonitor.reset(MONITOR_DURATION_MS);
        mSourceStateReady = false;
        mInputStreamEnded = false;
        mOutputStreamEnded = false;
        mPresentationTimeUs = positionUs;
        mPresentationCount = 0;
        mPreviousPositionUs = 0;
        mCurrentPositionUs = Long.MIN_VALUE;
        mInterpolatedTimeUs = Long.MIN_VALUE;
        mAudioClock.setPositionUs(positionUs);
    }

    @Override
    protected void seekTo(long positionUs) {
        mSource.seekToUs(positionUs);
        AUDIO_TRACK.reset();
        // resetSessionId() will create a new framework AudioTrack instead of reusing old one.
        if (!mIsSoftware) {
            AUDIO_TRACK.resetSessionId();
        }
        seekToInternal(positionUs);
    }

    @Override
    protected void onStarted() {
        AUDIO_TRACK.play();
        mAudioClock.start();
    }

    @Override
    protected void onStopped() {
        AUDIO_TRACK.pause();
        mAudioClock.stop();
    }

    @Override
    protected void maybeThrowError() throws ExoPlaybackException {
        try {
            mSource.maybeThrowError();
        } catch (IOException e) {
            throw new ExoPlaybackException(e);
        }
    }

    @Override
    protected void doSomeWork(long positionUs, long elapsedRealtimeUs) throws ExoPlaybackException {
        mMonitor.maybeLog();
        try {
            if (mEndOfStreamMs != 0) {
                // Ensure playback stops, after EoS was notified.
                // Sometimes MediaCodecTrackRenderer does not fetch EoS timely
                // after EoS was notified here long before.
                long diff = SystemClock.elapsedRealtime() - mEndOfStreamMs;
                if (diff >= KEEP_ALIVE_AFTER_EOS_DURATION_MS) {
                    throw new ExoPlaybackException("Much time has elapsed after EoS");
                }
            }
            boolean continueBuffering = mSource.continueBuffering(mTrackIndex, positionUs);
            if (mSourceStateReady != continueBuffering) {
                mSourceStateReady = continueBuffering;
                if (DEBUG) {
                    Log.d(TAG, "mSourceStateReady: " + String.valueOf(mSourceStateReady));
                }
            }
            if (mFormat == null) {
                readFormat();
                return;
            }

            // Process only one sample at a time for doSomeWork()
            if (processOutput()) {
                if (!mOutputReady) {
                    while (feedInputBuffer()) {
                        if (mOutputReady) break;
                    }
                }
            }
            mCodecCounters.ensureUpdated();
        } catch (IOException e) {
            throw new ExoPlaybackException(e);
        }
    }

    private void ensureAudioTrackInitialized() {
        if (!AUDIO_TRACK.isInitialized()) {
            try {
                if (DEBUG) {
                    Log.d(TAG, "AudioTrack initialized");
                }
                AUDIO_TRACK.initialize();
            } catch (AudioTrack.InitializationException e) {
                Log.e(TAG, "Error on AudioTrack initialization", e);
                notifyAudioTrackInitializationError(e);

                // Do not throw exception here but just disabling audioTrack to keep playing
                // video without audio.
                AUDIO_TRACK.setStatus(false);
            }
            if (getState() == TrackRenderer.STATE_STARTED) {
                if (DEBUG) {
                    Log.d(TAG, "AudioTrack played");
                }
                AUDIO_TRACK.play();
            }
        }
    }

    private void clearDecodeState() {
        mDecoder.startDecoder(this);
        mOutputReady = false;
        AUDIO_TRACK.reset();
    }

    private void readFormat() throws IOException, ExoPlaybackException {
        int result = mSource.readData(mTrackIndex, mCurrentPositionUs,
                mFormatHolder, mSampleHolder);
        if (result == SampleSource.FORMAT_READ) {
            onInputFormatChanged(mFormatHolder);
        }
    }

    private void onInputFormatChanged(MediaFormatHolder formatHolder)
            throws ExoPlaybackException {
        MediaFormat format = formatHolder.format;
        if (mIsSoftware) {
            mFormat = MediaFormatUtil.createAudioMediaFormat(MimeTypes.AUDIO_RAW, format.durationUs,
                    format.channelCount, format.sampleRate);
        } else {
            mFormat = format;
        }
        if (DEBUG) {
            Log.d(TAG, "AudioTrack was configured to FORMAT: " + mFormat.toString());
        }
        clearDecodeState();
        AUDIO_TRACK.reconfigure(mFormat.getFrameworkMediaFormatV16());
    }

    private boolean feedInputBuffer() throws IOException, ExoPlaybackException {
        if (mInputStreamEnded) {
            return false;
        }

        long discontinuity = mSource.readDiscontinuity(mTrackIndex);
        if (discontinuity != SampleSource.NO_DISCONTINUITY) {
            // TODO: handle input discontinuity for trickplay.
            Log.i(TAG, "Read discontinuity happened");
            AUDIO_TRACK.handleDiscontinuity();
            mPresentationTimeUs = discontinuity;
            mPresentationCount = 0;
            clearDecodeState();
            return false;
        }

        mSampleHolder.data.clear();
        mSampleHolder.size = 0;
        int result = mSource.readData(mTrackIndex, mPresentationTimeUs, mFormatHolder,
                mSampleHolder);
        switch (result) {
            case SampleSource.NOTHING_READ: {
                return false;
            }
            case SampleSource.FORMAT_READ: {
                Log.i(TAG, "Format was read again");
                onInputFormatChanged(mFormatHolder);
                return true;
            }
            case SampleSource.END_OF_STREAM: {
                Log.i(TAG, "End of stream from SampleSource");
                mInputStreamEnded = true;
                return false;
            }
            default: {
                mSampleHolder.data.flip();
                mDecoder.decode(mSampleHolder.data, mSampleHolder.timeUs);
                return true;
            }
        }
    }

    private boolean processOutput() throws ExoPlaybackException {
        if (mOutputStreamEnded) {
            return false;
        }
        if (!mOutputReady) {
            if (mInputStreamEnded) {
                mOutputStreamEnded = true;
                mEndOfStreamMs = SystemClock.elapsedRealtime();
                return false;
            }
            return true;
        }

        ensureAudioTrackInitialized();
        int handleBufferResult;
        try {
            // To reduce discontinuity, interpolate presentation time.
            mInterpolatedTimeUs = mPresentationTimeUs
                    + mPresentationCount * AC3_SAMPLE_DURATION_US;
            handleBufferResult = AUDIO_TRACK.handleBuffer(mOutputBuffer,
                    0, mOutputBuffer.limit(), mInterpolatedTimeUs);
        } catch (AudioTrack.WriteException e) {
            notifyAudioTrackWriteError(e);
            throw new ExoPlaybackException(e);
        }

        if ((handleBufferResult & AudioTrack.RESULT_POSITION_DISCONTINUITY) != 0) {
            Log.i(TAG, "Play discontinuity happened");
            mCurrentPositionUs = Long.MIN_VALUE;
        }
        if ((handleBufferResult & AudioTrack.RESULT_BUFFER_CONSUMED) != 0) {
            mCodecCounters.renderedOutputBufferCount++;
            mOutputReady = false;
            return true;
        }
        return false;
    }

    @Override
    protected long getDurationUs() {
        return mSource.getFormat(mTrackIndex).durationUs;
    }

    @Override
    protected long getBufferedPositionUs() {
        long pos = mSource.getBufferedPositionUs();
        return pos == UNKNOWN_TIME_US || pos == END_OF_TRACK_US
                ? pos : Math.max(pos, getPositionUs());
    }

    @Override
    public long getPositionUs() {
        if (!AUDIO_TRACK.isInitialized()) {
            return mAudioClock.getPositionUs();
        } if (!AUDIO_TRACK.isEnabled()) {
            if (mInterpolatedTimeUs > 0) {
                return mInterpolatedTimeUs - ESTIMATED_TRACK_RENDERING_DELAY_US;
            }
            return mPresentationTimeUs;
        }
        long audioTrackCurrentPositionUs = AUDIO_TRACK.getCurrentPositionUs(isEnded());
        if (audioTrackCurrentPositionUs == AudioTrack.CURRENT_POSITION_NOT_SET) {
            mPreviousPositionUs = 0L;
            if (DEBUG) {
                long oldPositionUs = Math.max(mCurrentPositionUs, 0);
                long currentPositionUs = Math.max(mPresentationTimeUs, mCurrentPositionUs);
                Log.d(TAG, "Audio position is not set, diff in us: "
                        + String.valueOf(currentPositionUs - oldPositionUs));
            }
            mCurrentPositionUs = Math.max(mPresentationTimeUs, mCurrentPositionUs);
        } else {
            if (!mIsSoftware && mPreviousPositionUs >
                    audioTrackCurrentPositionUs + BACKWARD_AUDIO_TRACK_MOVE_THRESHOLD_US) {
                Log.e(TAG, "audio_position BACK JUMP: "
                        + (mPreviousPositionUs - audioTrackCurrentPositionUs));
                mCurrentPositionUs = audioTrackCurrentPositionUs;
            } else {
                mCurrentPositionUs = Math.max(mCurrentPositionUs, audioTrackCurrentPositionUs);
            }
            mPreviousPositionUs = audioTrackCurrentPositionUs;
        }
        long upperBound = mPresentationTimeUs + CURRENT_POSITION_FROM_PTS_LIMIT_US;
        if (mCurrentPositionUs > upperBound) {
            mCurrentPositionUs = upperBound;
        }
        return mCurrentPositionUs;
    }

    @Override
    public void decodeDone(ByteBuffer outputBuffer, long presentationTimeUs) {
        if (outputBuffer == null || mOutputBuffer == null) {
            return;
        }
        if (presentationTimeUs < 0) {
            Log.e(TAG, "decodeDone - invalid presentationTimeUs");
            return;
        }

        if (UsbTunerDebug.ENABLED) {
            UsbTunerDebug.setAudioPtsUs(presentationTimeUs);
        }

        mOutputBuffer.clear();
        Assertions.checkState(mOutputBuffer.remaining() >= outputBuffer.limit());

        mOutputBuffer.put(outputBuffer);
        mMonitor.addPts(presentationTimeUs, mOutputBuffer.position(),
                mOutputBuffer.get(AC3_HEADER_BITRATE_OFFSET));
        if (presentationTimeUs == mPresentationTimeUs) {
            mPresentationCount++;
        } else {
            mPresentationCount = 0;
            mPresentationTimeUs = presentationTimeUs;
        }
        mOutputBuffer.flip();
        mOutputReady = true;
    }

    private void notifyAudioTrackInitializationError(final AudioTrack.InitializationException e) {
        if (mEventHandler == null || mEventListener == null) {
            return;
        }
        mEventHandler.post(new Runnable() {
            @Override
            public void run() {
                mEventListener.onAudioTrackInitializationError(e);
            }
        });
    }

    private void notifyAudioTrackWriteError(final AudioTrack.WriteException e) {
        if (mEventHandler == null || mEventListener == null) {
            return;
        }
        mEventHandler.post(new Runnable() {
            @Override
            public void run() {
                mEventListener.onAudioTrackWriteError(e);
            }
        });
    }

    @Override
    public void handleMessage(int messageType, Object message) throws ExoPlaybackException {
        switch (messageType) {
            case MSG_SET_VOLUME:
                AUDIO_TRACK.setVolume((Float) message);
                break;
            case MSG_SET_AUDIO_TRACK:
                AUDIO_TRACK.setStatus((Integer) message == 1);
                break;

            default:
                super.handleMessage(messageType, message);
        }
    }
}
