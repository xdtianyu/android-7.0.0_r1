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

import com.google.android.exoplayer.MediaFormat;
import com.google.android.exoplayer.MediaFormatHolder;
import com.google.android.exoplayer.MediaFormatUtil;
import com.google.android.exoplayer.SampleHolder;
import com.android.usbtuner.exoplayer.cache.CacheManager;
import com.android.usbtuner.exoplayer.cache.RecordingSampleBuffer;
import com.android.usbtuner.tvinput.PlaybackCacheListener;

import android.util.Pair;

import java.io.IOException;
import java.util.ArrayList;
import java.util.List;

/**
 * A class that plays a recorded stream without using {@link MediaExtractor},
 * since all samples are extracted and stored to the permanent storage already.
 */
public class ReplaySampleSourceExtractor implements SampleExtractor{
    private static final String TAG = "ReplaySampleSourceExt";
    private static final boolean DEBUG = false;

    private int mTrackCount;
    private android.media.MediaFormat[] mMediaFormats;
    private MediaFormat[] mTrackFormats;

    private boolean mReleased;


    private final CacheManager mCacheManager;
    private final PlaybackCacheListener mCacheListener;
    private CacheManager.SampleBuffer mSampleBuffer;

    public ReplaySampleSourceExtractor(
            CacheManager cacheManager, PlaybackCacheListener cacheListener) {
        mCacheManager = cacheManager;
        mCacheListener = cacheListener;
        mTrackCount = -1;
    }

    @Override
    public boolean prepare() throws IOException {
        ArrayList<Pair<String, android.media.MediaFormat>> trackInfos =
                mCacheManager.readTrackInfoFiles();
        if (trackInfos == null || trackInfos.size() <= 0) {
            return false;
        }
        mTrackCount = trackInfos.size();
        List<String> ids = new ArrayList<>();
        mMediaFormats = new android.media.MediaFormat[mTrackCount];
        mTrackFormats = new MediaFormat[mTrackCount];
        for (int i = 0; i < mTrackCount; ++i) {
            Pair<String, android.media.MediaFormat> pair = trackInfos.get(i);
            ids.add(pair.first);
            mMediaFormats[i] = pair.second;
            mTrackFormats[i] = MediaFormatUtil.createMediaFormat(mMediaFormats[i]);
        }
        mSampleBuffer = new RecordingSampleBuffer(mCacheManager, mCacheListener, true,
                RecordingSampleBuffer.CACHE_REASON_RECORDED_PLAYBACK);
        mSampleBuffer.init(ids, null);
        return true;
    }

    @Override
    public MediaFormat[] getTrackFormats() {
        return mTrackFormats;
    }

    @Override
    public void getTrackMediaFormat(int track, MediaFormatHolder outMediaFormatHolder) {
        outMediaFormatHolder.format = mTrackFormats[track];
        outMediaFormatHolder.drmInitData = null;
    }

    @Override
    public void release() {
        if (!mReleased) {
            mSampleBuffer.release();
        }
        mReleased = true;
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
    public void seekTo(long positionUs) {
        mSampleBuffer.seekTo(positionUs);
    }

    @Override
    public int readSample(int track, SampleHolder sampleHolder) {
        return mSampleBuffer.readSample(track, sampleHolder);
    }


    @Override
    public boolean continueBuffering(long positionUs) {
        return mSampleBuffer.continueBuffering(positionUs);
    }
}
