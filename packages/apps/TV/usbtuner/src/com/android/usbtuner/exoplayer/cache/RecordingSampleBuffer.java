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

package com.android.usbtuner.exoplayer.cache;

import android.media.MediaCodec;
import android.media.MediaFormat;
import android.os.ConditionVariable;
import android.support.annotation.IntDef;
import android.util.Log;
import android.util.Pair;

import com.google.android.exoplayer.C;
import com.google.android.exoplayer.SampleHolder;
import com.google.android.exoplayer.SampleSource;
import com.google.android.exoplayer.util.MimeTypes;
import com.android.usbtuner.tvinput.PlaybackCacheListener;

import java.io.IOException;
import java.lang.annotation.Retention;
import java.lang.annotation.RetentionPolicy;
import java.util.List;
import java.util.concurrent.TimeUnit;

import junit.framework.Assert;

/**
 * Handles I/O between {@link com.android.usbtuner.exoplayer.SampleExtractor} and
 * {@link CacheManager}.Reads & writes samples from/to {@link SampleCache} which is backed
 * by physical storage.
 */
public class RecordingSampleBuffer implements CacheManager.SampleBuffer,
        CacheManager.EvictListener {
    private static final String TAG = "RecordingSampleBuffer";
    private static final boolean DEBUG = false;

    @IntDef({CACHE_REASON_LIVE_PLAYBACK, CACHE_REASON_RECORDED_PLAYBACK, CACHE_REASON_RECORDING})
    @Retention(RetentionPolicy.SOURCE)
    public @interface CacheReason {}

    /**
     * A cache reason for live-stream playback.
     */
    public static final int CACHE_REASON_LIVE_PLAYBACK = 0;

    /**
     * A cache reason for playback of a recorded program.
     */
    public static final int CACHE_REASON_RECORDED_PLAYBACK = 1;

    /**
     * A cache reason for recording a program.
     */
    public static final int CACHE_REASON_RECORDING = 2;

    private static final long CACHE_WRITE_TIMEOUT_MS = 10 * 1000;  // 10 seconds
    private static final long CHUNK_DURATION_US = TimeUnit.MILLISECONDS.toMicros(500);
    private static final long LIVE_THRESHOLD_US = TimeUnit.SECONDS.toMicros(1);

    private final CacheManager mCacheManager;
    private final PlaybackCacheListener mCacheListener;
    private final int mCacheReason;

    private int mTrackCount;
    private List<String> mIds;
    private List<MediaFormat> mMediaFormats;
    private volatile long mCacheDurationUs = 0;
    private long[] mCacheEndPositionUs;
    // SampleCache to append the latest live sample.
    private SampleCache[] mSampleCaches;
    private CachedSampleQueue[] mPlayingSampleQueues;
    private final SamplePool mSamplePool = new SamplePool();
    private long mLastBufferedPositionUs = C.UNKNOWN_TIME_US;
    private long mCurrentPlaybackPositionUs = 0;
    private boolean mEos = false;

    private class CachedSampleQueue extends SampleQueue {
        private SampleCache mCache = null;

        public CachedSampleQueue(SamplePool samplePool) {
            super(samplePool);
        }

        public void setSource(SampleCache newCache) {
            for (SampleCache cache = mCache; cache != null; cache = cache.getNext()) {
                cache.clear();
                cache.close();
            }
            mCache = newCache;
            for (SampleCache cache = mCache; cache != null; cache = cache.getNext()) {
                cache.resetRead();
            }
        }

        public boolean maybeReadSample() {
            if (isDurationGreaterThan(CHUNK_DURATION_US)) {
                return false;
            }
            SampleHolder sample = mCache.maybeReadSample();
            if (sample == null) {
                if (!mCache.canReadMore() && mCache.getNext() != null) {
                    mCache.clear();
                    mCache.close();
                    mCache = mCache.getNext();
                    mCache.resetRead();
                    return maybeReadSample();
                } else {
                    if (mCacheReason == CACHE_REASON_RECORDED_PLAYBACK
                            && !mCache.canReadMore() && mCache.getNext() == null) {
                        // At the end of the recorded playback.
                        setEos();
                    }
                    return false;
                }
            } else {
                queueSample(sample);
                return true;
            }
        }

        public int dequeueSample(SampleHolder sample) {
            maybeReadSample();
            return super.dequeueSample(sample);
        }

        @Override
        public void clear() {
            super.clear();
            for (SampleCache cache = mCache; cache != null; cache = cache.getNext()) {
                cache.clear();
                cache.close();
            }
            mCache = null;
        }

        public long getSourceStartPositionUs() {
            return mCache == null ? -1 : mCache.getStartPositionUs();
        }
    }

    /**
     * Creates {@link com.android.usbtuner.exoplayer.cache.CacheManager.SampleBuffer} with
     * cached I/O backed by physical storage (e.g. trickplay,recording,recorded-playback).
     *
     * @param cacheManager
     * @param cacheListener
     * @param enableTrickplay {@code true} when trickplay should be enabled
     * @param cacheReason the reason for caching samples {@link RecordingSampleBuffer.CacheReason}
     */
    public RecordingSampleBuffer(CacheManager cacheManager, PlaybackCacheListener cacheListener,
            boolean enableTrickplay, @CacheReason int cacheReason) {
        mCacheManager = cacheManager;
        mCacheListener = cacheListener;
        if (cacheListener != null) {
            cacheListener.onCacheStateChanged(enableTrickplay);
        }
        mCacheReason = cacheReason;
    }

    private String getTrackId(int index) {
        return mIds.get(index);
    }

    @Override
    public synchronized void init(List<String> ids, List<MediaFormat> mediaFormats)
            throws IOException {
        mTrackCount = ids.size();
        if (mTrackCount <= 0) {
            throw new IOException("No tracks to initialize");
        }
        mIds = ids;
        if (mCacheReason == CACHE_REASON_RECORDING && mediaFormats == null) {
            throw new IOException("MediaFormat is not provided.");
        }
        mMediaFormats = mediaFormats;
        mSampleCaches = new SampleCache[mTrackCount];
        mPlayingSampleQueues = new CachedSampleQueue[mTrackCount];
        mCacheEndPositionUs = new long[mTrackCount];
        for (int i = 0; i < mTrackCount; i++) {
            if (mCacheReason != CACHE_REASON_RECORDED_PLAYBACK) {
                mSampleCaches[i] = mCacheManager.createNewWriteFile(getTrackId(i), 0, mSamplePool);
                mPlayingSampleQueues[i] = null;
                mCacheEndPositionUs[i] = CHUNK_DURATION_US;
            } else {
                mCacheManager.loadTrackFormStorage(mIds.get(i), mSamplePool);
            }
        }
    }

    private boolean isLiveLocked(long positionUs) {
        Long livePositionUs = null;
        for (SampleCache cache : mSampleCaches) {
            if (livePositionUs == null || livePositionUs < cache.getEndPositionUs()) {
                livePositionUs = cache.getEndPositionUs();
            }
        }
        return (livePositionUs == null
                || Math.abs(livePositionUs - positionUs) < LIVE_THRESHOLD_US);
    }

    private void seekIndividualTrackLocked(int index, long positionUs, boolean isLive) {
        CachedSampleQueue queue = mPlayingSampleQueues[index];
        if (queue == null) {
            return;
        }
        queue.clear();
        if (isLive) {
            queue.setSource(mSampleCaches[index]);
        } else {
            queue.setSource(mCacheManager.getReadFile(getTrackId(index), positionUs));
        }
        queue.maybeReadSample();
    }

    @Override
    public synchronized void selectTrack(int index) {
        if (mPlayingSampleQueues[index] == null) {
            String trackId = getTrackId(index);
            mPlayingSampleQueues[index] = new CachedSampleQueue(mSamplePool);
            mCacheManager.registerEvictListener(trackId, this);
            seekIndividualTrackLocked(index, mCurrentPlaybackPositionUs,
                    mCacheReason != CACHE_REASON_RECORDED_PLAYBACK && isLiveLocked(
                            mCurrentPlaybackPositionUs));
            mPlayingSampleQueues[index].maybeReadSample();
        }
    }

    @Override
    public synchronized void deselectTrack(int index) {
        if (mPlayingSampleQueues[index] != null) {
            mPlayingSampleQueues[index].clear();
            mPlayingSampleQueues[index] = null;
            mCacheManager.unregisterEvictListener(getTrackId(index));
        }
    }

    @Override
    public void writeSample(int index, SampleHolder sample,
            ConditionVariable conditionVariable) throws IOException {
        synchronized (this) {
            SampleCache cache = mSampleCaches[index];
            if ((sample.flags & MediaCodec.BUFFER_FLAG_KEY_FRAME) != 0) {
                if (sample.timeUs > mCacheDurationUs) {
                    mCacheDurationUs = sample.timeUs;
                }
                if (sample.timeUs >= mCacheEndPositionUs[index]) {
                    try {
                        SampleCache nextCache = mCacheManager.createNewWriteFile(
                                getTrackId(index), mCacheEndPositionUs[index], mSamplePool);
                        cache.finishWrite(nextCache);
                        mSampleCaches[index] = cache = nextCache;
                        mCacheEndPositionUs[index] =
                                ((sample.timeUs / CHUNK_DURATION_US) + 1) * CHUNK_DURATION_US;
                    } catch (IOException e) {
                        cache.finishWrite(null);
                        throw e;
                    }
                }
            }
            cache.writeSample(sample, conditionVariable);
        }

        if (!conditionVariable.block(CACHE_WRITE_TIMEOUT_MS)) {
            Log.e(TAG, "Error: Serious delay on writing cache");
            conditionVariable.block();
        }
    }

    @Override
    public boolean isWriteSpeedSlow(int sampleSize, long writeDurationNs) {
        if (mCacheReason == CACHE_REASON_RECORDED_PLAYBACK) {
            return false;
        }
        mCacheManager.addWriteStat(sampleSize, writeDurationNs);
        return mCacheManager.isWriteSlow();
    }

    @Override
    public void handleWriteSpeedSlow() {
        Log.w(TAG, "Disk is too slow for trickplay. Disable trickplay.");
        mCacheManager.disable();
        mCacheListener.onDiskTooSlow();
    }

    @Override
    public synchronized void setEos() {
        mEos = true;
    }

    private synchronized boolean reachedEos() {
        return mEos;
    }

    @Override
    public synchronized int readSample(int track, SampleHolder sampleHolder) {
        CachedSampleQueue queue = mPlayingSampleQueues[track];
        Assert.assertNotNull(queue);
        queue.maybeReadSample();
        int result = queue.dequeueSample(sampleHolder);
        if (result != SampleSource.SAMPLE_READ && reachedEos()) {
            return SampleSource.END_OF_STREAM;
        }
        return result;
    }

    @Override
    public synchronized void seekTo(long positionUs) {
        boolean isLive = mCacheReason != CACHE_REASON_RECORDED_PLAYBACK && isLiveLocked(positionUs);

        // Seek video track first
        for (int i = 0; i < mPlayingSampleQueues.length; ++i) {
            CachedSampleQueue queue = mPlayingSampleQueues[i];
            if (queue == null) {
                continue;
            }
            seekIndividualTrackLocked(i, positionUs, isLive);
            if (DEBUG) {
                Log.d(TAG, "start time = " + queue.getSourceStartPositionUs());
            }
        }
        mLastBufferedPositionUs = positionUs;
    }

    @Override
    public synchronized long getBufferedPositionUs() {
        Long result = null;
        for (CachedSampleQueue queue : mPlayingSampleQueues) {
            if (queue == null) {
                continue;
            }
            Long bufferedPositionUs = queue.getEndPositionUs();
            if (bufferedPositionUs == null) {
                continue;
            }
            if (result == null || result > bufferedPositionUs) {
                result = bufferedPositionUs;
            }
        }
        if (result == null) {
            return mLastBufferedPositionUs;
        } else {
            return (mLastBufferedPositionUs = result);
        }
    }

    @Override
    public synchronized boolean continueBuffering(long positionUs) {
        boolean hasSamples = true;
        mCurrentPlaybackPositionUs = positionUs;
        for (CachedSampleQueue queue : mPlayingSampleQueues) {
            if (queue == null) {
                continue;
            }
            queue.maybeReadSample();
            if (queue.isEmpty()) {
                hasSamples = false;
            }
        }
        return hasSamples;
    }

    @Override
    public synchronized void release() {
        if (mSampleCaches == null) {
            return;
        }
        if (mCacheReason == CACHE_REASON_RECORDED_PLAYBACK) {
            mCacheManager.close();
        }
        for (int i = 0; i < mTrackCount; ++i) {
            if (mCacheReason != CACHE_REASON_RECORDED_PLAYBACK) {
                mSampleCaches[i].finishWrite(null);
            }
            mCacheManager.unregisterEvictListener(getTrackId(i));
        }
        if (mCacheReason == CACHE_REASON_RECORDING && mTrackCount > 0) {
            // Saves meta information for recording.
            Pair<String, android.media.MediaFormat> audio = null, video = null;
            for (int i = 0; i < mTrackCount; ++i) {
                MediaFormat mediaFormat = mMediaFormats.get(i);
                String mime = mediaFormat.getString(MediaFormat.KEY_MIME);
                mediaFormat.setLong(android.media.MediaFormat.KEY_DURATION, mCacheDurationUs);
                if (MimeTypes.isAudio(mime)) {
                    audio = new Pair<>(getTrackId(i), mediaFormat);
                }
                else if (MimeTypes.isVideo(mime)) {
                    video = new Pair<>(getTrackId(i), mediaFormat);
                }
            }
            mCacheManager.writeMetaFiles(audio, video);
        }

        for (int i = 0; i < mTrackCount; ++i) {
            mCacheManager.clearTrack(getTrackId(i));
        }
    }

    // CacheEvictListener
    @Override
    public void onCacheEvicted(String id, long createdTimeMs) {
        if (mCacheListener != null) {
            mCacheListener.onCacheStartTimeChanged(
                    createdTimeMs + TimeUnit.MICROSECONDS.toMillis(CHUNK_DURATION_US));
        }
    }
}
