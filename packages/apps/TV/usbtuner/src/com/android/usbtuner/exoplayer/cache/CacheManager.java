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

package com.android.usbtuner.exoplayer.cache;

import android.media.MediaFormat;
import android.os.ConditionVariable;
import android.os.HandlerThread;
import android.support.annotation.VisibleForTesting;
import android.support.annotation.NonNull;
import android.support.annotation.Nullable;
import android.util.ArrayMap;
import android.util.ArraySet;
import android.util.Log;
import android.util.Pair;

import com.google.android.exoplayer.SampleHolder;

import java.io.File;
import java.io.FileNotFoundException;
import java.io.IOException;
import java.text.SimpleDateFormat;
import java.util.ArrayList;
import java.util.Date;
import java.util.List;
import java.util.Locale;
import java.util.Map;
import java.util.Set;
import java.util.SortedMap;
import java.util.TreeMap;

/**
 * Manages {@link SampleCache} objects.
 * <p>
 * The cache manager can be disabled, while running, if the write throughput to the associated
 * external storage is detected to be lower than a threshold {@code MINIMUM_DISK_WRITE_SPEED_MBPS}".
 * This leads to restarting playback flow.
 */
public class CacheManager {
    private static final String TAG = "CacheManager";
    private static final boolean DEBUG = false;

    // Constants for the disk write speed checking
    private static final long MINIMUM_WRITE_SIZE_FOR_SPEED_CHECK =
            10L * 1024 * 1024;  // Checks for every 10M disk write
    private static final int MINIMUM_SAMPLE_SIZE_FOR_SPEED_CHECK = 15 * 1024;
    private static final int MAXIMUM_SPEED_CHECK_COUNT = 5;  // Checks only 5 times
    private static final int MINIMUM_DISK_WRITE_SPEED_MBPS = 3;  // 3 Megabytes per second

    private final SampleCache.SampleCacheFactory mSampleCacheFactory;
    private final Map<String, SortedMap<Long, SampleCache>> mCacheMap = new ArrayMap<>();
    private final Map<String, EvictListener> mEvictListeners = new ArrayMap<>();
    private final StorageManager mStorageManager;
    private final HandlerThread mIoHandlerThread = new HandlerThread(TAG);
    private long mCacheSize = 0;
    private final CacheSet mPendingDelete = new CacheSet();
    private final CacheListener mCacheListener = new CacheListener() {
        @Override
        public void onWrite(SampleCache cache) {
            mCacheSize += cache.getSize();
        }

        @Override
        public void onDelete(SampleCache cache) {
            mPendingDelete.remove(cache);
            mCacheSize -= cache.getSize();
        }
    };

    private volatile boolean mClosed = false;
    private int mMinSampleSizeForSpeedCheck = MINIMUM_SAMPLE_SIZE_FOR_SPEED_CHECK;
    private long mTotalWriteSize;
    private long mTotalWriteTimeNs;
    private volatile int mSpeedCheckCount;
    private boolean mDisabled = false;

    public interface CacheListener {
        void onWrite(SampleCache cache);
        void onDelete(SampleCache cache);
    }

    public interface EvictListener {
        void onCacheEvicted(String id, long createdTimeMs);
    }

    /**
     * Handles I/O
     * between CacheManager and {@link com.android.usbtuner.exoplayer.SampleExtractor}.
     */
    public interface SampleBuffer {

        /**
         * Initializes SampleBuffer.
         * @param Ids track identifiers for storage read/write.
         * @param mediaFormats meta-data for each track, this will be saved to storage in recording.
         * @throws IOException
         */
        void init(@NonNull List<String> Ids, @Nullable List<MediaFormat> mediaFormats)
                throws IOException;

        /**
         * Selects the track {@code index} for reading sample data.
         */
        void selectTrack(int index);

        /**
         * Deselects the track at {@code index},
         * so that no more samples will be read from the track.
         */
        void deselectTrack(int index);

        /**
         * Writes sample to storage.
         *
         * @param index track index
         * @param sample sample to write at storage
         * @param conditionVariable notifies the completion of writing sample.
         * @throws IOException
         */
        void writeSample(int index, SampleHolder sample, ConditionVariable conditionVariable)
                throws IOException;

        /**
         * Checks whether storage write speed is slow.
         */
        boolean isWriteSpeedSlow(int sampleSize, long writeDurationNs);

        /**
         * Handles when write speed is slow.
         */
        void handleWriteSpeedSlow();

        /**
         * Sets the flag when EoS was met.
         */
        void setEos();

        /**
         * Reads the next sample in the track at index {@code track} into {@code sampleHolder},
         * returning {@link com.google.android.exoplayer.SampleSource#SAMPLE_READ}
         * if it is available.
         * If the next sample is not available,
         * returns {@link com.google.android.exoplayer.SampleSource#NOTHING_READ}.
         */
        int readSample(int index, SampleHolder outSample);

        /**
         * Seeks to the specified time in microseconds.
         */
        void seekTo(long positionUs);

        /**
         * Returns an estimate of the position up to which data is buffered.
         */
        long getBufferedPositionUs();

        /**
         * Returns whether there is buffered data.
         */
        boolean continueBuffering(long positionUs);

        /**
         * Cleans up and releases everything.
         */
        void release();
    }

    /**
     * Storage configuration and policy manager for {@link CacheManager}
     */
    public interface StorageManager {

        /**
         * Provides eligible storage directory for {@link CacheManager}.
         *
         * @return a directory to save cache chunks and meta files
         */
        File getCacheDir();

        /**
         * Cleans up storage.
         */
        void clearStorage();

        /**
         * Informs whether the storage is used for persistent use. (eg. dvr recording/play)
         *
         * @return {@code true} if stored files are persistent
         */
        boolean isPersistent();

        /**
         * Informs whether the storage usage exceeds pre-determined size.
         *
         * @param cacheSize the current total usage of Storage in bytes.
         * @param pendingDelete the current storage usage which will be deleted in near future by
         *                      bytes
         * @return {@code true} if it reached pre-determined max size
         */
        boolean reachedStorageMax(long cacheSize, long pendingDelete);

        /**
         * Informs whether the storage has enough remained space.
         *
         * @param pendingDelete the current storage usage which will be deleted in near future by
         *                      bytes
         * @return {@code true} if it has enough space
         */
        boolean hasEnoughBuffer(long pendingDelete);

        /**
         * Reads track name & {@link MediaFormat} from storage.
         *
         * @param isAudio {@code true} if it is for audio track
         * @return {@link Pair} of track name & {@link MediaFormat}
         * @throws {@link java.io.IOException}
         */
        Pair<String, MediaFormat> readTrackInfoFile(boolean isAudio) throws IOException;

        /**
         * Reads sample indexes for each written sample from storage.
         *
         * @param trackId track name
         * @return
         * @throws {@link java.io.IOException}
         */
        ArrayList<Long> readIndexFile(String trackId) throws IOException;

        /**
         * Writes track information to storage.
         *
         * @param trackId track name
         * @param format {@link android.media.MediaFormat} of the track
         * @param isAudio {@code true} if it is for audio track
         * @throws {@link java.io.IOException}
         */
        void writeTrackInfoFile(String trackId, MediaFormat format, boolean isAudio)
                throws IOException;

        /**
         * Writes index file to storage.
         *
         * @param trackName track name
         * @param index {@link SampleCache} container
         * @throws {@link java.io.IOException}
         */
        void writeIndexFile(String trackName, SortedMap<Long, SampleCache> index)
                throws IOException;
    }

    private static class CacheSet {
        private final Set<SampleCache> mCaches = new ArraySet<>();

        public synchronized void add(SampleCache cache) {
            mCaches.add(cache);
        }

        public synchronized void remove(SampleCache cache) {
            mCaches.remove(cache);
        }

        public synchronized long getSize() {
            long size = 0;
            for (SampleCache cache : mCaches) {
                size += cache.getSize();
            }
            return size;
        }
    }

    public CacheManager(StorageManager storageManager) {
        this(storageManager, new SampleCache.SampleCacheFactory());
    }

    public CacheManager(StorageManager storageManager,
            SampleCache.SampleCacheFactory sampleCacheFactory) {
        mStorageManager = storageManager;
        mSampleCacheFactory = sampleCacheFactory;
        clearCache(true);
        mIoHandlerThread.start();
    }

    public void registerEvictListener(String id, EvictListener evictListener) {
        mEvictListeners.put(id, evictListener);
    }

    public void unregisterEvictListener(String id) {
        mEvictListeners.remove(id);
    }

    private void clearCache(boolean deleteFiles) {
        mCacheMap.clear();
        if (deleteFiles) {
            mStorageManager.clearStorage();
        }
        mCacheSize = 0;
    }

    private static String getFileName(String id, long positionUs) {
        return String.format(Locale.ENGLISH, "%s_%016x.cache", id, positionUs);
    }

    /**
     * Creates a new {@link SampleCache} for caching samples.
     *
     * @param id the name of the track
     * @param positionUs starting position of the {@link SampleCache} in micro seconds.
     * @param samplePool {@link SamplePool} for the fast creation of samples.
     * @return returns the created {@link SampleCache}.
     * @throws {@link java.io.IOException}
     */
    public SampleCache createNewWriteFile(String id, long positionUs, SamplePool samplePool)
            throws IOException {
        if (!maybeEvictCache()) {
            throw new IOException("Not enough storage space");
        }
        SortedMap<Long, SampleCache> map = mCacheMap.get(id);
        if (map == null) {
            map = new TreeMap<>();
            mCacheMap.put(id, map);
        }
        File file = new File(mStorageManager.getCacheDir(), getFileName(id, positionUs));
        SampleCache sampleCache = mSampleCacheFactory.createSampleCache(samplePool, file,
                positionUs, mCacheListener, mIoHandlerThread.getLooper());
        map.put(positionUs, sampleCache);
        return sampleCache;
    }

    /**
     * Loads a track using {@link CacheManager.StorageManager}.
     *
     * @param trackId the name of the track.
     * @param samplePool {@link SamplePool} for the fast creation of samples.
     * @throws {@link java.io.IOException}
     */
    public void loadTrackFormStorage(String trackId, SamplePool samplePool) throws IOException {
        ArrayList<Long> keyPositions = mStorageManager.readIndexFile(trackId);

        // TODO: notify the end position
        SortedMap<Long, SampleCache> map = mCacheMap.get(trackId);
        if (map == null) {
            map = new TreeMap<>();
            mCacheMap.put(trackId, map);
        }
        SampleCache cache = null;
        for (long positionUs: keyPositions) {
            cache = mSampleCacheFactory.createSampleCacheFromFile(samplePool,
                    mStorageManager.getCacheDir(), getFileName(trackId, positionUs), positionUs,
                    mCacheListener, mIoHandlerThread.getLooper(), cache);
            map.put(positionUs, cache);
        }
    }

    /**
     * Finds a {@link SampleCache} for the specified track name and the position.
     *
     * @param id the name of the track.
     * @param positionUs the position.
     * @return returns the found {@link SampleCache}.
     */
    public SampleCache getReadFile(String id, long positionUs) {
        SortedMap<Long, SampleCache> map = mCacheMap.get(id);
        if (map == null) {
            return null;
        }
        SampleCache sampleCache;
        SortedMap<Long, SampleCache> headMap = map.headMap(positionUs + 1);
        if (!headMap.isEmpty()) {
            sampleCache = headMap.get(headMap.lastKey());
        } else {
            sampleCache = map.get(map.firstKey());
        }
        return sampleCache;
    }

    private boolean maybeEvictCache() {
        long pendingDelete = mPendingDelete.getSize();
        while (mStorageManager.reachedStorageMax(mCacheSize, pendingDelete)
                || !mStorageManager.hasEnoughBuffer(pendingDelete)) {
            if (mStorageManager.isPersistent()) {
                // Since cache is persistent, we cannot evict caches.
                return false;
            }
            SortedMap<Long, SampleCache> earliestCacheMap = null;
            SampleCache earliestCache = null;
            String earliestCacheId = null;
            for (Map.Entry<String, SortedMap<Long, SampleCache>> entry : mCacheMap.entrySet()) {
                SortedMap<Long, SampleCache> map = entry.getValue();
                if (map.isEmpty()) {
                    continue;
                }
                SampleCache cache = map.get(map.firstKey());
                if (earliestCache == null
                        || cache.getCreatedTimeMs() < earliestCache.getCreatedTimeMs()) {
                    earliestCacheMap = map;
                    earliestCache = cache;
                    earliestCacheId = entry.getKey();
                }
            }
            if (earliestCache == null) {
                break;
            }
            mPendingDelete.add(earliestCache);
            earliestCache.delete();
            earliestCacheMap.remove(earliestCache.getStartPositionUs());
            if (DEBUG) {
                Log.d(TAG, String.format("cacheSize = %d; pendingDelete = %b; "
                                + "earliestCache size = %d; %s@%d (%s)",
                        mCacheSize, pendingDelete, earliestCache.getSize(), earliestCacheId,
                        earliestCache.getStartPositionUs(),
                        new SimpleDateFormat().format(new Date(earliestCache.getCreatedTimeMs()))));
            }
            EvictListener listener = mEvictListeners.get(earliestCacheId);
            if (listener != null) {
                listener.onCacheEvicted(earliestCacheId, earliestCache.getCreatedTimeMs());
            }
            pendingDelete = mPendingDelete.getSize();
        }
        return true;
    }

    /**
     * Reads track information which includes {@link MediaFormat}.
     *
     * @return returns all track information which is found by {@link CacheManager.StorageManager}.
     * @throws {@link java.io.IOException}
     */
    public ArrayList<Pair<String, MediaFormat>> readTrackInfoFiles() throws IOException {
        ArrayList<Pair<String, MediaFormat>> trackInfos = new ArrayList<>();
        try {
            trackInfos.add(mStorageManager.readTrackInfoFile(false));
        } catch (FileNotFoundException e) {
            // There can be a single track only recording. (eg. audio-only, video-only)
            // So the exception should not stop the read.
        }
        try {
            trackInfos.add(mStorageManager.readTrackInfoFile(true));
        } catch (FileNotFoundException e) {
            // See above catch block.
        }
        return trackInfos;
    }

    /**
     * Writes track information and index information for all tracks.
     *
     * @param audio audio information.
     * @param video video information.
     */
    public void writeMetaFiles(Pair<String, MediaFormat> audio, Pair<String, MediaFormat> video) {
        try {
            if (audio != null) {
                mStorageManager.writeTrackInfoFile(audio.first, audio.second, true);
                SortedMap<Long, SampleCache> map = mCacheMap.get(audio.first);
                if (map == null) {
                    throw new IOException("Audio track index missing");
                }
                mStorageManager.writeIndexFile(audio.first, map);
            }
            if (video != null) {
                mStorageManager.writeTrackInfoFile(video.first, video.second, false);
                SortedMap<Long, SampleCache> map = mCacheMap.get(video.first);
                if (map == null) {
                    throw new IOException("Video track index missing");
                }
                mStorageManager.writeIndexFile(video.first, map);
            }
        } catch (IOException e) {
            // TODO: throw exception and notify this failure properly.
        }
    }

    /**
     * Marks it is closed and it is not used anymore.
     */
    public void close() {
        // Clean-up may happen after this is called.
        mClosed = true;
    }

    /**
     * Cleans up the specified track.
     *
     * @param trackId the name of the track.
     */
    public void clearTrack(String trackId) {
        SortedMap<Long, SampleCache> map = mCacheMap.get(trackId);
        if (map == null) {
            Log.w(TAG, "Cache with specified ID (" + trackId + ") not found");
            return;
        }
        for (SampleCache cache : map.values()) {
            cache.clear();
            cache.close();
            if (!mStorageManager.isPersistent()) {
                cache.delete();
            }
        }
        mCacheMap.remove(trackId);
        if (mCacheMap.isEmpty() && mClosed) {
            mIoHandlerThread.quitSafely();
            clearCache(!mStorageManager.isPersistent());
        }
    }

    private void resetWriteStat() {
        mTotalWriteSize = 0;
        mTotalWriteTimeNs = 0;
    }

    /**
     * Adds a disk write sample size to calculate the average disk write bandwidth.
     */
    public void addWriteStat(long size, long timeNs) {
        if (size >= mMinSampleSizeForSpeedCheck) {
            mTotalWriteSize += size;
            mTotalWriteTimeNs += timeNs;
        }
    }

    /**
     * Returns if the average disk write bandwidth is slower than
     * threshold {@code MINIMUM_DISK_WRITE_SPEED_MBPS}.
     */
    public boolean isWriteSlow() {
        if (mTotalWriteSize < MINIMUM_WRITE_SIZE_FOR_SPEED_CHECK) {
            return false;
        }

        // Checks write speed for only MAXIMUM_SPEED_CHECK_COUNT times to ignore outliers
        // by temporary system overloading during the playback.
        if (mSpeedCheckCount > MAXIMUM_SPEED_CHECK_COUNT) {
            return false;
        }
        mSpeedCheckCount++;
        float megabytePerSecond = getWriteBandwidth();
        resetWriteStat();
        if (DEBUG) {
            Log.d(TAG, "Measured disk write performance: " + megabytePerSecond + "MBps");
        }
        return megabytePerSecond < MINIMUM_DISK_WRITE_SPEED_MBPS;
    }

    /**
     * Returns the disk write speed in megabytes per second.
     */
    private float getWriteBandwidth() {
        if (mTotalWriteTimeNs == 0) {
            return -1;
        }
        return ((float) mTotalWriteSize * 1000 / mTotalWriteTimeNs);
    }

    /**
     * Marks {@link CacheManger} object disabled to prevent it from the future use.
     */
    public void disable() {
        mDisabled = true;
    }

    /**
     * Returns if {@link CacheManger} object is disabled.
     */
    public boolean isDisabled() {
        return mDisabled;
    }

    /**
     * Returns if {@link CacheManager} has checked the write speed, which is suitable for Trickplay.
     */
    @VisibleForTesting
    public boolean hasSpeedCheckDone() {
        return mSpeedCheckCount > 0;
    }

    /**
     * Sets minimum sample size for write speed check.
     * @param sampleSize minimum sample size for write speed check.
     */
    @VisibleForTesting
    public void setMinimumSampleSizeForSpeedCheck(int sampleSize) {
        mMinSampleSizeForSpeedCheck = sampleSize;
    }
}
