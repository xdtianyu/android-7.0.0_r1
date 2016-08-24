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

package com.android.usbtuner.exoplayer;

import android.media.MediaDataSource;
import android.media.MediaExtractor;
import android.os.ConditionVariable;
import android.os.SystemClock;
import android.util.Log;

import com.google.android.exoplayer.SampleHolder;
import com.android.usbtuner.exoplayer.cache.CacheManager;
import com.android.usbtuner.exoplayer.cache.RecordingSampleBuffer;
import com.android.usbtuner.tvinput.PlaybackCacheListener;

import java.io.IOException;
import java.util.ArrayList;
import java.util.List;
import java.util.Locale;
import java.util.concurrent.atomic.AtomicLong;

/**
 * Records live streams on the disk for DVR.
 */
public class Recorder {
    private static final String TAG = "Recorder";

    // Maximum bandwidth of 1080p channel is about 2.2MB/s. 2MB for a sample will suffice.
    private static final int SAMPLE_BUFFER_SIZE = 1024 * 1024 * 2;
    private static final AtomicLong ID_COUNTER = new AtomicLong(0);

    private final MediaDataSource mDataSource;
    private final MediaExtractor mMediaExtractor;
    private final ExtractorThread mExtractorThread;
    private int mTrackCount;
    private List<android.media.MediaFormat> mMediaFormats;

    private final CacheManager.SampleBuffer mSampleBuffer;

    private boolean mReleased = false;
    private boolean mResultNotified = false;
    private final long mId;

    private final RecordListener mRecordListener;

    /**
     * Listeners for events which happens during the recording.
     */
    public interface RecordListener {

        /**
         * Notifies recording completion.
         *
         * @param success {@code true} when the recording succeeded, {@code false} otherwise
         */
        void notifyRecordingFinished(boolean success);
    }

    /**
     * Create a recorder for a {@link android.media.MediaDataSource}.
     *
     * @param source {@link android.media.MediaDataSource} to record from
     * @param cacheManager the manager for recording samples to physical storage
     * @param cacheListener the {@link com.android.usbtuner.tvinput.PlaybackCacheListener}
     *                      to notify cache storage status change
     * @param recordListener RecordListener to notify events during the recording
     */
    public Recorder(MediaDataSource source, CacheManager cacheManager,
            PlaybackCacheListener cacheListener, RecordListener recordListener) {
        mDataSource = source;
        mMediaExtractor = new MediaExtractor();
        mExtractorThread = new ExtractorThread();
        mRecordListener = recordListener;

        mSampleBuffer = new RecordingSampleBuffer(cacheManager, cacheListener, false,
                RecordingSampleBuffer.CACHE_REASON_RECORDING);
        mId = ID_COUNTER.incrementAndGet();
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

    private void queueSample(int index, SampleHolder sample, ConditionVariable conditionVariable)
            throws IOException {
        long writeStartTimeNs = SystemClock.elapsedRealtimeNanos();
        mSampleBuffer.writeSample(index, sample, conditionVariable);

        // Check if the storage has enough bandwidth for recording. Otherwise we disable it
        // and notify the slowness.
        if (mSampleBuffer.isWriteSpeedSlow(sample.size,
                SystemClock.elapsedRealtimeNanos() - writeStartTimeNs)) {
            Log.w(TAG, "Disk is too slow for trickplay. Disable trickplay.");
            throw new IOException("Disk is too slow");
        }
    }

    /**
     * Prepares a recording.
     *
     * @return {@code true} when preparation finished successfully, {@code false} otherwise
     * @throws IOException
     */
    public boolean prepare() throws IOException {
        synchronized (this) {
            mMediaExtractor.setDataSource(mDataSource);

            mTrackCount = mMediaExtractor.getTrackCount();
            List<String> ids = new ArrayList<>();
            mMediaFormats = new ArrayList<>();
            for (int i = 0; i < mTrackCount; i++) {
                ids.add(String.format(Locale.ENGLISH, "%s_%x", Long.toHexString(mId), i));
                android.media.MediaFormat format = mMediaExtractor.getTrackFormat(i);
                mMediaExtractor.selectTrack(i);
                mMediaFormats.add(format);
            }
            mSampleBuffer.init(ids, mMediaFormats);
        }
        mExtractorThread.start();
        return true;
    }

    /**
     * Releases all the resources which were used in the recording.
     */
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

    private synchronized void cleanUp() {
        if (!mReleased) {
            if (!mResultNotified) {
                mRecordListener.notifyRecordingFinished(false);
                mResultNotified = true;
            }
            return;
        }
        mSampleBuffer.release();
        if (!mResultNotified) {
            mRecordListener.notifyRecordingFinished(true);
            mResultNotified = true;
        }
        mMediaExtractor.release();
    }

}
