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

import android.media.MediaDataSource;
import android.media.MediaExtractor;
import android.os.ConditionVariable;
import android.os.SystemClock;
import android.util.Log;

import com.google.android.exoplayer.MediaFormat;
import com.google.android.exoplayer.MediaFormatHolder;
import com.google.android.exoplayer.MediaFormatUtil;
import com.google.android.exoplayer.SampleHolder;
import com.android.usbtuner.exoplayer.cache.CacheManager;
import com.android.usbtuner.exoplayer.cache.RecordingSampleBuffer;
import com.android.usbtuner.exoplayer.cache.SimpleSampleBuffer;
import com.android.usbtuner.tvinput.PlaybackCacheListener;

import java.io.IOException;
import java.util.ArrayList;
import java.util.List;
import java.util.Locale;
import java.util.concurrent.atomic.AtomicLong;

/**
 * A class that plays a live stream from a given media extractor using an extractor thread.
 */
public class PlaySampleExtractor implements SampleExtractor {
    private static final String TAG = "PlaySampleExtractor";

    // Maximum bandwidth of 1080p channel is about 2.2MB/s. 2MB for a sample will suffice.
    private static final int SAMPLE_BUFFER_SIZE = 1024 * 1024 * 2;
    private static final AtomicLong ID_COUNTER = new AtomicLong(0);

    private final MediaDataSource mDataSource;
    private final MediaExtractor mMediaExtractor;
    private final ExtractorThread mExtractorThread;
    private final CacheManager.SampleBuffer mSampleBuffer;
    private final long mId;
    private MediaFormat[] mTrackFormats;

    private boolean mReleased = false;

    public PlaySampleExtractor(MediaDataSource source, CacheManager cacheManager,
            PlaybackCacheListener cacheListener, boolean useCache) {
        mId = ID_COUNTER.incrementAndGet();
        mDataSource = source;
        mMediaExtractor = new MediaExtractor();
        mExtractorThread = new ExtractorThread();
        if (useCache) {
            mSampleBuffer = new RecordingSampleBuffer(cacheManager, cacheListener, true,
                    RecordingSampleBuffer.CACHE_REASON_LIVE_PLAYBACK);
        } else {
            mSampleBuffer = new SimpleSampleBuffer(cacheListener);
        }
    }

    private class ExtractorThread extends Thread {
        private volatile boolean mQuitRequested = false;

        public ExtractorThread() {
            super("ExtractorThread");
        }

        @Override
        public void run() {
            SampleHolder sample = new SampleHolder(SampleHolder.BUFFER_REPLACEMENT_MODE_NORMAL);
            sample.ensureSpaceForWrite(SAMPLE_BUFFER_SIZE);
            ConditionVariable conditionVariable = new ConditionVariable();
            while (!mQuitRequested) {
                fetchSample(sample, conditionVariable);
            }
            cleanUp();
        }

        private void fetchSample(SampleHolder sample, ConditionVariable conditionVariable) {
            int index = mMediaExtractor.getSampleTrackIndex();
            if (index < 0) {
                Log.i(TAG, "EoS");
                mQuitRequested = true;
                mSampleBuffer.setEos();
                return;
            }
            sample.data.clear();
            sample.size = mMediaExtractor.readSampleData(sample.data, 0);
            if (sample.size < 0 || sample.size > SAMPLE_BUFFER_SIZE) {
                // Should not happen
                Log.e(TAG, "Invalid sample size: " + sample.size);
                mMediaExtractor.advance();
                return;
            }
            sample.data.position(sample.size);
            sample.timeUs = mMediaExtractor.getSampleTime();
            sample.flags = mMediaExtractor.getSampleFlags();

            mMediaExtractor.advance();
            try {
                queueSample(index, sample, conditionVariable);
            } catch (IOException e) {
                mQuitRequested = true;
                mSampleBuffer.setEos();
            }
        }

        public void quit() {
            mQuitRequested = true;
        }
    }

    public void queueSample(int index, SampleHolder sample, ConditionVariable conditionVariable)
            throws IOException {
        long writeStartTimeNs = SystemClock.elapsedRealtimeNanos();
        mSampleBuffer.writeSample(index, sample, conditionVariable);

        // Check if the storage has enough bandwidth for trickplay. Otherwise we disable it
        // and notify the slowness through the playback cache listener.
        if (mSampleBuffer.isWriteSpeedSlow(sample.size,
                SystemClock.elapsedRealtimeNanos() - writeStartTimeNs)) {
            mSampleBuffer.handleWriteSpeedSlow();
        }
    }

    @Override
    public boolean prepare() throws IOException {
        synchronized (this) {
            mMediaExtractor.setDataSource(mDataSource);

            int trackCount = mMediaExtractor.getTrackCount();
            mTrackFormats = new MediaFormat[trackCount];
            for (int i = 0; i < trackCount; i++) {
                mTrackFormats[i] =
                        MediaFormatUtil.createMediaFormat(mMediaExtractor.getTrackFormat(i));
                mMediaExtractor.selectTrack(i);
            }
            List<String> ids = new ArrayList<>();
            for (int i = 0; i < trackCount; i++) {
                ids.add(String.format(Locale.ENGLISH, "%s_%x", Long.toHexString(mId), i));

            }
            mSampleBuffer.init(ids, null);

        }
        mExtractorThread.start();
        return true;
    }

    @Override
    public synchronized MediaFormat[] getTrackFormats() {
        return mTrackFormats;
    }

    @Override
    public void getTrackMediaFormat(int track, MediaFormatHolder outMediaFormatHolder) {
        outMediaFormatHolder.format = mTrackFormats[track];
        outMediaFormatHolder.drmInitData = null;
    }

    @Override
    public void selectTrack(int index) {
        mSampleBuffer.selectTrack(index);
    }

    @Override
    public void deselectTrack(int index) {
        mSampleBuffer.deselectTrack(index);
    }

    @Override
    public long getBufferedPositionUs() {
        return mSampleBuffer.getBufferedPositionUs();
    }

    @Override
    public boolean continueBuffering(long positionUs)  {
        return mSampleBuffer.continueBuffering(positionUs);
    }

    @Override
    public void seekTo(long positionUs) {
        mSampleBuffer.seekTo(positionUs);
    }

    @Override
    public int readSample(int track, SampleHolder sampleHolder) {
        return mSampleBuffer.readSample(track, sampleHolder);
    }

    @Override
    public void release() {
        synchronized (this) {
            mReleased = true;
        }
        if (mExtractorThread.isAlive()) {
            mExtractorThread.quit();

            // We don't join here to prevent hang --- MediaExtractor is released at the thread.
        } else {
            cleanUp();
        }
    }

    public void cleanUpImpl() {
        mSampleBuffer.release();
    }

    public synchronized void cleanUp() {
        if (!mReleased) {
            return;
        }
        cleanUpImpl();
        mMediaExtractor.release();
    }
}
