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

import com.google.android.exoplayer.MediaFormat;
import com.google.android.exoplayer.MediaFormatHolder;
import com.google.android.exoplayer.MediaFormatUtil;
import com.google.android.exoplayer.SampleHolder;
import com.google.android.exoplayer.SampleSource;
import com.google.android.exoplayer.util.MimeTypes;
import com.android.usbtuner.exoplayer.cache.CacheManager;
import com.android.usbtuner.tvinput.PlaybackCacheListener;

import java.io.IOException;
import java.nio.ByteBuffer;

/**
 * Extracts samples from {@link MediaDataSource} for MPEG-TS streams.
 */
public final class MpegTsSampleSourceExtractor implements SampleExtractor {
    public static final String MIMETYPE_TEXT_CEA_708 = "text/cea-708";

    private static final int CC_BUFFER_SIZE_IN_BYTES = 9600 / 8;

    private final SampleExtractor mSampleExtractor;
    private MediaFormat[] mTrackFormats;
    private boolean[] mGotEos;
    private int mVideoTrackIndex;
    private int mCea708TextTrackIndex;
    private boolean mCea708TextTrackSelected;
    private ByteBuffer mCea708CcBuffer;

    private long mCea708PresentationTimeUs;
    private CcParser mCcParser;

    private void init() {
        mVideoTrackIndex = -1;
        mCea708TextTrackIndex = -1;
        mCea708CcBuffer = ByteBuffer.allocate(CC_BUFFER_SIZE_IN_BYTES);
        mCea708PresentationTimeUs = -1;
        mCea708TextTrackSelected = false;
    }

    /**
     * Creates MpegTsSampleSourceExtractor for {@link MediaDataSource}.
     *
     * @param source the {@link MediaDataSource} to extract from
     * @param cacheManager the manager for reading & writing samples backed by physical storage
     * @param cacheListener the {@link com.android.usbtuner.tvinput.PlaybackCacheListener}
     *                      to notify cache storage status change
     */
    public MpegTsSampleSourceExtractor(MediaDataSource source,
            CacheManager cacheManager, PlaybackCacheListener cacheListener) {
        if (cacheManager == null || cacheManager.isDisabled()) {
            mSampleExtractor =
                    new PlaySampleExtractor(source, cacheManager, cacheListener, false);

        } else {
            mSampleExtractor =
                    new PlaySampleExtractor(source, cacheManager, cacheListener, true);
        }
        init();
    }

    /**
     * Creates MpegTsSampleSourceExtractor for a recorded program.
     *
     * @param cacheManager the samples provider which is stored in physical storage
     * @param cacheListener the {@link com.android.usbtuner.tvinput.PlaybackCacheListener}
     *                      to notify cache storage status change
     */
    public MpegTsSampleSourceExtractor(CacheManager cacheManager,
            PlaybackCacheListener cacheListener) {
        mSampleExtractor = new ReplaySampleSourceExtractor(cacheManager, cacheListener);
        init();
    }

    @Override
    public boolean prepare() throws IOException {
        if(!mSampleExtractor.prepare()) {
            return false;
        }
        MediaFormat trackFormats[] = mSampleExtractor.getTrackFormats();
        int trackCount = trackFormats.length;
        mGotEos = new boolean[trackCount];

        for (int i = 0; i < trackCount; ++i) {
            String mime = trackFormats[i].mimeType;
            if (MimeTypes.isVideo(mime) && mVideoTrackIndex == -1) {
                mVideoTrackIndex = i;
                if (android.media.MediaFormat.MIMETYPE_VIDEO_MPEG2.equals(mime)) {
                    mCcParser = new Mpeg2CcParser();
                } else if (android.media.MediaFormat.MIMETYPE_VIDEO_AVC.equals(mime)) {
                    mCcParser = new H264CcParser();
                }
            }
        }

        if (mVideoTrackIndex != -1) {
            mCea708TextTrackIndex = trackCount;
        }
        mTrackFormats = new MediaFormat[mCea708TextTrackIndex < 0 ? trackCount : trackCount + 1];
        System.arraycopy(trackFormats, 0, mTrackFormats, 0, trackCount);
        if (mCea708TextTrackIndex >= 0) {
            mTrackFormats[trackCount] = MediaFormatUtil.createTextMediaFormat(MIMETYPE_TEXT_CEA_708,
                    mTrackFormats[0].durationUs);
        }
        return true;
    }

    @Override
    public MediaFormat[] getTrackFormats() {
        return mTrackFormats;
    }

    @Override
    public void selectTrack(int index) {
        if (index == mCea708TextTrackIndex) {
            mCea708TextTrackSelected = true;
            return;
        }
        mSampleExtractor.selectTrack(index);
    }

    @Override
    public void deselectTrack(int index) {
        if (index == mCea708TextTrackIndex) {
            mCea708TextTrackSelected = false;
            return;
        }
        mSampleExtractor.deselectTrack(index);
    }

    @Override
    public long getBufferedPositionUs() {
        return mSampleExtractor.getBufferedPositionUs();
    }

    @Override
    public void seekTo(long positionUs) {
        mSampleExtractor.seekTo(positionUs);
    }

    @Override
    public void getTrackMediaFormat(int track, MediaFormatHolder outMediaFormatHolder) {
        if (track != mCea708TextTrackIndex) {
            mSampleExtractor.getTrackMediaFormat(track, outMediaFormatHolder);
        }
    }

    @Override
    public int readSample(int track, SampleHolder sampleHolder) {
        if (track == mCea708TextTrackIndex) {
            if (mCea708TextTrackSelected && mCea708CcBuffer.position() > 0) {
                mCea708CcBuffer.flip();
                sampleHolder.timeUs = mCea708PresentationTimeUs;
                sampleHolder.data.put(mCea708CcBuffer);
                mCea708CcBuffer.clear();
                return SampleSource.SAMPLE_READ;
            } else {
                return mVideoTrackIndex < 0 || mGotEos[mVideoTrackIndex]
                        ? SampleSource.END_OF_STREAM : SampleSource.NOTHING_READ;
            }
        }

        // Should read CC track first.
        if (mCea708TextTrackSelected && mCea708CcBuffer.position() > 0) {
            return mGotEos[track] ? SampleSource.END_OF_STREAM : SampleSource.NOTHING_READ;
        }

        int result = mSampleExtractor.readSample(track, sampleHolder);
        switch (result) {
            case SampleSource.END_OF_STREAM: {
                mGotEos[track] = true;
                break;
            }
            case SampleSource.SAMPLE_READ: {
                if (mCea708TextTrackSelected && track == mVideoTrackIndex
                        && sampleHolder.data != null) {
                    mCcParser.mayParseClosedCaption(sampleHolder.data, sampleHolder.timeUs);
                }
                break;
            }
        }
        return result;
    }

    @Override
    public void release() {
        mSampleExtractor.release();
        mVideoTrackIndex = -1;
        mCea708TextTrackIndex = -1;
        mCea708TextTrackSelected = false;
    }

    @Override
    public boolean continueBuffering(long positionUs) {
        return mSampleExtractor.continueBuffering(positionUs);
    }

    private abstract class CcParser {
        abstract void mayParseClosedCaption(ByteBuffer buffer, long presentationTimeUs);

        protected void parseClosedCaption(ByteBuffer buffer, int offset, long presentationTimeUs) {
            // For the details of user_data_type_structure, see ATSC A/53 Part 4 - Table 6.9.
            int pos = offset;
            if (pos + 2 >= buffer.position()) {
                return;
            }
            boolean processCcDataFlag = (buffer.get(pos) & 64) != 0;
            int ccCount = buffer.get(pos) & 0x1f;
            pos += 2;
            if (!processCcDataFlag || pos + 3 * ccCount >= buffer.position() || ccCount == 0) {
                return;
            }
            for (int i = 0; i < 3 * ccCount; i++) {
                mCea708CcBuffer.put(buffer.get(pos + i));
            }
            mCea708PresentationTimeUs = presentationTimeUs;
        }
    }

    private class Mpeg2CcParser extends CcParser {
        @Override
        public void mayParseClosedCaption(ByteBuffer buffer, long presentationTimeUs) {
            int pos = 0;
            while (pos + 9 < buffer.position()) {
                // Find the start prefix code of private user data.
                if (buffer.get(pos) == 0
                        && buffer.get(pos + 1) == 0
                        && buffer.get(pos + 2) == 1
                        && (buffer.get(pos + 3) & 0xff) == 0xb2) {
                    // ATSC closed caption data embedded in MPEG2VIDEO stream has 'GA94' user
                    // identifier and user data type code 3.
                    if (buffer.get(pos + 4) == 'G'
                            && buffer.get(pos + 5) == 'A'
                            && buffer.get(pos + 6) == '9'
                            && buffer.get(pos + 7) == '4'
                            && buffer.get(pos + 8) == 3) {
                        parseClosedCaption(buffer, pos + 9, presentationTimeUs);
                    }
                    pos += 9;
                } else {
                    ++pos;
                }
            }
        }
    }

    private class H264CcParser extends CcParser {
        @Override
        public void mayParseClosedCaption(ByteBuffer buffer, long presentationTimeUs) {
            int pos = 0;
            while (pos + 7 < buffer.position()) {
                // Find the start prefix code of a NAL Unit.
                if (buffer.get(pos) == 0
                        && buffer.get(pos + 1) == 0
                        && buffer.get(pos + 2) == 1) {
                    int nalType = buffer.get(pos + 3) & 0x1f;
                    int payloadType = buffer.get(pos + 4) & 0xff;

                    // ATSC closed caption data embedded in H264 private user data has NAL type 6,
                    // payload type 4, and 'GA94' user identifier for ATSC.
                    if (nalType == 6 && payloadType == 4 && buffer.get(pos + 9) == 'G'
                            && buffer.get(pos + 10) == 'A'
                            && buffer.get(pos + 11) == '9'
                            && buffer.get(pos + 12) == '4') {
                        parseClosedCaption(buffer, pos + 14, presentationTimeUs);
                    }
                    pos += 7;
                } else {
                    ++pos;
                }
            }
        }
    }
}
