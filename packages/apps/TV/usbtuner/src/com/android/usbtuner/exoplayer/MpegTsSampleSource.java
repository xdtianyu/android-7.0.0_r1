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
package com.android.usbtuner.exoplayer;

import com.google.android.exoplayer.C;
import com.google.android.exoplayer.MediaFormat;
import com.google.android.exoplayer.MediaFormatHolder;
import com.google.android.exoplayer.SampleHolder;
import com.google.android.exoplayer.SampleSource;
import com.google.android.exoplayer.SampleSource.SampleSourceReader;
import com.google.android.exoplayer.util.Assertions;

import java.io.IOException;

/** {@link SampleSource} that extracts sample data using a {@link SampleExtractor}. */
public final class MpegTsSampleSource implements SampleSource, SampleSourceReader {

    private static final int TRACK_STATE_DISABLED = 0;
    private static final int TRACK_STATE_ENABLED = 1;
    private static final int TRACK_STATE_FORMAT_SENT = 2;

    private final SampleExtractor mSampleExtractor;

    private MediaFormat[] mTrackFormats;
    private boolean mPrepared;
    private IOException mPreparationError;
    private int mRemainingReleaseCount;
    private int[] mTrackStates;
    private boolean[] mPendingDiscontinuities;

    private long mLastSeekPositionUs;
    private long mPendingSeekPositionUs;

    /**
     * Creates a new sample source that extracts samples using {@code mSampleExtractor}.
     *
     * @param sampleExtractor a sample extractor for accessing media samples
     */
    public MpegTsSampleSource(SampleExtractor sampleExtractor) {
        mSampleExtractor = Assertions.checkNotNull(sampleExtractor);
    }

    @Override
    public SampleSourceReader register() {
        mRemainingReleaseCount++;
        return this;
    }

    @Override
    public boolean prepare(long positionUs) {
        if (!mPrepared) {
            if (mPreparationError != null) {
                return false;
            }
            try {
                if (mSampleExtractor.prepare()) {
                    mTrackFormats = mSampleExtractor.getTrackFormats();
                    mTrackStates = new int[mTrackFormats.length];
                    mPendingDiscontinuities = new boolean[mTrackStates.length];
                    mPrepared = true;
                }
            } catch (IOException e) {
                mPreparationError = e;
                return false;
            }
        }
        return true;
    }

    @Override
    public int getTrackCount() {
        Assertions.checkState(mPrepared);
        return mTrackFormats.length;
    }

    @Override
    public MediaFormat getFormat(int track) {
        Assertions.checkState(mPrepared);
        return mTrackFormats[track];
    }

    @Override
    public void enable(int track, long positionUs) {
        Assertions.checkState(mPrepared);
        Assertions.checkState(mTrackStates[track] == TRACK_STATE_DISABLED);
        mTrackStates[track] = TRACK_STATE_ENABLED;
        mSampleExtractor.selectTrack(track);
        seekToUsInternal(positionUs, positionUs != 0);
    }

    @Override
    public void disable(int track) {
        Assertions.checkState(mPrepared);
        Assertions.checkState(mTrackStates[track] != TRACK_STATE_DISABLED);
        mSampleExtractor.deselectTrack(track);
        mPendingDiscontinuities[track] = false;
        mTrackStates[track] = TRACK_STATE_DISABLED;
    }

    @Override
    public boolean continueBuffering(int track, long positionUs) {
        return mSampleExtractor.continueBuffering(positionUs);
    }

    @Override
    public long readDiscontinuity(int track) {
        if (mPendingDiscontinuities[track]) {
            mPendingDiscontinuities[track] = false;
            return mLastSeekPositionUs;
        }
        return NO_DISCONTINUITY;
    }

    @Override
    public int readData(int track, long positionUs, MediaFormatHolder formatHolder,
      SampleHolder sampleHolder) {
        Assertions.checkState(mPrepared);
        Assertions.checkState(mTrackStates[track] != TRACK_STATE_DISABLED);
        if (mPendingDiscontinuities[track]) {
            return NOTHING_READ;
        }
        if (mTrackStates[track] != TRACK_STATE_FORMAT_SENT) {
            mSampleExtractor.getTrackMediaFormat(track, formatHolder);
            mTrackStates[track] = TRACK_STATE_FORMAT_SENT;
            return FORMAT_READ;
        }

        mPendingSeekPositionUs = C.UNKNOWN_TIME_US;
        return mSampleExtractor.readSample(track, sampleHolder);
    }

    @Override
    public void maybeThrowError() throws IOException {
        if (mPreparationError != null) {
            throw mPreparationError;
        }
    }

    @Override
    public void seekToUs(long positionUs) {
        Assertions.checkState(mPrepared);
        seekToUsInternal(positionUs, false);
    }

    @Override
    public long getBufferedPositionUs() {
        Assertions.checkState(mPrepared);
        return mSampleExtractor.getBufferedPositionUs();
    }

    @Override
    public void release() {
        Assertions.checkState(mRemainingReleaseCount > 0);
        if (--mRemainingReleaseCount == 0) {
            mSampleExtractor.release();
        }
    }

    private void seekToUsInternal(long positionUs, boolean force) {
        // Unless forced, avoid duplicate calls to the underlying extractor's seek method
        // in the case that there have been no interleaving calls to readSample.
        if (force || mPendingSeekPositionUs != positionUs) {
            mLastSeekPositionUs = positionUs;
            mPendingSeekPositionUs = positionUs;
            mSampleExtractor.seekTo(positionUs);
            for (int i = 0; i < mTrackStates.length; ++i) {
                if (mTrackStates[i] != TRACK_STATE_DISABLED) {
                    mPendingDiscontinuities[i] = true;
                }
            }
        }
    }
}
